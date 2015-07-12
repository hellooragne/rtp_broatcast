#ifndef __udp_interface__h
#define __udp_interface__h

#include <syslog.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>  
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <time.h>

#include <list>
#include <map>

using namespace std;

typedef int (*udp_ind_callback)(uint8_t* ip, uint16_t len, uint8_t* data);

int udp_interface_init(int port, udp_ind_callback udp_ind);
int32_t udp_interface_init(uint32_t port);
int udp_interface_send(int32_t sockfd, in_addr_t ip, uint16_t port, 
        uint8_t* data, uint32_t len);
int udp_interface_read();
int udp_interface_data_read(int fd, string &out_data);
void udp_interface_destory(int udp_sock_fd);
#endif
