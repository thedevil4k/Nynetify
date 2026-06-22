#ifndef VIEWMANAGER_H
#define VIEWMANAGER_H

#include <string>
#include <vector>
#include <FL/Fl_JPEG_Image.H>
#include "YoutubeService.h"

/*
 * ViewManager — Navigation between app views
 *
 * Manages the three view groups (Home, Search, Playlist/Channel)
 * and their associated widgets.  Handles async loading of:
 *   - Cover art (thumbnail for currently playing track)
 *   - Channel content + avatar
 *   - Region detection at startup
 *
 * Each async operation downloads data in a background thread and
 * calls Fl::awake() to update the GUI on the main thread.
 */

/* ── View switchers ──────────────────────────────── */
void show_home_view();
void show_search_view();
void show_playlist_view(const std::string& playlist_name);
void show_favorites_view();
void show_radio_view();
void refresh_radio_browser();
void show_youtube_playlist_view(const std::string& playlist_id,
                                const std::string& playlist_name,
                                const std::string& uploader);
void show_channel_view(const std::string& channel_id,
                       const std::string& channel_name);

/* ── Cover art (async) ───────────────────────────── */
void update_cover_art(const std::string& video_id);
void cover_art_completed_cb(void* data);

/* ── Channel loading (async) ─────────────────────── */
struct ChannelLoadTask {
    std::string channel_id;
    std::string channel_name;
    std::vector<SearchResult> results;
    std::string avatar_path;
};
void channel_content_completed_cb(void* data);

/* ── Region detection (async) ────────────────────── */
void detect_region();
void region_detected_cb(void* data);

/* ── Status bar (1 s timer) ──────────────────────── */
void update_status_bar_cb(void* data);

#endif // VIEWMANAGER_H
