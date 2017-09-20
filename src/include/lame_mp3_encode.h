#ifndef __LAME_MP3_ENCODE_H__
#define __LAME_MP3_ENCODE_H__

#include "oid_common.h"





typedef struct lame_mp3_info {
	lame_t  handle;

//	void * mp3_buffer;
}LAME_MP3_INFO;


int LAME_MP3_encode_begin(MIXER_RECORD_INFO * p_info);

int LAME_MP3_encode_stop(MIXER_RECORD_INFO * p_info);

int LAME_MP3_encode_loop(MIXER_RECORD_INFO * p_info);



#endif
