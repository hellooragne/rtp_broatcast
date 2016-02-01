#include<sys/time.h>

#include <map>
#include <pthread.h>
#include <unistd.h>

#include "data_plane.h"
#include "media_file.h"
#include "udp_interface.h"

using namespace std;

typedef struct {
	uint32_t port_start;
	uint32_t port_end;
} data_plane_port_range_t;


static data_plane_port_range_t data_plane_port_range;

static map <int64_t, data_plane_media_sdp_t> data_plane_f_map;
static map <int64_t, data_plane_media_sdp_t> data_plane_c_map;

static uint8_t *media_file_buf = NULL;
static int      media_file_len; 

static global_confs_t data_global_confs;

static pthread_spinlock_t media_map_lock = 0;

int data_plane_init(global_confs_t global_confs) {
	pthread_spin_init(&media_map_lock, PTHREAD_PROCESS_SHARED);
	SF_INFO sf_info;
	syslog(LOG_DEBUG, "open sound file %s\n", global_confs.aMohNoConn);
	SNDFILE *fd = media_file_open(global_confs.aMohNoConn, sf_info);
	if (fd != NULL) {
		media_file_buf = NULL;
		media_file_buf = (uint8_t *)malloc(sf_info.frames * 10);
		int file_ret = media_file_read(fd, media_file_buf, sf_info.frames);
		media_file_len = sf_info.frames;
		syslog(LOG_DEBUG, "file ret %d media file len %lld\n", file_ret, sf_info.frames);
	} else {
		syslog(LOG_DEBUG, "can not open file\n");
	}

	data_global_confs = global_confs;
}

const data_plane_media_sdp_t data_plane_add_sender(sdp_process_type_t sdp_type, in_addr_t d_addr, uint16_t d_port) {
	struct  timeval time_now;
	gettimeofday(&time_now, NULL);
	int64_t key = time_now.tv_sec * 1000000 + time_now.tv_usec;

	//printf("add key %lu\n", key);

	if (sdp_type == SDP_F) {

		data_plane_media_sdp_t media_sdp;

		media_sdp.fd =  udp_interface_init(0);
		struct sockaddr_in s_addr = udp_get_addr(media_sdp.fd);

		media_sdp.s_addr = inet_addr(data_global_confs.media_ip);
		media_sdp.s_port = ntohs(s_addr.sin_port);
		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;
		media_sdp.key    = key;

		media_sdp.is_suspend = SUSPEND_OFF;

		syslog(LOG_DEBUG, "[%s +%d] add [f] key [%llu] src [%s:%d] \n",
				__FILE__, __LINE__,
				key,
				inet_ntoa(*(struct in_addr *)&media_sdp.s_addr), media_sdp.s_port);

		syslog(LOG_DEBUG, "[%s +%d] add [f] key [%llu] dst [%s:%d]\n",
				__FILE__, __LINE__,
				key,
				inet_ntoa(*(struct in_addr *)&media_sdp.d_addr), media_sdp.d_port);


		rtp_process_context_init(&(media_sdp.rtp_process_context), media_sdp.d_addr, media_sdp.d_port);

		pthread_spin_lock(&media_map_lock);

		data_plane_f_map[key] = media_sdp;
		pthread_spin_unlock(&media_map_lock);

		return media_sdp;

	} else if (sdp_type == SDP_C) {

		data_plane_media_sdp_t media_sdp;

		media_sdp.fd =  udp_interface_init(0);
		struct sockaddr_in s_addr = udp_get_addr(media_sdp.fd);

		media_sdp.s_addr = inet_addr(data_global_confs.media_ip);
		media_sdp.s_port = ntohs(s_addr.sin_port);

		media_sdp.d_addr = d_addr;
		media_sdp.d_port = d_port;
		media_sdp.key    = key;
		syslog(LOG_DEBUG, "[%s +%d] add [c] key [%llu] src [%s:%d] \n",
				__FILE__, __LINE__,
				key,
				inet_ntoa(*(struct in_addr *)&media_sdp.s_addr), media_sdp.s_port);

		syslog(LOG_DEBUG, "[%s +%d] add [c] key [%llu] dst [%s:%d]\n",
				__FILE__, __LINE__,
				key,
				inet_ntoa(*(struct in_addr *)&media_sdp.d_addr), media_sdp.d_port);

		media_sdp.is_suspend = SUSPEND_OFF;

		pthread_spin_lock(&media_map_lock);
		data_plane_c_map[key] = media_sdp;
		pthread_spin_unlock(&media_map_lock);
		return media_sdp;
	}
}

int data_plane_del_sender(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp) {

	pthread_spin_lock(&media_map_lock);

	syslog(LOG_DEBUG, "[%s +%d] del key [%llu]\n", __FILE__, __LINE__, media_sdp.key);

	if (sdp_type == SDP_F) {

		int64_t key = media_sdp.key;

		data_plane_f_map.erase(key);

	} else if (sdp_type == SDP_C) {

		data_plane_c_map.erase(media_sdp.key);
	}

	pthread_spin_unlock(&media_map_lock);
}


int data_plane_suspend(sdp_process_type_t sdp_type, const data_plane_media_sdp_t media_sdp, suspend_type_t is_suspend) {

	pthread_spin_lock(&media_map_lock);

	syslog(LOG_DEBUG, "[%s +%d] suspend key [%llu]\n", __FILE__, __LINE__, media_sdp.key);

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
				syslog(LOG_DEBUG, "media Index %p \n", media_index + 160 * i);
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
		pthread_spin_lock(&media_map_lock);
		if (data_plane_f_map.size() != 0) {
			if (data_plane_c_map.size() == 0) {
				map<int64_t, data_plane_media_sdp_t>::iterator it = data_plane_f_map.begin();
				syslog(LOG_DEBUG, "send hint sound to f \n");
				while (it != data_plane_f_map.end()) {
					pthread_spin_unlock(&media_map_lock);
					syslog(LOG_DEBUG, "send hint sound to f src ip [%s] port [%d] dst ip [%s] port [%d]\n",
							inet_ntoa(*(struct in_addr *)&(it->second).s_addr), (it->second).s_port,
							inet_ntoa(*(struct in_addr *)&(it->second).d_addr), (it->second).d_port);
					send_hint_sound(&(it->second));
					pthread_spin_lock(&media_map_lock);
					++it;
				}
			}
		}

		if (data_plane_c_map.size() != 0) {
			if (data_plane_f_map.size() == 0) {
				syslog(LOG_DEBUG, "data plane f size %d\n", data_plane_f_map.size());
				map<int64_t, data_plane_media_sdp_t>::iterator it = data_plane_c_map.begin();
				while (it != data_plane_c_map.end()) {
					pthread_spin_unlock(&media_map_lock);
					syslog(LOG_DEBUG, "send hint sound to c src ip [%s] port [%d] dst ip [%s] port [%d]\n",
							inet_ntoa(*(struct in_addr *)&(it->second).s_addr), (it->second).s_port,
							inet_ntoa(*(struct in_addr *)&(it->second).d_addr), (it->second).d_port);
					send_hint_sound(&(it->second));
					syslog(LOG_DEBUG, "end send sound to c\n");
					pthread_spin_lock(&media_map_lock);
					++it;
				}

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
				map<int64_t, data_plane_media_sdp_t>::iterator it = data_plane_f_map.begin();
				while (it != data_plane_f_map.end()) {
					pthread_spin_unlock(&media_map_lock);

					string out_data;

					int len = udp_interface_data_read(it->second.fd, out_data);

					if (len > 0) {

						syslog(LOG_DEBUG, "data plane recv data  src ip [%s] port [%d] dst ip [%s] port [%d]\n",
									inet_ntoa(*(struct in_addr *)&(it->second).s_addr), (it->second).s_port,
									inet_ntoa(*(struct in_addr *)&(it->second).d_addr), (it->second).d_port);

						pthread_spin_lock(&media_map_lock);
						//send to c

						map<int64_t, data_plane_media_sdp_t>::iterator it_c = data_plane_c_map.begin();

						while (it_c != data_plane_c_map.end()) {

							syslog(LOG_DEBUG, "send sound to c src ip [%s] port [%d] dst ip [%s] port [%d]\n",
									inet_ntoa(*(struct in_addr *)&(it_c->second).s_addr), (it_c->second).s_port,
									inet_ntoa(*(struct in_addr *)&(it_c->second).d_addr), (it_c->second).d_port);

							udp_interface_send(it_c->second.fd, it_c->second.d_addr, it_c->second.d_port, (uint8_t *)out_data.c_str(), out_data.length());

							++it_c;
						}
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
		syslog(LOG_DEBUG, "\ncan't create thread :[%s]", strerror(err));
	}

	pthread_t tid2;
	int err2 = pthread_create(&tid2, NULL, &data_plane_switch_data_run_thread, NULL);
	if (err2 != 0) {
		syslog(LOG_DEBUG, "\ncan't create thread :[%s]", strerror(err2));
	}
}

/********************************************************************************/
int data_plane_test() {
	
	global_confs_t global_confs;
    load_config(&global_confs, FALSE_B8);

	data_plane_init(global_confs);

	data_plane_media_sdp_t data_media_sdp_f = data_plane_add_sender(SDP_F, inet_addr("192.168.1.100"), 88);
	//data_plane_media_sdp_t data_media_sdp_f2 = data_plane_add_sender(SDP_F, inet_addr("192.168.1.101"), 88);
	//printf("test: get server ip %s\n", inet_ntoa(*(struct in_addr *)&data_media_sdp_f.s_addr));


	data_plane_media_sdp_t data_media_sdp_c = data_plane_add_sender(SDP_C, inet_addr("10.32.27.12"), 88);
	//printf("test: get server ip %s\n", inet_ntoa(*(struct in_addr *)&data_media_sdp_c.s_addr));

	//data_plane_del_sender(SDP_F, data_media_sdp_f);

	data_plane_run();

	while (1) {
		sleep(1);
	}
}
