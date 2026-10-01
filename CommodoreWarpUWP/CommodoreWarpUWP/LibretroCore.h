#pragma once
#include "libretro.h"
#include <windows.h>
#include <vector>
namespace CommodoreWarpUWP {
public ref class LibretroCore sealed {
public:
 LibretroCore(); virtual ~LibretroCore();
 bool LoadCore(Platform::String^ dllPath); bool LoadGame(Platform::String^ path); void Reset(); void RunFrames(int count); void Unload();
 property unsigned Width{unsigned get();} property unsigned Height{unsigned get();} property double Fps{double get();} property Platform::String^ Error{Platform::String^ get();};
 Platform::Array<unsigned char>^ GetFrameCopy(); void SetPad(unsigned id,bool down); void Key(bool down,unsigned key,uint32_t ch);
private:
 HMODULE module; retro_set_environment_t setEnvironment; retro_init_t init; retro_deinit_t deinit; retro_api_version_t api; retro_get_system_info_t info; retro_get_system_av_info_t av;
 retro_set_video_refresh_t setVideo; retro_set_audio_sample_t setAudio; retro_set_audio_sample_batch_t setAudioBatch; retro_set_input_poll_t setPoll; retro_set_input_state_t setState;
 retro_set_controller_port_device_t setController; retro_reset_t reset; retro_load_game_t loadGame; retro_unload_game_t unloadGame; retro_run_t run;
 retro_keyboard_event_t keyboard; std::vector<unsigned char> frame; std::vector<unsigned char> gamePath;
 unsigned width,height; size_t pitch; double fps; bool loaded,pad[10]; Platform::String^ error;
 static bool Environment(unsigned,void*); static void Video(const void*,unsigned,unsigned,size_t); static void Audio(int16_t,int16_t); static size_t Batch(const int16_t*,size_t); static void Poll(); static int16_t State(unsigned,unsigned,unsigned,unsigned); static void Keyboard(bool,unsigned,uint32_t,uint16_t);
}; }