#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include "PlayerController.h"
#include "Globals.h"
#include "PlayerEngine.h"
#include "ProgressSlider.h"
#include "ModernButton.h"
#include "PlaylistManager.h"
#include "RadioManager.h"
#include "UIWidgets.h"   /* CircularButton, HeartButton */

/* ================================================================
 * Queue traversal
 * ================================================================ */

void play_index(int index) {
    if (index < 0 || index >= (int)play_queue.size()) return;

    RadioManager::set_radio_mode(false);
    current_queue_index = index;
    std::string video_id = play_queue[index].video_id;
    std::string title    = play_queue[index].title;
    std::string author   = play_queue[index].author;

    std::cout << "[UI] Queue Play Index " << index << ": "
              << title << " (" << video_id << ")" << std::endl;

    /* Highlight in browser */
    if (resultsBrowser) resultsBrowser->select(index + 1);

    /* Now-playing display (truncated) */
    if (nowPlayingBox) {
        std::string np = title;
        if (np.size() > 24) np = np.substr(0, 21) + "...";
        nowPlayingBox->copy_label(np.c_str());
        nowPlayingBox->redraw();
    }
    if (nowPlayingArtistBox) {
        std::string art = author;
        if (art.size() > 28) art = art.substr(0, 25) + "...";
        nowPlayingArtistBox->copy_label(art.c_str());
        nowPlayingArtistBox->redraw();
    }

    /* Heart state */
    if (heartBtn) {
        heartBtn->active = PlaylistManager::is_favorite(video_id);
        heartBtn->redraw();
    }

    /* Load cover art (async — see ViewManager) */
    if (!play_queue[index].is_twitch)
        update_cover_art(video_id);

    /* Start playback via mpv + yt-dlp */
    std::string stream_url;
    if (play_queue[index].is_twitch) {
        /* Construct Twitch URL — mpv + yt-dlp handles HLS extraction */
        if (play_queue[index].is_live)
            stream_url = "https://www.twitch.tv/" + video_id;
        else
            stream_url = "https://www.twitch.tv/videos/" + video_id;
    } else {
        stream_url = YoutubeService::get_audio_url(video_id);
    }
    if (!stream_url.empty()) {
        player->play(stream_url);
        if (playBtn) playBtn->redraw();
    } else {
        std::cerr << "[ERROR] Could not get stream URL" << std::endl;
    }
}

void play_next() {
    if (RadioManager::get_radio_mode()) {
        RadioManager::radio_next();
        auto& st = RadioManager::stations[RadioManager::current_radio_index];
        if (nowPlayingBox) {
            std::string np = st.name;
            if (np.size() > 32) np = np.substr(0, 29) + "...";
            nowPlayingBox->copy_label(np.c_str());
        }
        if (nowPlayingArtistBox) nowPlayingArtistBox->copy_label(lang->radio_live);
        player->play(st.stream_url);
        return;
    }
    if (play_queue.empty()) return;
    if (is_repeat)  { play_index(current_queue_index); return; }
    if (is_shuffle) { play_index(rand() % (int)play_queue.size()); return; }

    int next = current_queue_index + 1;
    if (next >= (int)play_queue.size()) next = 0;   /* wrap */
    play_index(next);
}

void play_prev() {
    if (RadioManager::get_radio_mode()) {
        RadioManager::radio_prev();
        auto& st = RadioManager::stations[RadioManager::current_radio_index];
        if (nowPlayingBox) {
            std::string np = st.name;
            if (np.size() > 32) np = np.substr(0, 29) + "...";
            nowPlayingBox->copy_label(np.c_str());
        }
        if (nowPlayingArtistBox) nowPlayingArtistBox->copy_label(lang->radio_live);
        player->play(st.stream_url);
        return;
    }
    if (play_queue.empty()) return;
    int prev = current_queue_index - 1;
    if (prev < 0) prev = (int)play_queue.size() - 1;  /* wrap */
    play_index(prev);
}

/* ================================================================
 * Time formatting
 * ================================================================ */
std::string format_time(double seconds) {
    if (seconds < 0) seconds = 0;
    int m = (int)(seconds / 60);
    int s = (int)seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << m << ":"
        << std::setw(2) << std::setfill('0') << s;
    return oss.str();
}

/* ================================================================
 * 200 ms UI update timer
 * ================================================================ */
void update_ui_cb(void* data) {
    if (player && progressBar) {
        int ev = player->update();          /* process mpv events */

        if (RadioManager::get_radio_mode()) {
            // Radio mode: show LIVE, fetch stream metadata
            if (currentTimeBox) { currentTimeBox->copy_label(lang->radio_live); currentTimeBox->redraw(); }
            if (totalTimeBox)   { totalTimeBox->copy_label("");                totalTimeBox->redraw();   }
            progressBar->maximum(1);
            progressBar->value(0);
            progressBar->set_buffered(0);

            // Update stream metadata (icy-title) every few ticks
            static int meta_ticks = 0;
            meta_ticks++;
            if (meta_ticks >= 15) {  // every ~3 seconds
                meta_ticks = 0;
                std::string meta = player->get_stream_metadata();
                if (!meta.empty() && nowPlayingArtistBox) {
                    if (meta.size() > 28) meta = meta.substr(0, 25) + "...";
                    nowPlayingArtistBox->copy_label(meta.c_str());
                    nowPlayingArtistBox->redraw();
                }
            }

            if (ev == 2) {  // error → try next
                int failed_id = RadioManager::stations[RadioManager::current_radio_index].id;
                RadioManager::mark_failed(failed_id);
                refresh_radio_browser();
                RadioManager::radio_next();
                auto& st = RadioManager::stations[RadioManager::current_radio_index];
                if (nowPlayingBox) {
                    std::string np = st.name;
                    if (np.size() > 32) np = np.substr(0, 29) + "...";
                    nowPlayingBox->copy_label(np.c_str());
                    nowPlayingBox->redraw();
                }
                if (nowPlayingArtistBox) {
                    nowPlayingArtistBox->copy_label(lang->radio_live);
                    nowPlayingArtistBox->redraw();
                }
                player->play(st.stream_url);
            }
        } else {
            if (ev == 1) { play_next(); }       /* EOF → auto-advance */

            double pos = player->get_position();
            double dur = player->get_duration();

            if (dur > 0) {
                progressBar->maximum(dur);
                progressBar->value(pos);
                double cache_dur = player->get_cache_duration();
                progressBar->set_buffered(pos + cache_dur);

                if (currentTimeBox) {
                    currentTimeBox->copy_label(format_time(pos).c_str());
                    currentTimeBox->redraw();
                }
                if (totalTimeBox) {
                    totalTimeBox->copy_label(format_time(dur).c_str());
                    totalTimeBox->redraw();
                }
            } else {
                if (currentTimeBox) { currentTimeBox->copy_label("00:00"); currentTimeBox->redraw(); }
                if (totalTimeBox)   { totalTimeBox->copy_label("00:00");   totalTimeBox->redraw();   }
            }
        }
    }
    Fl::repeat_timeout(0.2, update_ui_cb);
}
