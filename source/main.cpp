#include <stdio.h>
#include "types.h"
#include "config.h"

#include "data_plane.h"


static global_confs_t global_confs;


int main() {
	INT32 n32Result;
	printf("hello broadcast rtp\n");
    
    // load configure data
    //n32Result = load_config(&global_confs, FALSE_B8);
    
	data_plane_test();

	return 0;
}
