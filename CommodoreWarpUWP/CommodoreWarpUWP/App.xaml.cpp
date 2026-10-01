#include "pch.h"
#include "App.xaml.h"
#include "MainPage.xaml.h"
using namespace Platform; using namespace Windows::ApplicationModel::Activation; using namespace Windows::UI::Xaml; using namespace Windows::ApplicationModel;
namespace CommodoreWarpUWP {
App::App(){InitializeComponent();Suspending+=ref new SuspendingEventHandler(this,&App::OnSuspending);}
void App::OnLaunched(LaunchActivatedEventArgs^){Window::Current->Content=ref new MainPage();Window::Current->Activate();}
void App::OnSuspending(Object^,SuspendingEventArgs^){}
}