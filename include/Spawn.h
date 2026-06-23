#ifndef SPAWN_H
#define SPAWN_H

#include <string>
#include <cstdio>
#include <memory>
#include <array>
#include <stdexcept>
#include <iostream>
#include <fstream>
#ifdef _WIN32
#include <windows.h>
#endif

inline std::string run_hidden(const std::string& cmd, bool debugMode = false, bool capture_output = false) {
#ifdef _WIN32
    STARTUPINFOA si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = debugMode ? SW_SHOW : SW_HIDE;
    PROCESS_INFORMATION pi;
    std::string tempFile;

    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };

    if (capture_output) {
        char tmpPath[MAX_PATH + 1] = {0};
        char tmpFile[MAX_PATH + 1] = {0};
        GetTempPathA(MAX_PATH, tmpPath);
        GetTempFileNameA(tmpPath, "NYT", 0, tmpFile);
        tempFile = tmpFile;

        HANDLE hFile = CreateFileA(tmpFile, GENERIC_WRITE, FILE_SHARE_READ, &sa,
                                   CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE) {
            si.hStdOutput = hFile;
            si.hStdError = hFile;
            si.dwFlags |= STARTF_USESTDHANDLES;
        }
    }

    DWORD creation_flags = debugMode ? 0 : CREATE_NO_WINDOW;
    std::string cmdline = cmd;
    if (CreateProcessA(NULL, &cmdline[0], NULL, NULL, TRUE, creation_flags, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        if (capture_output && !tempFile.empty()) {
            /* Close the handle we opened for writing */
            HANDLE hFile = si.hStdOutput;
            if (hFile != INVALID_HANDLE_VALUE) {
                CloseHandle(hFile);
            }

            std::string result;
            std::ifstream f(tempFile);
            if (f) {
                std::string line;
                while (std::getline(f, line)) {
                    if (!result.empty()) result += '\n';
                    result += line;
                }
            }
            DeleteFileA(tempFile.c_str());
            return result;
        }
        return {};
    }
    /* CreateProcess failed — clean up temp file if created */
    if (!tempFile.empty()) {
        if (si.hStdOutput && si.hStdOutput != INVALID_HANDLE_VALUE)
            CloseHandle(si.hStdOutput);
        DeleteFileA(tempFile.c_str());
    }
    return {};
#else
    std::string fullCmd = cmd;
    if (!debugMode) fullCmd += " 2>/dev/null";
    if (capture_output) {
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(fullCmd.c_str(), "r"), pclose);
        if (!pipe) throw std::runtime_error("popen() failed!");
        std::array<char, 256> buffer;
        std::string result;
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr)
            result += buffer.data();
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
        return result;
    } else {
        system(fullCmd.c_str());
        return {};
    }
#endif
}

#endif
