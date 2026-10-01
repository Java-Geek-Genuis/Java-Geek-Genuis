#pragma once
#include <stdint.h>
#include <stddef.h>
extern "C" {
typedef bool (*retro_environment_t)(unsigned,void*);
typedef void (*retro_video_refresh_t)(const void*,unsigned,unsigned,size_t);
typedef void (*retro_audio_sample_t)(int16_t,int16_t);
typedef size_t (*retro_audio_sample_batch_t)(const int16_t*,size_t);
typedef void (*retro_input_poll_t)(void);
typedef int16_t (*retro_input_state_t)(unsigned,unsigned,unsigned,unsigned);
struct retro_game_info { const char* path; const void* data; size_t size; const char* meta; };
struct retro_system_info { const char* library_name; const char* library_version; const char* valid_extensions; bool need_fullpath; bool block_extract; };
struct retro_game_geometry { unsigned base_width,base_height,max_width,max_height; float aspect_ratio; };
struct retro_system_timing { double fps,sample_rate; };
struct retro_system_av_info { retro_game_geometry geometry; retro_system_timing timing; };
struct retro_variable { const char* key; const char* value; };
enum retro_pixel_format { RETRO_PIXEL_FORMAT_0RGB1555=0, RETRO_PIXEL_FORMAT_XRGB8888=1, RETRO_PIXEL_FORMAT_RGB565=2 };
enum { RETRO_DEVICE_NONE=0, RETRO_DEVICE_JOYPAD=1 };
enum { RETRO_DEVICE_ID_JOYPAD_B=0,RETRO_DEVICE_ID_JOYPAD_Y=1,RETRO_DEVICE_ID_JOYPAD_SELECT=2,RETRO_DEVICE_ID_JOYPAD_START=3,RETRO_DEVICE_ID_JOYPAD_UP=4,RETRO_DEVICE_ID_JOYPAD_DOWN=5,RETRO_DEVICE_ID_JOYPAD_LEFT=6,RETRO_DEVICE_ID_JOYPAD_RIGHT=7,RETRO_DEVICE_ID_JOYPAD_A=8,RETRO_DEVICE_ID_JOYPAD_X=9 };
enum { RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY=9,RETRO_ENVIRONMENT_SET_PIXEL_FORMAT=10,RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK=12,RETRO_ENVIRONMENT_GET_VARIABLE=15,RETRO_ENVIRONMENT_SET_VARIABLES=16,RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE=17,RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY=31,RETRO_ENVIRONMENT_SET_CORE_OPTIONS_VERSION=52,RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION=53 };
enum retro_key { RETROK_BACKSPACE=8,RETROK_RETURN=13,RETROK_ESCAPE=27,RETROK_SPACE=32,RETROK_A='a',RETROK_B='b',RETROK_C='c',RETROK_D='d',RETROK_E='e',RETROK_F='f',RETROK_G='g',RETROK_H='h',RETROK_I='i',RETROK_J='j',RETROK_K='k',RETROK_L='l',RETROK_M='m',RETROK_N='n',RETROK_O='o',RETROK_P='p',RETROK_Q='q',RETROK_R='r',RETROK_S='s',RETROK_T='t',RETROK_U='u',RETROK_V='v',RETROK_W='w',RETROK_X='x',RETROK_Y='y',RETROK_Z='z' };
typedef void (*retro_keyboard_event_t)(bool,unsigned,uint32_t,uint16_t);
typedef void (*retro_set_environment_t)(retro_environment_t);
typedef void (*retro_init_t)(void); typedef void (*retro_deinit_t)(void); typedef unsigned (*retro_api_version_t)(void);
typedef void (*retro_get_system_info_t)(retro_system_info*); typedef void (*retro_get_system_av_info_t)(retro_system_av_info*);
typedef void (*retro_set_video_refresh_t)(retro_video_refresh_t); typedef void (*retro_set_audio_sample_t)(retro_audio_sample_t); typedef void (*retro_set_audio_sample_batch_t)(retro_audio_sample_batch_t);
typedef void (*retro_set_input_poll_t)(retro_input_poll_t); typedef void (*retro_set_input_state_t)(retro_input_state_t); typedef void (*retro_set_controller_port_device_t)(unsigned,unsigned);
typedef void (*retro_reset_t)(void); typedef bool (*retro_load_game_t)(const retro_game_info*); typedef void (*retro_unload_game_t)(void); typedef void (*retro_run_t)(void);
}
