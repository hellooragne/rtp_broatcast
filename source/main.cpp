#include <stdio.h>
#include "types.h"
#include "config.h"
#include "control_plane.h"
#include <time.h>
#include <errno.h>

#include "data_plane.h"

static global_confs_t global_confs;
BOOL8  blWifiRunning = FALSE_B8;

BOOL8 init_socket(UINT32 *);

int main() {
    data_plane_test();
	
    BOOL8  blResult;
	INT32  n32Result;
	printf("hello broadcast rtp\n");
    
    // load data from .conf file
    n32Result = load_config(&global_confs, FALSE_B8);
    
    // init modules
    blResult = control_plane_init(global_confs.u16SrvPort, global_confs.u16MaxConn, 
            global_confs.aFName, strlen(global_confs.aFName), global_confs.u32KATimer, 
            global_confs.u32OfflineTimer );
    if (!blResult) {
        printf("ERROR. control-plane init failure.\n");
        return -1;
    }
    
    // init media plane
    //data_plane_test();
    data_plane_init(2000, 3000, "./sound/1.wav");
    data_plane_run();
    
    
    /**
     *  system running now
     */
    blWifiRunning = TRUE_B8;
    
    // listen socket, process all signals
    
// comment this code after test....
    if (false)
    {
        S_EV_L1L3_MESSAGE    ev;   // message to Transaction layer
    
        memset(&ev, 0, sizeof(ev));
        
        if(false) {
        // test time-tick
            ev.eType = eEV_TYPE_TIMER_TICK;
            signal_callback(&ev);
        }

        //if (false) 
        {
        // test recv a signal message
            INT32  nMsgLth;
/*
CONN wifi01@192.168.0.1
From: "F-007"
CALLID: 129d1446-0063-122c-15aa-001a923f6a0f
CSeq: 1 CONN
Media: 192.168.0.98:54321
User-Agent: WifiClient/0.1
Content-Length: 0
*/
            const char strSignal[] = "CONN wifi01@192.168.0.1\r\nFrom: F-007\r\nCALL-ID: 129d1446-0063-122c-15aa-001a923f6a0f\r\nCSeq: 1 CONN\r\nMedia: 192.168.0.98:54321\r\nUser-Agent: WifiClient/0.1\r\nContent-Length: 0\r\n\r\n";
            nMsgLth = strlen(strSignal);
            strcpy((char *)ev.msgBuf, strSignal);
            //nMsgLth = recvfrom(Sockfd, ev.msgBuf, MAX_LTH, 0, &tPeerAddr, &u32Lth);
            
            ev.eType  = eEV_TYPE_MSG;
            ev.msgLen = nMsgLth;
            //ev.srcAddr.port = htons(9999);
            ev.srcAddr.port = 9999;

            ev.srcAddr.ip_addr = inet_addr("192.168.1.209");

            // * recvieved a Useful Message, send to upper layer
            printf("INFO. send TEST signal to control-plane.\n");
            signal_callback(&ev);
        }
    }
    
    //if (false)
//
    {
        struct sockaddr_in tPeerAddr;
        INT32  nMsgLth, n32Ready;
        UINT32 u32SockLth, u32EXTSockfd;
        INT32  nCheckTime = time( (time_t *)NULL );
        INT32  nCurrentTime;

        fd_set tRset;
        struct timeval tTimeV;

        S_EV_L1L3_MESSAGE    ev;   // message to Transaction layer

        memset(&ev, 0, sizeof(ev));
        memset(&tPeerAddr, 0, sizeof(tPeerAddr));
        
        if (!init_socket(&u32EXTSockfd)) {
            return FALSE_B8;
        };
        
        setSvrSockfd(u32EXTSockfd);

        while( blWifiRunning )
        {
            tTimeV.tv_sec = 1;
            tTimeV.tv_usec = 0;

            FD_ZERO(&tRset);
            FD_SET( u32EXTSockfd, &tRset );
            
            // send clock-tick every 1s
            nCurrentTime = time( (time_t *)NULL );
            if (nCurrentTime - nCheckTime > 1) {
                nCheckTime = nCurrentTime;
                
                ev.eType = eEV_TYPE_TIMER_TICK;
                signal_callback(&ev);
            }

            // get signal from F/C
            if( ( n32Ready = select(u32EXTSockfd+1, &tRset, NULL, NULL, &tTimeV) ) == -1 )
            {
                printf("select() error : errornum = %d\n ", errno);
                continue;    // * next one 
            }

            u32SockLth = sizeof( tPeerAddr );
            
            nMsgLth = recvfrom( u32EXTSockfd, ev.msgBuf, MAX_MESSAGE_LTH,
                                    0, (struct sockaddr *)&tPeerAddr, &u32SockLth);

            if ( nMsgLth == -1 )
            {
                /*if ( nMsgLth != xx )
                    me->m_nSkipedPackets++;*/
            }
            else     /// * recv a Useful msg
            {
                ev.eType  = eEV_TYPE_MSG;
                ev.msgLen = nMsgLth;
                //ev.srcAddr.port = htons(tPeerAddr.sin_port);
                ev.srcAddr.port = tPeerAddr.sin_port;

                memcpy(&ev.srcAddr.ip_addr, &tPeerAddr.sin_addr.s_addr, 4);

                // * recvieved a Useful Message, send to upper layer
                signal_callback(&ev);

                // re-init the event struct
                memset(&tPeerAddr, 0, sizeof(tPeerAddr));
                memset(&ev, 0, sizeof(ev));
            }
        }

    }
    
	return 0;
}



// bind Service socket.
BOOL8 init_socket(UINT32 *svrSockfd)
{
    struct sockaddr_in  tSrvaddr;
    INT32 n32Temp, n32ReturnValue;
    UINT32 svrfd;
    
    n32Temp = 1; 
    
    if (!svrSockfd)
        return FALSE_B8;
        
    svrfd = socket(AF_INET, SOCK_DGRAM,0);
    
    n32Temp = fcntl(svrfd, F_GETFL, 0);
	n32ReturnValue = fcntl(svrfd, F_SETFL, n32Temp | O_NONBLOCK);	
    if ( n32ReturnValue == -1)
    {
        printf("Error. fcntl: EXT socket error = %d", errno);
        return FALSE_B8;
    }
         
    memset( (void*)&tSrvaddr,0,sizeof(tSrvaddr) );
    
    tSrvaddr.sin_family = AF_INET;
    tSrvaddr.sin_addr.s_addr = htonl( INADDR_ANY );

    //* port 59051 is the port receive msg from internet
    tSrvaddr.sin_port = htons( global_confs.u16SrvPort );
    n32ReturnValue = bind(svrfd, (struct sockaddr *)&tSrvaddr, sizeof( tSrvaddr ));
    if ( n32ReturnValue != 0)
    {
        printf("Error. bind: EXT socket error errno = %d", errno);
        return FALSE_B8;
    }
    
    *svrSockfd = svrfd;
    
    return TRUE_B8;
}

