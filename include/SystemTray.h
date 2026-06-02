#pragma once
#include <FL/Fl_Window.H>

#ifdef _WIN32

class SystemTray {
    void* hwnd;
    void* old_proc;
    void* nid_data;
    void* hMenu;
    bool isQuitting;
    Fl_Window* fl_win;

    void create_icon();
    void destroy_icon();
    void show_menu();
    void hide_window();
    void show_window();

    static long __stdcall wnd_proc(void*, unsigned int, unsigned long long, long long);
    static SystemTray* self;

public:
    SystemTray(Fl_Window* win);
    ~SystemTray();
    void quit();
};

#else

class SystemTray {
public:
    SystemTray(Fl_Window*) {}
    ~SystemTray() {}
    void quit() {}
};

#endif
