#include "app/Application.h"

#include "app/AppController.h"
#include "ui/MainWindow.h"

namespace garageplaymate
{

GaragePlaymateApplication::GaragePlaymateApplication() = default;

GaragePlaymateApplication::~GaragePlaymateApplication() = default;

const juce::String GaragePlaymateApplication::getApplicationName()
{
    return "GaragePlaymate";
}

const juce::String GaragePlaymateApplication::getApplicationVersion()
{
    return "0.1.0-dev";
}

bool GaragePlaymateApplication::moreThanOneInstanceAllowed()
{
    return false;
}

void GaragePlaymateApplication::initialise(const juce::String&)
{
    controller_ = std::make_unique<AppController>();
    mainWindow_ = std::make_unique<MainWindow>();
}

void GaragePlaymateApplication::shutdown()
{
    mainWindow_.reset();
    controller_.reset();
}

void GaragePlaymateApplication::systemRequestedQuit()
{
    juce::JUCEApplication::quit();
}

} // namespace garageplaymate
