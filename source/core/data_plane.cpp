#include<sys/time.h>

#include <map>
#include <pthread.h>

#include "data_plane.h"

using namespace std;

typedef struct {
	uint32_t port_start;
	uint32_t port_end;
} data_plane_port_range_t;


static data_plane_port_range_t data_plane_port_range;

static map <uint64_t, data_plane_media_sdp_t> data_plane_f_map;
static map <uint64_t, data_plane_media_sdp_t> data_plane_c_map;


int data_plane_init(uint32_t rtp_port_start, uint32_t rtp_port_end) {

	data_plane_port_range.port_start = rtp_port_start;
	data_plane_port_range.port_end = rtp_port_end;
}

const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, struct sockaddr d_addr, uint16_t d_port) {

	struct  timeval time_now;
	uint64_t key;
	gettimeofday(&time_now, NULL);
	key = time_now.tv_sec * 1000000 + time_now.tv_usec;

	if (sdp_type == SDP_SENDER) {

		data_plane_media_sdp_t media_sdp;
		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;

		data_plane_f_map[key] = media_sdp;

		return media_sdp;

	} else if (sdp_type == SDP_RECEIVER) {

		data_plane_media_sdp_t media_sdp;
		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;

		data_plane_c_map[key] = media_sdp;
		return media_sdp;
	}
}

int data_plane_del_sender(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp) {

	if (sdp_type == SDP_SENDER) {

		data_plane_f_map.erase(media_sdp.key);

	} else if (sdp_type == SDP_RECEIVER) {

		data_plane_c_map.erase(media_sdp.key);
	}
}


static void *data_plane_run_thread(void *arg) {

}

int data_plane_run() {

	pthread_t tid;
	int err = pthread_create(&tid, NULL, &data_plane_run_thread, NULL);
	if (err != 0) {
		printf("\ncan't create thread :[%s]", strerror(err));
	}
}
