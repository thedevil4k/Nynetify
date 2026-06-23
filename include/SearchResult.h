#ifndef SEARCHRESULT_H
#define SEARCHRESULT_H

#include <string>

struct SearchResult {
    std::string title;
    std::string author;
    std::string video_id;
    std::string channel_id;
    std::string duration;
    bool is_playlist = false;
    bool is_channel = false;
    bool is_live = false;
    bool is_twitch = false;
    bool is_video = false;
    std::string thumbnail_url;
    std::string length;
};

#endif
