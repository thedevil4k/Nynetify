#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <cstring>
#include <new>

#include "SystemTray.h"
#include <FL/platform.H>

SystemTray* SystemTray::self = nullptr;

#define WM_TRAY_NOTIFY (WM_APP + 1)
#define ID_TRAY_SHOW   1001
#define ID_TRAY_EXIT   1002

SystemTray::SystemTray(Fl_Window* win)
    : hwnd(nullptr), old_proc(nullptr),
      nid_data(nullptr), hMenu(nullptr),
      isQuitting(false), fl_win(win)
{
    self = this;
    nid_data = new NOTIFYICONDATAA();
    std::memset(nid_data, 0, sizeof(NOTIFYICONDATAA));

    hwnd = (void*)fl_xid(fl_win);

    old_proc = (void*)SetWindowLongPtr((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)wnd_proc);

    create_icon();
}

SystemTray::~SystemTray() {
    destroy_icon();
    if (old_proc)
        SetWindowLongPtr((HWND)hwnd, GWLP_WNDPROC, (LONG_PTR)old_proc);
    if (hMenu) DestroyMenu((HMENU)hMenu);
    delete (NOTIFYICONDATAA*)nid_data;
    if (self == this) self = nullptr;
}

void SystemTray::create_icon() {
    NOTIFYICONDATAA* pnid = (NOTIFYICONDATAA*)nid_data;
    pnid->cbSize = sizeof(NOTIFYICONDATAA);
    pnid->hWnd = (HWND)hwnd;
    pnid->uID = 1;
    pnid->uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    pnid->uCallbackMessage = WM_TRAY_NOTIFY;
    pnid->hIcon = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(1));
    strcpy_s(pnid->szTip, "Nynetify");
    pnid->uVersion = NOTIFYICON_VERSION_4;

    Shell_NotifyIconA(NIM_ADD, pnid);
    Shell_NotifyIconA(NIM_SETVERSION, pnid);
}

void SystemTray::destroy_icon() {
    NOTIFYICONDATAA* pnid = (NOTIFYICONDATAA*)nid_data;
    Shell_NotifyIconA(NIM_DELETE, pnid);
}

void SystemTray::show_menu() {
    if (hMenu) DestroyMenu((HMENU)hMenu);
    HMENU menu = CreatePopupMenu();

    AppendMenuA(menu, MF_STRING, ID_TRAY_SHOW, "Show Nynetify");
    AppendMenuA(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuA(menu, MF_STRING, ID_TRAY_EXIT, "Exit");

    hMenu = (void*)menu;

    SetForegroundWindow((HWND)hwnd);
    POINT pt;
    GetCursorPos(&pt);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, (HWND)hwnd, nullptr);
    PostMessage((HWND)hwnd, WM_NULL, 0, 0);
}

void SystemTray::hide_window() {
    fl_win->hide();
    create_icon();
}

void SystemTray::show_window() {
    destroy_icon();
    fl_win->show();
    SetForegroundWindow((HWND)hwnd);
}

void SystemTray::quit() {
    isQuitting = true;
    destroy_icon();
    fl_win->hide();
    DestroyWindow((HWND)hwnd);
}

long __stdcall SystemTray::wnd_proc(void* hwnd, unsigned int msg, unsigned long long wParam, long long lParam) {
    if (!self) return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_CLOSE:
            if (self->isQuitting) {
                return CallWindowProc((WNDPROC)self->old_proc, (HWND)hwnd, msg, wParam, lParam);
            }
            self->hide_window();
            return 0;

        case WM_TRAY_NOTIFY:
            switch (lParam) {
                case WM_LBUTTONDBLCLK:
                    self->show_window();
                    break;
                case WM_RBUTTONUP:
                    self->show_menu();
                    break;
            }
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_SHOW:
                    self->show_window();
                    return 0;
                case ID_TRAY_EXIT:
                    self->quit();
                    return 0;
            }
            break;

        case WM_DESTROY:
            self->destroy_icon();
            PostQuitMessage(0);
            return 0;
    }

    return CallWindowProc((WNDPROC)self->old_proc, (HWND)hwnd, msg, wParam, lParam);
}
#else
#include "SystemTray.h"
SystemTray::SystemTray(Fl_Window* win) {}
SystemTray::~SystemTray() {}
void SystemTray::quit() {}
#endif
