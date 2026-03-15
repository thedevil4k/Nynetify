#ifndef YOUTUBESERVICE_H
#define YOUTUBESERVICE_H

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>

// Full path to yt-dlp so it works regardless of system PATH
#ifdef _WIN32
    static const char* YT_DLP = "C:/msys64/mingw64/bin/yt-dlp.exe";
#else
    static const char* YT_DLP = "yt-dlp";
#endif

struct SearchResult {
    std::string title;
    std::string author;
    std::string video_id;
    std::string duration;
};

class YoutubeService {
public:
    static std::vector<SearchResult> get_metadata(const std::vector<std::string>& ids) {
        std::vector<SearchResult> results;
        if (ids.empty()) return results;

        std::string command = std::string(YT_DLP) + " --ignore-config --no-warnings --flat-playlist --print title --print uploader --print id";
        for (const auto& id : ids) {
            command += " \"https://www.youtube.com/watch?v=" + id + "\"";
        }
        command += " 2>NUL";

        std::cout << "[DEBUG] Fetching metadata: " << command << std::endl;
        std::string output = execute(command);
        
        std::istringstream ss(output);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }

        for (size_t i = 0; i + 2 < lines.size(); i += 3) {
            SearchResult r;
            r.title    = lines[i];
            r.author   = lines[i + 1];
            r.video_id = lines[i + 2];
            results.push_back(r);
        }
        return results;
    }

    static std::vector<SearchResult> search(const std::string& query, const std::string& region = "", int filter_type = 0) {
        std::vector<SearchResult> results;

        std::string prefix = "ytsearch20:";
        std::string modified_query = query;

        if (filter_type == 1) { // Only Songs
            prefix = "ytsearch20:";
            modified_query += " song"; // Simple heuristic
        } else if (filter_type == 2) { // Playlist
            prefix = "ytsearchplaylist20:";
        }

        std::string command = std::string(YT_DLP)
            + " \"" + prefix + modified_query + "\""
            + " --flat-playlist --ignore-config --print title --print uploader --print id --no-warnings";
        
        if (!region.empty()) {
            command += " --geo-bypass-country " + region;
        }

        command += " 2>NUL";

        std::cout << "[DEBUG] Executing search command: " << command << std::endl;
        std::string output = execute(command);
        
        if (output.empty()) {
            std::cout << "[DEBUG] Command output was empty." << std::endl;
            return results;
        }

        std::istringstream ss(output);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }

        std::cout << "[DEBUG] Received " << lines.size() << " lines of output." << std::endl;

        for (size_t i = 0; i + 2 < lines.size(); i += 3) {
            SearchResult r;
            r.title    = lines[i];
            r.author   = lines[i + 1];
            r.video_id = lines[i + 2];
            results.push_back(r);
        }

        return results;
    }

    static std::string get_audio_url(const std::string& video_id) {
        return "https://www.youtube.com/watch?v=" + video_id;
    }

    static std::string get_thumbnail_url(const std::string& video_id) {
        // We use the predictable HQ thumbnail URL for performance
        return "https://img.youtube.com/vi/" + video_id + "/hqdefault.jpg";
    }

private:
    static std::string execute(const std::string& cmd) {
        std::array<char, 256> buffer;
        std::string result;
#ifdef _WIN32
        std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
#else
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
#endif
        if (!pipe) {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
            result += buffer.data();
        }
        // Remove trailing newline
        while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
            result.pop_back();
        return result;
    }
};

#endif
