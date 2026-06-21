#include "TwitchClient.h"
#include "nlohmann/json.hpp"
#include <curl/curl.h>
#include <iostream>
#include <sstream>

/*
 * TwitchClient.cpp — Helix API search implementation.
 *
 * Flow:
 *   1. Obtain anonymous token via POST /oauth2/token (cached ~60 days)
 *   2. GET /helix/search/channels?query=...&first=N
 *   3. GET /helix/search/categories?query=...&first=N  (to show game/category)
 *   4. Combine results into SearchResult vector
 *
 * For playback, construct https://www.twitch.tv/{login} — mpv + yt-dlp
 * handles the rest natively (HLS extraction).
 */

/* ── curl write callback ─────────────────────────────── */
size_t TwitchClient::write_cb(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/* ── HTTP GET with Bearer + Client-ID ────────────────── */
std::string TwitchClient::http_get(const std::string& url,
                                    const std::string& client_id,
                                    const std::string& bearer) {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return response;

    struct curl_slist* headers = nullptr;
    std::string auth_header = "Authorization: Bearer " + bearer;
    std::string cid_header  = "Client-Id: " + client_id;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, cid_header.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        std::cerr << "[TWITCH] HTTP GET failed: " << curl_easy_strerror(res) << std::endl;
    } else if (http_code != 200) {
        std::cerr << "[TWITCH] HTTP " << http_code << " for " << url << std::endl;
        if (debug) std::cerr << "[TWITCH] Response: " << response.substr(0, 500) << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

/* ── Anonymous app access token ──────────────────────── */
std::string TwitchClient::get_anonymous_token() {
    time_t now = time(nullptr);
    if (!cached_token.empty() && now < token_expiry)
        return cached_token;

    /* The Twitch website's public client ID (visible in twitch.tv source).
     * Used by many open-source Twitch clients for anonymous access. */
    cached_client_id = "kimne78kx3ncx6rggo4klvchwqklzqbw";

    std::string url = "https://id.twitch.tv/oauth2/token"
                      "?client_id=" + cached_client_id +
                      "&grant_type=client_credentials";

    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return cached_token;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        std::cerr << "[TWITCH] Failed to get anonymous token (HTTP " << http_code << ")" << std::endl;
        return cached_token;
    }

    try {
        auto j = nlohmann::json::parse(response);
        cached_token = j["access_token"].get<std::string>();
        int expires_in = j.value("expires_in", 0);
        token_expiry = now + expires_in - 60; // refresh 60s early
        std::cout << "[TWITCH] Got anonymous token, expires in " << expires_in << "s" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[TWITCH] Token parse error: " << e.what() << std::endl;
    }

    return cached_token;
}

/* ── Search: channels + categories → SearchResult vector ── */
std::vector<SearchResult> TwitchClient::search(const std::string& query, int max_results) {
    std::vector<SearchResult> results;

    std::string token = get_anonymous_token();
    if (token.empty() || cached_client_id.empty()) {
        std::cerr << "[TWITCH] No token available, cannot search" << std::endl;
        return results;
    }

    std::cout << "[TWITCH] Searching: \"" << query << "\"" << std::endl;

    /* ── 1. Search channels ───────────────────────────── */
    std::string ch_url = "https://api.twitch.tv/helix/search/channels?query="
                         + std::string(curl_easy_escape(nullptr, query.c_str(), 0))
                         + "&first=" + std::to_string(max_results);
    std::string ch_resp = http_get(ch_url, cached_client_id, token);

    std::vector<nlohmann::json> channel_data;
    try {
        auto j = nlohmann::json::parse(ch_resp);
        if (j.contains("data") && j["data"].is_array()) {
            for (auto& ch : j["data"]) {
                channel_data.push_back(ch);

                SearchResult sr;
                sr.is_channel  = true;
                sr.is_twitch   = true;
                sr.is_live     = ch.value("is_live", false);
                sr.video_id    = ch.value("broadcaster_login", "");  // channel login for URL
                sr.channel_id  = ch.value("id", "");
                sr.title       = ch.value("display_name", "");
                sr.author      = ch.value("broadcaster_login", "");

                /* Build a descriptive title: "DisplayName - game if live" */
                std::string display = ch.value("display_name", "");
                std::string game    = ch.value("game_name", "");
                bool live           = ch.value("is_live", false);
                int  viewers        = ch.value("view_count", 0);

                if (live && !game.empty())
                    sr.title = display + " - " + game;
                else if (!game.empty())
                    sr.title = display + " - " + game;
                else
                    sr.title = display;

                results.push_back(sr);

                if ((int)results.size() >= max_results) break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[TWITCH] Channel parse error: " << e.what() << std::endl;
    }

    std::cout << "[TWITCH] Found " << results.size() << " channel results" << std::endl;
    return results;
}
