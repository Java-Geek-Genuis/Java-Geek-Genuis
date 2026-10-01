#include "pch.h"
#include "LibretroCore.h"
#include <cstring>
#include <string>
using namespace Platform;
using namespace Windows::Storage;
namespace CommodoreWarpUWP {
LibretroCore^ LibretroCore::Active=nullptr;
LibretroCore::LibretroCore():module(nullptr),setEnvironment(nullptr),init(nullptr),deinit(nullptr),api(nullptr),info(nullptr),av(nullptr),setVideo(nullptr),setAudio(nullptr),setAudioBatch(nullptr),setPoll(nullptr),setState(nullptr),setController(nullptr),reset(nullptr),loadGame(nullptr),unloadGame(nullptr),run(nullptr),keyboard(nullptr),width(0),height(0),pitch(0),fps(60),loaded(false),error(nullptr){memset(pad,0,sizeof(pad));}
LibretroCore::~LibretroCore(){Unload();}
void SetErr(String^& dst,const char* s){std::wstring w;for(const char* p=s;p&&*p;p++)w.push_back((wchar_t)(unsigned char)*p);dst=ref new String(w.c_str());}
bool LibretroCore::LoadCore(String^ dllPath){
 Unload(); module=LoadPackagedLibrary(dllPath->Data(),0); if(!module){SetErr(error,"Could not load packaged VICE core DLL.");return false;}
#define GET(n,t) n=(t)GetProcAddress(module,#n); if(!n){SetErr(error,"VICE DLL missing required libretro export.");Unload();return false;}
 GET(retro_set_environment,setEnvironment);GET(retro_init,init);GET(retro_api_version,api);GET(retro_get_system_info,info);GET(retro_get_system_av_info,av);
 GET(retro_set_video_refresh,setVideo);GET(retro_set_audio_sample,setAudio);GET(retro_set_audio_sample_batch,setAudioBatch);GET(retro_set_input_poll,setPoll);GET(retro_set_input_state,setState);GET(retro_set_controller_port_device,setController);GET(retro_reset,reset);GET(retro_load_game,loadGame);GET(retro_unload_game,unloadGame);GET(retro_run,run);
 Active=this;setEnvironment(&Environment);setVideo(&Video);setAudio(&Audio);setAudioBatch(&Batch);setPoll(&Poll);setState(&State);setController(0,RETRO_DEVICE_JOYPAD);init();return true;
}
bool LibretroCore::LoadGame(String^ path){
 if(!module||!loadGame)return false; std::wstring w(path->Data());std::string p(w.begin(),w.end());gamePath.assign(p.begin(),p.end());gamePath.push_back(0);
 retro_game_info g={};g.path=(const char*)gamePath.data(); if(!loadGame(&g)){SetErr(error,"VICE rejected the selected file.");return false;}
 retro_system_av_info a={};av(&a);width=a.geometry.base_width;height=a.geometry.base_height;pitch=width*4;fps=a.timing.fps>1?a.timing.fps:60;loaded=true;return true;
}
void LibretroCore::RunFrames(int count){if(!loaded)return;for(int i=0;i<count;i++)run();}
void LibretroCore::Reset(){if(loaded&&reset)reset();}
void LibretroCore::Unload(){if(module){if(loaded&&unloadGame)unloadGame();if(deinit)deinit();FreeLibrary(module);}module=nullptr;loaded=false;Active=nullptr;keyboard=nullptr;frame.clear();}
unsigned LibretroCore::Width::get(){return width;} unsigned LibretroCore::Height::get(){return height;} double LibretroCore::Fps::get(){return fps;}
Array<unsigned char>^ LibretroCore::GetFrameCopy(){auto a=ref new Array<unsigned char>((unsigned)frame.size());for(unsigned i=0;i<a->Length;i++)a[i]=frame[i];return a;}
String^ LibretroCore::Error::get(){return error;}
bool LibretroCore::Environment(unsigned cmd,void* data){
 LibretroCore^ c=Active;if(!c)return false;
 if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)return data&&*(retro_pixel_format*)data==RETRO_PIXEL_FORMAT_XRGB8888;
 if(cmd==RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY||cmd==RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY){
  static std::string p;std::wstring w(ApplicationData::Current->LocalFolder->Path->Data());p.assign(w.begin(),w.end());*(const char**)data=p.c_str();return true;
 }
 if(cmd==RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION){*(unsigned*)data=1;return true;}
 if(cmd==RETRO_ENVIRONMENT_SET_CORE_OPTIONS_VERSION)return true;
 if(cmd==RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK){c->keyboard=*(retro_keyboard_event_t*)data;return true;}
 if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE){*(bool*)data=false;return true;}
 if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE){retro_variable* v=(retro_variable*)data;if(v)v->value=nullptr;return false;}
 if(cmd==RETRO_ENVIRONMENT_SET_VARIABLES)return true;
 return false;
}
void LibretroCore::Video(const void* data,unsigned w,unsigned h,size_t p){LibretroCore^ c=Active;if(!c||!data)return;c->width=w;c->height=h;c->pitch=p;c->frame.resize(w*h*4);for(unsigned y=0;y<h;y++)memcpy(c->frame.data()+y*w*4,(const unsigned char*)data+y*p,w*4);}
void LibretroCore::Audio(int16_t,int16_t){} size_t LibretroCore::Batch(const int16_t*,size_t n){return n;} void LibretroCore::Poll(){}
int16_t LibretroCore::State(unsigned,unsigned device,unsigned index,unsigned id){LibretroCore^ c=Active;if(!c||device!=RETRO_DEVICE_JOYPAD||index!=0)return 0;return id<10&&c->pad[id];}
void LibretroCore::Keyboard(bool d,unsigned k,uint32_t ch,uint16_t m){LibretroCore^ c=Active;if(c&&c->keyboard)c->keyboard(d,k,ch,m);}
void LibretroCore::SetPad(unsigned id,bool d){if(id<10)pad[id]=d;}
void LibretroCore::Key(bool d,unsigned k,uint32_t ch){Keyboard(d,k,ch,0);}
}