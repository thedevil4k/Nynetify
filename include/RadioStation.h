#ifndef RADIOSTATION_H
#define RADIOSTATION_H

#include <string>
#include <vector>

struct RadioStation {
    int id;
    std::string name;
    std::string stream_url;
    std::string genre;
    std::string country_code;
    std::string country_name;
    std::string codec;
    int bitrate;
    bool is_custom = false;
    std::string logo;
};

#endif
