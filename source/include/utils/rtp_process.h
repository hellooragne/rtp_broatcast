#ifndef __RTP_PROCESS_H_
#define __RTP_PROCESS_H_
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdint.h>
#include <string.h>

#include "rtp.h"

typedef struct {
	unsigned int seq;
	in_addr_t ip;
	uint16_t port;
} rtp_process_t;


void rtp_prcess_init();

void rtp_process_context_init(rtp_process_t *rtp_process_context, in_addr_t ip, uint16_t port);

int rtp_process_set_seq(rtp_process_t *rtp_process_context);

int rtp_process_set_head(rtp_process_t *rtp_process_context, uint8_t *data, uint32_t len);

void rtp_process_send(rtp_process_t *rtp_process_context, uint8_t *data, uint32_t len);

void rtp_porcess_context_destory(rtp_process_t *rtp_process_context);

void rtp_porcess_destory();



#endif
