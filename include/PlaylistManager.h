#ifndef PLAYLISTMANAGER_H
#define PLAYLISTMANAGER_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

class PlaylistManager {
public:
    static void ensure_directories() {
        if (!fs::exists("FAVS")) fs::create_directory("FAVS");
        if (!fs::exists("PLAYLIST")) fs::create_directory("PLAYLIST");
        
        // Ensure favs.txt has header
        std::string favPath = "FAVS/favs.txt";
        if (!fs::exists(favPath)) {
            std::ofstream f(favPath);
            f << "[FAVS]\n";
        }
    }

    static void add_to_favorites(const std::string& video_id) {
        if (is_favorite(video_id)) return;
        std::ofstream f("FAVS/favs.txt", std::ios::app);
        f << "v=" << video_id << "\n";
    }

    static void remove_from_favorites(const std::string& video_id) {
        auto favs = get_favorites();
        std::ofstream f("FAVS/favs.txt", std::ios::trunc);
        f << "[FAVS]\n";
        for (const auto& item : favs) {
            if (item.is_playlist) {
                f << "playlist=" << item.value << "\n";
            } else {
                if (item.value != video_id) {
                    f << "v=" << item.value << "\n";
                }
            }
        }
    }

    static bool is_favorite(const std::string& video_id) {
        auto favs = get_favorites();
        for (const auto& item : favs) {
            if (!item.is_playlist && item.value == video_id) return true;
        }
        return false;
    }

    static void add_playlist_to_favorites(const std::string& playlist_name) {
        std::ofstream f("FAVS/favs.txt", std::ios::app);
        f << "playlist=" << playlist_name << ".txt\n";
    }

    static void create_playlist(const std::string& name, const std::string& comment = "") {
        std::string path = "PLAYLIST/" + name + ".txt";
        if (!fs::exists(path)) {
            std::ofstream f(path);
            f << "[PLAYLIST]\n";
            if (!comment.empty()) {
                f << "# Comment: " << comment << "\n";
            }
        }
    }

    static std::string get_playlist_comment(const std::string& filename) {
        std::string path = "PLAYLIST/" + filename;
        if (!fs::exists(path)) return "";
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (line.substr(0, 11) == "# Comment: ") {
                return line.substr(11);
            }
        }
        return "";
    }

    static void delete_playlist(const std::string& filename) {
        std::string path = "PLAYLIST/" + filename;
        if (fs::exists(path)) {
            fs::remove(path);
        }
        // Also remove from favs if referenced
        auto favs = get_favorites();
        std::ofstream f("FAVS/favs.txt", std::ios::trunc);
        f << "[FAVS]\n";
        for (const auto& item : favs) {
            if (item.is_playlist) {
                if (item.value != filename) f << "playlist=" << item.value << "\n";
            } else {
                f << "v=" << item.value << "\n";
            }
        }
    }

    static void remove_song_from_playlist(const std::string& filename, const std::string& video_id) {
        std::string path = "PLAYLIST/" + filename;
        if (!fs::exists(path)) return;
        
        std::ifstream f_in(path);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(f_in, line)) {
            bool matches = false;
            // Check if line is "v=<video_id>"
            if (line.substr(0, 2) == "v=") {
                std::string id = line.substr(2);
                size_t cpos = id.find(' ');
                if (cpos != std::string::npos) id = id.substr(0, cpos);
                if (id == video_id) matches = true;
            }
            if (!matches) lines.push_back(line);
        }
        f_in.close();

        std::ofstream f_out(path, std::ios::trunc);
        for (const auto& l : lines) f_out << l << "\n";
    }

    static void add_to_playlist(const std::string& playlist_name, const std::string& video_id) {
        std::string path = "PLAYLIST/" + playlist_name + ".txt";
        std::ofstream f(path, std::ios::app);
        f << "v=" << video_id << "\n";
    }

    struct FavItem {
        bool is_playlist;
        std::string value; // ID or filename
    };

    static std::vector<FavItem> get_favorites() {
        std::vector<FavItem> items;
        std::ifstream f("FAVS/favs.txt");
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '[' || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                FavItem item;
                std::string key = line.substr(0, pos);
                std::string val = line.substr(pos + 1);
                // Remove potential comments
                size_t cpos = val.find(' ');
                if (cpos != std::string::npos) val = val.substr(0, cpos);
                
                if (key == "v") {
                    item.is_playlist = false;
                    item.value = val;
                    items.push_back(item);
                } else if (key == "playlist") {
                    item.is_playlist = true;
                    item.value = val;
                    items.push_back(item);
                }
            }
        }
        return items;
    }

    static std::vector<std::string> get_playlist_songs(const std::string& filename) {
        std::vector<std::string> ids;
        std::string path = "PLAYLIST/" + filename;
        if (!fs::exists(path)) return ids;

        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (line.substr(0, 2) == "v=") {
                std::string id = line.substr(2);
                size_t cpos = id.find(' ');
                if (cpos != std::string::npos) id = id.substr(0, cpos);
                ids.push_back(id);
            }
        }
        return ids;
    }

    static std::vector<std::string> get_all_playlists() {
        std::vector<std::string> lists;
        if (!fs::exists("PLAYLIST")) return lists;
        for (const auto& entry : fs::directory_iterator("PLAYLIST")) {
            if (entry.path().extension() == ".txt") {
                lists.push_back(entry.path().filename().string());
            }
        }
        return lists;
    }
};

#endif
