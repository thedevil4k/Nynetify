#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <string>
#include <vector>
#include <unordered_set>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>
#include "nlohmann/json.hpp"

using json = nlohmann::json;
namespace fs = std::filesystem;

class PlaylistManager {
public:
    static inline std::unordered_set<std::string> fav_cache;
    static inline std::unordered_set<std::string> twitch_fav_cache;
    static inline std::unordered_set<std::string> soundcloud_fav_cache;

    // ── YouTube Favorites ─────────────────────────
    static void load_fav_cache() {
        fav_cache.clear();
        std::string fp = "youtube_favs.json";
        if (!fs::exists(fp)) return;
        try {
            std::ifstream ifs(fp);
            json j;
            ifs >> j;
            if (j.contains("videos") && j["videos"].is_array()) {
                for (const auto& v : j["videos"])
                    if (v.is_string()) fav_cache.insert(v.get<std::string>());
            }
        } catch (...) {}
    }

    static void save_youtube_favs() {
        json j;
        j["videos"] = json::array();
        for (const auto& id : fav_cache) j["videos"].push_back(id);
        // Preserve playlist favs
        json existing = load_json_file("youtube_favs.json");
        if (existing.contains("playlists"))
            j["playlists"] = existing["playlists"];
        else
            j["playlists"] = json::array();
        std::ofstream ofs("youtube_favs.json");
        ofs << j.dump(2) << std::endl;
    }

    static void ensure_directories() {
        load_fav_cache();
        load_twitch_fav_cache();
        load_soundcloud_fav_cache();
    }

    static void add_to_favorites(const std::string& video_id) {
        if (is_favorite(video_id)) return;
        fav_cache.insert(video_id);
        save_youtube_favs();
    }

    static void remove_from_favorites(const std::string& video_id) {
        fav_cache.erase(video_id);
        save_youtube_favs();
    }

    static bool is_favorite(const std::string& video_id) {
        return fav_cache.contains(video_id);
    }

    // ── Playlist Favorites ─────────────────────────
    static void add_playlist_to_favorites(const std::string& playlist_name) {
        json j = load_json_file("youtube_favs.json");
        if (!j.contains("playlists") || !j["playlists"].is_array())
            j["playlists"] = json::array();
        for (const auto& p : j["playlists"])
            if (p.is_string() && p.get<std::string>() == playlist_name) return;
        j["playlists"].push_back(playlist_name);
        std::ofstream ofs("youtube_favs.json");
        ofs << j.dump(2) << std::endl;
    }

    // ── Playlists ──────────────────────────────────
    static void create_playlist(const std::string& name, const std::string& comment = "") {
        json j = load_json_file("playlists.json");
        std::string nm = name;
        if (j.contains(nm)) return; // already exists
        j[nm] = {{"comment", comment}, {"tracks", json::array()}};
        std::ofstream ofs("playlists.json");
        ofs << j.dump(2) << std::endl;
    }

    static std::string get_playlist_comment(const std::string& filename) {
        json j = load_json_file("playlists.json");
        std::string nm = strip_ext(filename);
        if (j.contains(nm) && j[nm].contains("comment"))
            return j[nm]["comment"].get<std::string>();
        return "";
    }

    static void delete_playlist(const std::string& filename) {
        std::string nm = strip_ext(filename);
        json j = load_json_file("playlists.json");
        if (j.contains(nm)) {
            j.erase(nm);
            std::ofstream ofs("playlists.json");
            ofs << j.dump(2) << std::endl;
        }
        // Also remove from playlist favs
        json favs = load_json_file("youtube_favs.json");
        if (favs.contains("playlists") && favs["playlists"].is_array()) {
            json new_pls = json::array();
            for (const auto& p : favs["playlists"])
                if (p.is_string() && p.get<std::string>() != nm && p.get<std::string>() != filename)
                    new_pls.push_back(p);
            favs["playlists"] = new_pls;
            std::ofstream ofs("youtube_favs.json");
            ofs << favs.dump(2) << std::endl;
        }
    }

    static void remove_song_from_playlist(const std::string& filename, const std::string& video_id) {
        std::string nm = strip_ext(filename);
        json j = load_json_file("playlists.json");
        if (!j.contains(nm) || !j[nm].contains("tracks")) return;
        json new_tracks = json::array();
        for (const auto& t : j[nm]["tracks"])
            if (t.is_string() && t.get<std::string>() != video_id)
                new_tracks.push_back(t);
        j[nm]["tracks"] = new_tracks;
        std::ofstream ofs("playlists.json");
        ofs << j.dump(2) << std::endl;
    }

    static void add_to_playlist(const std::string& playlist_name, const std::string& video_id) {
        std::string nm = strip_ext(playlist_name);
        json j = load_json_file("playlists.json");
        if (!j.contains(nm)) {
            j[nm] = {{"comment", ""}, {"tracks", json::array()}};
        }
        j[nm]["tracks"].push_back(video_id);
        std::ofstream ofs("playlists.json");
        ofs << j.dump(2) << std::endl;
    }

    struct FavItem {
        bool is_playlist;
        std::string value;
    };

    static std::vector<FavItem> get_favorites() {
        std::vector<FavItem> items;
        // YouTube video favorites
        for (const auto& id : fav_cache) {
            items.push_back({false, id});
        }
        // Playlist favorites
        json j = load_json_file("youtube_favs.json");
        if (j.contains("playlists") && j["playlists"].is_array()) {
            for (const auto& p : j["playlists"])
                if (p.is_string()) items.push_back({true, p.get<std::string>()});
        }
        return items;
    }

    static std::vector<std::string> get_playlist_songs(const std::string& filename) {
        std::vector<std::string> ids;
        std::string nm = strip_ext(filename);
        json j = load_json_file("playlists.json");
        if (j.contains(nm) && j[nm].contains("tracks") && j[nm]["tracks"].is_array()) {
            for (const auto& t : j[nm]["tracks"])
                if (t.is_string()) ids.push_back(t.get<std::string>());
        }
        return ids;
    }

    static std::vector<std::string> get_all_playlists() {
        std::vector<std::string> lists;
        json j = load_json_file("playlists.json");
        for (auto it = j.begin(); it != j.end(); ++it)
            lists.push_back(it.key());
        return lists;
    }

    // ── Twitch Favorites ───────────────────────────
    static void load_twitch_fav_cache() {
        twitch_fav_cache.clear();
        std::string fp = "twitch_favs.json";
        if (!fs::exists(fp)) return;
        try {
            std::ifstream ifs(fp);
            json j;
            ifs >> j;
            if (j.contains("channels") && j["channels"].is_array()) {
                for (const auto& c : j["channels"])
                    if (c.is_string()) twitch_fav_cache.insert(c.get<std::string>());
            }
        } catch (...) {}
    }

    static void add_twitch_favorite(const std::string& channel_login) {
        twitch_fav_cache.insert(channel_login);
        save_twitch_favs();
    }

    static void remove_twitch_favorite(const std::string& channel_login) {
        twitch_fav_cache.erase(channel_login);
        save_twitch_favs();
    }

    static bool is_twitch_favorite(const std::string& channel_login) {
        return twitch_fav_cache.contains(channel_login);
    }

    static std::vector<std::string> get_twitch_favorites() {
        return std::vector<std::string>(twitch_fav_cache.begin(), twitch_fav_cache.end());
    }

    // ── SoundCloud Favorites ──────────────────────
    static void load_soundcloud_fav_cache() {
        soundcloud_fav_cache.clear();
        std::string fp = "soundcloud_favs.json";
        if (!fs::exists(fp)) return;
        try {
            std::ifstream ifs(fp);
            json j;
            ifs >> j;
            if (j.contains("tracks") && j["tracks"].is_array()) {
                for (const auto& t : j["tracks"])
                    if (t.is_string()) soundcloud_fav_cache.insert(t.get<std::string>());
            }
        } catch (...) {}
    }

    static void add_soundcloud_favorite(const std::string& track_url) {
        soundcloud_fav_cache.insert(track_url);
        save_soundcloud_favs();
    }

    static void remove_soundcloud_favorite(const std::string& track_url) {
        soundcloud_fav_cache.erase(track_url);
        save_soundcloud_favs();
    }

    static bool is_soundcloud_favorite(const std::string& track_url) {
        return soundcloud_fav_cache.contains(track_url);
    }

    static std::vector<std::string> get_soundcloud_favorites() {
        return std::vector<std::string>(soundcloud_fav_cache.begin(), soundcloud_fav_cache.end());
    }

private:
    static json load_json_file(const std::string& path) {
        if (!fs::exists(path)) return json::object();
        try {
            std::ifstream ifs(path);
            json j;
            ifs >> j;
            return j;
        } catch (...) { return json::object(); }
    }

    static std::string strip_ext(const std::string& name) {
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            return name.substr(0, name.size() - 4);
        return name;
    }

    static void save_twitch_favs() {
        json j;
        j["channels"] = json::array();
        for (const auto& c : twitch_fav_cache) j["channels"].push_back(c);
        std::ofstream ofs("twitch_favs.json");
        ofs << j.dump(2) << std::endl;
    }

    static void save_soundcloud_favs() {
        json j;
        j["tracks"] = json::array();
        for (const auto& t : soundcloud_fav_cache) j["tracks"].push_back(t);
        std::ofstream ofs("soundcloud_favs.json");
        ofs << j.dump(2) << std::endl;
    }
};

#endif
