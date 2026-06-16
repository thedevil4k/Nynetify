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
};

#endif
