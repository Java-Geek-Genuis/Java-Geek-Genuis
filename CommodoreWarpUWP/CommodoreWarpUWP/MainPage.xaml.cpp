#include "pch.h"
#include "MainPage.xaml.h"
#include <windows.storage.streams.h>
#include <cstring>
using namespace Platform;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::System;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace concurrency;
using Microsoft::WRL::ComPtr;

namespace CommodoreWarpUWP {
MainPage::MainPage():core(ref new LibretroCore()),bitmap(nullptr),accumulator(0),c128(false),warpHeld(false),running(false){}
String^ MainPage::CorePath(){return c128?L"Cores\\vice_x128_libretro.dll":L"Cores\\vice_x64_libretro.dll";}
void MainPage::OnLoaded(Object^,RoutedEventArgs^){CompositionTarget::Rendering+=ref new RenderingEventHandler(this,&MainPage::Frame);SpeedSlider->Value=1;SpeedText->Text=L"1.0x";StatusText->Text=L"Ready. Select C64/C128 and load software.";}
void MainPage::Machine_Toggled(Object^ sender,RoutedEventArgs^){ToggleSwitch^ t=dynamic_cast<ToggleSwitch^>(sender);if(!t)return;c128=t->IsOn;MachineText->Text=c128?L"C128":L"C64";if(running){running=false;core->Unload();bitmap=nullptr;}StatusText->Text=c128?L"C128 selected. Load software.":L"C64 selected. Load software.";}
void MainPage::Load_Click(Object^,RoutedEventArgs^){
 FileOpenPicker^ p=ref new FileOpenPicker();p->ViewMode=PickerViewMode::List;p->SuggestedStartLocation=PickerLocationId::DocumentsLibrary;
 p->FileTypeFilter->Append(L".prg");p->FileTypeFilter->Append(L".d64");p->FileTypeFilter->Append(L".g64");p->FileTypeFilter->Append(L".d81");p->FileTypeFilter->Append(L".t64");p->FileTypeFilter->Append(L".tap");p->FileTypeFilter->Append(L".crt");p->FileTypeFilter->Append(L".zip");
 create_task(p->PickSingleFileAsync()).then([this](StorageFile^ file){
  if(!file)return;StorageFolder^ local=ApplicationData::Current->LocalFolder;
  create_task(file->CopyAsync(local,L"warp_input_"+file->Name,NameCollisionOption::ReplaceExisting)).then([this,file](StorageFile^ copy){
   running=false;core->Unload();bitmap=nullptr;StatusText->Text=L"Loading "+file->Name+L"...";
   if(!core->LoadCore(CorePath())){StatusText->Text=core->Error;return;}
   if(!core->LoadGame(copy->Path)){StatusText->Text=core->Error;return;}
   bitmap=ref new WriteableBitmap(core->Width,core->Height);accumulator=0;running=true;
   StatusText->Text=L"Running "+file->Name+L" on "+(c128?L"C128":L"C64");
  });
 });
}
void MainPage::Frame(Object^,Object^){
 if(!running||!bitmap)return;double dt=0.01667,speed=SpeedSlider->Value;if(warpHeld)speed=16.0;accumulator+=speed*core->Fps()*dt;int frames=(int)accumulator;if(frames<1)return;if(frames>24)frames=24;accumulator-=frames;core->RunFrames(frames);Present();
}
void MainPage::Present(){
 if(core->Width==0||core->Height==0)return;
 if(bitmap->PixelWidth!=core->Width||bitmap->PixelHeight!=core->Height)bitmap=ref new WriteableBitmap(core->Width,core->Height);
 ComPtr<Windows::Storage::Streams::IBufferByteAccess> access;IInspectable* obj=reinterpret_cast<IInspectable*>(bitmap->PixelBuffer);
 if(FAILED(obj->QueryInterface(IID_PPV_ARGS(&access))))return;unsigned char* dst=nullptr;if(FAILED(access->Buffer(&dst)))return;
 Array<unsigned char>^ src=core->GetFrameCopy();unsigned bytes=(unsigned)std::min<unsigned>(src->Length,core->Width*core->Height*4);if(dst&&bytes)memcpy(dst,src->Data,bytes);
 bitmap->Invalidate();ScreenImage->Source=bitmap;
}
void MainPage::SpeedChanged(Object^,Windows::UI::Xaml::Controls::RangeBaseValueChangedEventArgs^ e){double v=e->NewValue;SpeedText->Text=ref new String(std::to_wstring(v).c_str())+L"x";}
void MainPage::WarpPressed(Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e){warpHeld=true;e->Handled=true;}
void MainPage::WarpReleased(Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e){warpHeld=false;e->Handled=true;}
void MainPage::Reset_Click(Object^,RoutedEventArgs^){core->Reset();StatusText->Text=L"Reset.";accumulator=0;}
void MainPage::TouchPressed(Object^ sender,Windows::UI::Xaml::Input::PointerRoutedEventArgs^){Button^ b=dynamic_cast<Button^>(sender);if(!b)return;String^ t=dynamic_cast<String^>(b->Tag);if(t==L"U")core->SetPad(4,true);else if(t==L"D")core->SetPad(5,true);else if(t==L"L")core->SetPad(6,true);else if(t==L"R")core->SetPad(7,true);else if(t==L"F")core->SetPad(8,true);}
void MainPage::TouchReleased(Object^ sender,Windows::UI::Xaml::Input::PointerRoutedEventArgs^){Button^ b=dynamic_cast<Button^>(sender);if(!b)return;String^ t=dynamic_cast<String^>(b->Tag);if(t==L"U")core->SetPad(4,false);else if(t==L"D")core->SetPad(5,false);else if(t==L"L")core->SetPad(6,false);else if(t==L"R")core->SetPad(7,false);else if(t==L"F")core->SetPad(8,false);}
void MainPage::KeyDown(Object^,KeyRoutedEventArgs^ e){unsigned k=(unsigned)e->Key;if(e->Key==VirtualKey::Enter)core->Key(true,RETROK_RETURN,'\r');else if(e->Key==VirtualKey::Back)core->Key(true,RETROK_BACKSPACE,'\b');else if(e->Key==VirtualKey::Space)core->Key(true,RETROK_SPACE,' ');else if(k>=(unsigned)VirtualKey::A&&k<=(unsigned)VirtualKey::Z){unsigned code=(unsigned)('a'+k-(unsigned)VirtualKey::A);core->Key(true,code,code);}}
void MainPage::KeyUp(Object^,KeyRoutedEventArgs^ e){unsigned k=(unsigned)e->Key;if(e->Key==VirtualKey::Enter)core->Key(false,RETROK_RETURN,'\r');else if(e->Key==VirtualKey::Back)core->Key(false,RETROK_BACKSPACE,'\b');else if(e->Key==VirtualKey::Space)core->Key(false,RETROK_SPACE,' ');else if(k>=(unsigned)VirtualKey::A&&k<=(unsigned)VirtualKey::Z){unsigned code=(unsigned)('a'+k-(unsigned)VirtualKey::A);core->Key(false,code,code);}}
}