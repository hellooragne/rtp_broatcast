
#ifndef _TYPES_H
#define _TYPES_H


#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>
#include <time.h>   // add for SIP_DATE  ///ClockStruc

#ifdef __cplusplus
extern "C"{
#endif 


// basic types defined here

#ifdef WIN32
#include <windows.h>
#endif
#ifdef linux
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <fcntl.h>
#endif


//typedef static   LOCAL;

typedef unsigned int    UINT32;
typedef int             INT32;
typedef unsigned short  UINT16;
typedef short           INT16;
typedef unsigned char   UINT8;

typedef long            LONG;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef  float          FLOAT;

#ifdef linux
typedef unsigned int    DWORD;
typedef unsigned char   BOOL; 
typedef void VOID;
#endif

typedef int             RET_CODE;
/*
#define FALSE           0
#define TRUE            1
*/

// this type is good for string parsing
typedef struct _off_len
{
  int      offset;
  int      length;

} OFF_LEN; 

typedef struct
{
	UINT32  low;
	UINT32  high;

} _UINT64;


// the error_code is based on different application

typedef struct _error_info
{
    int    error_code;
    int    error_offset;

} ERROR_INFO;


typedef  unsigned  char         IPV4_ADDR[4];
typedef  unsigned  char         IPV6_ADDR[16];
typedef  unsigned  short        IP_PORT;

typedef struct _ip_address_port
{
    UINT32         ip_addr;
    UINT16         port;
} IP_ADDR_PORT_V4;


/* Node address: IPV4, IPV6, DomainName... */
#define  NODE_ADDR_LTH      32

typedef  union {
	BYTE        bHostName[NODE_ADDR_LTH];
	IPV4_ADDR   Ipv4Addr;
	IPV6_ADDR   Ipv6Addr;
}UNNODE;

typedef enum {
    eNAT_UNKNOWN,
    eNAT_HOSTNAME,
    eNAT_IPV4,
    eNAT_IPV6,
    eNAT_MAX
}eNODE_ADDR_TYPE;

typedef struct{
//	BYTE      bID;      // 1:hostName; 2:IPV4; 3:IPV6
	eNODE_ADDR_TYPE   eType;
	UNNODE            unHost;
}NODE_ADDR;


typedef struct
{
    int           num;
    UINT32        data[5];

} S_ONE_DATA;


typedef struct
{
    int           num;
    UINT32        data1[5];
    UINT32        data2[5];

} S_TWO_DATA;


#define GET_OFFSET(structure, member)      ((int) &(((structure *)0)->member))
#define BYOFFSET(offset, structure, member )  ((char*)offset -(char*) GET_OFFSET(structure, member))

/* C lib functions * /

RET_CODE  ascii_hex_to_long( char  *string,
						     int    string_length,
							 long  *return_value );

RET_CODE  ascii_to_long( char  *string,
                         int    string_length,
                         long  *return_value );

RET_CODE  ascii_to_ulong( char    *string,
                          int      string_length,
                          UINT32  *return_value );


RET_CODE  ascii_to_double_long( char    *string,
                                int      string_length,
                                _UINT64 *return_value );


UINT32 ip_address_to_ulong( char *ptr, UINT32 length );

RET_CODE  ipaddrToHost( char *ptr, UINT32 ipaddr, UINT16 port );
*/

#ifdef WIN32
//int    strncasecmp( const char*, const char *, int );

#define srandom         srand
#define random          rand

#define strcasecmp      stricmp
#define strncasecmp     strnicmp

#endif



#if 0
char   toupper( char );
int    strcasecmp( const char*, const char* );
int    strlen( const char * );
int    strncmp( const char *, const char *, int );
int    strncpy( const char *, const char *, int );
void   memcpy( void *, const void *, int );
void   memset( void *, int, int );
void  *malloc( UINT32 );
void  *calloc( 1, UINT32 );
void   free( void *);
int    sprintf( char*, const char *, ... );
char  *strchr( const char*, char );
int    isdigit( char );
int    ishexdigit( char );
UINT32 random();
#endif




/////////////////////////////////////////////////////////////
// UDP ports are defined here
// NOTE:  these ports will take effect only if they are
//        not defined in the <app.ini>
/////////////////////////////////////////////////////////////

#define TRAP_DEFAULT_REMOTE_PORT      888
#define SNMP_DEFAULT_LOCAL_PORT       999
#define MGCP_DEFAULT_LOCAL_PORT      2427   // MG  default
#define MGCP_DEFAULT_REMOTE_PORT     2727   // MGC default

#define nUDP_SEND_BUF_SIZE_FOR_MGCP  4*1024
#define nUDP_RECV_BUF_SIZE_FOR_MGCP  16*1024

#define nMIN_RTP_PORT                3000
#define nMAX_RTP_PORT                7000

#define SIP_DEFAULT_LOCAL_PORT       (UINT16)5060
#define SIP_DEFAULT_REMOTE_PORT      5060

#define SNMP_DEFAULT_PORT            2700
#define SNMP_TRAP_PORT               162


///////////////////////////////
// add for vc_parser
typedef unsigned char   BOOL8;
#define FALSE_B8  0
#define TRUE_B8   1

#define inet_ntoa_b(inaddr, bTemp) do{ strcpy(bTemp, (BYTE*)inet_ntoa(inaddr)); }while(0);

typedef struct tm ClockStruc;

#ifdef __cplusplus
}
#endif


/*
#include <ascii.h>
#include <app_log.h>
#include <app_timer.h>
#include "filedir.h"
*/

#endif // _TYPES_H



