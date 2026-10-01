#include "pch.h"
#include "LibretroCore.h"
#include <cstring>
#include <string>
using namespace Platform;
using namespace Windows::Storage;

namespace CommodoreWarpUWP {
static LibretroCore^ g_active = nullptr;

LibretroCore::LibretroCore()
 : module(nullptr),setEnvironment(nullptr),init(nullptr),deinit(nullptr),api(nullptr),info(nullptr),av(nullptr),
   setVideo(nullptr),setAudio(nullptr),setAudioBatch(nullptr),setPoll(nullptr),setState(nullptr),
   setController(nullptr),reset(nullptr),loadGame(nullptr),unloadGame(nullptr),run(nullptr),
   keyboard(nullptr),width(0),height(0),pitch(0),fps(60),loaded(false),error(nullptr)
{
    memset(pad,0,sizeof(pad));
}
LibretroCore::~LibretroCore(){Unload();}

static void SetErr(String^& dst,const char* s)
{
    std::wstring w;
    for(const char* p=s;p&&*p;++p) w.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*p)));
    dst=ref new String(w.c_str());
}

bool LibretroCore::LoadCore(String^ dllPath)
{
    Unload();
    module=LoadPackagedLibrary(dllPath->Data(),0);
    if(!module){SetErr(error,"Could not load packaged VICE core DLL.");return false;}

    setEnvironment=reinterpret_cast<retro_set_environment_t>(GetProcAddress(module,"retro_set_environment"));
    init=reinterpret_cast<retro_init_t>(GetProcAddress(module,"retro_init"));
    api=reinterpret_cast<retro_api_version_t>(GetProcAddress(module,"retro_api_version"));
    info=reinterpret_cast<retro_get_system_info_t>(GetProcAddress(module,"retro_get_system_info"));
    av=reinterpret_cast<retro_get_system_av_info_t>(GetProcAddress(module,"retro_get_system_av_info"));
    setVideo=reinterpret_cast<retro_set_video_refresh_t>(GetProcAddress(module,"retro_set_video_refresh"));
    setAudio=reinterpret_cast<retro_set_audio_sample_t>(GetProcAddress(module,"retro_set_audio_sample"));
    setAudioBatch=reinterpret_cast<retro_set_audio_sample_batch_t>(GetProcAddress(module,"retro_set_audio_sample_batch"));
    setPoll=reinterpret_cast<retro_set_input_poll_t>(GetProcAddress(module,"retro_set_input_poll"));
    setState=reinterpret_cast<retro_set_input_state_t>(GetProcAddress(module,"retro_set_input_state"));
    setController=reinterpret_cast<retro_set_controller_port_device_t>(GetProcAddress(module,"retro_set_controller_port_device"));
    reset=reinterpret_cast<retro_reset_t>(GetProcAddress(module,"retro_reset"));
    loadGame=reinterpret_cast<retro_load_game_t>(GetProcAddress(module,"retro_load_game"));
    unloadGame=reinterpret_cast<retro_unload_game_t>(GetProcAddress(module,"retro_unload_game"));
    run=reinterpret_cast<retro_run_t>(GetProcAddress(module,"retro_run"));

    if(!setEnvironment||!init||!api||!info||!av||!setVideo||!setAudio||!setAudioBatch||
       !setPoll||!setState||!setController||!reset||!loadGame||!unloadGame||!run)
    {
        SetErr(error,"VICE DLL is missing one or more required libretro exports.");
        Unload();
        return false;
    }

    g_active=this;
    setEnvironment(&Environment);
    setVideo(&Video);
    setAudio(&Audio);
    setAudioBatch(&Batch);
    setPoll(&Poll);
    setState(&State);
    setController(0,RETRO_DEVICE_JOYPAD);
    init();
    return true;
}

bool LibretroCore::LoadGame(String^ path)
{
    if(!module||!loadGame)return false;
    std::wstring w(path->Data());
    std::string p(w.begin(),w.end());
    gamePath.assign(p.begin(),p.end());
    gamePath.push_back(0);
    retro_game_info g={};
    g.path=reinterpret_cast<const char*>(gamePath.data());
    if(!loadGame(&g)){SetErr(error,"VICE rejected the selected file.");return false;}
    retro_system_av_info a={};
    av(&a);
    width=a.geometry.base_width;height=a.geometry.base_height;pitch=width*4;
    fps=a.timing.fps>1?a.timing.fps:60;loaded=true;return true;
}

void LibretroCore::RunFrames(int count){if(!loaded||!run)return;for(int i=0;i<count;++i)run();}
void LibretroCore::Reset(){if(loaded&&reset)reset();}

void LibretroCore::Unload()
{
    if(module)
    {
        if(loaded&&unloadGame)unloadGame();
        if(deinit)deinit();
        FreeLibrary(module);
    }
    module=nullptr;loaded=false;g_active=nullptr;keyboard=nullptr;frame.clear();gamePath.clear();
    width=0;height=0;
}
unsigned LibretroCore::Width::get(){return width;}
unsigned LibretroCore::Height::get(){return height;}
double LibretroCore::Fps::get(){return fps;}

Array<unsigned char>^ LibretroCore::GetFrameCopy()
{
    auto a=ref new Array<unsigned char>(static_cast<unsigned>(frame.size()));
    for(unsigned i=0;i<a->Length;++i)a[i]=frame[i];
    return a;
}
String^ LibretroCore::Error::get(){return error;}

bool LibretroCore::Environment(unsigned cmd,void* data)
{
    LibretroCore^ c=g_active;
    if(!c)return false;
    if(cmd==RETRO_ENVIRONMENT_SET_PIXEL_FORMAT)
        return data&&*reinterpret_cast<retro_pixel_format*>(data)==RETRO_PIXEL_FORMAT_XRGB8888;
    if(cmd==RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY||cmd==RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY)
    {
        static std::string p;
        std::wstring w(ApplicationData::Current->LocalFolder->Path->Data());
        p.assign(w.begin(),w.end());
        *reinterpret_cast<const char**>(data)=p.c_str();
        return true;
    }
    if(cmd==RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION){*reinterpret_cast<unsigned*>(data)=1;return true;}
    if(cmd==RETRO_ENVIRONMENT_SET_CORE_OPTIONS_VERSION)return true;
    if(cmd==RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK){c->keyboard=*reinterpret_cast<retro_keyboard_event_t*>(data);return true;}
    if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE){*reinterpret_cast<bool*>(data)=false;return true;}
    if(cmd==RETRO_ENVIRONMENT_GET_VARIABLE){retro_variable* v=reinterpret_cast<retro_variable*>(data);if(v)v->value=nullptr;return false;}
    if(cmd==RETRO_ENVIRONMENT_SET_VARIABLES)return true;
    return false;
}

void LibretroCore::Video(const void* data,unsigned w,unsigned h,size_t p)
{
    LibretroCore^ c=g_active;
    if(!c||!data)return;
    c->width=w;c->height=h;c->pitch=p;c->frame.resize(w*h*4);
    for(unsigned y=0;y<h;++y)
        memcpy(c->frame.data()+y*w*4,reinterpret_cast<const unsigned char*>(data)+y*p,w*4);
}
void LibretroCore::Audio(int16_t,int16_t){}
size_t LibretroCore::Batch(const int16_t*,size_t n){return n;}
void LibretroCore::Poll(){}
int16_t LibretroCore::State(unsigned,unsigned device,unsigned index,unsigned id)
{
    LibretroCore^ c=g_active;
    if(!c||device!=RETRO_DEVICE_JOYPAD||index!=0)return 0;
    return(id<10&&c->pad[id])?1:0;
}
void LibretroCore::Keyboard(bool d,unsigned k,uint32_t ch,uint16_t m)
{
    LibretroCore^ c=g_active;
    if(c&&c->keyboard)c->keyboard(d,k,ch,m);
}
void LibretroCore::SetPad(unsigned id,bool d){if(id<10)pad[id]=d;}
void LibretroCore::Key(bool d,unsigned k,uint32_t ch){Keyboard(d,k,ch,0);}
}