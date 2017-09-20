#ifndef __IMA_ADPCM_DECODE_H__
#define __IMA_ADPCM_DECODE_H__

int   ima_adpcm_start(void* param);
int   ima_adpcm_decode_a_frame(void  * param);
void  ima_adpcm_end_play(void * param);


#endif
