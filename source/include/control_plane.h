#ifndef __CONTROL_PLANE_H__
#define __CONTROL_PLANE_H__

#include "types.h"


typedef enum {
    eEV_TYPE_UNKNOWN,
    eEV_TYPE_MSG,
    eEV_TYPE_TIMER_TICK,
    eEV_TYPE_MAX
}eEV_TYPE;

// message to Transaction layer
#define  MAX_MESSAGE_LTH    1400
typedef struct {
    eEV_TYPE          eType;
    
    IP_ADDR_PORT_V4   srcAddr;       // source address
    UINT16            msgLen;
    BYTE              msgBuf[MAX_MESSAGE_LTH];
}S_EV_L1L3_MESSAGE;


BOOL8  control_plane_init(UINT16, UINT16, char *, UINT8, UINT32, UINT32);

void  signal_callback(S_EV_L1L3_MESSAGE *);
void  setSvrSockfd(UINT32);



#endif // __CONTROL_PLANE_H__

