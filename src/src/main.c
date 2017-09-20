#include "oid_common.h"
//#include "Thread.h"
//#include "msg_manage.h"
//#include "oid_msg.h"
//#include "PlaySound.h"
#include "bear_block.h"

typedef struct app_var{
        PMSG_ID_Q    p_app_event;
        PMSG_ID_Q    p_record_event;
        oid_thread_t record_thread;
	
	oid_thread_t oid_thread;
	oid_thread_t socket_thread;

	int    play_sound_value;
	int    capture_value;
}APP_VAR;

APP_VAR  global_app_var;



//Begin record
void audio_send_open_adc(void * param)
{
        if(global_app_var.p_record_event != NULL) {
                msgQSend(global_app_var.p_record_event, MSG_OPEN_ADC, &param, sizeof(param), MSG_PRI_NORMAL);
	}
}

void audio_send_record_data()
{
        if(global_app_var.p_record_event != NULL) {
                msgQSend(global_app_var.p_record_event, MSG_RECORD_DATA, NULL, 0, MSG_PRI_NORMAL);
        }
}

void audio_send_stop_adc()
{
        if(global_app_var.p_record_event != NULL) {
                msgQSend(global_app_var.p_record_event, MSG_CLOSE_ADC, NULL, 0, MSG_PRI_NORMAL);
        }
}

void oid_send_number(void * param)
{
	msgQSend(global_app_var.p_app_event, APP_MSG_OID_NUMBER, param, sizeof(param), MSG_PRI_NORMAL);
}



void app_init(void)
{
	global_app_var.play_sound_value = 80;

	global_app_var.capture_value = 50;
	mixer_init();
	mixer_set_volume(global_app_var.play_sound_value);
//0 - 16 
	mixer_set_record_volume(global_app_var.capture_value);

	Sound_play_init();

//	oid_clone(&global_app_var.record_thread,  Record_Task_Entry , NULL, OID_THREAD_PRIORITY_INPUT);
}

void app_end_play_sound(void * msg)
{
	msgQSend(global_app_var.p_app_event, APP_MSG_END_PLAY, msg, sizeof(msg), MSG_PRI_NORMAL);
}

void app_end_encode_sound(void * msg)
{

	msgQSend(global_app_var.p_app_event, APP_MSG_END_ENCODE, msg, sizeof(msg), MSG_PRI_NORMAL);

}

//------------------------------------------------------------------------------------------------
void msg_info_callback(MSG_INFORMATION * p_info)
{
	if(p_info->ack_flag) {
		oid_sem_post(&p_info->ack_sem);
	}

}


void msg_info_set_ack_state(MSG_INFORMATION * p_info, int ack_flag)
{

	p_info->ack_flag = ack_flag;

}


void msg_info_init(MSG_INFORMATION * p_info)
{
	oid_sem_init(&p_info->ack_sem, 0);
	p_info->ack_flag = 1;
	p_info->ack_callback = msg_info_callback;
}

void msg_info_wait(MSG_INFORMATION * p_info)
{
	oid_sem_wait(&p_info->ack_sem);
}

void msg_info_destroy(MSG_INFORMATION * p_info)
{
	oid_sem_destroy(&p_info->ack_sem);
}

//*******************************************************************************************************
int main(char argc, char * argv[])
{
	INT32S  nRet;

	int sound_handle = 0;

        INT32U  msg_id;  
	
	INT32U       wParam[APP_Q_MSGLEN/sizeof(INT32U)];

	APP_VAR * p_app = &global_app_var;
	
	app_init();

	p_app->p_app_event = msgQCreate(APP_Q_SIZE, APP_Q_SIZE, APP_Q_MSGLEN);

	if(!p_app->p_app_event) {
		
		printf("Not enough memory 1111 .... \r\n");
		return -1;	
	}

	p_app->p_record_event = msgQCreate(AUDIO_RECORD_Q_SIZE, AUDIO_RECORD_Q_SIZE, AUDIO_RECORD_Q_MSGLEN);
	if(!p_app->p_record_event) {

		printf("Not enough memory 2222 .... \r\n");
		return -1;
	}

	oid_clone(&global_app_var.record_thread,  Record_Task_Entry , p_app->p_record_event , OID_THREAD_PRIORITY_INPUT);
	
	oid_clone(&global_app_var.oid_thread,  OID_Task_Entry , NULL , OID_THREAD_PRIORITY_INPUT);

	oid_clone(&global_app_var.socket_thread,  bear_socket_task , NULL , OID_THREAD_PRIORITY_OUTPUT);


	Sound_play("/tmp/1.mp3");

//	msleep(10000000);

//	Sound_play("/tmp/2.mp3");
#if 0	
	Sound_play("/home/ken/oid_pen/2.mp3");
	msleep(2000000);
	Sound_play("/home/ken/oid_pen/2.wav");
	msleep(2000000);
	Sound_play("/home/ken/oid_pen/1.mp3");	
	msleep(2000000);
	Sound_play("/home/ken/oid_pen/3.mp3");
#endif
//	void * p_mix = Mixer_RecordStart("/home/ken/oid_pen/lame.mp3", BEAR_AUD_FORMAT_MP3, 44100, 256000, 2);
//	void * p_mix = Mixer_RecordStart("/home/ken/oid_pen/adpcm.wav", BEAR_AUD_FORMAT_ADPCM, 22050, 128000, 1);
//	msleep(20000000);
//	if(p_mix != NULL)
//	{
//		Mixer_RecordStop(p_mix);
//	}
//	Mixer_RecordStart("/home/ken/oid_pen/pcm.wav", BEAR_AUD_FORMAT_PCM, 16000, 128000, 1);
	while(1)
	{
		nRet = msgQReceive(p_app->p_app_event, &msg_id, &wParam, sizeof(wParam));

		printf("app msg id  = %d \r\n", msg_id);
		switch(msg_id)
		{
			case APP_MSG_END_PLAY:
			{
			//	int sound_index;

			//	MSG_INFORMATION   * p_msg_info = (MSG_INFORMATION *)malloc(sizeof(MSG_INFORMATION));

				PLAYSOUND_VAR * p_sound = (PLAYSOUND_VAR *)wParam[0];
				Waiting_sound_play_stop(p_sound);
			#if 0	
				msg_info_init(p_msg_info);
				
				audio_send_stop_decode(p_sound->p_decode_event, &p_msg_info);

				msg_info_wait(p_msg_info);

				msg_info_destroy(p_msg_info);

				free(p_msg_info);


				oid_cancel(p_sound->p_decode_thread);
			        oid_join(p_sound->p_decode_thread, NULL );

				close(p_sound->fd);
				msgQDelete(p_sound->p_decode_event);
				free(p_sound->p_audio_decode);
		                block_FifoRelease(p_sound->p_audio_output_fifo);
				sound_index = p_sound->play_sound_table_index;
				
                		free(p_sound);
		//		Sound_play("/home/ken/oid_pen/alarm.wav");
				printf("free play sound ................... \r\n");
			#endif
			}
			break;
			case APP_MSG_END_ENCODE:
			{
				MIXER_RECORD_INFO * p_mixer_info = (MIXER_RECORD_INFO *)wParam[0];
				//close(p_mixer_info->fd_record );
				printf("YYYYYYYY %d, %d, %d  \r\n", p_mixer_info->info.channels, p_mixer_info->info.sample_rate,p_mixer_info->record_length);
				oid_cancel(p_mixer_info->p_encode_thread);
                                oid_join(p_mixer_info->p_encode_thread, NULL );

		                msgQDelete(p_mixer_info->p_encode_event);
				block_FifoRelease(p_mixer_info->p_audio_input_fifo);
               			free(p_mixer_info);
				audio_send_stop_adc();
				printf("End Record ................................................................................. \r\n");
				
			}	
			break;
			case APP_MSG_OID_NUMBER:
			{
				int oid_number = wParam[0];
				printf("APP_MSG_OID_NUMBER : %d \r\n", oid_number);
				if(oid_number == 0x29b3) {
					bear_volume_set(1);
					//if(sound_handle > 0)  Sound_stop(sound_handle);
					//sound_handle = Sound_play("/tmp/1.mp3");
				}
				else if(oid_number == 0x29b4) {
					bear_volume_set(0);
					//if(sound_handle > 0) {
					//	Sound_stop(sound_handle);
					//}
				}
				else {
					bear_send_data_to_socket(&oid_number);
				}	
			}

			break;
			default:
			break;

		}
	}

}

void bear_volume_set(int flag)
{
	if(flag)
	{
		global_app_var.play_sound_value += 10;
		if(global_app_var.play_sound_value >= 99) {
			global_app_var.play_sound_value = 99;
			
		}

	}
	else
	{
		global_app_var.play_sound_value -= 10;
		if(global_app_var.play_sound_value <= 0) {
			global_app_var.play_sound_value = 0;
		}
	}

	mixer_set_volume(global_app_var.play_sound_value);

}


















