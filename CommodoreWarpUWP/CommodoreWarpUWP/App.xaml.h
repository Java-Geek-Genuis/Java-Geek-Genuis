#pragma once
#include "App.g.h"
// ARM32 package build marker
// ApplicationDefinition and DependentUpon verified.
namespace CommodoreWarpUWP {
public ref class App sealed {
public:
    App();
protected:
    virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;
    void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
};
}