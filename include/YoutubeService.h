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

static const char* YT_DLP = "yt-dlp";
static const char* YT_DLP_FAST = " --extractor-args \"youtube:skip=hls,dash;player_client=android\" --socket-timeout 5 --retries 1";
#ifdef _WIN32
static const char* NYN_NULL_REDIRECT = " 2>NUL";
#else
static const char* NYN_NULL_REDIRECT = " 2>/dev/null";
#endif

struct SearchResult {
    std::string title;
    std::string author;
    std::string video_id;
    std::string channel_id;
    std::string duration;
    bool is_playlist = false;
    bool is_channel = false;
};

class YoutubeService {
public:
    static std::vector<SearchResult> get_metadata(const std::vector<std::string>& ids) {
        std::vector<SearchResult> results;
        if (ids.empty()) return results;

        std::string command = std::string(YT_DLP) + std::string(YT_DLP_FAST) + " --ignore-config --no-warnings --flat-playlist --print title --print uploader --print channel_id --print id --no-playlist";
        for (const auto& id : ids) {
            command += " \"https://www.youtube.com/watch?v=" + id + "\"";
        }
        command += NYN_NULL_REDIRECT;

        std::cout << "[DEBUG] Fetching metadata: " << command << std::endl;
        std::string output = execute(command);
        
        std::istringstream ss(output);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }

        for (size_t i = 0; i + 3 < lines.size(); i += 4) {
            SearchResult r;
            r.title      = lines[i];
            r.author     = lines[i + 1];
            r.channel_id = lines[i + 2];
            r.video_id   = lines[i + 3];
            results.push_back(r);
        }
        return results;
    }

    static std::vector<SearchResult> search(const std::string& query, const std::string& region = "", int filter_type = 0, int max_results = 50) {
        std::vector<SearchResult> results;

        std::string command;

        if (filter_type == 2) { // Playlist: use YouTube search URL with playlist filter
            std::string encoded = query;
            size_t p = 0;
            while ((p = encoded.find(' ', p)) != std::string::npos) {
                encoded.replace(p, 1, "+");
                p += 1;
            }
            command = std::string(YT_DLP) + std::string(YT_DLP_FAST)
                + " \"https://www.youtube.com/results?search_query=" + encoded + "&sp=EgIQAw==\""
                + " --flat-playlist --ignore-config --print title --print uploader --print id --no-warnings";
        } else if (filter_type == 3) { // Channels: use YouTube search URL with channel filter
            std::string encoded = query;
            size_t p = 0;
            while ((p = encoded.find(' ', p)) != std::string::npos) {
                encoded.replace(p, 1, "+");
                p += 1;
            }
            command = std::string(YT_DLP) + std::string(YT_DLP_FAST)
                + " \"https://www.youtube.com/results?search_query=" + encoded + "&sp=EgIQAg==\""
                + " --flat-playlist --ignore-config --print title --print channel --print channel_id --no-warnings";
        } else {
            std::string prefix = "ytsearch" + std::to_string(max_results) + ":";
            std::string modified_query = query;
            if (filter_type == 1) { // Only Songs
                modified_query += " song";
            }
            command = std::string(YT_DLP) + std::string(YT_DLP_FAST)
                + " \"" + prefix + modified_query + "\""
                + " --flat-playlist --ignore-config --print title --print uploader --print channel_id --print id --no-warnings";
        }

        if (!region.empty()) {
            command += " --geo-bypass-country " + region;
        }

        command += NYN_NULL_REDIRECT;

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

        // Songs (filter_type 0/1): 4 fields per result (title, uploader, channel_id, id)
        // Playlists (filter_type 2): 3 fields per result (title, uploader, id)
        // Channels (filter_type 3): 3 fields per result (title, channel, channel_id)
        int step = (filter_type == 0 || filter_type == 1) ? 4 : 3;
        for (size_t i = 0; i + step - 1 < lines.size(); i += step) {
            SearchResult r;
            r.title  = lines[i];
            r.author = lines[i + 1];
            if (step == 4) {
                r.channel_id = lines[i + 2];
                r.video_id   = lines[i + 3];
            } else {
                r.video_id   = lines[i + 2]; // playlist_id or channel_id
                if (filter_type == 2) {
                    r.is_playlist = true;
                } else if (filter_type == 3) {
                    r.is_channel = true;
                    r.channel_id = lines[i + 2];
                }
            }
            results.push_back(r);
        }

        return results;
    }

    static std::vector<SearchResult> get_playlist_videos(const std::string& playlist_id) {
        std::vector<SearchResult> results;
        std::string url = "https://www.youtube.com/playlist?list=" + playlist_id;
        std::string command = std::string(YT_DLP) + std::string(YT_DLP_FAST)
            + " \"" + url + "\""
            + " --flat-playlist --ignore-config --print title --print uploader --print channel_id --print id --no-warnings" + NYN_NULL_REDIRECT;

        std::cout << "[DEBUG] Fetching playlist videos: " << command << std::endl;
        std::string output = execute(command);

        if (output.empty()) {
            std::cout << "[DEBUG] Playlist output was empty." << std::endl;
            return results;
        }

        std::istringstream ss(output);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(ss, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) lines.push_back(line);
        }

        std::cout << "[DEBUG] Received " << lines.size() << " lines for playlist." << std::endl;

        for (size_t i = 0; i + 3 < lines.size(); i += 4) {
            SearchResult r;
            r.title      = lines[i];
            r.author     = lines[i + 1];
            r.channel_id = lines[i + 2];
            r.video_id   = lines[i + 3];
            results.push_back(r);
        }

        return results;
    }

    static std::vector<SearchResult> get_channel_content(const std::string& channel_id, const std::string& channel_name) {
        std::vector<SearchResult> results;

        // Fetch playlists from the channel
        std::string pl_url = "https://www.youtube.com/channel/" + channel_id + "/playlists";
        std::string cmd = std::string(YT_DLP) + std::string(YT_DLP_FAST)
            + " \"" + pl_url + "\""
            + " --flat-playlist --ignore-config --print title --print uploader --print id --no-warnings" + NYN_NULL_REDIRECT;
        std::cout << "[DEBUG] Fetching channel playlists: " << cmd << std::endl;
        std::string output = execute(cmd);
        if (!output.empty()) {
            std::istringstream ss(output);
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(ss, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) lines.push_back(line);
            }
            for (size_t i = 0; i + 2 < lines.size(); i += 3) {
                SearchResult r;
                r.title       = lines[i];
                r.author      = lines[i + 1];
                r.video_id    = lines[i + 2];
                r.channel_id  = channel_id;
                r.is_playlist = true;
                results.push_back(r);
            }
        }

        // Fetch videos from the channel
        std::string vid_url = "https://www.youtube.com/channel/" + channel_id + "/videos";
        cmd = std::string(YT_DLP) + std::string(YT_DLP_FAST)
            + " \"" + vid_url + "\""
            + " --flat-playlist --ignore-config --print title --print uploader --print id --no-warnings" + NYN_NULL_REDIRECT;
        std::cout << "[DEBUG] Fetching channel videos: " << cmd << std::endl;
        output = execute(cmd);
        if (!output.empty()) {
            std::istringstream ss(output);
            std::string line;
            std::vector<std::string> lines;
            while (std::getline(ss, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) lines.push_back(line);
            }
            for (size_t i = 0; i + 2 < lines.size(); i += 3) {
                SearchResult r;
                r.title      = lines[i];
                r.author     = channel_name;
                r.video_id   = lines[i + 2];
                r.channel_id = channel_id;
                results.push_back(r);
            }
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

    static std::string get_channel_id(const std::string& video_id) {
        std::string cmd = std::string(YT_DLP) + std::string(YT_DLP_FAST)
            + " --ignore-config --print channel_id --no-playlist"
            + " \"https://www.youtube.com/watch?v=" + video_id + "\"" + NYN_NULL_REDIRECT;
        return execute(cmd);
    }

    static std::string get_channel_avatar_url(const std::string& channel_id) {
        std::string cmd = std::string("curl -s \"https://www.youtube.com/channel/") + channel_id + "\"" + NYN_NULL_REDIRECT;
        std::string html = execute(cmd);
        std::string marker = "property=\"og:image\" content=\"";
        size_t pos = html.find(marker);
        if (pos != std::string::npos) {
            pos += marker.size();
            size_t end = html.find('\"', pos);
            if (end != std::string::npos)
                return html.substr(pos, end - pos);
        }
        return "";
    }

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
