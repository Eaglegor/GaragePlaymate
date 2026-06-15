#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace garageplaymate
{

class MainWindow : public juce::DocumentWindow,
                   private juce::MenuBarModel
{
public:
    MainWindow();
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    static void showAboutDialog();

    enum MenuItemIds
    {
        exitMenuItem = 1,
        aboutMenuItem = 2,
    };

    juce::Label placeholderLabel_;
};

} // namespace garageplaymate
