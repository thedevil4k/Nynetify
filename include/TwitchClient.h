#ifndef TWITCHCLIENT_H
#define TWITCHCLIENT_H

#include <string>
#include <vector>
#include "SearchResult.h"

/*
 * TwitchClient — Search Twitch via the public GraphQL API.
 *
 * Uses the Twitch website's Client-ID to query gql.twitch.tv/gql
 * directly. No registration, no token, no OAuth needed.
 */
class TwitchClient {
public:
    static std::vector<SearchResult> search(const std::string& query, int max_results = 25);

    static void set_debug(bool d) { debug = d; }

private:
    static std::string gql_post(const std::string& query_graphql);
    static size_t write_cb(void* contents, size_t size, size_t nmemb, std::string* s);

    static inline bool debug = false;
};

#endif
