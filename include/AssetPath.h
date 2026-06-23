#ifndef ASSETPATH_H
#define ASSETPATH_H

#include <string>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

inline std::string get_asset_path(const std::string& filename) {
    std::filesystem::path exe_path;
#ifdef _WIN32
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    exe_path = buf;
#else
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf));
    if (len > 0) {
        buf[len] = '\0';
        exe_path = buf;
    }
#endif
    return (exe_path.parent_path() / "assets" / filename).string();
}

#endif
