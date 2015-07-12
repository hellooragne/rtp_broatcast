#include "media_file.h"

SNDFILE *media_file_open(const char *filename, SF_INFO &sf_info) {
	SNDFILE *snd_file = NULL;
	snd_file = sf_open(filename, SFM_READ, &sf_info); 
	return snd_file;
}

int  media_file_read(SNDFILE *snd_file, void *buf, int inlen) {
	int len = (size_t) sf_read_raw(snd_file, buf, inlen);
	return len;
}
