#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <string>

#ifdef _WIN32
#define NYN_PATH_SEP "\\"
#else
#define NYN_PATH_SEP "/"
#endif

/*
 * AppSettings: persistent user configuration
 *
 * Stores UI preferences (thumbnails, status bar, buffer size),
 * search behaviour (fetch size, scroll batch) and the download
 * path.  Loaded from / saved to settings.cfg next to the exe.
 */
struct AppSettings {
    bool loadThumbnails = true;
    bool showStatusBar  = true;
    int  bufferSizeMB   = 2;
    int  initialFetchSize = 50;
    int  scrollBatchSize = 2;
    bool enableAntialiasing = true;
    bool debugMode = false;
    std::string downloadPath;
    
    // Search provider preference (YouTube backend)
    enum class SearchProvider { Invidious, YTDLP };
    SearchProvider searchProvider = SearchProvider::Invidious;

    // Search platform preference (persisted across sessions)
    int searchPlatform = 0; // 0 = YouTube, 1 = Twitch
};

/* Persistence helpers */
void save_settings();
void load_settings();

/* Returns the OS default Downloads folder (%USERPROFILE%\Downloads) */
std::string get_default_downloads_path();

#endif // APPSETTINGS_H
