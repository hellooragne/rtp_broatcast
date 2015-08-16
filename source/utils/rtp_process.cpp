/*Dependencies.............................................................................*/

#include "udp_interface.h"

#include "rtp_process.h"

using namespace std;

/*Constants.............................................................................*/

/*Types.................................................................................*/

/*Globals...............................................................................*/

static rtp_hdr_t rtp_header;
static int rtp_socket_fd = 0;

/*Functions.............................................................................*/


void rtp_prcess_init() {

	rtp_header.version = 2;
	rtp_header.p  = 0;
	rtp_header.x  = 0;
	rtp_header.cc = 0;
	rtp_header.m  = 0;
	rtp_header.pt = 0;
	rtp_header.seq = 0;
	rtp_header.ts = 0;
	rtp_header.ssrc = 0;

	rtp_socket_fd = udp_interface_init(0);

	udp_get_addr(rtp_socket_fd);
	if (rtp_socket_fd <= 0) {
		printf("rtp socket init error\n");
	}
}

void rtp_process_context_init(rtp_process_t *rtp_process_context, in_addr_t ip, uint16_t port) {

	rtp_process_context->seq = 0;
	rtp_process_context->ip  = ip;
	rtp_process_context->port = port;

}

int rtp_process_set_seq(rtp_process_t *rtp_process_context) {

	rtp_process_context->seq++;
}

int rtp_process_set_head(rtp_process_t *rtp_process_context, uint8_t *data, uint32_t len) {

	memcpy(data, (const void *)&rtp_header, sizeof(rtp_header));
	return sizeof(rtp_header);
}

void rtp_process_send(rtp_process_t *rtp_process_context, uint8_t *data, uint32_t len) {
	uint8_t out[2000];
	rtp_process_set_seq(rtp_process_context);
	rtp_process_set_head(rtp_process_context, out, sizeof(out));
	if (len < (sizeof(out) - sizeof(rtp_header))) {
		memcpy(out + sizeof(rtp_header), data, len);
		udp_interface_send(rtp_socket_fd, rtp_process_context->ip, rtp_process_context->port, out, len + sizeof(rtp_header));
	} else {

	}
}

void rtp_porcess_context_destory(rtp_process_t *rtp_process_context) {

	rtp_process_context->seq = 0;
}

void rtp_porcess_destory() {

	udp_interface_destory(rtp_socket_fd);
}
