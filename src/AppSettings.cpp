#include <fstream>
#include <cstdlib>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#define NYN_PATH_SEP "\\"
#else
#include <unistd.h>
#include <linux/limits.h>
#define NYN_PATH_SEP "/"
#endif
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

/* ── Save ────────────────────────────────────────── */
void save_settings() {
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    path += "settings.cfg";
#else
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len < 0) return;
    exePath[len] = '\0';
    std::string path(exePath);
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    path += "settings.cfg";
#endif

    std::ofstream f(path);
    if (!f) return;
    f << "loadThumbnails="   << (settings.loadThumbnails ? 1 : 0) << "\n";
    f << "showStatusBar="    << (settings.showStatusBar  ? 1 : 0) << "\n";
    f << "bufferSizeMB="     << settings.bufferSizeMB             << "\n";
    f << "initialFetchSize=" << settings.initialFetchSize         << "\n";
    f << "scrollBatchSize="  << settings.scrollBatchSize          << "\n";
    f << "downloadPath="     << settings.downloadPath             << "\n";
}

/* ── Load ────────────────────────────────────────── */
void load_settings() {
    settings.downloadPath = get_default_downloads_path();
#ifdef _WIN32
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path(exePath);
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    path += "settings.cfg";
#else
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    if (len < 0) return;
    exePath[len] = '\0';
    std::string path(exePath);
    size_t pos = path.find_last_of('/');
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    path += "settings.cfg";
#endif

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
        else if (key == "downloadPath" && !val.empty()) settings.downloadPath = val;
    }
}
