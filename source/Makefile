all:
	#gcc -o broadcast_rtp main.c config.c control_plane.c data_plane.c -I./include 
	g++ -g -o broadcast_rtp  main.cpp core/control_plane.cpp  core/data_plane.cpp utils/media_file.cpp utils/udp_interface.cpp utils/rtp_process.cpp -I./include -I./include/utils -lsndfile -lpthread
