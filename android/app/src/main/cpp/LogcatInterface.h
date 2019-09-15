//
// Created by athos on 9/9/19.
//

#ifndef REACTIDROID_LOGCATINTERFACE_H
#define REACTIDROID_LOGCATINTERFACE_H

#include "UserInterface.h"


class LogcatInterface: public UserInterface
{

public:
    LogcatInterface();
    ~LogcatInterface() {};

    bool openDisplay(VisionEngine *engine);
    void updateDisplay();
    void closeDisplay();

    void setHelpText(std::vector<std::string> hlp);
    void setBuffers(unsigned char *src, unsigned char *dest, int width, int height, int format);
    void processEvents();

    void printMessage(std::string message);
    void displayMessage(const char *message);
    void displayControl(const char *title, int min, int max, int value);
    void displayError(const char* error);
    void drawText(int xpos, int ypos, const char *text);

    void setColor(unsigned char r, unsigned char g, unsigned char b) {};
    void drawPoint(int x,int y) {};
    void drawLine(int x1,int y1, int x2, int y2) {};
    void drawRect(int x,int y, int w, int h) {};
    void drawRect(int x,int y, int w, int h, float r) {};
    void fillRect(int x,int y, int w, int h) {};
    void drawEllipse(int x,int y, int w, int h) {};
    void drawEllipse(int x,int y, int w, int h, float r) {};
    void fillEllipse(int x,int y, int w, int h) {};
};


#endif //REACTIDROID_LOGCATINTERFACE_H
