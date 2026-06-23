#include "TwitchClient.h"
#include "nlohmann/json.hpp"
#include <curl/curl.h>
#include <iostream>

/*
 * TwitchClient.cpp — Twitch GQL search implementation.
 *
 * Uses the Twitch website's public Client-ID to query
 * gql.twitch.tv/gql directly (no OAuth, no registration).
 *
 * For playback, construct https://www.twitch.tv/{login} —
 * mpv + yt-dlp handles HLS extraction natively.
 */

static const char* TWITCH_GQL_URL     = "https://gql.twitch.tv/gql";
static const char* TWITCH_GQL_CLIENT_ID = "kimne78kx3ncx6brgo4mv6wki5h1ko";

/* ── curl write callback ─────────────────────────────── */
size_t TwitchClient::write_cb(void* contents, size_t size, size_t nmemb, std::string* s) {
    s->append((char*)contents, size * nmemb);
    return size * nmemb;
}

/* ── POST raw GraphQL to gql.twitch.tv ───────────────── */
std::string TwitchClient::gql_post(const std::string& query_graphql) {
    std::string response;
    CURL* curl = curl_easy_init();
    if (!curl) return response;

    struct curl_slist* headers = nullptr;
    std::string cid_header = std::string("Client-ID: ") + TWITCH_GQL_CLIENT_ID;
    headers = curl_slist_append(headers, cid_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, TWITCH_GQL_URL);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, query_graphql.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)query_graphql.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (res != CURLE_OK) {
        std::cerr << "[TWITCH] GQL POST failed: " << curl_easy_strerror(res) << std::endl;
    } else if (http_code != 200) {
        std::cerr << "[TWITCH] GQL HTTP " << http_code << std::endl;
        if (debug) std::cerr << "[TWITCH] Response: " << response.substr(0, 500) << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return response;
}

/* ── Search: GQL query → SearchResult vector ─────────── */
std::vector<SearchResult> TwitchClient::search(const std::string& query, int max_results) {
    std::vector<SearchResult> results;

    /* Escape the query for JSON embedding */
    char* escaped = curl_easy_escape(nullptr, query.c_str(), 0);
    std::string q_escaped(escaped ? escaped : query);
    if (escaped) curl_free(escaped);

    /* Build the GraphQL request — raw query, no persisted hash needed */
    std::string gql_body = R"([
  {
    "operationName": "SearchResultsPage_SearchResults",
    "variables": {"searchTerm": ")" + q_escaped + R"("},
    "query": "query SearchResultsPage_SearchResults($searchTerm: String!) { searchFor(userQuery: $searchTerm, platform: \"all\") { channels { items { ... on User { id displayName login description followers { totalCount } profileImageURL(width: 50) stream { id viewersCount title game { displayName } } } } } videos { items { id title previewThumbnailURL lengthSeconds game { displayName } owner { login displayName } } } } }"
  }
])";

    std::cout << "[TWITCH] Searching: \"" << query << "\"" << std::endl;

    std::string resp = gql_post(gql_body);
    if (resp.empty()) {
        std::cerr << "[TWITCH] Empty response from GQL" << std::endl;
        return results;
    }

    try {
        auto j = nlohmann::json::parse(resp);
        /* GQL wraps in array; unwrap */
        if (j.is_array() && !j.empty()) j = j[0];

        auto& searchFor = j["data"]["searchFor"];

        /* Parse channels */
        auto& channels = searchFor["channels"]["items"];
        if (channels.is_array()) {
            for (auto& item : channels) {
                SearchResult sr;
                sr.is_twitch   = true;
                sr.is_channel  = true;
                sr.channel_id  = item.value("id", "");
                sr.video_id    = item.value("login", "");
                sr.title       = item.value("displayName", "");
                sr.author      = item.value("displayName", "");

                if (!item["stream"].is_null()) {
                    sr.is_live = true;
                    auto& stream = item["stream"];
                    std::string game;
                    if (stream.contains("game") && !stream["game"].is_null())
                        game = stream["game"].value("displayName", "");
                    std::string stream_title = stream.value("title", "");
                    int viewers = stream.value("viewersCount", 0);

                    if (!game.empty())
                        sr.title = sr.author + " - " + game;
                    if (viewers > 0)
                        sr.title += " [" + std::to_string(viewers) + " viewers]";
                    if (!stream_title.empty())
                        sr.title += " | " + stream_title;
                }

                results.push_back(sr);
                if ((int)results.size() >= max_results) break;
            }
        }

        /* Parse videos */
        auto& videos = searchFor["videos"]["items"];
        if (videos.is_array() && (int)results.size() < max_results) {
            for (auto& item : videos) {
                SearchResult sr;
                sr.is_twitch   = true;
                sr.is_video    = true;
                sr.channel_id  = item["owner"].value("login", "");
                sr.video_id    = item.value("id", "");
                sr.title       = item.value("title", "");
                sr.author      = item["owner"].value("displayName", "");
                sr.thumbnail_url = item.value("previewThumbnailURL", "");
                sr.length      = std::to_string(item.value("lengthSeconds", 0));

                if (item.contains("game") && !item["game"].is_null())
                    sr.title += " - " + item["game"].value("displayName", "");

                results.push_back(sr);
                if ((int)results.size() >= max_results) break;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[TWITCH] Parse error: " << e.what() << std::endl;
        if (debug) std::cerr << "[TWITCH] Raw: " << resp.substr(0, 1000) << std::endl;
    }

    std::cout << "[TWITCH] Found " << results.size() << " results (channels + videos)" << std::endl;
    return results;
}
