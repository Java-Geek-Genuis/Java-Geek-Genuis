#pragma once
#include "App.g.h"
namespace CommodoreWarpUWP { ref class App sealed { public: App(); protected: void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e); void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e); }; }