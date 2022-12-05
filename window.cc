#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>
#include "window.h"

using namespace std;

int X11ErrorHandler(Display* display, XErrorEvent* error) {
    std::cerr << "error handled" << std::endl;
    return 0;
}

Xwindow::Xwindow(int width, int height): width{width}, height{height} {

    d = XOpenDisplay(NULL);
    if (d == NULL) exit(1);
    XSetErrorHandler(&X11ErrorHandler);

    s = DefaultScreen(d);
    w = XCreateSimpleWindow(d, RootWindow(d, s), 10, 10, height, width, 1, BlackPixel(d, s), WhitePixel(d, s));
    XSelectInput(d, w, ExposureMask | KeyPressMask);
    XMapRaised(d, w);

    Pixmap pix = XCreatePixmap(d, w, height, width, DefaultDepth(d, DefaultScreen(d)));
    gc = XCreateGC(d, pix, 0, (XGCValues *)0);

    XFlush(d);
    XFlush(d);

    // Set up colours.
    XColor xcolour;
    Colormap cmap;
    char color_vals[10][10] = {"white", "black", "red", "green", "blue", "cyan", "yellow", "magenta", "orange", "brown"};

    cmap = DefaultColormap(d, DefaultScreen(d));
    for (int i = 0; i < 5; ++i) {
        XParseColor(d, cmap, color_vals[i], &xcolour);
        XAllocColor(d, cmap, &xcolour);
        colours[i] = xcolour.pixel;
    }

    XSetForeground(d, gc, colours[Black]);

    // Make window non-resizeable.
    XSizeHints hints;
    hints.flags = (USPosition | PSize | PMinSize | PMaxSize);
    hints.width = hints.base_width = hints.min_width = hints.max_width = height;
    hints.height = hints.base_height = hints.min_height = hints.max_height = width;
    XSetNormalHints(d, w, &hints);

    Atom WW_DELETE_WINDOW = XInternAtom(d, "WW_DELETE_WINDOW", False);
    XSetWMProtocols(d, w, &WW_DELETE_WINDOW, True);

    XSynchronize(d, True);

    usleep(1000);

    // Make sure we don't race against the Window being shown
    XEvent ev;
    while (1) {
        XNextEvent(d, &ev);
        if (ev.type == Expose) break;
        else if (ev.type == ClientMessage) break;
    }
}

Xwindow::~Xwindow() {
    XFreeGC(d, gc);
    XCloseDisplay(d);
}

void Xwindow::fillRectangle(int x, int y, int width, int height, int colour) {
    XSetForeground(d, gc, colours[colour]);
    XFillRectangle(d, w, gc, x, y, width, height);
    XSetForeground(d, gc, colours[Black]);
}

void Xwindow::drawString(int x, int y, string msg) {
    XDrawString(d, w, DefaultGC(d, s), x, y, msg.c_str(), msg.length());
}

