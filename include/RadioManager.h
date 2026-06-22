#ifndef RADIOMANAGER_H
#define RADIOMANAGER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include "RadioStation.h"

class RadioManager {
public:
    static inline std::vector<RadioStation> stations;
    static inline std::unordered_set<int> fav_cache;
    static inline std::unordered_set<int> failed_stations;
    static inline std::string user_country_code = "ES";
    static inline std::string current_country_filter = "";
    static inline bool radio_mode = false;
    static inline int current_radio_index = -1;

    static void init(const std::string& country_code) {
        user_country_code = country_code;
        stations = bundled_stations;
        load_customs();
        load_favs();
        sort_by_country();
    }

    static void sort_by_country() {
        std::stable_sort(stations.begin(), stations.end(),
            [](const RadioStation& a, const RadioStation& b) {
                bool a_home = (a.country_code == user_country_code);
                bool b_home = (b.country_code == user_country_code);
                if (a_home != b_home) return a_home > b_home;
                if (a.country_code != b.country_code) return a.country_code < b.country_code;
                return a.name < b.name;
            });
    }

    static std::vector<RadioStation> filtered() {
        std::vector<RadioStation> result;
        for (const auto& s : stations) {
            if (!current_country_filter.empty() && s.country_code != current_country_filter)
                continue;
            result.push_back(s);
        }
        return result;
    }

    static std::vector<RadioStation> favorites() {
        std::vector<RadioStation> result;
        for (const auto& s : stations) {
            if (fav_cache.contains(s.id))
                result.push_back(s);
        }
        return result;
    }

    static std::vector<std::string> all_countries() {
        std::vector<std::string> codes;
        for (const auto& s : stations) {
            if (std::find(codes.begin(), codes.end(), s.country_code) == codes.end())
                codes.push_back(s.country_code);
        }
        return codes;
    }

    static bool get_radio_mode() { return radio_mode; }
    static void set_radio_mode(bool v) { radio_mode = v; }

    static void toggle_favorite(int station_id) {
        if (fav_cache.contains(station_id))
            fav_cache.erase(station_id);
        else
            fav_cache.insert(station_id);
        save_favs();
    }

    static bool is_favorite(int station_id) {
        return fav_cache.contains(station_id);
    }

    static bool is_failed(int station_id) {
        return failed_stations.contains(station_id);
    }

    static void mark_failed(int station_id) {
        failed_stations.insert(station_id);
    }

    static void add_custom(const std::string& name, const std::string& url,
                           const std::string& genre, const std::string& country_code) {
        RadioStation s;
        s.id = 10000 + (int)stations.size();
        s.name = name;
        s.stream_url = url;
        s.genre = genre;
        s.country_code = country_code;
        s.country_name = country_code;
        s.codec = "MP3";
        s.bitrate = 128;
        s.is_custom = true;
        stations.push_back(s);
        save_customs();
    }

    static void remove_custom(int station_id) {
        for (size_t i = 0; i < stations.size(); i++) {
            if (stations[i].id == station_id && stations[i].is_custom) {
                stations.erase(stations.begin() + i);
                break;
            }
        }
        fav_cache.erase(station_id);
        save_customs();
        save_favs();
    }

    static void play_radio(int station_index) {
        if (station_index < 0 || station_index >= (int)stations.size()) return;
        current_radio_index = station_index;
        radio_mode = true;
    }

    static void radio_next() {
        auto filtered_list = filtered();
        if (filtered_list.empty()) return;
        int cur_idx = -1;
        if (current_radio_index >= 0) {
            int cur_id = stations[current_radio_index].id;
            for (int i = 0; i < (int)filtered_list.size(); i++) {
                if (filtered_list[i].id == cur_id) { cur_idx = i; break; }
            }
        }
        int next = (cur_idx + 1) % (int)filtered_list.size();
        // Find the global index
        for (int i = 0; i < (int)stations.size(); i++) {
            if (stations[i].id == filtered_list[next].id) {
                current_radio_index = i;
                break;
            }
        }
    }

    static void radio_prev() {
        auto filtered_list = filtered();
        if (filtered_list.empty()) return;
        int cur_idx = -1;
        if (current_radio_index >= 0) {
            int cur_id = stations[current_radio_index].id;
            for (int i = 0; i < (int)filtered_list.size(); i++) {
                if (filtered_list[i].id == cur_id) { cur_idx = i; break; }
            }
        }
        int prev = cur_idx <= 0 ? (int)filtered_list.size() - 1 : cur_idx - 1;
        for (int i = 0; i < (int)stations.size(); i++) {
            if (stations[i].id == filtered_list[prev].id) {
                current_radio_index = i;
                break;
            }
        }
    }

    static std::vector<RadioStation> search_online(const std::string& query);

private:
    static void load_customs();
    static void save_customs();
    static void load_favs();
    static void save_favs();
};

#endif
