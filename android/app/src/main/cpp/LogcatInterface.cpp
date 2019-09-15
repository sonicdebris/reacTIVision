#include "LogcatInterface.h"
#include "VisionEngine.h"
#include "Logging.h"

// the principal program sequence
bool LogcatInterface::openDisplay(VisionEngine *engine) {
    engine_ = engine;
    return true;
}

void LogcatInterface::closeDisplay() {}
void LogcatInterface::updateDisplay() {}
void LogcatInterface::processEvents() {}
void LogcatInterface::setBuffers(unsigned char *src, unsigned char *dest, int width, int height, int format) {}

void LogcatInterface::printMessage(std::string message)
{
    if (verbose_) {
        LOGI("%s", message.c_str());
    }
}

void LogcatInterface::displayError(const char* error)
{
    LOGE("%s",error);
}

void LogcatInterface::displayMessage(const char *message)
{
    LOGI("%s", message);
}

void LogcatInterface::displayControl(const char *title, int min, int max, int value) {}

void LogcatInterface::drawText(int xpos, int ypos, const char *text) {
    LOGI("Draw text: [%d, %d] %s", xpos, ypos, text);
}

void LogcatInterface::setHelpText(std::vector<std::string> hlp) {}
LogcatInterface::LogcatInterface() {}



