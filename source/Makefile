all:
	#gcc -o broadcast_rtp main.c config.c control_plane.c data_plane.c -I./include 
	mipsel-openwrt-linux-g++   -ggdb -O0 -fstack-check -g  -o wifi_dispatch_mips  main.cpp core/config.cpp core/control_plane.cpp core/s_codec.cpp core/data_plane.cpp utils/media_file.cpp utils/udp_interface.cpp utils/rtp_process.cpp utils/lib_utils.cpp -I./include -I./include/utils ../sndfile/lib/libsndfile.a -lpthread
	#g++ -g  -o wifi_dispatch_x64  main.cpp core/config.cpp core/control_plane.cpp core/s_codec.cpp core/data_plane.cpp utils/media_file.cpp utils/udp_interface.cpp utils/rtp_process.cpp utils/lib_utils.cpp -I./include -I./include/utils -lsndfile -lpthread
