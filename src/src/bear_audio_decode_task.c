#include "oid_common.h"





void audio_send_next_frame_q(PMSG_ID_Q hAudioTaskQ)
{
	msgQSend(hAudioTaskQ, MSG_AUD_DECODE_NEXT_FRAME, NULL, 0, MSG_PRI_NORMAL);
}

void audio_send_stop_play(PMSG_ID_Q hAudioTaskQ)
{
	msgQSend(hAudioTaskQ, MSG_AUD_STOP, NULL, 0, MSG_PRI_NORMAL);
}

void audio_send_stop_decode(PMSG_ID_Q hAudioTaskQ, void * param)
{
	msgQSend(hAudioTaskQ, MSG_AUD_STOP_DECODE, param, sizeof(param), MSG_PRI_NORMAL);

}

void * audio_decode_thread(void *param)
{
	INT32S  nRet;

	INT32U  msg_id,wParam[AUDIO_DECODE_Q_MSGLEN/sizeof(INT32U)];

	PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)param;

	switch(p_sound->audio_format) {
		case AUDIO_TYPE_WAV:
			p_sound->p_audio_decode->start_play = wav_start;	
			p_sound->p_audio_decode->loop_decode = wav_decode_a_frame;
			p_sound->p_audio_decode->end_play = wav_end_play;
		break;

		case AUDIO_TYPE_MP3:
			p_sound->p_audio_decode->start_play = mp3_start;
                        p_sound->p_audio_decode->loop_decode = mp3_decode_a_frame;
                        p_sound->p_audio_decode->end_play = mp3_end_play;
		break;
		case AUDIO_TYPE_IMA_ADCPM:
			p_sound->p_audio_decode->start_play = ima_adpcm_start;
                        p_sound->p_audio_decode->loop_decode = ima_adpcm_decode_a_frame;
                        p_sound->p_audio_decode->end_play = ima_adpcm_end_play;

			break;	
		default:
		break;
	}

	
	while(1){
		nRet = msgQReceive(p_sound->p_decode_event, &msg_id, &wParam, sizeof(wParam));
		switch(msg_id) 
		{
			case MSG_AUD_PLAY:
				if(p_sound->p_audio_decode->start_play != NULL ) {
					if(!p_sound->p_audio_decode->start_play(p_sound)) {
						app_end_play_sound(&param);			
					}		
				}
			break;	
			
			case MSG_AUD_DECODE_NEXT_FRAME:
				if(p_sound->play_state == AUDIO_PLAY_STATE) 
				{
					if(p_sound->p_audio_decode->loop_decode != NULL) {
						p_sound->p_audio_decode->loop_decode(p_sound);
					}
				}
			break;
			case MSG_AUD_STOP:
				if(p_sound->p_audio_decode->end_play != NULL ) 
				{
					p_sound->p_audio_decode->end_play(p_sound);
				}
			break;
			case MSG_AUD_STOP_DECODE:
			{
				MSG_INFORMATION * p_info = (MSG_INFORMATION *)wParam[0];
				if(p_info->ack_flag) {
					p_info->ack_callback(p_info);
				}
				
				while(1) {
					oid_testcancel();
					msleep(1000);
				}
			}
				break;
			
			default:
			break;
		

		}

	}


}
