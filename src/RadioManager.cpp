#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <iomanip>
#include "RadioManager.h"

static std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            escaped << c;
        else
            escaped << '%' << std::setw(2) << (int)c;
    }
    return escaped.str();
}

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef NYN_PATH_SEP
#ifdef _WIN32
#define NYN_PATH_SEP "\\"
#else
#define NYN_PATH_SEP "/"
#endif
#endif

void RadioManager::load_customs() {
    std::string path = "RADIO" NYN_PATH_SEP "custom.txt";
    if (!std::filesystem::exists(path)) return;
    std::ifstream ifs(path);
    std::string line;
    int max_id = (int)stations.size();
    while (std::getline(ifs, line)) {
        std::stringstream ss(line);
        std::string name, url, genre, country;
        std::getline(ss, name, '|');
        std::getline(ss, url, '|');
        std::getline(ss, genre, '|');
        std::getline(ss, country, '|');
        if (name.empty() || url.empty()) continue;
        RadioStation s;
        s.id = max_id++;
        s.name = name;
        s.stream_url = url;
        s.genre = genre.empty() ? "Other" : genre;
        s.country_code = country.empty() ? "INT" : country;
        s.country_name = s.country_code;
        s.codec = "MP3";
        s.bitrate = 128;
        s.is_custom = true;
        stations.push_back(s);
    }
}

void RadioManager::save_customs() {
    std::filesystem::create_directories("RADIO");
    std::ofstream ofs("RADIO" NYN_PATH_SEP "custom.txt");
    for (const auto& s : stations) {
        if (!s.is_custom) continue;
        ofs << s.name << "|" << s.stream_url << "|"
            << s.genre << "|" << s.country_code << "\n";
    }
}

void RadioManager::load_favs() {
    fav_cache.clear();
    std::string path = "RADIO" NYN_PATH_SEP "favs.txt";
    if (!std::filesystem::exists(path)) return;
    std::ifstream ifs(path);
    int id;
    while (ifs >> id) fav_cache.insert(id);
}

void RadioManager::save_favs() {
    std::filesystem::create_directories("RADIO");
    std::ofstream ofs("RADIO" NYN_PATH_SEP "favs.txt");
    for (int id : fav_cache) ofs << id << "\n";
}

struct RadioSearchTask {
    std::string query;
    std::vector<RadioStation> results;
    bool done = false;
};

std::vector<RadioStation> RadioManager::search_online(const std::string& query) {
    std::vector<RadioStation> results;
    std::string url = "https://de1.api.radio-browser.info/json/stations/byname/"
                      + url_encode(query);
    std::string temp_file = "radio_search.json";
    std::string cmd = "curl -s -L -o " + temp_file + " --max-time 15 \"" + url + "\"";
    system(cmd.c_str());

    if (!std::filesystem::exists(temp_file)) return results;

    try {
        std::ifstream ifs(temp_file);
        std::string json((std::istreambuf_iterator<char>(ifs)), {});
        ifs.close();
        std::filesystem::remove(temp_file);

        // Rough JSON parsing — find station objects
        size_t pos = 0;
        int next_id = 20000;
        while ((pos = json.find("\"name\"", pos)) != std::string::npos) {
            size_t name_start = json.find('"', pos + 7);
            if (name_start == std::string::npos) break;
            size_t name_end = json.find('"', name_start + 1);
            if (name_end == std::string::npos) break;
            std::string name = json.substr(name_start + 1, name_end - name_start - 1);

            size_t url_pos = json.find("\"url\"", name_end);
            if (url_pos == std::string::npos || url_pos > pos + 500) { pos = name_end; continue; }
            size_t url_start = json.find('"', url_pos + 6);
            if (url_start == std::string::npos) break;
            size_t url_end = json.find('"', url_start + 1);
            if (url_end == std::string::npos) break;
            std::string stream = json.substr(url_start + 1, url_end - url_start - 1);
            for (auto& c : stream) if (c == '\\') { c = '/'; break; } // unescape

            // Find genre
            std::string genre = "Other";
            size_t genre_pos = json.find("\"tags\"", url_end);
            if (genre_pos != std::string::npos && genre_pos < url_end + 200) {
                size_t gs = json.find('"', genre_pos + 7);
                if (gs != std::string::npos) {
                    size_t ge = json.find('"', gs + 1);
                    if (ge != std::string::npos) {
                        std::string raw = json.substr(gs + 1, ge - gs - 1);
                        if (!raw.empty()) {
                            // Take first tag
                            size_t comma = raw.find(',');
                            genre = (comma == std::string::npos) ? raw : raw.substr(0, comma);
                        }
                    }
                }
            }

            // Find country code
            std::string country = "INT";
            size_t cc_pos = json.find("\"countrycode\"", url_end);
            if (cc_pos != std::string::npos && cc_pos < url_end + 200) {
                size_t cs = json.find('"', cc_pos + 14);
                if (cs != std::string::npos) {
                    size_t ce = json.find('"', cs + 1);
                    if (ce != std::string::npos)
                        country = json.substr(cs + 1, ce - cs - 1);
                }
            }

            RadioStation s;
            s.id = next_id++;
            s.name = name;
            s.stream_url = stream;
            s.genre = genre;
            s.country_code = country;
            s.country_name = country;
            s.codec = "MP3";
            s.bitrate = 128;
            s.is_custom = true;
            results.push_back(s);
            if (results.size() >= 50) break;

            pos = url_end;
        }
    } catch (...) {
        std::filesystem::remove(temp_file);
    }
    return results;
}
