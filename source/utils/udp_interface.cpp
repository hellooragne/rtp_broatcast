/******************************************************************************
 * C Source    :    
 * Instance    :       
 * Description :     
 * created_by  :   menghao 
 * History     :
 * date_created:   2011/11/ 
 *****************************************************************************/
/*Dependencies.............................................................................*/
#include "udp_interface.h"
/*Constants.............................................................................*/

/*Types.............................................................................*/

#define RCV_BUF_SIZE 65535
/*Globals.............................................................................*/
static uint8_t ip_data[RCV_BUF_SIZE];
static uint8_t recv_buffer[RCV_BUF_SIZE];

static udp_ind_callback udp_ind_cb = NULL;

static map<int, udp_ind_callback> udp_context;


/*Functions.............................................................................*/

/*..........................................................................
 * function name    :   g 
 * Description  	:   g
 * input  parm      :     
 * output parm      : 
 * return parm      :
 * others           :
 *.........................................................................*/

int udp_interface_init(int port, udp_ind_callback udp_ind) {
	int32_t fd = 0;
	struct sockaddr_in	addr;
	if((fd = socket(AF_INET , SOCK_DGRAM , 0)) < 0)
	{
		printf("Can't Open DGRAM socket\n");
		//perror("socket_agent :");
		return -1;
	}

	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags|O_NONBLOCK);

	bzero(&addr , sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	
	if(bind(fd , (struct sockaddr *)(&addr), sizeof(addr)) < 0) {
		printf("Can't Bind(socket_agent)\n");
		//perror("Err socket_agent:");
		close(fd);
		return -1;
	}

	//printf("\nafter bind port %d\n", addr.sin_port);

	udp_context[fd] = udp_ind;
	//udp_ind_cb = udp_ind;
	return fd;
}



int32_t udp_interface_init(uint32_t port) {
	int32_t fd = 0;
	struct sockaddr_in	addr;
	if((fd = socket(AF_INET , SOCK_DGRAM , 0)) < 0)
	{
		printf("Can't Open DGRAM socket\n");
		//perror("socket_agent :");
		return -1;
	}

	bzero(&addr , sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if(bind(fd , (struct sockaddr *)(&addr), sizeof(addr)) < 0) {
		printf("Can't Bind(socket_agent)\n");
		//perror("Err socket_agent:");
		close(fd);
		return -1;
	}

	//printf("\nafter bind port %d\n", addr.sin_port);
	return fd;
}

struct sockaddr_in udp_get_addr(int32_t sock) {
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(sock, (struct sockaddr *)&sin, &len) == -1) {
		perror("getsockname");
	} else {
		printf("port number %d\n", ntohs(sin.sin_port));
		return sin;
	}
}


int udp_interface_send(int32_t sockfd, in_addr_t ip, uint16_t port, uint8_t* data, uint32_t len) {
	struct sockaddr_in client_addr;
	bzero(&client_addr , sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = ip;
	client_addr.sin_port = htons(port);

	if(sendto(sockfd , data, len, 0 , 
				(struct sockaddr *)(&client_addr) , sizeof(client_addr)) == -1) {
		printf("Sendto Fail\n");
		return 0;
	} else {
		//printf("Sendto OK\n");
	}
	return -1;
}


#if 0
int udp_interface_recv(uint8_t* ip, uint16_t len, uint8_t* data) {
	//printf("%s %d %s\n", __FILE__, __LINE__, __FUNCTION__);
	if (udp_ind_cb != NULL) {
		return udp_ind_cb(ip, len, data);
	} else {
		printf("%s %d %s handle is null\n", __FILE__, __LINE__, __FUNCTION__);
	} 
}
#endif

int udp_interface_read() {
	int maxfd = -1;
	struct timeval tv;	/* How long to select() */
	fd_set fds;			/* For select() */
	FD_ZERO(&fds);

#if 0
	if (udp_sock_fd != -1) {
		FD_SET(udp_sock_fd, &fds);
	} else {
		//sys_err(LOG_ERR, __FILE__, __LINE__, 0, "socket fd error");
		return -1;
	}
#endif

	map<int, udp_ind_callback>::iterator it;
	for (it = udp_context.begin(); it != udp_context.end(); it++) {
		FD_SET(it->first, &fds);
	}

	tv.tv_sec  = 0;
	tv.tv_usec = 0;
	switch (select(FD_SETSIZE, &fds, NULL, NULL, &tv)) {
		case -1:	/* errno == EINTR : unblocked signal */
			printf("select return -1 error\n");
			/* On error, select returns without modifying fds */
			//FD_ZERO(&fds);
			return -1;   
		default:
			break;
	}

	for (it = udp_context.begin(); it != udp_context.end(); it++) {
		if (FD_ISSET(it->first, &fds)) {
			struct    sockaddr_in from_addr;
			socklen_t fromlen = sizeof(from_addr);
			int len = 0;
			len = recvfrom(it->first, recv_buffer, RCV_BUF_SIZE , 0 , 
					(struct sockaddr *)(&from_addr) , &fromlen);
			if (len > 0) {
				char *ip = inet_ntoa(from_addr.sin_addr);
				if (ip != NULL) {
					it->second((uint8_t*)ip, len, recv_buffer);
				}
			}
		}
	}
}

//int udp_interface_data_read(int fd, uint8_t *out_data, int out_len) {
int udp_interface_data_read(int fd, string &out_data) {
	int maxfd = -1;
	struct timeval tv;  /* How long to select( */
	fd_set fds;         /* For select( */
	FD_ZERO(&fds);

	FD_SET(fd, &fds);

	tv.tv_sec  = 0;
	tv.tv_usec = 100;
	char tmp[RCV_BUF_SIZE];
	switch (select(FD_SETSIZE, &fds, NULL, NULL, &tv)) {
		case -1:    /* errno == EINTR : unblocked signal */
			printf("select return -1 error\n");
			return -1;
		default:
			break;
	}
	if (FD_ISSET(fd, &fds)) {
		struct    sockaddr_in from_addr;
		socklen_t fromlen = sizeof(from_addr);
		int ret = recvfrom(fd, tmp, RCV_BUF_SIZE, 0, (struct sockaddr *)(&from_addr) , &fromlen);
		if (ret > 0) {
			out_data.assign(tmp, ret);
		} else {
			printf("[%s +%d]warning socket recv overflow", __FILE__, __LINE__);
		}
		return ret;
	}
	return 0;
}


void udp_interface_destory(int udp_sock_fd) {
	close(udp_sock_fd);
}


/*-------------------unit test----------------------*/
int udp_test() {
	uint8_t ip_data[] = {
		0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		0x54,0x00,0x00,0x00
	};
}
