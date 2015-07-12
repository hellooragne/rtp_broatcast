#ifndef __MEDIA_FILE__
#define __MEDIA_FILE__

#include <stdio.h>
#include <sndfile.h>

SNDFILE *media_file_open(const char *filename, SF_INFO &sf_info);

int  media_file_read(SNDFILE *snd_file, void *buf, int inlen);

#endif
