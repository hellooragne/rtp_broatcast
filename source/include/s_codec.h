#ifndef __P_SIGNAL_CODEC_H__
#define __P_SIGNAL_CODEC_H__

#include <string.h>


//#include "_noalign.h"
//#pragma pack(1)

// special include 
#include "types.h"

#define  ISNOTLWS(a)    ((a!=32)&&(a!=9)&&(a!=13))
#define  TIME_ZONE      8


/* =================================================*/
/* ----------- general defination : BEGIN --------- */
/* =================================================*/
typedef enum
{
    MSG_RQST_CONN = 101,
    MSG_RQST_DISCONN,
    MSG_RQST_RESET,
    MSG_RQST_NOTIFY,
    MSG_RQST_UNKNOWN,
    
    MSG_RESP_1XX = 201,
    MSG_RESP_2XX,
    MSG_RESP_3XX,
    MSG_RESP_4XX,
    MSG_RESP_5XX,
    MSG_RESP_6XX,
    
    MSG_UNKNOWN,
    MSG_MAX,
}MSG_TYPE;

#define MAX_SIGNAL_LTH    1400  /* max length of the request/response */

#define CONNECT_MTD       1  /* CONNECT    Method*/
#define DISCONN_MTD       2  /* DISCONNECT Method*/
#define RESET_MTD         3  /* RESET      Method*/
#define NOTIFY_MTD        4  /* NOTIFY     Method*/

#define COMMEN_LTH        64   /* used for acquire some memory. */
#define METHOD_LTH        16   /* used by multi headers */
#define HNAME_LTH         32   /* header name */

/* host */
#define  HOST_NAME_LTH    32
#define  HOST_PORT_LTH     6


/* URL : sip, sips */
#define USER_LTH          48

typedef struct {
	BYTE       bMap[3];      // attention:  begin with "1"
	BYTE       bUser[USER_LTH];          // 1:
	NODE_ADDR  tHost;
	WORD       wPort;
}URL;


/* =================================================*/
/* ----------- general defination : END   --------- */
/* =================================================*/







/* ================================================*/
/* ----------- header defination : BEGIN --------- */
/* ================================================*/

/* status-line */
#define  REASON_LTH          128

typedef  struct {
	BYTE        bMap[2];
	WORD        wSCode;
	BYTE        bReason[REASON_LTH];
}STATUSLINE;

/* request-line */
typedef  struct {
	BYTE        bMap[2];
	BYTE        bMethod[METHOD_LTH];
	URL         tUrl;
}REQUESTLINE;

/* Call-ID */
#define ID_TOKEN_LTH         48

typedef struct {
	BYTE       bMap[1];
	BYTE       bIDToken[ID_TOKEN_LTH];
}CALLID;

/* Content-Length */
typedef struct {
	BYTE       bMap[1];
	WORD       wCntLength;
}CNTLENGTH;

/* CSeq */
typedef struct {
	BYTE       bMap[2];
	DWORD      dwSeq;
	BYTE       bMethod[METHOD_LTH];
}CSEQ;

/* Event */
#define EVENT_NAME_LTH         48

typedef struct {
	BYTE       bMap[1];
	BYTE       bName[EVENT_NAME_LTH];
}EVENT;

/* From */
typedef struct {
	BYTE       bMap[1];
	BYTE       bName[USER_LTH];
}FROM;

/* Media */
typedef struct {
	BYTE       bMap[1];
	URL        tUrl;
}MEDIA;

/* User-Agent */
typedef struct {
	BYTE       bMap[1];
	BYTE       bDescription[COMMEN_LTH];
}USERAGENT;




/* ================================================*/
/* ----------- header defination : END   --------- */
/* ================================================*/





#define HEAD_NUM       10

typedef enum {
    REQ_LINE_SEQ_NUM = 0x01,
    RESP_LINE_SEQ_NUM,
    
    CALLID_SEQ_NUM,
    CONTENTLENGTH_SEQ_NUM,
    CSEQ_SEQ_NUM,
    EVENT_SEQ_NUM,
    FROM_SEQ_NUM,
    MEDIA_SEQ_NUM,
    USERAGENT_SEQ_NUM,
    
    //UNKNOWNHEADER_SEQ_NUM = 0xf0,
    MAX_SEQ_NUM,
}SEQ_NUM;

typedef struct {
    MSG_TYPE            eType;
    BYTE                bMap[HEAD_NUM];
    
    // start-line
    REQUESTLINE         tRequestLine;
    STATUSLINE          tResponseLine;
    
    
    // headers
    CALLID              tCallID;
    FROM                tFrom;
    CNTLENGTH           tCntLength;
    CSEQ                tCSeq;
    EVENT               tEvent;
    MEDIA               tMedia;
    USERAGENT           tUserAgent;
/*
    //UNKNOWNHDR          tUnknownHdr;
*/
    
}CONN_SESSION;



/*## operation P_ComDecode(BYTE *,WORD,CONN_SESSION *) */
BOOL8 P_ComDecode(BYTE *  pbMsg, WORD  wLth, CONN_SESSION * ptConnSession);

/*## operation P_ComEncode(BYTE *,WORD *,CONN_SESSION *) */
BOOL8 P_ComEncode(BYTE *  pbMsg, WORD *pwLth, CONN_SESSION * ptConnSession);


#endif // __P_SIGNAL_CODEC_H__

