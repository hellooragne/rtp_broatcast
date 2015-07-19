#include "control_plane.h"
#include "control_plane_pvt.h"
#include "lib_utils.h"

#include <linux/limits.h>
#include <netdb.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



wifi_control_plane_confs_t   m_confs;

/**
 atClient[0]      :   F
 atClient[1-60]   :   C1 - C60
 */
CLIENT_t atClient[MAX_CONNECT_CLIENT];




BOOL8  control_plane_init(UINT16 u16LocalPort, UINT16 u16MaxConn, 
    char *aFName, UINT8 u8FNameLth, UINT32 u32KATimer, UINT32 u32OfflineTimer )
{
    struct hostent  *hp;
    char  hname[PATH_MAX];
    char  *pStrIp;
    
    // get host-ip
    gethostname(hname, PATH_MAX);
    printf("INFO. Host name is :%s\n", hname);
    hp = gethostbyname((char *)&hname[0]);
    pStrIp = inet_ntoa(*((struct in_addr *)(hp->h_addr_list[0])));
    //pStrIp = inet_ntoa(*((struct in_addr *)(hp->h_addr)));
    if (!pStrIp) {
        printf("ERROR. Can NOT get host ip.\n");
    } 
    strcpy(&m_confs.aLocalIp[0], pStrIp);
    printf("INFO. hostip = %s \n", m_confs.aLocalIp);
    
    m_confs.u16LocalPort         = u16LocalPort;
    m_confs.u16MaxConn           = u16MaxConn;
    if (u8FNameLth > USER_LTH)
        u8FNameLth = USER_LTH;
    aFName[u8FNameLth] = '\0';
    strcpy(m_confs.aFName, aFName);
    m_confs.u32KATimer           = u32KATimer;
    m_confs.u32OfflineTimer      = u32OfflineTimer;
    
    memset(&atClient, 0, sizeof(atClient));
    
    return TRUE_B8;
}

void  signal_callback(S_EV_L1L3_MESSAGE *pEv)
{
    BOOL8   blResult;
    
    printf("INFO recv ev.ip=[%d], ev.port=[%d], ev.len=[%d], ev.buf=%s\n", 
        pEv->srcAddr.ip_addr, pEv->srcAddr.port, pEv->msgLen, pEv->msgBuf);
    
    // time-tick event
    if (pEv->eType == eEV_TYPE_TIMER_TICK) {
        handle_req_time_tick();
        return;
    }
    
    if (pEv->eType != eEV_TYPE_MSG) {
        printf("ERROR. Got event bug NOT eEV_TYPE_MSG!!! \n");
        return;
    }
    
    // dispatch signal message
    {
        CONN_SESSION   tSession;
        CLIENT_t      *ptClient;
        BOOL8          blF;
        BYTE          *pName = NULL;
        
        memset(&tSession, 0, sizeof(tSession));
        
        blResult = P_ComDecode(pEv->msgBuf, pEv->msgLen, &tSession);
        if (!blResult) {
            printf("ERROR. Got mal-formed request, ignore. \n");
            
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
                        ptClient = &atClient[0];
                        if (ptClient->eState != eCLIENT_STATE_NONE) {
                            printf("ERROR. there has already another F logged on system, must LOGOUT or RESET first.");
                            sendMgrResponse(&pEv->srcAddr, &tSession, 486, reinterpret_cast<const unsigned char *>("Busy. Already has one F here."));
                            return;
                        }
                        
                        // init F
                        init_client(ptClient, &tSession);
                        
                        dispatch_ev_toF(&tSession);
                    } else {
                        ptClient = getClient();
                        if (!ptClient) {
                            printf("ERROR. System busy, there has no idle C-slot, please try LOGON later.");
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
                
            case MSG_RQST_DISCONN:
            case MSG_RQST_RESET:
            case MSG_RQST_NOTIFY:
                // mis-target request OR instance is already released!
                sendMgrResponse(&pEv->srcAddr, &tSession, 404, reinterpret_cast<const unsigned char *>("Not found. The requested instance does not exist."));
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
        if (atClient[i].eState == eCLIENT_STATE_NONE) {
            continue;
        }
        
        if (StrICmp(pUUID,atClient[i].aUUID)) {
            // found, return
            if (i == 0)
                *pblF = TRUE_B8;
            *ptC = &atClient[i];
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
    
    // only get C client. F is always the first element [0] in atClient.
    for (i=1; i<MAX_CONNECT_CLIENT; i++)
    {
        if (atClient[i].eState == eCLIENT_STATE_NONE) {
            // init this instance
            //memset(ptC, 0, sizeof(*ptC));
            
            return &atClient[i];
        }
        
    }
    
    return NULL;
}

void freeClient(CLIENT_t *ptC)
{
    if (!ptC) {
        printf("ERROR. Free instance with NULL pointer.\n");
        return;
    }
    
    //memset(ptC, 0, sizeof(*ptC));
    ptC->eState = eCLIENT_STATE_NONE;
    return;
}

void init_client(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    BYTE     *pbMsg;
    DWORD    dwIPAddr;
    
    memset(ptC, 0, sizeof(*ptC));
    
    //ptC->eState = eCLIENT_STATE_NONE;
    
    if (ptSession->bMap[FROM_SEQ_NUM] && ptSession->tFrom.bMap[0]) {
        pbMsg = (BYTE *)&ptC->aName[0];
        WriteString(&pbMsg, ptSession->tFrom.bName, USER_LTH);
        *pbMsg = '\0';
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

    printf("INFO. send response to client.");

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
        printf("ERROR. response encoding error.");
        return;
    }

    memset(&tPeerAddr, 0, sizeof(tPeerAddr));
    
    tPeerAddr.sin_family = AF_INET;
    memcpy(&tPeerAddr.sin_addr.s_addr, &ptDest->ip_addr, 4);
    tPeerAddr.sin_port = htons(ptDest->port);

    wReady = sendto(m_confs.u32SvrSockfd, bStrResp, wLen, 0, (struct sockaddr*)&tPeerAddr, sizeof(tPeerAddr));
    if(wReady != wLen)
    {
        printf("ERROR. sendto() return:%d!", wReady);
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
    WORD  wLen;
    BYTE *pbMsg;
    
    CONN_SESSION  *ptResp;
    

    printf("INFO. server manager send response to client.");

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
        printf("ERROR. response encoding error. ignore this response.");
        return;
    }

    memset(&tPeerAddr, 0, sizeof(tPeerAddr));
    
    tPeerAddr.sin_family = AF_INET;
    memcpy(&tPeerAddr.sin_addr.s_addr, &ptDest->ip_addr, 4);
    tPeerAddr.sin_port = htons(ptDest->port);

    wReady = sendto(m_confs.u32SvrSockfd, bStrResp, wLen, 0, (struct sockaddr*)&tPeerAddr, sizeof(tPeerAddr));
    if(wReady != wLen)
    {
        printf("ERROR. sendto() return:%d!", wReady);
        return;
    }

    return;
}

void dispatch_ev_toF(CONN_SESSION *ptSession)
{
    CLIENT_t   *ptC = &atClient[0]; 
    
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
        printf("ERROR. Unsupported request type...");
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
        if (atClient[i].eState == eCLIENT_STATE_NONE) {
            continue;
        }
        
        nDiffSecs = nSecs - atClient[i].n32TimeGotKA;
        if (nDiffSecs > m_confs.u32OfflineTimer) {
            // release this client SCILENTLY...
            memset(&atClient[i], 0, sizeof(atClient[i]));

            // NOTIFY media plane
            // TODO...
        } else if (nDiffSecs > nInactiveSecs) {
            // inactive, NOTIFY media module t stop sending/recieving???
            atClient[i].eState = eCLIENT_STATE_INACTIVE;
            
        }
        
    }
}

void handle_request_CONN(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    //WORD    wMediaPort;
    CONN_SESSION   *ptResp;
    BYTE           *pbMsg;
    

    // TODO...
    // compare target URL is same as our service-url
    
    // save remote-media ip + port
    ptC->tAddrRemoteMedia.ip_addr = INADDR_NONE;
    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[1]) 
    {
        ptC->tAddrRemoteMedia.ip_addr = inet_addr((const char *)ptSession->tMedia.tUrl.tHost.unHost.bHostName);
    }
    if (ptC->tAddrRemoteMedia.ip_addr == INADDR_NONE)
    {
        printf("ERROR. receive CONN request, but Media IP is invalid. Ignore request AND release instance...");
        
        ptC->eState = eCLIENT_STATE_NONE;
        sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 400, reinterpret_cast<const unsigned char *>("Media header error."));
        return;
    }
    
    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[2]) 
    {
        //ptC->tAddrRemoteMedia.port = htons(ptSession->tMedia.tUrl.wPort);
        ptC->tAddrRemoteMedia.port = ptSession->tMedia.tUrl.wPort;
    }
    

    // activate media plane
    sdp_process_type_t eClientType = SDP_C;
    if (ptC == &atClient[0]) {
        eClientType = SDP_F;
    }
    //const data_plane_media_sdp_t   test = data_plane_add_sender(eClientType, *((struct sockaddr *)&ptC->tAddrRemoteMedia.ip_addr), ptC->tAddrRemoteMedia.port);
    //ptC->tMedia = data_plane_add_sender(eClientType, *((struct sockaddr *)&ptC->tAddrRemoteMedia.ip_addr), ptC->tAddrRemoteMedia.port);
    //data_plane_media_sdp_t data_media_sdp = data_plane_add_sender(SDP_F, inet_addr("127.0.0.1"), 88);

    /* useless */
    //ptC->tAddrLocalMedia.ip_addr = inet_addr(&m_confs.aLocalIp[0]);
    ptC->tAddrLocalMedia.ip_addr = inet_addr("192.169.1.1");
    ptC->tAddrLocalMedia.port    = 12345;
    /* */
    
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

        //pStrIp = inet_ntoa( *((struct in_addr *)&ptC->tMedia.s_addr) );
        pStrIp = inet_ntoa( *((struct in_addr *)&ptC->tAddrLocalMedia.ip_addr) );
        if (!pStrIp) {
            printf("ERROR. Can NOT get host ip. Check media plane.\n");
            pStrIp = &m_confs.aLocalIp[0];
            //WriteString(&pbMsg, (BYTE *)&m_confs.aLocalIp[0], NODE_ADDR_LTH);
            //*pbMsg = '\0';
        }
        WriteString(&pbMsg, (BYTE *)pStrIp, NODE_ADDR_LTH);
        pbMsg[0] = '\0';

        ptUrl->bMap[2] = 1;
        //ptUrl->wPort = ptC->tMedia.s_port;
        ptUrl->wPort = ptC->tAddrLocalMedia.port;
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
    UINT32          u32IP   = 0;
    UINT16          u16Port = 0;
    

    // TODO...
    // compare target URL is same as our service-url
    
    // remote-media ip + port
    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[1]) 
    {
        u32IP = inet_addr((const char *)ptSession->tMedia.tUrl.tHost.unHost.bHostName);
    }
    if (u32IP == INADDR_NONE)
    {
        printf("ERROR. receive re-CONN request, but Media IP is invalid. Ignore request...");
        
        sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 400, reinterpret_cast<const unsigned char *>("Media header error."));
        return;
    }

    if (ptSession->bMap[MEDIA_SEQ_NUM] && ptSession->tMedia.bMap[0] && ptSession->tMedia.tUrl.bMap[2]) 
    {
        u16Port = htons(ptSession->tMedia.tUrl.wPort);
    }
    
    // validation check
    // TODO...
    
    // send to media-plane
    if ( (u32IP == ptC->tAddrRemoteMedia.ip_addr) && (u16Port == ptC->tAddrRemoteMedia.ip_addr) ) {
        // just return 200-OK
    }
    else {
        ptC->tAddrRemoteMedia.ip_addr = u32IP;
        ptC->tAddrRemoteMedia.port = u16Port;
        
        // TODO... activate media plane
        //const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, struct sockaddr d_addr, uint16_t d_port);
    }
    
    // send out response
    ptResp = ptSession;

    memset(&ptResp->bMap[0], 0, sizeof(ptResp->bMap));
    
    ptResp->bMap[CALLID_SEQ_NUM] = 1;
    ptResp->bMap[CSEQ_SEQ_NUM] = 1;
    
    // send response to peer.
    sendResponse(&ptC->tAddrRemoteSignal, ptResp, 200, reinterpret_cast<const unsigned char *>("OK"));
    
    return;
}

void handle_request_DISCONN(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    // DIS-CONNECT request
    
    // TODO...
    // compare target URL is same as our service-url
    
    // validation check
    // TODO...
    
    // delete this endpoint from media-plane
    sdp_process_type_t eClientType = SDP_C;
    if (ptC == &atClient[0]) {
        eClientType = SDP_F;
    }
    
    data_plane_del_sender(eClientType, ptC->tMedia);
    
    sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 200, reinterpret_cast<const unsigned char *>("OK"));

    return;
}

void handle_request_RESET(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 501, reinterpret_cast<const unsigned char *>("Not implemented. Try next version"));
    return;
}

void handle_request_NOTIFY(CLIENT_t *ptC, CONN_SESSION *ptSession)
{
    sendMgrResponse(&ptC->tAddrRemoteSignal, ptSession, 501, reinterpret_cast<const unsigned char *>("Not implemented. Try next version"));
    return;
}

void  setSvrSockfd(UINT32 fd)
{
    m_confs.u32SvrSockfd = fd;
    return;
}


