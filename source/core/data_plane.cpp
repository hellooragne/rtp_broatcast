#include<sys/time.h>

#include <map>
#include <pthread.h>

#include "data_plane.h"
#include "media_file.h"
#include "udp_interface.h"

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

static pthread_spinlock_t media_map_lock = 0;

int data_plane_init(uint32_t rtp_port_start, uint32_t rtp_port_end, const char *filename) {
	pthread_spin_init(&media_map_lock, PTHREAD_PROCESS_SHARED);
	data_plane_port_range.port_start = rtp_port_start;
	data_plane_port_range.port_end = rtp_port_end;
	SF_INFO sf_info;
	SNDFILE *fd = media_file_open(filename, sf_info);
	if (fd != NULL) {
		media_file_buf = NULL;
		media_file_buf = (uint8_t *)malloc(sf_info.frames);
		int file_ret = media_file_read(fd, media_file_buf, sf_info.frames);
		media_file_len = sf_info.frames;
		printf("file ret %d media file len %lld\n", file_ret, sf_info.frames);
	} else {
		printf("can not open file\n");
	}
}

const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, in_addr_t d_addr, uint16_t d_port) {
	struct  timeval time_now;
	gettimeofday(&time_now, NULL);
	uint64_t key = time_now.tv_sec * 1000000 + time_now.tv_usec;

	//printf("add key %lu\n", key);

	if (sdp_type == SDP_F) {

		data_plane_media_sdp_t media_sdp;

		media_sdp.fd =  udp_interface_init(0);
		struct sockaddr_in s_addr = udp_get_addr(media_sdp.fd);

		media_sdp.s_addr = s_addr.sin_addr.s_addr;
		media_sdp.s_port = s_addr.sin_port;
		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;

		media_sdp.is_suspend = SUSPEND_OFF;

		rtp_process_context_init(&(media_sdp.rtp_process_context), media_sdp.d_addr, media_sdp.d_port);

		pthread_spin_lock(&media_map_lock);
		data_plane_f_map[key] = media_sdp;
		pthread_spin_unlock(&media_map_lock);

		return media_sdp;

	} else if (sdp_type == SDP_C) {

		data_plane_media_sdp_t media_sdp;

		media_sdp.fd =  udp_interface_init(0);
		struct sockaddr_in s_addr = udp_get_addr(media_sdp.fd);

		media_sdp.s_addr = s_addr.sin_addr.s_addr;
		media_sdp.s_port = s_addr.sin_port;

		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;

		media_sdp.is_suspend = SUSPEND_OFF;

		pthread_spin_lock(&media_map_lock);
		data_plane_c_map[key] = media_sdp;
		pthread_spin_unlock(&media_map_lock);
		return media_sdp;
	}
}

int data_plane_del_sender(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp) {

	pthread_spin_lock(&media_map_lock);

	printf("del key %llu\n", media_sdp.key);

	if (sdp_type == SDP_F) {

		data_plane_f_map.erase(media_sdp.key);

	} else if (sdp_type == SDP_C) {

		data_plane_c_map.erase(media_sdp.key);
	}

	pthread_spin_unlock(&media_map_lock);
}


int data_plane_suspend(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp, suspend_type_t is_suspend) {

	pthread_spin_lock(&media_map_lock);

	//printf("suspend key %llu\n", media_sdp.key);

	if (sdp_type == SDP_F) {
		if (data_plane_f_map.end() != data_plane_f_map.find(media_sdp.key)) {
			data_plane_f_map[media_sdp.key].is_suspend = is_suspend;
		}
	} else if (sdp_type == SDP_C) {
		if (data_plane_c_map.end() != data_plane_c_map.find(media_sdp.key)) {
			data_plane_c_map[media_sdp.key].is_suspend = is_suspend;
		}
	}

	pthread_spin_unlock(&media_map_lock);
}

static void send_hint_sound(data_plane_media_sdp_t *sdp) {
	struct  timeval time_start;
	gettimeofday(&time_start, NULL);
	uint8_t *media_index = media_file_buf;
	if (sdp->is_suspend == SUSPEND_ON) {
		return;
	}

	int i = 0;

	while (1) {
		struct  timeval time_now;
		gettimeofday(&time_now, NULL);
		if ((time_now.tv_sec * 1000000 + time_now.tv_usec) >= (20000 + (time_start.tv_sec * 1000000 + time_start.tv_usec))) {
			if (i * 160 <= media_file_len) {
				rtp_process_send(sdp->fd, &(sdp->rtp_process_context), media_index + 160 * i, 160);
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

		pthread_spin_lock(&media_map_lock);
		if (data_plane_f_map.size() != 0) {
			if (data_plane_c_map.size() == 0) {
				map<uint64_t, data_plane_media_sdp_t>::iterator it = data_plane_f_map.begin();
				while (it != data_plane_f_map.end()) {
					pthread_spin_unlock(&media_map_lock);
					send_hint_sound(&(it->second));
					pthread_spin_lock(&media_map_lock);
					++it;
				}
			}
		}

		if (data_plane_c_map.size() != 0) {
			if (data_plane_f_map.size() == 0) {

			}
		}
		pthread_spin_unlock(&media_map_lock);

		usleep(2000000);
	}
}

static void *data_plane_switch_data_run_thread(void *arg) {
	while (1) {
		pthread_spin_lock(&media_map_lock);
		if (data_plane_f_map.size() != 0) {
			if (data_plane_c_map.size() != 0) {
				map<uint64_t, data_plane_media_sdp_t>::iterator it = data_plane_f_map.begin();
				while (it != data_plane_f_map.end()) {
					pthread_spin_unlock(&media_map_lock);

					string out_data;
					int len = udp_interface_data_read(it->second.fd, out_data);
					pthread_spin_lock(&media_map_lock);
					//send to c

					map<uint64_t, data_plane_media_sdp_t>::iterator it_c = data_plane_c_map.begin();

					while (it_c != data_plane_c_map.end()) {

						udp_interface_send(it_c->second.fd, it_c->second.d_addr, it_c->second.d_port, (uint8_t *)out_data.c_str(), out_data.length());

						++it_c;
					}

					
					++it;
				}
			}
		}
		pthread_spin_unlock(&media_map_lock);
	}
}

int data_plane_run() {

	rtp_prcess_init();

	pthread_t tid;
	int err = pthread_create(&tid, NULL, &data_plane_send_hint_run_thread, NULL);
	if (err != 0) {
		printf("\ncan't create thread :[%s]", strerror(err));
	}

	pthread_t tid2;
	int err2 = pthread_create(&tid2, NULL, &data_plane_switch_data_run_thread, NULL);
	if (err2 != 0) {
		printf("\ncan't create thread :[%s]", strerror(err2));
	}
}

/********************************************************************************/
int data_plane_test() {
	data_plane_init(2000, 3000, "./sound/1.wav");
	data_plane_media_sdp_t data_media_sdp = data_plane_add_sender(SDP_F, inet_addr("192.168.1.100"), 88);
	data_plane_run();
	while (1) {
		sleep(1);
	}
}
