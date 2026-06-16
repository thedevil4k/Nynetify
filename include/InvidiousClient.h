#ifndef INVIDIOUSCLIENT_H
#define INVIDIOUSCLIENT_H

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <curl/curl.h>
#include "nlohmann/json.hpp"

struct SearchResult;

class InvidiousClient {
public:
    // Base URL de la instancia de Invidious (puede cambiarse en runtime)
    static inline std::string base_url = "https://iv.datura.dev";

    // Buscar videos/canales/playlists
    static std::vector<SearchResult> search(const std::string& query, int max_results = 20);

    static std::vector<SearchResult> get_playlist_videos(const std::string& playlist_id);

    static std::vector<SearchResult> get_channel_videos(const std::string& channel_id);

    static std::string get_audio_url(const std::string& video_id);

private:
    static std::string url_encode(const std::string& value);
    static std::string http_get(const std::string& url);
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* response);
};

#endif
