#include "control_plane.h"
#include "control_plane_pvt.h"
#include "lib_utils.h"

#include <linux/limits.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <syslog.h>

wifi_control_plane_confs_t   m_confs;

/**
 atClient[0]      :   F
 atClient[1-60]   :   C1 - C60
 */
//CLIENT_t atClient[MAX_CONNECT_CLIENT];
CLIENT_t * patClient = NULL;
/*
typedef enum {
    eCLIENT_STATE_NONE,
    eCLIENT_STATE_CONNECTED,
    eCLIENT_STATE_INACTIVE,
    eCLIENT_STATE_LOST,
    eCLIENT_STATE_MAX
}eCLIENT_STATE;
*/
BYTE abClientState[][32] = {
    "STATE_NONE",
    "STATE_CONNECTED",
    "STATE_INACTIVE",
    "STATE_LOST",
    "",
};



BOOL8  control_plane_init(UINT16 u16LocalPort, UINT16 u16MaxConn, 
    char *aFName, UINT8 u8FNameLth, UINT32 u32KATimer, UINT32 u32OfflineTimer )
{
    struct hostent  *hp;
    char  hname[PATH_MAX];
    char  *pStrIp;

    // get host-ip
    gethostname(hname, PATH_MAX);
    syslog(LOG_DEBUG, "INFO. Host name is :%s\n", hname);
    hp = gethostbyname((char *)&hname[0]);
    pStrIp = inet_ntoa(*((struct in_addr *)(hp->h_addr_list[0])));
    if (!pStrIp) {
        syslog(LOG_ERR, "Can NOT get host ip.\n");
    }
    strcpy(&m_confs.aLocalIp[0], pStrIp);
    syslog(LOG_ERR, "INFO. hostip = %s \n", m_confs.aLocalIp);

    m_confs.u16LocalPort         = u16LocalPort;
    m_confs.u16MaxConn           = u16MaxConn;
    if (u8FNameLth > USER_LTH)
        u8FNameLth = USER_LTH;
    aFName[u8FNameLth] = '\0';
    strcpy(m_confs.aFName, aFName);
    m_confs.u32KATimer           = u32KATimer;
    m_confs.u32OfflineTimer      = u32OfflineTimer;

    patClient = new CLIENT_t[MAX_CONNECT_CLIENT];
    memset(patClient, 0, sizeof(*patClient));

    return TRUE_B8;
}

void  signal_callback(S_EV_L1L3_MESSAGE *pEv)
{
    BOOL8   blResult;

    // time-tick event
    if (pEv->eType == eEV_TYPE_TIMER_TICK) {
        handle_req_time_tick();
        return;
    }

    if (pEv->eType != eEV_TYPE_MSG) {
        syslog(LOG_ERR, "Got event but NOT eEV_TYPE_MSG!!!\n");
        return;
    }

    syslog(LOG_DEBUG, "INFO recv signal msg: ev.ip=[%d], ev.port=[%d], ev.len=[%d], ev.buf=\n%s\n",
           pEv->srcAddr.ip_addr, pEv->srcAddr.port, pEv->msgLen, pEv->msgBuf);

    // dispatch signal message
    {
        CONN_SESSION   tSession;
        CLIENT_t      *ptClient;
        BOOL8          blF;
        BYTE          *pName = NULL;

        memset(&tSession, 0, sizeof(tSession));

        blResult = P_ComDecode(pEv->msgBuf, pEv->msgLen, &tSession);
        if (!blResult) {
            syslog(LOG_ERR, "Got mal-formed request, ignore.\n");

            sendMgrResponse(&pEv->srcAddr, &tSession, 400, reinterpret_cast<const unsigned char *>("Bad request. Mal-formed request."));
            return;
        }

        if (tSession.bMap[FROM_SEQ_NUM] && tSession.tFrom.bMap[0]) {
            pName = tSession.tFrom.bName;
        }

        // find: ptClient && blF
        locateClientByUUID(pName, tSession.tCallID.bIDToken, &blF, &ptClient);

        // FOUND
        if (ptClient) {
            if (blF) {
                dispatch_ev_toF(&tSession);
            } else {
                dispatch_ev_toC(ptClient, &tSession);
            }
        }
        // NOT FOUND
        else {
            // session create request?
            switch (tSession.eType) {
            case MSG_RQST_CONN:
                {
                    if (blF) {
                        ptClient = &patClient[0];
                        if (ptClient->eState != eCLIENT_STATE_NONE) {
                            syslog(LOG_WARNING, "there has already another F logged on system, must LOGOUT or RESET first.\n");
                            sendMgrResponse(&pEv->srcAddr, &tSession, 486, reinterpret_cast<const unsigned char *>("Busy. Already has one F here."));
                            return;
                        }

                        // init F
                        init_client(ptClient, &tSession);

                        // save signal socket
                        ptClient->tAddrRemoteSignal = pEv->srcAddr;

                        dispatch_ev_toF(&tSession);
                    } else {
                        ptClient = getClient();
                        if (!ptClient) {
                            syslog(LOG_WARNING, "System busy, there has no idle C-slot, please try LOGON later.\n");
                            sendMgrResponse(&pEv->srcAddr, &tSession, 486, reinterpret_cast<const unsigned char *>("Busy. System C slots are full. try again later."));
                            return;
                        }

                        // init C
                        init_client(ptClient, &tSession);

                        // save signal socket
                        ptClient->tAddrRemoteSignal = pEv->srcAddr;

                        dispatch_ev_toC(ptClient, &tSession);

                    }
                }
                break;

            case MSG_RQST_RESET:
                {
                    // RESET channel when F/C restart
                    locateClientByName(pName, &blF, &ptClient);

                    if (ptClient) {
                    // FOUND
                        if (blF) {
                            dispatch_ev_toF(&tSession);
                        } else {
                            dispatch_ev_toC(ptClient, &tSession);
                        }
                    } else {
                    // NOT FOUND
                        // ignore. instance is already released.
                        syslog(LOG_WARNING, "Instance of user[%s] is already released.\n", (char *)pName);
                    }
                }
                break;

            case MSG_RQST_DISCONN:
            case MSG_RQST_NOTIFY:
                // mis-target request OR instance is already released!
                sendMgrResponse(&pEv->srcAddr, &tSession, 404, reinterpret_cast<const unsigned char *>("Not found. The requested instance does not exist."));
                syslog(LOG_WARNING, "Instance of request[%d] DOES NOT found.\n", tSession.eType);
                return;

                break;
            default:
                return;
            };

        }

    }


    return;
}

void locateClientByUUID(BYTE *pName, BYTE *pUUID, BOOL8 *pblF, CLIENT_t **ptC)
{
    *ptC = NULL;
    *pblF = FALSE_B8;
    INT32 i;

    for (i=0; i<MAX_CONNECT_CLIENT; i++)
    {
        if (patClient[i].eState == eCLIENT_STATE_NONE) {
            continue;
        }

        if (StrICmp(pUUID,patClient[i].aUUID)) {
            // found, return
            if (i == 0)
                *pblF = TRUE_B8;
            *ptC = &patClient[i];
            return;
        }

    }

    // NOT found, check F-Name
    if (StrICmp((BYTE *)m_confs.aFName, pName)) {
        *pblF = TRUE_B8;
    }

    return;
}

void locateClientByName(BYTE *pName, BOOL8 *pblF, CLIENT_t **ptC)
{
    *ptC = NULL;
    *pblF = FALSE_B8;

    INT32 i;
    for (i=0; i<MAX_CONNECT_CLIENT; i++)
    {
        if (patClient[i].eState == eCLIENT_STATE_NONE) {
            continue;
        }

        if (StrICmp(pName,(BYTE *)patClient[i].aName)) {
            // found, return
            if (i == 0)
                *pblF = TRUE_B8;
            *ptC = &patClient[i];
            return;
        }

    }

    // NOT found, check F-Name
    if (StrICmp((BYTE *)m_confs.aFName, pName)) {
        *pblF = TRUE_B8;
    }

    return;
}

CLIENT_t * getClient()
{
    INT32 i;

    // only get C client. F is always the first element [0] in patClient.
    for (i=1; i<MAX_CONNECT_CLIENT; i++)
    {
        if (patClient[i].eState == eCLIENT_STATE_NONE) {
            // init this instance
            //memset(ptC, 0, sizeof(*ptC));

            return &patClient[i];
        }

    }

    return NULL;
}

void freeClient(CLIENT_t *ptC)
{
    if (!ptC) {
        syslog(LOG_ERR, "Free instance with NULL pointer.\n");
        return;
    }

    ptC->eState = eCLIENT_STATE_NONE;
    return;
}

void init_client(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    BYTE     *pbMsg;
    DWORD    dwIPAddr;

    memset(ptC, 0, sizeof(*ptC));

    if (ptSession->bMap[FROM_SEQ_NUM] && ptSession->tFrom.bMap[0]) {
        pbMsg = (BYTE *)&ptC->aName[0];
        WriteString(&pbMsg, ptSession->tFrom.bName, USER_LTH);
        *pbMsg = '\0';

        // ID
        if (ptSession->tFrom.bMap[1]) {
            pbMsg = (BYTE *)&ptC->aID[0];
            WriteString(&pbMsg, ptSession->tFrom.bID, USER_LTH);
            *pbMsg = '\0';
        }

    } else {
        // anonymous login
    }

    // save Session
    pbMsg = &ptC->aUUID[0];
    WriteString(&pbMsg, ptSession->tCallID.bIDToken, ID_TOKEN_LTH);
    *pbMsg = '\0';
    ptC->dwSeq = ptSession->tCSeq.dwSeq;
    pbMsg = &ptC->bMethod[0];
    WriteString(&pbMsg, ptSession->tCSeq.bMethod, METHOD_LTH);
    *pbMsg = '\0';

    ptC->n32TimeGotKA = time((time_t *)NULL);
    return;
}

void sendResponse(IP_ADDR_PORT_V4 *ptDest, CONN_SESSION *ptResp, DWORD dwSCode, const BYTE *abReason)
{
    UINT16 wReady;
    struct sockaddr_in tPeerAddr;
    BOOL8 blResult = FALSE_B8;
    BYTE  bStrResp[MAX_SIGNAL_LTH];
    WORD  wLen;
    BYTE *pbMsg;

    syslog(LOG_DEBUG, "send response to client.\n");

    // status-line
    ptResp->bMap[RESP_LINE_SEQ_NUM] = 1;
    ptResp->tResponseLine.bMap[0] = 1;
    ptResp->tResponseLine.wSCode = dwSCode;
    ptResp->tResponseLine.bMap[1] = 1;
    pbMsg = &ptResp->tResponseLine.bReason[0];
    WriteString(&pbMsg, (BYTE *)abReason, REASON_LTH);
    *pbMsg = '\0';

    // generate response string
    blResult = P_ComEncode(&bStrResp[0], &wLen, ptResp);
    if (!blResult) {
        syslog(LOG_ERR, "response encoding error.\n");
        return;
    }

    memset(&tPeerAddr, 0, sizeof(tPeerAddr));

    tPeerAddr.sin_family = AF_INET;
    memcpy(&tPeerAddr.sin_addr.s_addr, &ptDest->ip_addr, 4);
    //tPeerAddr.sin_port = htons(ptDest->port);
    tPeerAddr.sin_port = ptDest->port;

    wReady = sendto(m_confs.u32SvrSockfd, bStrResp, wLen, 0, (struct sockaddr*)&tPeerAddr, sizeof(tPeerAddr));
    if(wReady != wLen)
    {
        syslog(LOG_ERR, "sendto() return:%d!\n", wReady);
        return;
    }

    return;
}

void sendMgrResponse(IP_ADDR_PORT_V4 *ptDest, CONN_SESSION *ptReq, DWORD dwSCode, const BYTE *abReason)
{
    UINT16 wReady;
    struct sockaddr_in tPeerAddr;
    BOOL8 blResult = FALSE_B8;
    BYTE  bStrResp[MAX_SIGNAL_LTH];
    WORD  wLen = MAX_SIGNAL_LTH;
    BYTE *pbMsg;

    CONN_SESSION  *ptResp;


    syslog(LOG_DEBUG, "server manager send response to client.\n");

    ptResp = ptReq;
    memset(&ptResp->bMap[0], 0, sizeof(ptResp->bMap));

    // Call-ID
    ptResp->bMap[CALLID_SEQ_NUM] = 1;
    // CSeq
    ptResp->bMap[CSEQ_SEQ_NUM] = 1;

    // status-line
    ptResp->bMap[RESP_LINE_SEQ_NUM] = 1;
    ptResp->tResponseLine.bMap[0] = 1;
    ptResp->tResponseLine.wSCode = dwSCode;
    ptResp->tResponseLine.bMap[1] = 1;
    pbMsg = &ptResp->tResponseLine.bReason[0];
    WriteString(&pbMsg, (BYTE *)abReason, REASON_LTH);
    *pbMsg = '\0';

    // generate response string
    blResult = P_ComEncode(bStrResp, &wLen, ptResp);
    if (!blResult) {
        syslog(LOG_ERR, "response encoding error. ignore this response.\n");
        return;
    }

    memset(&tPeerAddr, 0, sizeof(tPeerAddr));

    tPeerAddr.sin_family = AF_INET;
    memcpy(&tPeerAddr.sin_addr.s_addr, &ptDest->ip_addr, 4);
    //tPeerAddr.sin_port = htons(ptDest->port);
    tPeerAddr.sin_port = ptDest->port;

    wReady = sendto(m_confs.u32SvrSockfd, bStrResp, wLen, 0, (struct sockaddr*)&tPeerAddr, sizeof(tPeerAddr));
    if(wReady != wLen)
    {
        syslog(LOG_ERR, "sendto() return:%d!\n", wReady);
        return;
    }

    return;
}

void dispatch_ev_toF(CONN_SESSION *ptSession)
{
    CLIENT_t   *ptC = &patClient[0];

    dispatch_ev_toC(ptC, ptSession);
    return;
}

void dispatch_ev_toC(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    switch (ptSession->eType) {
    case MSG_RQST_CONN:
        {
            // new client login
            if (ptC->eState == eCLIENT_STATE_NONE) {
                ptC->eState = eCLIENT_STATE_CONNECTED;
                handle_request_CONN(ptC, ptSession);
            }
            // client re-send CONN request
            else {
                handle_request_reCONN(ptC, ptSession);
            }
        }
        break;
    case MSG_RQST_DISCONN:
        handle_request_DISCONN(ptC, ptSession);
        break;
    case MSG_RQST_RESET:
        handle_request_RESET(ptC, ptSession);
        break;
    case MSG_RQST_NOTIFY:
        handle_request_NOTIFY(ptC, ptSession);
        break;
    default:
        syslog(LOG_ERR, "Unsupported request type...\n");
        return;
    }

    return;
}

void handle_req_time_tick()
{
    INT32   nSecs = time( (time_t *)NULL );
    INT32   nInactiveSecs = m_confs.u32KATimer * 3;
    INT32   nDiffSecs;
    INT32   i;

    for (i=0; i<MAX_CONNECT_CLIENT; i++)
    {
        if (patClient[i].eState == eCLIENT_STATE_NONE) {
            continue;
        }

        nDiffSecs = nSecs - patClient[i].n32TimeGotKA;
        if (nDiffSecs > m_confs.u32OfflineTimer) {
            syslog(LOG_WARNING, "reach offline-time[%d], release instance. \n", m_confs.u32OfflineTimer);

            // delete... media-plane
            sdp_process_type_t eClientType = (0 == i) ? SDP_F : SDP_C;
            data_plane_del_sender(eClientType, patClient[i].tMedia);

            // release this client SCILENTLY...
            freeClient(&patClient[i]);

        } else if (nDiffSecs > nInactiveSecs) {
            if (eCLIENT_STATE_INACTIVE == patClient[i].eState) {
                // already in-active
                continue;
            }

            syslog(LOG_WARNING, "reach MAX keep-alive timer[%d], stop media.\n", nInactiveSecs);

            // inactive state, suspend media.
            sdp_process_type_t eClientType = (0 == i) ? SDP_F : SDP_C;
            data_plane_suspend(eClientType, patClient[i].tMedia, SUSPEND_ON);


            // inactive, NOTIFY media module t stop sending/recieving
            patClient[i].eState = eCLIENT_STATE_INACTIVE;

        }

    }
}

void handle_request_CONN(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    //WORD    wMediaPort;
    CONN_SESSION   *ptResp;
    BYTE           *pbMsg;


    // save remote-media ip + port
    ptC->tAddrRemoteMedia.ip_addr = INADDR_NONE;
    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[1])
    {
        ptC->tAddrRemoteMedia.ip_addr = inet_addr((const char *)ptSession->tMedia.tUrl.tHost.unHost.bHostName);
    }
    if (ptC->tAddrRemoteMedia.ip_addr == INADDR_NONE)
    {
        syslog(LOG_ERR, "receive CONN request, but Media IP is invalid. Ignore request AND release instance...\n");

        freeClient(ptC);
        sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 400, reinterpret_cast<const unsigned char *>("Media header error."));
        return;
    }

    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[2])
    {
        ptC->tAddrRemoteMedia.port = ptSession->tMedia.tUrl.wPort;
    }


    // activate media plane
    sdp_process_type_t eClientType = (ptC == &patClient[0]) ? SDP_F : SDP_C;
    ptC->tMedia = data_plane_add_sender(eClientType, ptC->tAddrRemoteMedia.ip_addr, ptC->tAddrRemoteMedia.port);

    // send out response
    ptResp = ptSession;

    memset(&ptResp->bMap[0], 0, sizeof(ptResp->bMap));

    ptResp->bMap[CALLID_SEQ_NUM] = 1;

    ptResp->bMap[CSEQ_SEQ_NUM] = 1;

    ptResp->bMap[MEDIA_SEQ_NUM] = 1;
    {
        URL   *ptUrl = &ptResp->tMedia.tUrl;
        char  *pStrIp;

        ptResp->tMedia.bMap[0] = 1;

        ptUrl->bMap[1] = 1;
        ptUrl->tHost.eType = eNAT_HOSTNAME;
        pbMsg = &ptUrl->tHost.unHost.bHostName[0];

        pStrIp = inet_ntoa( *((struct in_addr *)&ptC->tMedia.s_addr) );
        if (!pStrIp) {
            syslog(LOG_ERR, "Can NOT get host ip. Check media plane.\n");
            pStrIp = &m_confs.aLocalIp[0];
        }
        WriteString(&pbMsg, (BYTE *)pStrIp, NODE_ADDR_LTH);
        pbMsg[0] = '\0';

        ptUrl->bMap[2] = 1;
        ptUrl->wPort = ptC->tMedia.s_port;
    }

    // send response to peer.
    sendResponse(&ptC->tAddrRemoteSignal, ptResp, 200, reinterpret_cast<const unsigned char *>("OK"));

    return;
}

void handle_request_reCONN(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    // reset-media request...

    CONN_SESSION   *ptResp;
    BYTE           *pbMsg;
    UINT32          u32IP   = INADDR_NONE;
    UINT16          u16Port = 0;


    // remote-media ip + port
    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[1])
    {
        u32IP = inet_addr((const char *)ptSession->tMedia.tUrl.tHost.unHost.bHostName);
    }
    if (u32IP == INADDR_NONE)
    {
        syslog(LOG_WARNING, "receive re-CONN request, but Media IP is invalid. Ignore request...\n");

        sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 400, reinterpret_cast<const unsigned char *>("Media header error."));
        return;
    }

    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[2])
    {
        //u16Port = htons(ptSession->tMedia.tUrl.wPort);
        u16Port = ptSession->tMedia.tUrl.wPort;
    }


    // send to media-plane
    if ( (u32IP == ptC->tAddrRemoteMedia.ip_addr) && (u16Port == ptC->tAddrRemoteMedia.ip_addr) ) {
        // goto below, just return 200-OK.
    }
    else {
        ptC->tAddrRemoteMedia.ip_addr = u32IP;
        ptC->tAddrRemoteMedia.port = u16Port;

        // update media plane
        sdp_process_type_t eClientType = (ptC == &patClient[0]) ? SDP_F : SDP_C;
        data_plane_del_sender(eClientType, ptC->tMedia);
        ptC->tMedia = data_plane_add_sender(eClientType, ptC->tAddrRemoteMedia.ip_addr, ptC->tAddrRemoteMedia.port);

    }

    // send out response
    ptResp = ptSession;

    memset(&ptResp->bMap[0], 0, sizeof(ptResp->bMap));

    ptResp->bMap[CALLID_SEQ_NUM] = 1;
    ptResp->bMap[CSEQ_SEQ_NUM] = 1;

    ptResp->bMap[MEDIA_SEQ_NUM] = 1;
    {
        URL   *ptUrl = &ptResp->tMedia.tUrl;
        char  *pStrIp;

        ptResp->tMedia.bMap[0] = 1;

        ptUrl->bMap[1] = 1;
        ptUrl->tHost.eType = eNAT_HOSTNAME;
        pbMsg = &ptUrl->tHost.unHost.bHostName[0];

        pStrIp = inet_ntoa( *((struct in_addr *)&ptC->tMedia.s_addr) );
        if (!pStrIp) {
            syslog(LOG_ERR, "Can NOT get host ip. Check media plane.\n");

            pStrIp = &m_confs.aLocalIp[0];
        }
        WriteString(&pbMsg, (BYTE *)pStrIp, NODE_ADDR_LTH);
        pbMsg[0] = '\0';

        ptUrl->bMap[2] = 1;
        ptUrl->wPort = ptC->tMedia.s_port;
    }

    // send response to peer.
    sendResponse(&ptC->tAddrRemoteSignal, ptResp, 200, reinterpret_cast<const unsigned char *>("OK"));

    return;
}

void handle_request_DISCONN(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    // DIS-CONNECT request

    // delete media-plane
    sdp_process_type_t eClientType = (ptC == &patClient[0]) ? SDP_F : SDP_C;
    data_plane_del_sender(eClientType, ptC->tMedia);

    sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 200, reinterpret_cast<const unsigned char *>("OK"));

    // free instance
    freeClient(ptC);

    return;
}

void handle_request_RESET(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    sdp_process_type_t eClientType = (ptC == &patClient[0]) ? SDP_F : SDP_C;
    data_plane_del_sender(eClientType, ptC->tMedia);

    sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 200, reinterpret_cast<const unsigned char *>("OK. Instance is already released."));

    // free instance
    freeClient(ptC);

    return;
}

void handle_request_NOTIFY(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    CONN_SESSION   *ptResp;
    BYTE           *pbMsg;
    
    do
    {
        // each NOTIFY message will be considered to KEEP-ALIVE message.
        ptC->n32TimeGotKA = time( (time_t *)NULL );
        
        // if state already inactive | x | x ..., then we need resume media plane.
        if (ptC->eState == eCLIENT_STATE_INACTIVE) {
            ptC->eState = eCLIENT_STATE_CONNECTED;
            
            // resume media plane
            sdp_process_type_t eClientType = (ptC == &patClient[0]) ? SDP_F : SDP_C;
            data_plane_suspend(eClientType, ptC->tMedia, SUSPEND_OFF);
            
        }
            
        // keep-alive NOTIFY. simply break-out.
        if (0 == ptSession->bMap[EVENT_SEQ_NUM]) {
            break;
        }

        BYTE *pbEvName   = &ptSession->tEvent.bName[0];
        BYTE *pbEvParams = NULL;

        // get name & params
        {
            BYTE   bLocation = 0;
            while(pbEvName[bLocation] != '\0')
            {
                bLocation++;
                if (pbEvName[bLocation] == ' ') {
                    pbEvName[bLocation++]  = '\0';
                    pbEvParams = &pbEvName[bLocation];
                    break;
                }
            }
        }
        
        // client request for self-state
        //if (StrICmp(ptSession->tEvent.bName, (BYTE *)"logon-state")) {
        if (StrICmp(pbEvName, (BYTE *)"logon-state")) {
            BYTE   *pbMsg = &ptSession->bBody[0];
            
            WriteString(&pbMsg, (BYTE *)"State: ", MAX_BODY_LTH);
            WriteString(&pbMsg, abClientState[ptC->eState], MAX_BODY_LTH);
            //&pbMsg++ = '\0';
            
            ptSession->wBdyLth = pbMsg - &ptSession->bBody[0];
            
            break;
        }

        // F request for C-list
        //else if (StrICmp(ptSession->tEvent.bName, (BYTE *)"listener-list")) {
        else if (StrICmp(pbEvName, (BYTE *)"listener-list")) {
            BYTE   *pbMsg = &ptSession->bBody[0];
            BYTE    i;
            BYTE    bString[COMMEN_LTH];

            if (ptC != &patClient[0]) {
            /* requirement update: C CAN request F'state, F CAN request C state-list.  */
                WriteString(&pbMsg, (BYTE *)"1:", COMMEN_LTH);
                WriteString(&pbMsg, (BYTE *)&patClient[0].aName[0], USER_LTH);
                if ('\0' != patClient[0].aID[0]) {
                    WriteString(&pbMsg, (BYTE *)"<", 2);
                    WriteString(&pbMsg, (BYTE *)&patClient[0].aID[0], USER_LTH);
                    WriteString(&pbMsg, (BYTE *)">", 2);
                }
                *pbMsg++ = ':';
                WriteString(&pbMsg, (BYTE *)inet_ntoa( *((struct in_addr *)&patClient[0].tAddrRemoteSignal.ip_addr) ), HOST_NAME_LTH);

                *pbMsg++ = '\r';
                *pbMsg++ = '\n';

                ptSession->wBdyLth = pbMsg - &ptSession->bBody[0];

                break;
            }

            // Now, F request C-list.

            if (!pbEvParams) {
            // only get total number

                BYTE bCnt = 0;
                for (i=1; i<MAX_CONNECT_CLIENT; i++) {
                    if (patClient[i].eState == eCLIENT_STATE_NONE) {
                        continue;
                    }

                    bCnt++;
                }

                // format as:
                //    Total: xx
                WriteString(&pbMsg, (BYTE *)"Total: ", 8);
                sprintf((char*)bString, "%d", bCnt);
                WriteString(&pbMsg, &bString[0], COMMEN_LTH);

                *pbMsg++ = '\r';
                *pbMsg++ = '\n';

            } else {
            // get No.x detail info

                BYTE bIndex = atoi((const char*)pbEvParams);
                if ( (bIndex < 1) || (bIndex >= MAX_CONNECT_CLIENT)) {
                    bIndex = 1;   // No.0 is F self.
                }

                BYTE bCnt = 0;
                BOOL8 blFound = FALSE_B8;

                for (i=1; i<MAX_CONNECT_CLIENT; i++) {
                    if (patClient[i].eState == eCLIENT_STATE_NONE) {
                        continue;
                    }

                    if (++bCnt < bIndex) {
                        continue;
                    }

                    // found
                    blFound = TRUE_B8;
                    break;

                }

                if (blFound) {
                    //     format as: "slot-No.:Name<ID>:IP"
                    // OR: format as: "index-No.:Name<ID>:IP"
                    sprintf((char*)bString, "%d", i);
                    WriteString(&pbMsg, &bString[0], COMMEN_LTH);
                    *pbMsg++ = ':';
                    WriteString(&pbMsg, (BYTE *)&patClient[i].aName[0], USER_LTH);
                    if ('\0' != patClient[i].aID[0]) {
                        WriteString(&pbMsg, (BYTE *)"<", 2);
                        WriteString(&pbMsg, (BYTE *)&patClient[i].aID[0], USER_LTH);
                        WriteString(&pbMsg, (BYTE *)">", 2);
                    }
                    *pbMsg++ = ':';
                    WriteString(&pbMsg, (BYTE *)inet_ntoa( *((struct in_addr *)&patClient[i].tAddrRemoteSignal.ip_addr) ), HOST_NAME_LTH);

                    *pbMsg++ = '\r';
                    *pbMsg++ = '\n';

                } else {
                    // index error, return
                    sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 400, reinterpret_cast<const unsigned char *>("Index not found."));
                    return;
                }

            }

            ptSession->wBdyLth = pbMsg - &ptSession->bBody[0];

            break;

        }

        else {
            // NOT supported event.
            sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 400, reinterpret_cast<const unsigned char *>("Unknown event type."));
            return;
        }
    } while(0);

    // send out response
    ptResp = ptSession;

    memset(&ptResp->bMap[0], 0, sizeof(ptResp->bMap));

    ptResp->bMap[CALLID_SEQ_NUM] = 1;
    ptResp->bMap[CSEQ_SEQ_NUM] = 1;

    // content-length: Automatic done.
    //

    // send response to peer.
    sendResponse(&ptC->tAddrRemoteSignal, ptResp, 200, reinterpret_cast<const unsigned char *>("OK"));

    return;
}

void  setSvrSockfd(UINT32 fd)
{
    m_confs.u32SvrSockfd = fd;
    return;
}


