#ifndef __data_plane__
#define __data_plane__

#include <stdio.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include <netinet/in.h>
#include <arpa/inet.h>

/**
 *
 */

typedef struct {
	uint64_t key;
	in_addr_t s_addr;	
	in_addr_t d_addr;	
	uint16_t s_port;		
	uint16_t d_port;
} data_plane_media_sdp_t;

typedef enum {
	SDP_F = 0,
	SDP_C = 1,
} sdp_process_type_t;

/**
 * rtp_port_start: rtp port range 
 * rtp_port_end:   rtp port range
 */

int data_plane_init(uint32_t rtp_port_start, uint32_t rtp_port_end, const char *filename);

const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, in_addr_t d_addr, uint16_t d_port);

//const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, struct sockaddr d_addr, uint16_t d_port);

int data_plane_del_sender(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp);

int data_plane_run();

/*********************************************************************************************/

int data_plane_test();

#endif
