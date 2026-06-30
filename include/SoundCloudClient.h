#ifndef SOUNDCLOUDCLIENT_H
#define SOUNDCLOUDCLIENT_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include "SearchResult.h"
#include "Spawn.h"
#include "YoutubeService.h"

class SoundCloudClient {
public:
    static inline bool debugMode = false;

    static void setDebugMode(bool enabled) { debugMode = enabled; }

    static std::vector<SearchResult> search(const std::string& query, int max_results = 20) {
        std::vector<SearchResult> results;

        std::string prefix = "scsearch" + std::to_string(max_results) + ":";
        std::string command = std::string(YT_DLP) + std::string(YT_DLP_FAST)
            + " \"" + prefix + query + "\""
            + " --flat-playlist --ignore-config"
            + " --print title --print uploader --print webpage_url --print duration_string"
            + " --no-warnings" + NYN_NULL_REDIRECT;

        std::cout << "[SOUNDCLOUD] Searching: \"" << query << "\"" << std::endl;
        std::string output = run_hidden(command, debugMode, true);

        if (output.empty()) {
            std::cerr << "[SOUNDCLOUD] Empty search output" << std::endl;
            return results;
        }

        std::istringstream ss(output);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }

        for (size_t i = 0; i + 3 < lines.size(); i += 4) {
            SearchResult r;
            r.title       = lines[i];
            r.author      = lines[i + 1];
            r.video_id    = lines[i + 2];
            r.is_soundcloud = true;
            r.duration    = lines[i + 3];
            r.length      = lines[i + 3];
            results.push_back(r);
        }

        std::cout << "[SOUNDCLOUD] Found " << results.size() << " results" << std::endl;
        return results;
    }

    static std::string resolve_audio_url(const std::string& track_url) {
        std::string cmd = std::string(YT_DLP) + std::string(YT_DLP_FAST)
            + " -g -f bestaudio \"" + track_url + "\"" + NYN_NULL_REDIRECT;
        std::string url = run_hidden(cmd, debugMode, true);
        while (!url.empty() && (url.back() == '\r' || url.back() == '\n' || url.back() == ' ')) {
            url.pop_back();
        }
        return url;
    }
};

#endif
