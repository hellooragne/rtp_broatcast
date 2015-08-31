#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "types.h"

typedef struct {
    char     aSrvName[64];
    UINT16   u16SrvPort;      // signal port
    UINT16   u16MediPort;     // port for send/recv rtp
    UINT16   u16MaxConn;      // total max connection. 1F+60C = 61
    char     aFName[64];      // F name
    char     aMohCN[128];     // commfortable noise
    char     aMohNoConn[128]; // prompt when no connection to Server. For both F/C.
    
    UINT32   u32KATimer;      // keep-alive (ms)
    UINT32   u32OfflineTimer;  // offline timer (s)
    
	char     media_ip[128];
}global_confs_t;

INT32 load_config(global_confs_t *, BOOL8);

#endif //__CONFIG_H__

