#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

namespace garageplaymate
{

class AppController;
class MainWindow;

class GaragePlaymateApplication : public juce::JUCEApplication
{
public:
    GaragePlaymateApplication();
    ~GaragePlaymateApplication() override;

    const juce::String getApplicationName() override;
    const juce::String getApplicationVersion() override;
    bool moreThanOneInstanceAllowed() override;

    void initialise(const juce::String& commandLineParameters) override;
    void shutdown() override;
    void systemRequestedQuit() override;

private:
    std::unique_ptr<AppController> controller_;
    std::unique_ptr<MainWindow> mainWindow_;
};

} // namespace garageplaymate
