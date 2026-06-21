#ifndef TWITCHCLIENT_H
#define TWITCHCLIENT_H

#include <string>
#include <vector>
#include <ctime>
#include "SearchResult.h"

/*
 * TwitchClient — Search Twitch via the Helix API.
 *
 * Uses an anonymous app access token (no user login required).
 * The token is obtained via client_credentials grant and cached
 * for the duration of the session.
 *
 * Helix endpoints used:
 *   GET /helix/search/channels?query=...
 *   GET /helix/search/categories?query=...
 */
class TwitchClient {
public:
    static std::vector<SearchResult> search(const std::string& query, int max_results = 25);

    static void set_debug(bool d) { debug = d; }

private:
    static std::string get_anonymous_token();
    static std::string http_get(const std::string& url,
                                const std::string& client_id,
                                const std::string& bearer);
    static size_t write_cb(void* contents, size_t size, size_t nmemb, std::string* s);

    static inline std::string cached_token;
    static inline std::string cached_client_id;
    static inline time_t token_expiry = 0;
    static inline bool debug = false;
};

#endif
