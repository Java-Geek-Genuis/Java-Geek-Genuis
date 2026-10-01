#include "pch.h"
#include "MainPage.xaml.h"
#include <windows.storage.streams.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::AccessCache;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace concurrency;
using Microsoft::WRL::ComPtr;

namespace CommodoreWarpUWP {
static std::string BaseName(String^ name){
    std::string s(name->Data(),name->Data()+name->Length()); size_t dot=s.find_last_of('.');
    if(dot!=std::string::npos)s=s.substr(0,dot); std::string r;
    for(char c:s)if(std::isalnum((unsigned char)c)||c=='_'||c=='-')r.push_back(c);
    return r.empty()?"MYPROGRAM":r;
}
static String^ ToPlatformString(const std::string& s){std::wstring w(s.begin(),s.end());return ref new String(w.c_str());}

MainPage::MainPage():core(ref new LibretroCore()),bitmap(nullptr),sdFolder(nullptr),accumulator(0),c128(false),warpHeld(false),running(false){}
String^ MainPage::CorePath(){return c128?L"Cores\\vice_x128_libretro.dll":L"Cores\\vice_x64_libretro.dll";}

void MainPage::OnLoaded(Object^,RoutedEventArgs^){
    CompositionTarget::Rendering+=ref new RenderingEventHandler(this,&MainPage::Frame);
    CpuSlider->Value=1;CpuText->Text=L"1.0 MHz";
    StatusText->Text=L"Ready. C64 is the default.";
    auto v=ApplicationData::Current->LocalSettings->Values;
    if(v->HasKey(L"SdConsent")&&(bool)v->Lookup(L"SdConsent"))PrepareSdFolder();
    else SdText->Text=L"SD workspace: not enabled.";
}

void MainPage::Machine_Toggled(Object^ sender,RoutedEventArgs^){
    ToggleSwitch^ t=dynamic_cast<ToggleSwitch^>(sender);if(!t)return;
    c128=t->IsOn;MachineText->Text=c128?L"C128":L"C64";
    if(running){running=false;core->Unload();bitmap=nullptr;}
    StatusText->Text=c128?L"C128 selected.":L"C64 selected.";
}

void MainPage::Load_Click(Object^,RoutedEventArgs^){
    FileOpenPicker^ p=ref new FileOpenPicker();p->ViewMode=PickerViewMode::List;
    p->SuggestedStartLocation=PickerLocationId::DocumentsLibrary;
    p->FileTypeFilter->Append(L".prg");p->FileTypeFilter->Append(L".d64");p->FileTypeFilter->Append(L".g64");
    p->FileTypeFilter->Append(L".d81");p->FileTypeFilter->Append(L".t64");p->FileTypeFilter->Append(L".tap");p->FileTypeFilter->Append(L".crt");p->FileTypeFilter->Append(L".zip");
    create_task(p->PickSingleFileAsync()).then([this](StorageFile^ f){
        if(!f)return;
        return create_task(f->CopyAsync(ApplicationData::Current->LocalFolder,L"warp_input_"+f->Name,NameCollisionOption::ReplaceExisting))
        .then([this,f](StorageFile^ copy){
            running=false;core->Unload();bitmap=nullptr;StatusText->Text=L"Loading "+f->Name+L"...";
            if(!core->LoadCore(CorePath())){StatusText->Text=core->Error;return;}
            if(!core->LoadGame(copy->Path)){StatusText->Text=core->Error;return;}
            bitmap=ref new WriteableBitmap(core->Width,core->Height);accumulator=0;running=true;
            StatusText->Text=L"Running "+f->Name+L" on "+(c128?L"C128":L"C64")+L".";
        });
    });
}

void MainPage::Frame(Object^,Object^){
    if(!running||!bitmap)return; double dt=.01667; double mhz=CpuSlider->Value;
    if(warpHeld)mhz=16; accumulator+=mhz*core->Fps()*dt; int frames=(int)accumulator;
    if(frames<1)return; if(frames>400)frames=400; accumulator-=frames; core->RunFrames(frames); Present();
}

void MainPage::Present(){
    if(core->Width==0||core->Height==0)return;
    if(bitmap->PixelWidth!=core->Width||bitmap->PixelHeight!=core->Height)bitmap=ref new WriteableBitmap(core->Width,core->Height);
    ComPtr<Windows::Storage::Streams::IBufferByteAccess> access;
    IInspectable* obj=reinterpret_cast<IInspectable*>(bitmap->PixelBuffer);
    if(FAILED(obj->QueryInterface(IID_PPV_ARGS(&access))))return;
    unsigned char* dst=nullptr;if(FAILED(access->Buffer(&dst)))return;
    Array<unsigned char>^ src=core->GetFrameCopy();
    unsigned bytes=(unsigned)std::min<unsigned>(src->Length,core->Width*core->Height*4);
    if(dst&&bytes)std::memcpy(dst,src->Data,bytes);bitmap->Invalidate();ScreenImage->Source=bitmap;
}

void MainPage::CpuChanged(Object^,Windows::UI::Xaml::Controls::RangeBaseValueChangedEventArgs^ e){
    double v=e->NewValue;CpuText->Text=ref new String((std::to_wstring(v)+L" MHz").c_str());
}
void MainPage::WarpPressed(Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e){warpHeld=true;e->Handled=true;}
void MainPage::WarpReleased(Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e){warpHeld=false;e->Handled=true;}
void MainPage::Reset_Click(Object^,RoutedEventArgs^){core->Reset();accumulator=0;StatusText->Text=L"Reset.";}

void MainPage::TouchPressed(Object^ sender,Windows::UI::Xaml::Input::PointerRoutedEventArgs^){
    Button^ b=dynamic_cast<Button^>(sender);if(!b)return;String^ t=dynamic_cast<String^>(b->Tag);
    if(t==L"U")core->SetPad(4,true);else if(t==L"D")core->SetPad(5,true);else if(t==L"L")core->SetPad(6,true);else if(t==L"R")core->SetPad(7,true);else if(t==L"F")core->SetPad(8,true);
}
void MainPage::TouchReleased(Object^ sender,Windows::UI::Xaml::Input::PointerRoutedEventArgs^){
    Button^ b=dynamic_cast<Button^>(sender);if(!b)return;String^ t=dynamic_cast<String^>(b->Tag);
    if(t==L"U")core->SetPad(4,false);else if(t==L"D")core->SetPad(5,false);else if(t==L"L")core->SetPad(6,false);else if(t==L"R")core->SetPad(7,false);else if(t==L"F")core->SetPad(8,false);
}
void MainPage::KeyDown(Object^,KeyRoutedEventArgs^ e){
    unsigned k=(unsigned)e->Key;
    if(e->Key==VirtualKey::Enter)core->Key(true,RETROK_RETURN,'\r');else if(e->Key==VirtualKey::Back)core->Key(true,RETROK_BACKSPACE,'\b');
    else if(e->Key==VirtualKey::Space)core->Key(true,RETROK_SPACE,' ');
    else if(k>=(unsigned)VirtualKey::A&&k<=(unsigned)VirtualKey::Z){unsigned c=(unsigned)('a'+k-(unsigned)VirtualKey::A);core->Key(true,c,c);}
}
void MainPage::KeyUp(Object^,KeyRoutedEventArgs^ e){
    unsigned k=(unsigned)e->Key;
    if(e->Key==VirtualKey::Enter)core->Key(false,RETROK_RETURN,'\r');else if(e->Key==VirtualKey::Back)core->Key(false,RETROK_BACKSPACE,'\b');
    else if(e->Key==VirtualKey::Space)core->Key(false,RETROK_SPACE,' ');
    else if(k>=(unsigned)VirtualKey::A&&k<=(unsigned)VirtualKey::Z){unsigned c=(unsigned)('a'+k-(unsigned)VirtualKey::A);core->Key(false,c,c);}
}

void MainPage::Program_Click(Object^,RoutedEventArgs^){ProgramPanel->Visibility=Visibility::Visible;ProgramEditor->Focus(FocusState::Programmatic);}
void MainPage::ProgramBack_Click(Object^,RoutedEventArgs^){ProgramPanel->Visibility=Visibility::Collapsed;}
void MainPage::NewProgram_Click(Object^,RoutedEventArgs^){ProgramName->Text=L"MYPROGRAM";ProgramEditor->Text=L"10 PRINT \"HELLO FROM C64\"\n20 GOTO 10";}

void MainPage::LoadBasic_Click(Object^,RoutedEventArgs^){
    FileOpenPicker^ p=ref new FileOpenPicker();p->ViewMode=PickerViewMode::List;p->SuggestedStartLocation=PickerLocationId::DocumentsLibrary;
    p->FileTypeFilter->Append(L".bas");p->FileTypeFilter->Append(L".txt");
    create_task(p->PickSingleFileAsync()).then([this](StorageFile^ f){if(!f)return;return create_task(FileIO::ReadTextAsync(f)).then([this,f](String^ text){
        ProgramEditor->Text=text;ProgramName->Text=ToPlatformString(BaseName(f->Name));StatusText->Text=L"Loaded "+f->Name+L".";
    });});
}

void MainPage::SaveLocal_Click(Object^,RoutedEventArgs^){SaveProgramTo(ApplicationData::Current->LocalFolder,false);}

void MainPage::EnableSd_Click(Object^,RoutedEventArgs^){
    auto d=ref new ContentDialog();
    d->Title=L"Enable SD workspace?";
    d->Content=L"Create a CommodoreWarp folder on the first removable device and let this app save the programs you choose there?";
    d->PrimaryButtonText=L"ALLOW SD";d->CloseButtonText=L"CANCEL";
    create_task(d->ShowAsync()).then([this](ContentDialogResult r){
        if(r==ContentDialogResult::Primary){ApplicationData::Current->LocalSettings->Values->Insert(L"SdConsent",true);PrepareSdFolder();}
    });
}

void MainPage::PrepareSdFolder(){
    create_task(KnownFolders::RemovableDevices->GetFoldersAsync()).then([this](IVectorView<StorageFolder^>^ devices){
        if(!devices||devices->Size==0){
            SdText->Text=L"SD workspace: no removable card detected. Local saves still work.";
            return create_task_from_result<StorageFolder^>(nullptr);
        }
        return create_task(devices->GetAt(0)->CreateFolderAsync(L"CommodoreWarp",CreationCollisionOption::OpenIfExists));
    }).then([this](StorageFolder^ folder){
        if(!folder)return;sdFolder=folder;
        try{StorageApplicationPermissions::FutureAccessList->AddOrReplace(L"CommodoreWarpSD",folder);}catch(Exception^){}
        SdText->Text=L"SD workspace: "+folder->Name+L" ✓";
    }).then([](task<StorageFolder^> t){try{t.get();}catch(Exception^){}});
}

void MainPage::SaveSd_Click(Object^,RoutedEventArgs^){if(sdFolder)SaveProgramTo(sdFolder,false);else SdText->Text=L"Enable SD first.";}

void MainPage::SaveProgramTo(StorageFolder^ folder,bool prg){
    if(!folder)return;std::string base=BaseName(ProgramName->Text);std::wstring wb(base.begin(),base.end());
    String^ filename=ref new String((wb+(prg?L".prg":L".bas")).c_str());
    create_task(folder->CreateFileAsync(filename,CreationCollisionOption::ReplaceExisting)).then([this,prg](StorageFile^ file){
        if(prg)return create_task(FileIO::WriteBytesAsync(file,BuildPrg())).then([this,file](){StatusText->Text=L"Saved "+file->Name+L".";});
        return create_task(FileIO::WriteTextAsync(file,ProgramEditor->Text)).then([this,file](){StatusText->Text=L"Saved "+file->Name+L".";});
    }).then([](task<void> t){try{t.get();}catch(Exception^){}});
}

Array<unsigned char>^ MainPage::BuildPrg(){
    struct Line{int number;std::vector<unsigned char> code;};
    const std::vector<std::pair<std::string,unsigned char>> tokenList={
        {"INPUT#",0x84},{"PRINT#",0x98},{"RIGHT$",0xC9},{"LEFT$",0xC8},{"MID$",0xCA},{"RETURN",0x8E},{"RESTORE",0x8C},{"VERIFY",0x95},
        {"GOSUB",0x8D},{"CLOSE",0xA0},{"CONT",0x9A},{"INPUT",0x85},{"LOAD",0x93},{"SAVE",0x94},{"PRINT",0x99},{"LIST",0x9B},
        {"CLR",0x9C},{"CMD",0x9D},{"SYS",0x9E},{"OPEN",0x9F},{"GET",0xA1},{"NEW",0xA2},{"TAB(",0xA3},{"THEN",0xA7},{"STEP",0xA9},
        {"NEXT",0x82},{"DATA",0x83},{"DIM",0x86},{"READ",0x87},{"LET",0x88},{"GOTO",0x89},{"RUN",0x8A},{"IF",0x8B},{"REM",0x8F},
        {"STOP",0x90},{"ON",0x91},{"WAIT",0x92},{"DEF",0x96},{"POKE",0x97},{"FN",0xA5},{"TO",0xA4},{"SPC(",0xA6},{"END",0x80},
        {"FOR",0x81},{"NOT",0xA8},{"AND",0xAF},{"OR",0xB0},{"SGN",0xB4},{"INT",0xB5},{"ABS",0xB6},{"USR",0xB7},{"FRE",0xB8},
        {"POS",0xB9},{"SQR",0xBA},{"RND",0xBB},{"LOG",0xBC},{"EXP",0xBD},{"COS",0xBE},{"SIN",0xBF},{"TAN",0xC0},{"ATN",0xC1},
        {"PEEK",0xC2},{"LEN",0xC3},{"STR$",0xC4},{"VAL",0xC5},{"ASC",0xC6},{"CHR$",0xC7},{"<",0xB3},{">",0xB1},{"=",0xB2},
        {"^",0xAE},{"/",0xAD},{"*",0xAC},{"+",0xAA},{"-",0xAB}
    };
    auto tokens=tokenList;std::sort(tokens.begin(),tokens.end(),[](const auto&a,const auto&b){return a.first.size()>b.first.size();});
    std::string source(ProgramEditor->Text->Data(),ProgramEditor->Text->Data()+ProgramEditor->Text->Length);
    std::stringstream ss(source);std::string raw;std::vector<Line> lines;int autoNo=10;
    while(std::getline(ss,raw)){
        if(!raw.empty()&&raw.back()=='\r')raw.pop_back();size_t p=0;while(p<raw.size()&&raw[p]==' ')p++;
        size_t st=p;while(p<raw.size()&&std::isdigit((unsigned char)raw[p]))p++;bool numbered=p>st;
        int no=numbered?std::atoi(raw.substr(st,p-st).c_str()):autoNo;autoNo=numbered?no+10:autoNo+10;
        while(p<raw.size()&&raw[p]==' ')p++;
        std::string body=raw.substr(p),up=Upper(body);std::vector<unsigned char> out;bool inString=false;
        for(size_t i=0;i<body.size();){
            char ch=body[i];if(ch=='"'){out.push_back((unsigned char)ch);inString=!inString;i++;continue;}
            if(inString){out.push_back((unsigned char)ch);i++;continue;}
            bool matched=false;
            for(const auto& kv:tokens){
                const std::string& kw=kv.first;if(i+kw.size()>up.size()||up.compare(i,kw.size(),kw)!=0)continue;
                char prev=i?up[i-1]:' ';char next=i+kw.size()<up.size()?up[i+kw.size()]:' ';
                bool word=std::isalnum((unsigned char)kw[0]);bool pb=std::isalnum((unsigned char)prev)||prev=='$';bool nb=std::isalnum((unsigned char)next)||next=='$';
                if(word&&(pb||nb))continue;out.push_back(kv.second);i+=kw.size();matched=true;
                if(kw=="REM")while(i<body.size())out.push_back((unsigned char)body[i++]);break;
            }
            if(!matched){out.push_back((unsigned char)std::toupper((unsigned char)ch));i++;}
        }
        lines.push_back({no,out});
    }
    std::sort(lines.begin(),lines.end(),[](const Line&a,const Line&b){return a.number<b.number;});
    std::vector<Line> unique;for(auto& l:lines){if(!unique.empty()&&unique.back().number==l.number)unique.back()=l;else unique.push_back(l);}
    std::vector<unsigned char> prg={0x01,0x08};unsigned current=0x0801;
    for(const auto& l:unique){
        unsigned next=current+4+(unsigned)l.code.size()+1;prg.push_back((unsigned char)(next&255));prg.push_back((unsigned char)((next>>8)&255));
        prg.push_back((unsigned char)(l.number&255));prg.push_back((unsigned char)((l.number>>8)&255));prg.insert(prg.end(),l.code.begin(),l.code.end());prg.push_back(0);current=next;
    }
    prg.push_back(0);prg.push_back(0);auto a=ref new Array<unsigned char>((unsigned)prg.size());for(unsigned i=0;i<a->Length;i++)a[i]=prg[i];return a;
}

void MainPage::ExportPrg_Click(Object^,RoutedEventArgs^){
    FileSavePicker^ p=ref new FileSavePicker();p->SuggestedStartLocation=PickerLocationId::DocumentsLibrary;std::string b=BaseName(ProgramName->Text);std::wstring wb(b.begin(),b.end());
    p->SuggestedFileName=ref new String((wb+L".prg").c_str());auto types=ref new Platform::Collections::Vector<String^>();types->Append(L".prg");p->FileTypeChoices->Insert(L"C64 Program",types);
    create_task(p->PickSaveFileAsync()).then([this](StorageFile^ f){if(!f)return;return create_task(FileIO::WriteBytesAsync(f,BuildPrg())).then([this,f](){StatusText->Text=L"Exported C64 PRG: "+f->Name;});}).then([](task<void> t){try{t.get();}catch(Exception^){}});
}
void MainPage::ExportPrgSd_Click(Object^,RoutedEventArgs^){if(sdFolder)SaveProgramTo(sdFolder,true);else SdText->Text=L"Enable SD first.";}
}