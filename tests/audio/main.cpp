#include <catch2/catch_session.hpp>

#include <juce_events/juce_events.h>

// Custom main: JUCE audio classes (AudioDeviceManager, Timer) need a
// MessageManager owned by the test thread.
int main(int argc, char* argv[]) {
    juce::MessageManager::getInstance();
    const int result = Catch::Session().run(argc, argv);
    juce::DeletedAtShutdown::deleteAll();
    juce::MessageManager::deleteInstance();
    return result;
}
