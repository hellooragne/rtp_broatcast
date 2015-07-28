#include "lib_utils.h"
#include "s_codec.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


BOOL8 ReadHCOLON(BYTE **  ppbMessage) 
{
    /* HCOLON: ": " */
    if (**ppbMessage!=':')
    	return FALSE_B8;
    (*ppbMessage)++;  /* ":" */
    
    if (**ppbMessage==' ') {
        (*ppbMessage)++;  /* " " */
    }
    
    return TRUE_B8;
}

BOOL8 DecodeStartLine(REQUESTLINE *ptReq, STATUSLINE *ptResp, BYTE **ppbMessage) 
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    BOOL8 bl8RequestLine = FALSE_B8;
    BOOL8 bl8StatusLine  = FALSE_B8;
    
    /* status-line or request-line */
	if ( ((**ppbMessage>='0')&&(**ppbMessage<='9'))
		&&((*(*ppbMessage+1)>='0')&&(*(*ppbMessage+1)<='9'))
		&&((*(*ppbMessage+2)>='0')&&(*(*ppbMessage+2)<='9'))
		&&(*(*ppbMessage+3)==32) ) {
		bl8StatusLine = TRUE_B8;
		
		bString[0]= *(*ppbMessage)++;
		bString[1]= *(*ppbMessage)++;
		bString[2]= *(*ppbMessage)++;
		bString[3]= '\0';
	} else {
	    bl8RequestLine = TRUE_B8;
	}
	
    if(bl8StatusLine)  /* Status-Line */
    {
    	/* Status Code */
    	ptResp->wSCode = atoi((char*)bString);
    	ptResp->bMap[0] = 1;
    	(*ppbMessage)++;
    
        /* Decode Reason-Phrase */
        bLocation=0;
        while((**ppbMessage!=13)&&(bLocation<REASON_LTH)&&(**ppbMessage!='\0'))
        {
            ptResp->bReason[bLocation++]=*(*ppbMessage)++;
        }
    	if((**ppbMessage!=13)||(*(*ppbMessage+1)!=10)||(**ppbMessage=='\0')
    		||(bLocation==REASON_LTH))
        {
    		return FALSE_B8;
        }
        
        ptResp->bMap[1] = 1;
    
        (*ppbMessage)+=2;  /* \r\n */
    
    	return TRUE_B8;
    }

    if(bl8RequestLine)  /* Request-Line */
    {
    	/* scheme : MEHTOD URL */
    	bLocation=0;
    	while((**ppbMessage!=' ') && (bLocation<METHOD_LTH))
    	{
            ptReq->bMethod[bLocation++]=*(*ppbMessage)++;
    	}
    	if ((**ppbMessage!=' ') || (bLocation==METHOD_LTH))
    	{
    		return FALSE_B8;
    	}
    	ptReq->bMap[0] = 1;
    	(*ppbMessage)++;   /* ' ' */
    	
    	// user
    	bLocation=0;
    	while((**ppbMessage!='@') && (bLocation<USER_LTH))
    	{
            ptReq->tUrl.bUser[bLocation++]=*(*ppbMessage)++;
    	}
    	if ((**ppbMessage!='@') || (bLocation==USER_LTH))
    	{
    		return FALSE_B8;
    	}
    	
        ptReq->bMap[1] = 1;
    	ptReq->tUrl.bMap[0] = 1;
    	(*ppbMessage)++;   /* '@' */

        // host
        bLocation=0;
        while ( (**ppbMessage!='\r')&&(bLocation<NODE_ADDR_LTH)&&(**ppbMessage!='\0') )
        {
        	ptReq->tUrl.tHost.unHost.bHostName[bLocation++]=*(*ppbMessage)++;
        }
        
        if (bLocation >= NODE_ADDR_LTH)
            return FALSE_B8;
            
        ptReq->tUrl.bMap[1] = 1;
        ptReq->tUrl.tHost.eType = eNAT_HOSTNAME;
        
        (*ppbMessage)+=2;  /* \r\n */
        
        return TRUE_B8;
    }
        
        
    return FALSE_B8;
}

BOOL8 DecodeCallID(CALLID *ptCallID, BYTE **ppbMessage)
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    /* uuid */
	bLocation=0;
	while( (**ppbMessage!='\r') && (bLocation<ID_TOKEN_LTH) && (**ppbMessage!='\0') )
	{
        ptCallID->bIDToken[bLocation++]=*(*ppbMessage)++;
	}
	if ((**ppbMessage=='\0') || (bLocation==ID_TOKEN_LTH))
	{
		return FALSE_B8;
	}
	ptCallID->bMap[0] = 1;
	
	(*ppbMessage)+=2;  /* \r\n */
    
    return TRUE_B8;
}

BOOL8 DecodeCntLength(CNTLENGTH *ptCntLth, BYTE **ppbMessage)
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    /* length */
	bLocation=0;
	while( (**ppbMessage!='\r') && (bLocation<5) && (**ppbMessage!='\0') ) // 65535
	{
        bString[bLocation++] = *(*ppbMessage)++;
	}
	if ((**ppbMessage=='\0') || (bLocation==5))
	{
		return FALSE_B8;
	}
	bString[bLocation] = 0;
	
    ptCntLth->bMap[0] = 1;
    ptCntLth->wCntLength = atoi((char*)bString);
	
	(*ppbMessage)+=2;  /* \r\n */
	
    return TRUE_B8;
}

BOOL8 DecodeCSeq(CSEQ *ptCSeq, BYTE **ppbMessage)
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    /* digits */
	bLocation=0;
	while( (**ppbMessage!=' ') && (bLocation<11) && (**ppbMessage!='\0') ) // WORD
	{
        bString[bLocation++] = *(*ppbMessage)++;
	}
	if ((**ppbMessage=='\0') || (bLocation==11))
	{
		return FALSE_B8;
	}
	bString[bLocation] = 0;
	
    ptCSeq->bMap[0] = 1;
    ptCSeq->dwSeq = atoi((char*)bString);
	
	/* method */
	(*ppbMessage)++;   // SP
	
    bLocation=0;
    while( (**ppbMessage!='\r')&&(bLocation<METHOD_LTH)&&(**ppbMessage!='\0') )
    {
        ptCSeq->bMethod[bLocation++] = *(*ppbMessage)++;
    }
    if ((**ppbMessage=='\0') || (bLocation==METHOD_LTH))
    {
    	return FALSE_B8;
    }
    ptCSeq->bMap[1]=1;

    (*ppbMessage)+=2;  /* \r\n */
    
    return TRUE_B8;
}

BOOL8 DecodeEvent(EVENT *ptEvent, BYTE **ppbMessage)
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    /* event name */
	bLocation=0;
	while( (**ppbMessage!='\r') && (bLocation<EVENT_NAME_LTH) && (**ppbMessage!='\0') )
	{
        ptEvent->bName[bLocation++]=*(*ppbMessage)++;
	}
	if ((**ppbMessage=='\0') || (bLocation==EVENT_NAME_LTH))
	{
		return FALSE_B8;
	}
	ptEvent->bMap[0] = 1;
	
	(*ppbMessage)+=2;  /* \r\n */
    
    return TRUE_B8;
}

BOOL8 DecodeFrom(FROM *ptFrom, BYTE **ppbMessage)
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    /* From: */
	// user
	bLocation=0;
	while( (**ppbMessage!='\r') && (bLocation<USER_LTH) && (**ppbMessage!='\0') )
	{
        ptFrom->bName[bLocation++]=*(*ppbMessage)++;
	}
	if ((**ppbMessage=='\0') || (bLocation==USER_LTH))
	{
		return FALSE_B8;
	}
	ptFrom->bMap[0] = 1;
	
	(*ppbMessage)+=2;  /* \r\n */
    
    return TRUE_B8;
}

BOOL8 DecodeMedia(MEDIA *ptMedia, BYTE **ppbMessage) 
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    // host
    bLocation=0;
    while ( (**ppbMessage!=':')&&(bLocation<NODE_ADDR_LTH)&&(**ppbMessage!='\0') )
    {
    	ptMedia->tUrl.tHost.unHost.bHostName[bLocation++]=*(*ppbMessage)++;
    }
    if ((bLocation==NODE_ADDR_LTH) || (**ppbMessage=='\0'))
        return FALSE_B8;
        
    ptMedia->bMap[0] = 1;
    ptMedia->tUrl.bMap[1] = 1;
    ptMedia->tUrl.tHost.eType = eNAT_HOSTNAME;
    
    (*ppbMessage)++;  /* ':' */

    // port
    bLocation=0;
    while ( (**ppbMessage!='\r')&&(bLocation<HOST_PORT_LTH)&&(**ppbMessage!='\0') )
    {
    	bString[bLocation++] = *(*ppbMessage)++;
    }
    if ((bLocation==HOST_PORT_LTH) || (**ppbMessage=='\0'))
	{
		return FALSE_B8;
	}
	bString[bLocation] = 0;
	ptMedia->tUrl.bMap[2] = 1;
	ptMedia->tUrl.wPort = atoi((char*)bString);
	
	(*ppbMessage)+=2;  /* \r\n */

    return TRUE_B8;
}

BOOL8 DecodeUserAgent(USERAGENT *ptUA, BYTE **ppbMessage) 
{
    BYTE   bLocation;
    BYTE   bString[COMMEN_LTH];
    BOOL8  blResult;
    
    if (!ReadHCOLON(ppbMessage)) {
        return FALSE_B8;
    }
    
    // descption
    bLocation=0;
    while ( (**ppbMessage!='\r')&&(bLocation<COMMEN_LTH)&&(**ppbMessage!='\0') )
    {
    	ptUA->bDescription[bLocation++]=*(*ppbMessage)++;
    }
    if ((bLocation==COMMEN_LTH) || (**ppbMessage=='\0'))
        return FALSE_B8;
        
    ptUA->bMap[0] = 1;
    ptUA->bDescription[bLocation]='\0';
    
    (*ppbMessage)+=2;  /* \r\n */

    return TRUE_B8;
}


BOOL8 P_ComDecode(BYTE *pbMsg, WORD wLth, CONN_SESSION *ptConnSession) 
{
    BYTE       bLocation;
    BYTE       bString[COMMEN_LTH];
    BYTE       *pbMsgBegin;
    BOOL8      blResult = FALSE_B8;
    
    memset(ptConnSession, 0, sizeof(*ptConnSession));
    
    pbMsg[wLth] = 0;  /* be sure the Msg end with '\0' */
    pbMsgBegin  = pbMsg;

    /* Decode start-line */
    blResult = DecodeStartLine(&ptConnSession->tRequestLine, &ptConnSession->tResponseLine, &pbMsg);
    if(!blResult) {
        return blResult;
    }
    if (ptConnSession->tRequestLine.bMap[0] == 1) {
        ptConnSession->bMap[REQ_LINE_SEQ_NUM] = 1;
    } else if (ptConnSession->tResponseLine.bMap[0] == 1) {
        ptConnSession->bMap[RESP_LINE_SEQ_NUM] = 1;
    } else {
        // error
    }
    
    getMsgType(ptConnSession);
    
    /* Decode header */
    while( (*pbMsg!=13)&&(*pbMsg!=0) )
    {
    	/* header name */
        bLocation = 0;
        while( (*pbMsg!=':')&&(*pbMsg!=' ')&&(*pbMsg!=0)&&(bLocation<HNAME_LTH) )
        {
    		bString[bLocation++]=*pbMsg++;
        }
    	if( (*pbMsg==0) || (bLocation==HNAME_LTH) )
    	{
    		return FALSE_B8;
        }
    	bString[bLocation] = '\0';
    

    	/* header body */
    
    	/* frequently used */
    	if (StrICmp(bString,(BYTE*)"Call-ID"))
    	{
    		blResult = DecodeCallID(&ptConnSession->tCallID,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[CALLID_SEQ_NUM] = 1;
    		continue;
    	}
    	else if (StrICmp(bString,(BYTE*)"CSeq"))
    	{
    		blResult = DecodeCSeq(&ptConnSession->tCSeq,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[CSEQ_SEQ_NUM] = 1;
    		continue;
    	}
    	else if (StrICmp(bString,(BYTE*)"Media"))
    	{
    		blResult = DecodeMedia(&ptConnSession->tMedia,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[MEDIA_SEQ_NUM] = 1;
    		continue;
    	}
    	else if (StrICmp(bString,(BYTE*)"From"))
    	{
    		blResult = DecodeFrom(&ptConnSession->tFrom,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[FROM_SEQ_NUM] = 1;
    		continue;
    	}
    	else if (StrICmp(bString,(BYTE*)"Event"))
    	{
    		blResult = DecodeEvent(&ptConnSession->tEvent,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[EVENT_SEQ_NUM] = 1;
    		continue;
    	}
    	else if (StrICmp(bString,(BYTE*)"Content-Length"))
    	{
    		blResult = DecodeCntLength(&ptConnSession->tCntLength,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[CONTENTLENGTH_SEQ_NUM] = 1;
    		continue;
    	}
    	else if (StrICmp(bString,(BYTE*)"User-Agent"))
    	{
    		blResult = DecodeUserAgent(&ptConnSession->tUserAgent,&pbMsg);
    		if (!blResult) {
    		    return FALSE_B8;
    		}
    		ptConnSession->bMap[USERAGENT_SEQ_NUM] = 1;
    		continue;
    	}

    	/* unknown header * /
    	else
    	{
    	}
       	*/

    }
    pbMsg+=2;  /* "\r\n" */
    
/*    
    if ( StrICmp(ptConnSession->tCntType.bMType, "multipart") )
    {
        // we had set: "pbMsg[wLth] = 0;" before!
    	if (MAX_SDP_LTH <= ptConnSession->wSDPLth)
    	{
    		SET_ERROR(blResult, SDP_SEQ_NUM, 1, MEDIA_ERR);
    		return;
    	}
    
    	ptConnSession->wSDPLth = strlen(pbMsg);
    	{
    		memcpy(ptConnSession->bSDP, pbMsg, ptConnSession->wSDPLth+1);
    		ptConnSession->bMap[SDP_SEQ_NUM]=1;
    		return;
    	}

    }
*/
    return blResult;
}


BOOL8 P_ComEncode(BYTE *pbMsg, WORD *pwLth, CONN_SESSION *ptConnSession)
{
    BYTE    bString[COMMEN_LTH];

    // pre-process msg-body
    if (ptConnSession->wBdyLth) {
        ptConnSession->bMap[CONTENTLENGTH_SEQ_NUM] = 1;
        ptConnSession->tCntLength.wCntLength = ptConnSession->wBdyLth;
    }
    
    
    // request
    if (ptConnSession->bMap[REQ_LINE_SEQ_NUM])
    {
        /* Method */
    	WriteString(&pbMsg, ptConnSession->tRequestLine.bMethod, METHOD_LTH);
    	*pbMsg++ = ' ';
    
    	/* URL */
    	if (ptConnSession->tRequestLine.tUrl.bMap[1]) {
    	    WriteString(&pbMsg, ptConnSession->tRequestLine.tUrl.bUser, USER_LTH);
    	    
    	    if (ptConnSession->tRequestLine.tUrl.bMap[2])
    	        *pbMsg++ = '@';
    	}
        
        if (ptConnSession->tRequestLine.tUrl.bMap[2]) {
            NODE_ADDR  *ptAddr = &ptConnSession->tRequestLine.tUrl.tHost;
            
            if (eNAT_HOSTNAME == ptAddr->eType) {
                WriteString(&pbMsg, ptAddr->unHost.bHostName, NODE_ADDR_LTH);
            } else if (eNAT_IPV4 == ptAddr->eType) {
            	struct in_addr inaddr;
            	char * pStr;
            	memcpy(&inaddr, &ptAddr->unHost.Ipv4Addr, 4);
            	
            	pStr = inet_ntoa(inaddr);
            	WriteString(&pbMsg, (BYTE *)pStr, 16);
            } else {
                return FALSE_B8;
            }
            
        }
        
        // request-line do not need port. ignore port here.
        
        // CRLF
    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }
    else if (ptConnSession->bMap[RESP_LINE_SEQ_NUM])  /* Response */
    {
    	// Status-Code
    	sprintf((char*)bString, "%d", ptConnSession->tResponseLine.wSCode);
    	WriteString(&pbMsg, bString, 4);
    
    	// * Reason-Phrase
    	if (ptConnSession->tResponseLine.bMap[2])
    	{
    		*pbMsg++ = ' ';
    		WriteString(&pbMsg, ptConnSession->tResponseLine.bReason, REASON_LTH);
    	}
    
    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }
    else
    {
        return FALSE_B8;
    }
    
    
    // CALLID
    if (ptConnSession->bMap[CALLID_SEQ_NUM])  /* Response */
    {
        WriteString(&pbMsg, (BYTE*)"Call-ID: ", 10);
        WriteString(&pbMsg, ptConnSession->tCallID.bIDToken, ID_TOKEN_LTH);
    	
    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }
    
    // CSeq
    if (ptConnSession->bMap[CSEQ_SEQ_NUM])
    {
        WriteString(&pbMsg, (BYTE*)"CSeq: ", 7);
    	sprintf((char*)bString, "%d", ptConnSession->tCSeq.dwSeq);
    	WriteString(&pbMsg, bString, 11);
    
    	*pbMsg++ = ' ';
    	WriteString(&pbMsg, ptConnSession->tCSeq.bMethod, METHOD_LTH);
    	
    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }
    
    // Media
    if (ptConnSession->bMap[MEDIA_SEQ_NUM])
    {
        NODE_ADDR  *ptAddr = &ptConnSession->tMedia.tUrl.tHost;
        
        WriteString(&pbMsg, (BYTE*)"Media: ", 8);
        
        if (eNAT_HOSTNAME == ptAddr->eType) {
            WriteString(&pbMsg, ptAddr->unHost.bHostName, NODE_ADDR_LTH);
        } else if (eNAT_IPV4 == ptAddr->eType) {
        	struct in_addr inaddr;
        	char * pStr;
        	memcpy(&inaddr, &ptAddr->unHost.Ipv4Addr, 4);
        	
        	pStr = inet_ntoa(inaddr);
        	WriteString(&pbMsg, (BYTE *)pStr, 16);
        } else {
            return FALSE_B8;
        }
        
        if (ptConnSession->tMedia.tUrl.bMap[2] == 1) {
            *pbMsg++ = ':';
            
        	sprintf((char*)bString, "%d", ptConnSession->tMedia.tUrl.wPort);
        	WriteString(&pbMsg, bString, 5);
        }

    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }

    // User-Agent
    if (ptConnSession->bMap[USERAGENT_SEQ_NUM])
    {
        WriteString(&pbMsg, (BYTE*)"User-Agent: ", 12);
        WriteString(&pbMsg, ptConnSession->tUserAgent.bDescription, COMMEN_LTH);
    	
    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }
    
    // Content-Length
    if (ptConnSession->bMap[CONTENTLENGTH_SEQ_NUM])
    {
        WriteString(&pbMsg, (BYTE*)"Content-Length: ", 17);
    	sprintf((char*)bString, "%d", ptConnSession->tCntLength.wCntLength);
    	WriteString(&pbMsg, bString, 5);
    	
    	*pbMsg++ = '\r';
    	*pbMsg++ = '\n';
    }
    
	*pbMsg++ = '\r';
	*pbMsg++ = '\n';
	

	// body part
	if (ptConnSession->wBdyLth) {
	    WriteString(&pbMsg, &ptConnSession->bBody[0], MAX_BODY_LTH);
	}
	
	*pbMsg++ = '\0';
	
    return TRUE_B8;
}

void getMsgType(CONN_SESSION *pt)
{
    if (pt->bMap[REQ_LINE_SEQ_NUM]) {
    
        if (StrICmp(pt->tRequestLine.bMethod, (BYTE *)"CONN"))
        	pt->eType = MSG_RQST_CONN;
        else if (StrICmp(pt->tRequestLine.bMethod, (BYTE *)"DISCONN"))
        	pt->eType = MSG_RQST_DISCONN;
        else if (StrICmp(pt->tRequestLine.bMethod, (BYTE *)"RESET"))
        	pt->eType = MSG_RQST_RESET;
        else if (StrICmp(pt->tRequestLine.bMethod, (BYTE *)"NOTIFY"))
        	pt->eType = MSG_RQST_NOTIFY;
        else
        	pt->eType = MSG_RQST_UNKNOWN;
        
    } else if (pt->bMap[RESP_LINE_SEQ_NUM]) {
    
        if (pt->tResponseLine.wSCode/100==1)
        	pt->eType = MSG_RESP_1XX;
        else if (pt->tResponseLine.wSCode/100==2)
        	pt->eType = MSG_RESP_2XX;
        else if (pt->tResponseLine.wSCode/100==3)
        	pt->eType = MSG_RESP_3XX;
        else if (pt->tResponseLine.wSCode/100==4)
        	pt->eType = MSG_RESP_4XX;
        else if (pt->tResponseLine.wSCode/100==5)
        	pt->eType = MSG_RESP_5XX;
        else if (pt->tResponseLine.wSCode/100==6)
        	pt->eType = MSG_RESP_6XX;
        else
        	pt->eType = MSG_UNKNOWN;
        
    } else {
        // 
    }

}
    

BOOL8 WriteString(BYTE **ppbMessage, BYTE *pbStr, WORD wLth) 
{
    BYTE   bLocation;
    
    bLocation=0;
    while( (pbStr[bLocation]!='\0')&&(bLocation<wLth) )
    {
    	*(*ppbMessage)++ = pbStr[bLocation++];
    }
    if(bLocation==wLth)
        return FALSE_B8;
    else
        return TRUE_B8;
    
}




