#ifndef __MP3_DECODE_H__
#define __MP3_DECODE_H__
#include "mad.h"




int   mp3_start(void* param);
int   mp3_decode_a_frame(void  * param);
void  mp3_end_play(void * param);


#endif
