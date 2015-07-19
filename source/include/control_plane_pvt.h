#ifndef __CONTROL_PLANE_PVT_H__
#define __CONTROL_PLANE_PVT_H__

#include "types.h"
#include "s_codec.h"
#include <time.h>


/**
    internal use only, DO NOT Use this h-file.
*/

typedef enum {
    eCLIENT_STATE_NONE,
    eCLIENT_STATE_CONNECTED,
    eCLIENT_STATE_INACTIVE,
    eCLIENT_STATE_LOST,
    eCLIENT_STATE_MAX
}eCLIENT_STATE;

typedef struct {
    UINT32   u32SvrSockfd;      // server sock fd, used to send out event.
    
    char     aLocalIp[15];      // string. server ip "192.168.1.1"
    UINT16   u16LocalPort;      // signal port
    
    UINT16   u16MaxConn;        // total max connection. 1F+60C = 61
    char     aFName[USER_LTH];        // F name
    
    UINT32   u32KATimer;        // keep-alive (ms)
    UINT32   u32OfflineTimer;   // offline timer (s)
    
}wifi_control_plane_confs_t;


typedef struct {
    eCLIENT_STATE    eState;
    
    char             aName[USER_LTH];
    IP_ADDR_PORT_V4  tAddrRemoteSignal;        // client's singal socket // net-addr type
    IP_ADDR_PORT_V4  tAddrLocalMedia;          // server's media socket  // host-addr type
    IP_ADDR_PORT_V4  tAddrRemoteMedia;         // client's media socket  // net-addr type
    
    // Session
    BYTE             aUUID[ID_TOKEN_LTH];
    DWORD            dwSeq;
    BYTE             bMethod[METHOD_LTH];   // current transaction
    
    // keep-alive
    INT32            n32TimeGotKA;
    
}CLIENT_t;


#define  MAX_CONNECT_CLIENT   64


void locateClientByUUID(BYTE *, BYTE *, BOOL8 *, CLIENT_t **);
CLIENT_t * getClient();
void freeClient(CLIENT_t *);
void init_client(CLIENT_t *, CONN_SESSION *);

void dispatch_ev_toF(CONN_SESSION *);
void dispatch_ev_toC(CLIENT_t *, CONN_SESSION *);

void handle_req_time_tick();
void handle_request_CONN(CLIENT_t *, CONN_SESSION *);
void handle_request_reCONN(CLIENT_t *, CONN_SESSION *);
void handle_request_DISCONN(CLIENT_t *, CONN_SESSION *);
void handle_request_NOTIFY(CLIENT_t *, CONN_SESSION *);
void handle_request_RESET(CLIENT_t *, CONN_SESSION *);

void sendResponse(IP_ADDR_PORT_V4 *, CONN_SESSION *, DWORD, const BYTE *);
void sendMgrResponse(IP_ADDR_PORT_V4 *, CONN_SESSION *, DWORD, const BYTE *);

#endif // __CONTROL_PLANE_PVT_H__

