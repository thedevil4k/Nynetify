#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <iomanip>
#include <cctype>
#include "RadioManager.h"
#include "AssetPath.h"
#include "Spawn.h"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

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

static std::string stations_path() { return get_asset_path("stations.json"); }
static std::string favs_path()     { return "radio_favs.json"; }

void RadioManager::init(const std::string& country_code) {
    user_country_code = country_code;

    // Load stations from JSON
    std::string sp = stations_path();
    if (!std::filesystem::exists(sp)) {
        std::cerr << "[RadioManager] ERROR: " << sp << " not found. No stations loaded." << std::endl;
        return;
    }
    try {
        std::ifstream ifs(sp);
        json j;
        ifs >> j;
        stations.clear();
        for (const auto& obj : j) {
            RadioStation s;
            s.id           = obj.value("id", 0);
            s.name         = obj.value("name", "");
            s.stream_url   = obj.value("stream_url", "");
            s.genre        = obj.value("genre", "Other");
            s.country_code = obj.value("country_code", "INT");
            s.country_name = obj.value("country_name", "");
            s.codec        = obj.value("codec", "MP3");
            s.bitrate      = obj.value("bitrate", 128);
            s.is_custom    = obj.value("is_custom", false);
            s.logo         = obj.value("logo", "");
            stations.push_back(s);
        }
    } catch (const std::exception& e) {
        std::cerr << "[RadioManager] ERROR parsing " << sp << ": " << e.what() << std::endl;
        return;
    }

    load_favs();
    sort_by_country();
}

void RadioManager::save_stations() {
    json j = json::array();
    for (const auto& s : stations) {
        j.push_back({
            {"id",           s.id},
            {"name",         s.name},
            {"stream_url",   s.stream_url},
            {"genre",        s.genre},
            {"country_code", s.country_code},
            {"country_name", s.country_name},
            {"codec",        s.codec},
            {"bitrate",      s.bitrate},
            {"is_custom",    s.is_custom},
            {"logo",         s.logo}
        });
    }
    std::ofstream ofs(stations_path());
    ofs << j.dump(2) << std::endl;
}

void RadioManager::add_custom(const std::string& name, const std::string& url,
                              const std::string& genre, const std::string& country_code) {
    int max_id = 0;
    for (const auto& s : stations)
        if (s.id > max_id) max_id = s.id;

    RadioStation s;
    s.id = max_id + 1;
    s.name = name;
    s.stream_url = url;
    s.genre = genre.empty() ? "Other" : genre;
    s.country_code = country_code.empty() ? "INT" : country_code;
    s.country_name = s.country_code;
    s.codec = "MP3";
    s.bitrate = 128;
    s.is_custom = true;
    stations.push_back(s);
    save_stations();
}

void RadioManager::remove_custom(int station_id) {
    for (size_t i = 0; i < stations.size(); i++) {
        if (stations[i].id == station_id && stations[i].is_custom) {
            stations.erase(stations.begin() + i);
            break;
        }
    }
    fav_cache.erase(station_id);
    save_stations();
    save_favs();
}

void RadioManager::load_favs() {
    fav_cache.clear();
    std::string fp = favs_path();
    if (!std::filesystem::exists(fp)) return;
    try {
        std::ifstream ifs(fp);
        json j;
        ifs >> j;
        if (j.contains("ids") && j["ids"].is_array()) {
            for (const auto& id : j["ids"]) {
                if (id.is_number_integer()) fav_cache.insert(id.get<int>());
            }
        }
    } catch (...) {}
}

void RadioManager::save_favs() {
    json j;
    j["ids"] = json::array();
    for (int id : fav_cache) j["ids"].push_back(id);
    std::ofstream ofs(favs_path());
    ofs << j.dump(2) << std::endl;
}

std::vector<RadioStation> RadioManager::search_online(const std::string& query) {
    std::vector<RadioStation> results;
    std::string url = "https://de1.api.radio-browser.info/json/stations/byname/"
                      + url_encode(query);
    std::string temp_file = "radio_search.json";
    std::string cmd = "curl -s -L -o " + temp_file + " --max-time 15 \"" + url + "\"";
    run_hidden(cmd, false, false);

    if (!std::filesystem::exists(temp_file)) return results;

    try {
        std::ifstream ifs(temp_file);
        json j;
        ifs >> j;
        std::filesystem::remove(temp_file);

        int next_id = 20000;
        if (j.is_array()) {
            for (const auto& obj : j) {
                RadioStation s;
                s.id = next_id++;
                s.name         = obj.value("name", "");
                s.stream_url   = obj.value("url", "");
                s.genre        = obj.value("tags", "Other");
                s.country_code = obj.value("countrycode", "INT");
                s.country_name = s.country_code;
                s.codec        = obj.value("codec", "MP3");
                s.bitrate      = obj.value("bitrate", 128);
                s.is_custom    = true;
                // Take first tag as genre
                if (!s.genre.empty()) {
                    size_t comma = s.genre.find(',');
                    if (comma != std::string::npos) s.genre = s.genre.substr(0, comma);
                }
                results.push_back(s);
                if (results.size() >= 50) break;
            }
        }
    } catch (...) {
        std::filesystem::remove(temp_file);
    }
    return results;
}

bool RadioManager::refresh_station_url(int station_id) {
    if (station_id < 0 || station_id >= (int)stations.size()) return false;
    RadioStation& st = stations[station_id];
    if (st.is_custom) return false;

    std::string safe_name;
    for (char c : st.name) {
        if (c == ' ') safe_name += "%20";
        else if (c == '&') safe_name += "%26";
        else safe_name += c;
    }
    std::string url = "https://de1.api.radio-browser.info/json/stations/search?name="
                      + safe_name + "&countrycode=" + st.country_code
                      + "&limit=5&order=clickcount&reverse=true";
    std::string temp_file = "radio_refresh.json";
    std::string cmd = "curl -s -L -o " + temp_file + " --max-time 15 \"" + url + "\"";
    run_hidden(cmd, false, false);

    if (!std::filesystem::exists(temp_file)) return false;

    std::string best_url;
    int best_bitrate = 0;
    bool found = false;

    try {
        std::ifstream ifs(temp_file);
        json j;
        ifs >> j;
        std::filesystem::remove(temp_file);

        if (j.is_array()) {
            for (const auto& obj : j) {
                std::string name = obj.value("name", "");
                std::string stream = obj.value("url", "");
                int bitrate = obj.value("bitrate", 0);

                std::string st_lower = st.name;
                std::string api_lower = name;
                for (auto& c : st_lower) c = (char)tolower(c);
                for (auto& c : api_lower) c = (char)tolower(c);
                if (api_lower.find(st_lower) == std::string::npos &&
                    st_lower.find(api_lower) == std::string::npos) continue;

                if (bitrate > best_bitrate) {
                    best_bitrate = bitrate;
                    best_url = stream;
                    found = true;
                }
            }
        }
    } catch (...) {
        std::filesystem::remove(temp_file);
    }

    if (found && best_url != st.stream_url) {
        st.stream_url = best_url;
        return true;
    }
    return false;
}
