#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <cstdlib>
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>
#else
#include <unistd.h>
#endif
#include "ViewManager.h"
#include "Globals.h"
#include "Theme.h"
#include "ModernButton.h"
#include "PlayerEngine.h"
#include "ProgressSlider.h"
#include "PlaylistManager.h"
#include "YoutubeService.h"
#include "AppSettings.h"    /* settings */
#include "PlayerController.h"  /* play_index etc. */
#include "UIWidgets.h"          /* ResultsBrowser full type */

/* ================================================================
 * View-switching functions
 * ================================================================ */

void show_home_view() {
    homeGroup->show();
    searchGroup->hide();
    playlistGroup->hide();
    if (resultsBrowser) resultsBrowser->hide();
    if (sidebarPlaylistList) sidebarPlaylistList->value(0);
}

void show_search_view() {
    homeGroup->hide();
    searchGroup->show();
    playlistGroup->hide();
    if (resultsBrowser) {
        resultsBrowser->resize(220, 70, 760, 570);
        resultsBrowser->show();
    }
    if (sidebarPlaylistList) sidebarPlaylistList->value(0);
}

void show_playlist_view(const std::string& playlist_name) {
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();
    if (resultsBrowser) {
        resultsBrowser->resize(220, 220, 760, 420);
        resultsBrowser->show();
    }

    playlistNameBox->copy_label(playlist_name.c_str());
    playlistDescBox->copy_label(lang->playlist_desc_placeholder);
    if (playlistDeleteBtn) playlistDeleteBtn->show();

    current_playlist = playlist_name + ".txt";
    current_category = "MY PLAYLISTS";

    resultsBrowser->clear();
    std::vector<std::string> ids = PlaylistManager::get_playlist_songs(current_playlist);
    if (ids.empty()) {
        resultsBrowser->add(lang->search_failed);
    } else {
        last_results = YoutubeService::get_metadata(ids);
        for (const auto& res : last_results) {
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            resultsBrowser->add((star + "\t" + res.title + "\t" + res.author).c_str());
        }
    }
    resultsBrowser->redraw();
}

void show_favorites_view() {
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();
    if (resultsBrowser) {
        resultsBrowser->resize(220, 220, 760, 420);
        resultsBrowser->show();
    }

    playlistNameBox->copy_label(lang->liked_songs);
    playlistDescBox->copy_label(lang->feat_desc);
    if (playlistDeleteBtn) playlistDeleteBtn->hide();

    current_playlist = "";
    current_category = "MY FAVORITES";

    resultsBrowser->clear();
    auto favs = PlaylistManager::get_favorites();
    std::vector<std::string> ids;
    for (const auto& f : favs) {
        if (!f.is_playlist) ids.push_back(f.value);
    }
    if (ids.empty()) {
        resultsBrowser->add(lang->no_favorites);
    } else {
        last_results = YoutubeService::get_metadata(ids);
        for (const auto& res : last_results) {
            resultsBrowser->add((std::string("@C7\xe2\x98\x85\t") + res.title + "\t" + res.author).c_str());
        }
    }
    resultsBrowser->redraw();
}

/* ── YouTube playlist view ───────────────────────── */
void show_youtube_playlist_view(const std::string& playlist_id,
                                const std::string& playlist_name,
                                const std::string& uploader)
{
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();

    resultsBrowser->resize(220, 220, 760, 420);
    resultsBrowser->show();

    playlistNameBox->copy_label(playlist_name.c_str());
    playlistDescBox->copy_label((std::string(lang->yt_playlist_by) + uploader).c_str());
    if (playlistDeleteBtn) playlistDeleteBtn->hide();

    current_playlist = "";
    current_category = "YOUTUBE_PLAYLIST";

    resultsBrowser->clear();
    resultsBrowser->add(lang->loading_yt_playlist);
    resultsBrowser->redraw();
    Fl::check();

    last_results = YoutubeService::get_playlist_videos(playlist_id);
    resultsBrowser->clear();
    if (last_results.empty()) {
        resultsBrowser->add(lang->no_yt_tracks);
    } else {
        for (const auto& res : last_results) {
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            resultsBrowser->add((star + "\t" + res.title + "\t" + res.author).c_str());
        }
    }
    resultsBrowser->redraw();
}

/* ================================================================
 * Channel view (async)
 * ================================================================ */
void channel_content_completed_cb(void* data) {
    auto* task = static_cast<ChannelLoadTask*>(data);
    last_results = task->results;

    /* Load the pre-downloaded avatar */
    if (!task->avatar_path.empty() && std::filesystem::exists(task->avatar_path)) {
        if (playlistCoverBox) {
            auto* img = new Fl_JPEG_Image(task->avatar_path.c_str());
            if (img && img->w() > 0) {
                auto* scaled = img->copy(playlistCoverBox->w(), playlistCoverBox->h());
                playlistCoverBox->image(scaled);
                playlistCoverBox->redraw();
            }
            delete img;
        }
        std::filesystem::remove(task->avatar_path);
    }

    /* Populate the browser with playlists & videos */
    resultsBrowser->clear();
    if (task->results.empty()) {
        resultsBrowser->add(lang->no_channel_content);
    } else {
        bool has_playlists = false;
        for (const auto& res : task->results) {
            if (!res.is_playlist) continue;
            if (!has_playlists) {
                resultsBrowser->add((std::string("@C7") + lang->channel_playlists).c_str());
                has_playlists = true;
            }
            resultsBrowser->add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
        }
        bool has_videos = false;
        for (const auto& res : task->results) {
            if (res.is_playlist) continue;
            if (!has_videos) {
                if (has_playlists) resultsBrowser->add("");
                resultsBrowser->add((std::string("@C7") + lang->channel_videos).c_str());
                has_videos = true;
            }
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            resultsBrowser->add((star + "\t" + res.title + "\t" + res.author).c_str());
        }
    }
    resultsBrowser->redraw();
    delete task;
}

void show_channel_view(const std::string& channel_id,
                       const std::string& channel_name)
{
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();

    resultsBrowser->resize(220, 220, 760, 420);
    resultsBrowser->show();

    playlistNameBox->copy_label(channel_name.c_str());
    playlistDescBox->copy_label(lang->yt_channel);
    if (playlistDeleteBtn) playlistDeleteBtn->hide();

    current_playlist = "";
    current_category = "CHANNEL";

    resultsBrowser->clear();
    resultsBrowser->add(lang->loading_channel);
    resultsBrowser->redraw();
    Fl::check();

    /* Async fetch in background thread */
    auto* task = new ChannelLoadTask{channel_id, channel_name, {}, {}};
    std::thread([task]() {
        task->results = YoutubeService::get_channel_content(task->channel_id, task->channel_name);

        std::string avatar_url = YoutubeService::get_channel_avatar_url(task->channel_id);
        if (!avatar_url.empty()) {
            task->avatar_path = "cover_" + task->channel_id + ".jpg";
            std::string dl_cmd = "curl -s -L -o " + task->avatar_path + " \"" + avatar_url + "\"";
            system(dl_cmd.c_str());
        }

        Fl::awake(channel_content_completed_cb, task);
    }).detach();
}

/* ================================================================
 * Cover art (async)
 * ================================================================ */
struct CoverArtTask {
    std::string video_id;
    std::string url;
};

void cover_art_completed_cb(void* data) {
    auto* task = static_cast<CoverArtTask*>(data);
    std::string temp_file = "cover_" + task->video_id + ".jpg";

    if (std::filesystem::exists(temp_file)) {
        if (coverArtBox) {
            if (currentImage) delete currentImage;
            currentImage = new Fl_JPEG_Image(temp_file.c_str());
            if (currentImage && currentImage->w() > 0) {
                auto* scaled = currentImage->copy(coverArtBox->w(), coverArtBox->h());
                coverArtBox->image(scaled);
                coverArtBox->redraw();
            }
        }
        if (miniCoverArtBox) {
            if (miniCurrentImage) delete miniCurrentImage;
            miniCurrentImage = new Fl_JPEG_Image(temp_file.c_str());
            if (miniCurrentImage && miniCurrentImage->w() > 0) {
                auto* scaled = miniCurrentImage->copy(miniCoverArtBox->w(), miniCoverArtBox->h());
                miniCoverArtBox->image(scaled);
                miniCoverArtBox->redraw();
            }
        }
        std::filesystem::remove(temp_file);
    }
    delete task;
}

void update_cover_art(const std::string& video_id) {
    if (!settings.loadThumbnails) return;

    auto* task = new CoverArtTask{video_id, YoutubeService::get_thumbnail_url(video_id)};
    std::thread([task]() {
        std::string temp_file = "cover_" + task->video_id + ".jpg";
        std::string cmd = "curl -s -L -o " + temp_file + " " + task->url;
        system(cmd.c_str());
        Fl::awake(cover_art_completed_cb, task);
    }).detach();
}

/* ================================================================
 * Region detection (async)
 * ================================================================ */
void region_detected_cb(void* data) {
    auto* region = static_cast<std::string*>(data);
    if (region->length() == 2) {
        user_region = *region;
        std::cout << "[UI] Region detected: " << user_region << std::endl;
    }
    delete region;
}

void detect_region() {
    std::cout << "[UI] Detecting region..." << std::endl;
    std::thread([]() {
        std::string temp_file = "region_temp.txt";
        std::string cmd = "curl -s -L -o " + temp_file + " https://ipapi.co/country/";
        system(cmd.c_str());

        std::string region;
        if (std::filesystem::exists(temp_file)) {
            std::ifstream ifs(temp_file);
            ifs >> region;
            ifs.close();
            std::filesystem::remove(temp_file);
        }
        Fl::awake(region_detected_cb, new std::string(region));
    }).detach();
}

/* ================================================================
 * Status bar (1 s timer)
 * ================================================================ */
void update_status_bar_cb(void* data) {
    if (!statusBar) return;

    double ram_mb = 0.0;
    double down_kb = 0.0, up_kb = 0.0;

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(self, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    ram_mb = pmc.WorkingSetSize / (1024.0 * 1024.0);

    static ULONG lastIn = 0, lastOut = 0;
    static bool first_net = true;

    ULONG dwSize = 0;
    GetIfTable(nullptr, &dwSize, FALSE);
    auto* pIfTable = (MIB_IFTABLE*)malloc(dwSize);
    if (pIfTable && GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
        ULONG currentIn = 0, currentOut = 0;
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            currentIn  += pIfTable->table[i].dwInOctets;
            currentOut += pIfTable->table[i].dwOutOctets;
        }
        if (!first_net) {
            if (currentIn  >= lastIn)  down_kb = (double)(currentIn - lastIn)  / 1024.0;
            if (currentOut >= lastOut) up_kb   = (double)(currentOut - lastOut) / 1024.0;
        }
        lastIn = currentIn; lastOut = currentOut;
        first_net = false;
    }
    free(pIfTable);
#else
    std::ifstream statm("/proc/self/statm");
    if (statm.is_open()) {
        long pages = 0;
        statm >> pages; // total program size
        statm >> pages; // resident set size
        ram_mb = (pages * sysconf(_SC_PAGESIZE)) / (1024.0 * 1024.0);
    }
#endif

    time_t rawtime;
    time(&rawtime);
    auto* timeinfo = localtime(&rawtime);
    char time_str[80];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);

    double buf_usage = 0;
    if (player) buf_usage = player->get_buffer_usage_mb();

    char final_status[256];
    snprintf(final_status, sizeof(final_status),
             " %s | Region: %s | RAM: %.1f MB | BUF: %.1f MB | Net D: %.1f KB/s U: %.1f KB/s | %s",
             lang->status_prefix, user_region.c_str(), ram_mb, buf_usage, down_kb, up_kb, time_str);
    statusBar->copy_label(final_status);
    statusBar->redraw();

    Fl::repeat_timeout(1.0, update_status_bar_cb);
}
