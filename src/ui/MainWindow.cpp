#include "ui/MainWindow.h"

namespace garageplaymate
{

MainWindow::MainWindow()
    : DocumentWindow("GaragePlaymate",
                     juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(
                         juce::ResizableWindow::backgroundColourId),
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setResizable(true, true);
    setResizeLimits(800, 600, 4096, 2160);

    auto* placeholder = new juce::Label();
    placeholder->setText("GaragePlaymate — UI coming soon", juce::dontSendNotification);
    placeholder->setJustificationType(juce::Justification::centred);
    setContentOwned(placeholder, true);

    centreWithSize(1280, 800);
    setMenuBar(this);
    setVisible(true);
}

MainWindow::~MainWindow()
{
    setMenuBar(nullptr);
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

juce::StringArray MainWindow::getMenuBarNames()
{
    return {"File", "View", "Help"};
}

juce::PopupMenu MainWindow::getMenuForIndex(int topLevelMenuIndex, const juce::String&)
{
    juce::PopupMenu menu;

    switch (topLevelMenuIndex)
    {
        case 0:
            menu.addItem(exitMenuItem, "Exit");
            break;
        case 1:
            menu.addItem(-1, "(coming soon)", false);
            break;
        case 2:
            menu.addItem(aboutMenuItem, "About GaragePlaymate");
            break;
        default:
            break;
    }

    return menu;
}

void MainWindow::menuItemSelected(int menuItemID, int)
{
    switch (menuItemID)
    {
        case exitMenuItem:
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            break;
        case aboutMenuItem:
            showAboutDialog();
            break;
        default:
            break;
    }
}

void MainWindow::showAboutDialog()
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,
                                           "About GaragePlaymate",
                                           "GaragePlaymate 0.1.0-dev");
}

} // namespace garageplaymate
