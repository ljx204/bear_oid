#ifndef __ADPCM_ENCODE_H__
#define __ADPCM_ENCODE_H__
#include "oid_common.h"
#include "bear_audio_encode_task.h"

typedef struct {
    char ckID [4];
    uint32_t ckSize;
    char formType [4];
} RiffChunkHeader;

typedef struct {
    char ckID [4];
    uint32_t ckSize;
} ChunkHeader;

#define ChunkHeaderFormat "4L"

typedef struct {
    uint16_t FormatTag, NumChannels;
    uint32_t SampleRate, BytesPerSecond;
    uint16_t BlockAlign, BitsPerSample;
    uint16_t cbSize;
    union {
        uint16_t ValidBitsPerSample;
        uint16_t SamplesPerBlock;
        uint16_t Reserved;
    } Samples;
    int32_t ChannelMask;
    uint16_t SubFormat;
    char GUID [14];
} WaveHeader;

#define WaveHeaderFormat "SSLLSSSSLS"

typedef struct {
    char ckID [4];
    uint32_t ckSize;
    uint32_t TotalSamples;
} FactHeader;

#define FactHeaderFormat "4LL"

#define WAVE_FORMAT_PCM         0x1
#define WAVE_FORMAT_IMA_ADPCM   0x11
#define WAVE_FORMAT_EXTENSIBLE  0xfffe

typedef struct adpcm_encode_info {
        //WAVContainer_t wav;
	RiffChunkHeader riffhdr;
   	 ChunkHeader datahdr, fmthdr;
    	WaveHeader wavhdr;
    	FactHeader facthdr;
	void * adpcm_cnxt;
}ADPCM_ENCODE_INFO;


int ADPCM_encode_begin(MIXER_RECORD_INFO * p_info);

int ADPCM_encode_stop(MIXER_RECORD_INFO * p_info);

int ADPCM_encode_loop(MIXER_RECORD_INFO * p_info);



#endif
