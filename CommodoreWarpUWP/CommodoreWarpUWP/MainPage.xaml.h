#pragma once
#include "MainPage.g.h"
#include "LibretroCore.h"
// Final ARM32 packaging verification.
// Final artifact upload verification.
namespace CommodoreWarpUWP {
public ref class MainPage sealed {
public:
 MainPage();
private:
 LibretroCore^ core; Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ bitmap; Windows::Storage::StorageFolder^ sdFolder; Windows::UI::Xaml::DispatcherTimer^ timer;
 double accumulator; bool c128,warpHeld,running;
 void OnLoaded(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void Load_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void Machine_Toggled(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void CpuChanged(Platform::Object^,Windows::UI::Xaml::Controls::Primitives::RangeBaseValueChangedEventArgs^);
 void WarpPressed(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^); void WarpReleased(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^);
 void TouchPressed(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^); void TouchReleased(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^);
 void KeyDown(Platform::Object^,Windows::UI::Xaml::Input::KeyRoutedEventArgs^); void KeyUp(Platform::Object^,Windows::UI::Xaml::Input::KeyRoutedEventArgs^); void Reset_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void Program_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void ProgramBack_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void NewProgram_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void LoadBasic_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void RunProgram_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void SaveLocal_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void EnableSd_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void SaveSd_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void ExportPrg_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void ExportPrgSd_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void TimerTick(Platform::Object^,Platform::Object^); void Frame(Platform::Object^,Platform::Object^); void Present(); Platform::String^ CorePath(); void PrepareSdFolder(); void SaveProgramTo(Windows::Storage::StorageFolder^ folder,bool prg); Platform::Array<unsigned char>^ BuildPrg();
}; }