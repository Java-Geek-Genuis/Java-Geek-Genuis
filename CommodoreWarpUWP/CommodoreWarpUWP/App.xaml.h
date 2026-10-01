#pragma once
#include "App.g.h"
namespace CommodoreWarpUWP {
ref class App sealed : public Windows::UI::Xaml::Application {
public:
    App();
protected:
    virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ e) override;
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
};
}