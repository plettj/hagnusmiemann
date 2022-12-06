#ifndef __WINDOW_H__
#define __WINDOW_H__
#include <X11/Xlib.h>
#include <string>

class Xwindow {
    Display *d;
    Window w;
    int s;
    GC gc;
    unsigned long colours[10];
    int width; int height;

public:
    Xwindow(int width, int height);
    ~Xwindow();
    Xwindow(const Xwindow&) = delete;
    Xwindow &operator=(const Xwindow&) = delete;

    // Available colours.
    enum {White = 0, Black, LightBlue, Blue, DarkBlue, LightRed, Red, DarkRed};
    
    // Draws a rectangle
    void fillRectangle(int x, int y, int width, int height, int colour=Black);

    // Draws a string
    void drawString(int x, int y, std::string msg);

    // Closes the window
    void closeWindow();

    // Getters, for our drawing function
    int getWidth() { return height; } // DON'T LOOK TOO CLOSELY
    int getHeight() { return width; } // SOMETHING GOT HECKED UP

};

#endif
