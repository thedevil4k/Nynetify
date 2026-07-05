#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <thread>
#include "PlayerController.h"
#include "Globals.h"
#include "ViewManager.h"
#include "PlayerEngine.h"
#include "ProgressSlider.h"
#include "ModernButton.h"
#include "PlaylistManager.h"
#include "RadioManager.h"
#include "YoutubeService.h"
#include "UIWidgets.h"   /* CircularButton, HeartButton */

/* ================================================================
 * Queue traversal
 * ================================================================ */

static int play_resolve_sequence = 0;
static std::string pre_resolved_url;
static int pre_resolved_index = -1;

/* ── Callback: URL resolved, now actually start playback ─ */
void play_resolved_cb(void* data) {
    auto* task = static_cast<PlayResolveTask*>(data);

    /* Discard if stale or user changed track */
    if (task->sequence != play_resolve_sequence ||
        task->queue_index != current_queue_index) {
        delete task;
        return;
    }

    if (task->stream_url.empty()) {
        std::cerr << "[ERROR] Could not resolve URL for: " << task->video_id << std::endl;
        if (nowPlayingArtistBox) {
            nowPlayingArtistBox->copy_label("\xe2\x9d\x8c URL failed");
            nowPlayingArtistBox->redraw();
        }
        delete task;
        return;
    }

    player->play(task->stream_url);
    if (playBtn) playBtn->redraw();

    /* ── Pre-resolve next track's URL in background (P3) ─ */
    int next_idx = current_queue_index + 1;
    if (next_idx >= (int)play_queue.size()) next_idx = 0;
    if (next_idx != current_queue_index && !play_queue[next_idx].is_twitch) {
        auto* next_task = new PlayResolveTask{
            play_queue[next_idx].video_id,
            play_queue[next_idx].title,
            play_queue[next_idx].author,
            play_queue[next_idx].is_twitch,
            play_queue[next_idx].is_soundcloud,
            play_queue[next_idx].is_live,
            next_idx,
            0  // no sequence check needed for pre-resolve
        };
        pre_resolved_index = next_idx;
        std::thread([next_task]() {
            if (next_task->is_soundcloud)
                next_task->stream_url = SoundCloudClient::resolve_audio_url(next_task->video_id);
            else
                next_task->stream_url = YoutubeService::resolve_audio_url(next_task->video_id);
            Fl::awake([](void* d) {
                auto* t = static_cast<PlayResolveTask*>(d);
                if (t->queue_index == pre_resolved_index && !t->stream_url.empty()) {
                    pre_resolved_url = t->stream_url;
                }
                delete t;
            }, next_task);
        }).detach();
    }

    std::cout << "[UI] Playback started: " << task->title << std::endl;
    delete task;
}

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
        nowPlayingArtistBox->copy_label("\xe2\x8f\xb3 Loading...");
        nowPlayingArtistBox->redraw();
    }

    /* Heart state */
    if (heartBtn) {
        if (play_queue[index].is_soundcloud)
            heartBtn->active = PlaylistManager::is_soundcloud_favorite(video_id);
        else
            heartBtn->active = PlaylistManager::is_favorite(video_id);
        heartBtn->redraw();
    }

    /* Load cover art (async — see ViewManager) */
    if (!play_queue[index].is_twitch && !play_queue[index].is_soundcloud)
        update_cover_art(video_id);

    /* ── Async URL resolution (was BLOCKING, now in background thread) ─ */
    auto* task = new PlayResolveTask{
        video_id, title, author,
        play_queue[index].is_twitch,
        play_queue[index].is_soundcloud,
        play_queue[index].is_live,
        index,
        ++play_resolve_sequence
    };

    /* Check if next track was pre-resolved and matches */
    if (index == pre_resolved_index && !pre_resolved_url.empty()) {
        task->stream_url = pre_resolved_url;
        pre_resolved_url.clear();
        pre_resolved_index = -1;
        player->set_ytdl(false);
        /* Use Fl::awake to keep UI responsive even on instant result */
        Fl::awake(play_resolved_cb, task);
    } else if (play_queue[index].is_twitch) {
        /* Twitch is instant (no yt-dlp resolve needed) */
        task->stream_url = play_queue[index].is_live
            ? "https://www.twitch.tv/" + video_id
            : "https://www.twitch.tv/videos/" + video_id;
        player->set_ytdl(true);
        Fl::awake(play_resolved_cb, task);
    } else {
        /* YouTube / SoundCloud: resolve in background */
        player->set_ytdl(false);
        std::thread([task]() {
            if (task->is_soundcloud)
                task->stream_url = SoundCloudClient::resolve_audio_url(task->video_id);
            else
                task->stream_url = YoutubeService::resolve_audio_url(task->video_id);
            Fl::awake(play_resolved_cb, task);
        }).detach();
    }

    if (statusBar) {
        statusBar->copy_label("Resolving stream URL...");
        statusBar->redraw();
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
        update_radio_cover(st.logo);
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
        update_radio_cover(st.logo);
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
                update_radio_cover(st.logo);
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
