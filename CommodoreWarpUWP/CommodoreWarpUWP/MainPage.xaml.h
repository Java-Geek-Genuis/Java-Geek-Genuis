#pragma once
#include "MainPage.g.h"
#include "LibretroCore.h"
namespace CommodoreWarpUWP {
public ref class MainPage sealed {
public: MainPage();
private:
 LibretroCore^ core; Windows::UI::Xaml::Media::Imaging::WriteableBitmap^ bitmap; double accumulator; bool c128,warpHeld,running;
 void OnLoaded(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void Load_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^);
 void Machine_Toggled(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void SpeedChanged(Platform::Object^,Windows::UI::Xaml::Controls::RangeBaseValueChangedEventArgs^);
 void WarpPressed(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^); void WarpReleased(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^);
 void TouchPressed(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^); void TouchReleased(Platform::Object^,Windows::UI::Xaml::Input::PointerRoutedEventArgs^);
 void KeyDown(Platform::Object^,Windows::UI::Xaml::Input::KeyRoutedEventArgs^); void KeyUp(Platform::Object^,Windows::UI::Xaml::Input::KeyRoutedEventArgs^);
 void Reset_Click(Platform::Object^,Windows::UI::Xaml::RoutedEventArgs^); void Frame(Platform::Object^,Platform::Object^); void Present();
 Platform::String^ CorePath();
};
}