#include <fstream>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif
#include <FL/Fl.H>
#include "AppSettings.h"
#include "Globals.h"

/*
 * AppSettings.cpp — persistent configuration store
 *
 * Reads and writes a key=value config file (settings.cfg) located
 * in the same directory as the executable.  The global `settings`
 * variable is updated in-place.
 */

AppSettings settings;                     /* ← the one true instance */

/* ── Default downloads path ──────────────────────── */
std::string get_default_downloads_path() {
#ifdef _WIN32
    const char* user = getenv("USERPROFILE");
    if (user) {
        return std::string(user) + "\\Downloads";
    }
    return "C:\\Downloads";
#else
    const char* home = getenv("HOME");
    if (home) {
        return std::string(home) + "/Downloads";
    }
    return "/tmp";
#endif
}

static bool save_pending = false;

static void save_settings_impl() {
    save_pending = false;
    std::string path = "settings.cfg";
    std::ofstream f(path);
    if (!f) return;
    f << "loadThumbnails="   << (settings.loadThumbnails ? 1 : 0) << "\n";
    f << "showStatusBar="    << (settings.showStatusBar  ? 1 : 0) << "\n";
    f << "bufferSizeMB="     << settings.bufferSizeMB             << "\n";
    f << "initialFetchSize=" << settings.initialFetchSize         << "\n";
    f << "scrollBatchSize="  << settings.scrollBatchSize          << "\n";
    f << "enableAntialiasing=" << (settings.enableAntialiasing ? 1 : 0) << "\n";
    f << "searchProvider="   << static_cast<int>(settings.searchProvider) << "\n";
    f << "searchPlatform="   << settings.searchPlatform             << "\n";
    f << "downloadPath="     << settings.downloadPath             << "\n";
}

static void deferred_save_cb(void*) {
    save_settings_impl();
}

void save_settings() {
    if (save_pending) Fl::remove_timeout(deferred_save_cb);
    save_pending = true;
    Fl::add_timeout(0.5, deferred_save_cb);
}

void save_settings_now() {
    if (save_pending) Fl::remove_timeout(deferred_save_cb);
    save_settings_impl();
}

/* ── Load ────────────────────────────────────────── */
void load_settings() {
    settings.downloadPath = get_default_downloads_path();
    std::string path = "settings.cfg";
    std::ifstream f(path);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if      (key == "loadThumbnails")   settings.loadThumbnails   = (val == "1");
        else if (key == "showStatusBar")    settings.showStatusBar    = (val == "1");
        else if (key == "bufferSizeMB")     settings.bufferSizeMB     = std::stoi(val);
        else if (key == "initialFetchSize") settings.initialFetchSize = std::stoi(val);
        else if (key == "scrollBatchSize")  settings.scrollBatchSize  = std::stoi(val);
        else if (key == "enableAntialiasing") settings.enableAntialiasing = (val == "1");
        else if (key == "searchProvider") settings.searchProvider = static_cast<AppSettings::SearchProvider>(std::stoi(val));
        else if (key == "searchPlatform") settings.searchPlatform = std::stoi(val);
        else if (key == "downloadPath" && !val.empty()) settings.downloadPath = val;
    }
}
