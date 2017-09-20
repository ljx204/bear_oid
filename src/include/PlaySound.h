#ifndef __PLAYSOUND_H__
#define __PLAYSOUND_H__
#include "Thread.h"
#include "oid_msg.h"
#include "bear_block.h"
#include "bear_audio_dac_task.h"

//typedef struct Event 			Event_t;
//typedef struct TSREventQueue		TSREventQueue_t;
typedef struct Audio_decode		Audio_decode_t;
typedef struct Audio_output		Audio_output_t;
///typedef enum     			Audio_type_t;	
//typedef struct SNDPCMContainer 		SNDPCMContainer_t;

typedef struct  SNDPCMContainer {
        snd_pcm_t *handle;
    //    snd_output_t *log;
        snd_pcm_uframes_t chunk_size;
        snd_pcm_uframes_t buffer_size;
        snd_pcm_format_t format;
        uint16_t channels;
        size_t chunk_bytes;
        size_t bits_per_sample;
        size_t bits_per_frame;

  //      uint8_t *data_buf;
    } SNDPCMContainer_t;

typedef enum {
        AUDIO_TYPE_NONE = -1,
        AUDIO_TYPE_WAV,
        AUDIO_TYPE_MP3,
	AUDIO_TYPE_IMA_ADCPM, 
        AUDIO_TYPE_MAX
}AUDIO_TYPE;


typedef enum {
	AUDIO_PLAY_NULL = 1,
	AUDIO_PLAY_STATE,
	AUDIO_PLAY_PAUSE,
	AUDIO_PLAY_RESUME,
	AUDIO_PLAY_STOP,
	AUDIO_WAIT_PLAY_STOP,
	AUDIO_PLAY_SET_PAUSE,
	AUDIO_PLAY_SET_RESUME,
//	AUDIO_WATI_PLAY_STOP,
	AUDIO_PLAY_MAX


}PLAY_STATE;

typedef struct audio_information {
        unsigned short channels;
        int sample_rate;

//IMA_ADPCM need
	int BlockAlign;	  
        int num_samples;

//LAME MP3 need 
	int bit_rate;
}audio_information;

typedef struct {
	
//	TSREventQueue_t * p_decode_event;
	int    fd;
	int    play_mode;
	int    loop_mode;
	audio_information  info;
	AUDIO_TYPE	    audio_format;
	Audio_decode_t  * p_audio_decode;
	Audio_output_t  * p_audio_output;
	oid_thread_t    p_decode_thread;
	oid_thread_t    p_dac_thread;		
	PMSG_ID_Q       p_decode_event;
	block_fifo_t  * p_audio_output_fifo;
	int		play_state;
	oid_mutex_t     play_mutex;
	SNDPCMContainer_t  playback;

	int play_sound_table_index;
}PLAYSOUND_VAR;

typedef enum {
	MSG_AUDIO_PLAY,
}EventType;

typedef struct Event {
	EventType eType;
	union eventData {
		char * p_data;
	}ed;

}Event;

struct TSREventQueue {
	Event  queue[10];
	int    size;
	int    head;
	int    tail;
	oid_mutex_t   mutex;
};


typedef struct Audio_decode {

	int (*start_play)(void * p);
	void (*end_play)(void * p);
	int  (*loop_decode)(void * p);
        void * param;
}Audio_decode;


typedef struct Audio_output {
	snd_pcm_format_t format; 

	int channels;
	int sample_rate;
	void (*open_sound)(void * param);

}Audio_output;
#if 0
typedef enum AUDIO_TYPE {
	AUDIO_TYPE_NONE = -1,
	AUDIO_TYPE_WAV,
	AUDIO_TYPE_MP3,

	AUDIO_TYPE_MAX


}AUDIO_TYPE;
#endif


typedef enum
{
    MSG_AUD_MIN=0,
    MSG_AUD_PLAY,
    MSG_AUD_MIDI_PLAY,
    MSG_AUD_PAUSE,
    MSG_AUD_STOP,
    MSG_AUD_RESUME,
    MSG_AUD_AVI_PLAY,
    MSG_AUD_SET_MUTE,
    MSG_AUD_VOLUME_SET,
    MSG_AUD_SPU_RESTART,
    MSG_AUD_PLAY_RES,
    MSG_AUD_PLAY_SPI_RES,
    MSG_AUD_STOP_RES,
    MSG_AUD_PAUSE_RES,
    MSG_AUD_RESUME_RES,
    MSG_AUD_MUTE_SET_RES,
    MSG_AUD_VOLUME_SET_RES,
    MSG_AUD_DECODE_NEXT_FRAME,
    MSG_AUD_DECODE_START_FRAME,
    MSG_AUD_FORWARD,
    MSG_AUD_STOP_DECODE,
    MSG_AUD_MAX
} MSG_AUD_ENUM;

#define MAX_PLAYSOUND_NUMBER  16 


#endif

