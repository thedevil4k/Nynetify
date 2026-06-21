#include "InvidiousClient.h"
#include <curl/curl.h>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <sstream>
#include "nlohmann/json.hpp"
#include "SearchResult.h"

// URL encoding helper
std::string InvidiousClient::url_encode(const std::string& value) {
    std::string encoded;
    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char buf[4];
            snprintf(buf, sizeof(buf), "%%%02X", static_cast<unsigned char>(c));
            encoded += buf;
        }
    }
    return encoded;
}

// CURL write callback
size_t InvidiousClient::write_callback(void* contents, size_t size, size_t nmemb, std::string* response) {
    size_t total_size = size * nmemb;
    response->append(static_cast<char*>(contents), total_size);
    return total_size;
}

// HTTP GET via libcurl
std::string InvidiousClient::http_get(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "[Invidious] Failed to init CURL" << std::endl;
        return "";
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Nynetify/1.0");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "[Invidious] CURL error: " << curl_easy_strerror(res) << std::endl;
        curl_easy_cleanup(curl);
        return "";
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (http_code != 200) {
        std::cerr << "[Invidious] HTTP error: " << http_code << std::endl;
        return "";
    }

    return response;
}

// Search videos/playlists/channels via Invidious API
std::vector<SearchResult> InvidiousClient::search(const std::string& query, int max_results) {
    std::vector<SearchResult> results;
    std::string encoded = url_encode(query);
    std::string url = base_url + "/api/v1/search?q=" + encoded + "&type=all&sort_by=relevance";

    std::string response = http_get(url);
    if (response.empty()) {
        std::cerr << "[Invidious] Empty search response" << std::endl;
        return results;
    }

    try {
        auto json = nlohmann::json::parse(response);
        if (!json.is_array()) {
            std::cerr << "[Invidious] Invalid search response format" << std::endl;
            return results;
        }

        int count = 0;
        for (const auto& item : json) {
            if (count >= max_results) break;

            SearchResult result;
            auto type = item.value("type", "video");

            if (type == "video") {
                result.title = item.value("title", "Unknown");
                result.author = item.value("author", "Unknown");
                result.video_id = item.value("videoId", "");
                result.channel_id = item.value("authorId", "");
                auto length = item.value("lengthSeconds", 0);
                int mins = length / 60;
                int secs = length % 60;
                result.duration = std::to_string(mins) + ":" + (secs < 10 ? "0" : "") + std::to_string(secs);
                results.push_back(result);
                count++;
            } else if (type == "playlist") {
                result.title = item.value("title", "Unknown");
                result.author = item.value("author", "Unknown");
                result.video_id = item.value("playlistId", "");
                result.is_playlist = true;
                results.push_back(result);
                count++;
            } else if (type == "channel") {
                result.title = item.value("author", "Unknown");
                result.author = item.value("author", "Unknown");
                result.video_id = item.value("authorId", "");
                result.is_channel = true;
                results.push_back(result);
                count++;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Invidious] Parse error: " << e.what() << std::endl;
    }

    return results;
}

// Get videos from a playlist
std::vector<SearchResult> InvidiousClient::get_playlist_videos(const std::string& playlist_id) {
    std::vector<SearchResult> results;
    std::string url = base_url + "/api/v1/playlists/" + playlist_id;
    std::string response = http_get(url);

    if (response.empty()) return results;

    try {
        auto json = nlohmann::json::parse(response);
        auto videos = json.value("videos", nlohmann::json::array());
        for (const auto& item : videos) {
            SearchResult result;
            result.title = item.value("title", "Unknown");
            result.author = item.value("author", "Unknown");
            result.video_id = item.value("videoId", "");
            result.channel_id = item.value("authorId", "");
            results.push_back(result);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Invidious] Playlist parse error: " << e.what() << std::endl;
    }

    return results;
}

// Get videos from a channel
std::vector<SearchResult> InvidiousClient::get_channel_videos(const std::string& channel_id) {
    std::vector<SearchResult> results;
    std::string url = base_url + "/api/v1/channels/" + channel_id + "/videos";
    std::string response = http_get(url);

    if (response.empty()) return results;

    try {
        auto json = nlohmann::json::parse(response);
        auto videos = json.value("videos", nlohmann::json::array());
        for (const auto& item : videos) {
            SearchResult result;
            result.title = item.value("title", "Unknown");
            result.author = item.value("author", "Unknown");
            result.video_id = item.value("videoId", "");
            result.channel_id = channel_id;
            results.push_back(result);
        }
    } catch (const std::exception& e) {
        std::cerr << "[Invidious] Channel parse error: " << e.what() << std::endl;
    }

    return results;
}

// Get direct audio URL for a video
std::string InvidiousClient::get_audio_url(const std::string& video_id) {
    std::string url = base_url + "/api/v1/videos/" + video_id;
    std::string response = http_get(url);

    if (response.empty()) return "";

    try {
        auto json = nlohmann::json::parse(response);
        // Try formatStreams first (legacy)
        auto formats = json.value("formatStreams", nlohmann::json::array());
        for (const auto& format : formats) {
            if (format.value("mimeType", "").find("audio") != std::string::npos) {
                return format.value("url", "");
            }
        }
        // Fallback to adaptiveFormats
        auto adaptive = json.value("adaptiveFormats", nlohmann::json::array());
        for (const auto& format : adaptive) {
            if (format.value("mimeType", "").find("audio") != std::string::npos) {
                return format.value("url", "");
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[Invidious] URL parse error: " << e.what() << std::endl;
    }

    return "";
}
