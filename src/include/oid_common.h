#ifndef  __OID_COMMON_H__
#define  __OID_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <alsa/asoundlib.h> 

#include "Thread.h"
#include "PlaySound.h"
#include "msg_manage.h"
#include "oid_msg.h"
#include "bear_audio_decode_task.h"
#include "bear_audio_dac_task.h"
#include "bear_block.h"
#include "bear_audio_record_task.h"
#include "bear_audio_encode_task.h"
#include "bear_oid_task.h"
#include "bear_socket_task.h"


#ifndef INT64_C
#define INT64_C(c)  c ## LL
#endif

typedef          int       atomic_int;
typedef unsigned int       atomic_uint;
typedef          long      atomic_long;
typedef unsigned long      atomic_ulong;
typedef          long long atomic_llong;
typedef unsigned long long atomic_ullong;

typedef uint32_t vlc_fourcc_t;

#define CLOCK_FREQ INT64_C(1000000)

#define true 	1
#define false 	0

#define OID_SUCCESS        (-0) /**< No error */
#define OID_EGENERIC       (-1) /**< Unspecified error */
#define OID_ENOMEM         (-2) /**< Not enough memory */
#define OID_ETIMEOUT       (-3) /**< Timeout */
#define OID_ENOMOD         (-4) /**< Module not found */
#define OID_ENOOBJ         (-5) /**< Object not found */
#define OID_ENOVAR         (-6) /**< Variable not found */
#define OID_EBADVAR        (-7) /**< Bad variable value */
#define OID_ENOITEM        (-8) /**< Item not found */


#define likely(p)   (!!(p))
#define unlikely(p) (!!(p))

typedef int64_t mtime_t;
typedef int     bool;

typedef enum {
        APP_MSG_NULL = 1,
        APP_MSG_END_PLAY,
	APP_MSG_END_ENCODE,
	APP_MSG_OID_NUMBER,
        APP_MSG_MAX
}APP_MSG;


typedef struct msg_information {
	void (*ack_callback)(struct msg_information * p_info);
	int  ack_flag;
	oid_sem_t  ack_sem;
}MSG_INFORMATION;

typedef enum
{
	BEAR_AUD_FORMAT_PCM       = 0x0001,
	BEAR_AUD_FORMAT_ADPCM     = 0x0002,
	BEAR_AUD_FORMAT_ALAW      = 0x0006,
	BEAR_AUD_FORMAT_MULAW     = 0x0007,
	BEAR_AUD_FORMAT_IMA_ADPCM = 0x0011,
	BEAR_AUD_FORMAT_A1800     = 0x1000,
	BEAR_AUD_FORMAT_MP3	   = 0x2000,
	BEAR_AUD_FORMAT_MAX
} BEAR_AUD_ENC_FORMAT_ENUM;


#define AUDIO_DECODE_Q_SIZE	10
#define AUDIO_DECODE_Q_MSGLEN	12	


#define APP_Q_SIZE		10
#define APP_Q_MSGLEN		12


#define AUDIO_RECORD_Q_SIZE     10
#define AUDIO_RECORD_Q_MSGLEN   12

#define AUDIO_ENCODE_Q_SIZE     10
#define AUDIO_ENCODE_Q_MSGLEN   12

#define SOCKET_Q_SIZE              10
#define SOCKET_Q_MSGLEN            12

#endif
