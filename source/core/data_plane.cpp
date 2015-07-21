#include<sys/time.h>

#include <map>
#include <pthread.h>

#include "data_plane.h"
#include "media_file.h"
#include "rtp_process.h"

using namespace std;

typedef struct {
	uint32_t port_start;
	uint32_t port_end;
} data_plane_port_range_t;


static data_plane_port_range_t data_plane_port_range;

static map <uint64_t, data_plane_media_sdp_t> data_plane_f_map;
static map <uint64_t, data_plane_media_sdp_t> data_plane_c_map;

static uint8_t *media_file_buf;
static int      media_file_len; 

int data_plane_init(uint32_t rtp_port_start, uint32_t rtp_port_end, const char *filename) {
	data_plane_port_range.port_start = rtp_port_start;
	data_plane_port_range.port_end = rtp_port_end;
	SF_INFO sf_info;
	SNDFILE *fd = media_file_open(filename, sf_info);
	if (fd != NULL) {
		media_file_buf = NULL;
		media_file_buf = (uint8_t *)malloc(sf_info.frames);
		int file_ret = media_file_read(fd, media_file_buf, sf_info.frames);
		media_file_len = sf_info.frames;
		printf("media file len %lld\n", sf_info.frames);
	} else {
		printf("can not open file\n");
	}
}

const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, in_addr_t d_addr, uint16_t d_port) {

	struct  timeval time_now;
	gettimeofday(&time_now, NULL);
	uint64_t key = time_now.tv_sec * 1000000 + time_now.tv_usec;

	if (sdp_type == SDP_F) {

		data_plane_media_sdp_t media_sdp;
		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;

		data_plane_f_map[key] = media_sdp;

		return media_sdp;

	} else if (sdp_type == SDP_C) {

		data_plane_media_sdp_t media_sdp;
		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;

		data_plane_c_map[key] = media_sdp;
		return media_sdp;
	}
}

int data_plane_del_sender(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp) {

	if (sdp_type == SDP_F) {

		data_plane_f_map.erase(media_sdp.key);

	} else if (sdp_type == SDP_C) {

		data_plane_c_map.erase(media_sdp.key);
	}
}

static void send_hint_sound(data_plane_media_sdp_t sdp) {
	struct  timeval time_start;
	gettimeofday(&time_start, NULL);
	uint8_t *media_index = media_file_buf;
	rtp_process_t rtp_process_tmp;
	int i = 0;

	rtp_process_context_init(&rtp_process_tmp, sdp.d_addr, sdp.d_port);
	while (1) {
		struct  timeval time_now;
		gettimeofday(&time_now, NULL);
		if ((time_now.tv_sec * 1000000 + time_now.tv_usec) >= (20000 + (time_start.tv_sec * 1000000 + time_start.tv_usec))) {
			if (i * 160 <= media_file_len) {
				rtp_process_send(&rtp_process_tmp, media_index + 160 * i, 160);
			} else {
				return;
			}
			i++;
			gettimeofday(&time_start, NULL);
		}
	}
}

static void *data_plane_send_hint_run_thread(void *arg) {
	struct  timeval time_start;
	gettimeofday(&time_start, NULL);

	while (1) {
		/*
		struct  timeval time_now;
		gettimeofday(&time_now, NULL);

		if ((time_now.tv_sec * 1000000 + time_now.tv_usec) >= (2000000 + (time_start.tv_sec * 1000000 + time_start.tv_usec))) {
			if (data_plane_f_map.size() != 0) {
				if (data_plane_c_map.size() == 0) {
					map<uint64_t, data_plane_media_sdp_t>::iterator it = data_plane_f_map.begin();
					while (it != data_plane_f_map.end()) {
						send_hint_sound(it->second);
						++it;
					}
				}
			}

			if (data_plane_c_map.size() != 0) {
				if (data_plane_f_map.size() == 0) {

				}
			}
			gettimeofday(&time_start, NULL);
		}
		*/

		if (data_plane_f_map.size() != 0) {
			if (data_plane_c_map.size() == 0) {
				map<uint64_t, data_plane_media_sdp_t>::iterator it = data_plane_f_map.begin();
				while (it != data_plane_f_map.end()) {
					send_hint_sound(it->second);
					++it;
				}
			}
		}

		if (data_plane_c_map.size() != 0) {
			if (data_plane_f_map.size() == 0) {

			}
		}
		usleep(2000000);
	}
}

static void *data_plane_switch_data_run_thread(void *arg) {

}

int data_plane_run() {

	rtp_prcess_init();

	pthread_t tid;
	int err = pthread_create(&tid, NULL, &data_plane_send_hint_run_thread, NULL);
	if (err != 0) {
		printf("\ncan't create thread :[%s]", strerror(err));
	}
}

/********************************************************************************/
int data_plane_test() {
	data_plane_init(2000, 3000, "./sound/1.wav");
	data_plane_media_sdp_t data_media_sdp = data_plane_add_sender(SDP_F, inet_addr("127.0.0.1"), 88);
	data_plane_run();
	while (1) {
		sleep(1);
	}
}
