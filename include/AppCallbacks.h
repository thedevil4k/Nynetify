#ifndef APPCALLBACKS_H
#define APPCALLBACKS_H

/*
 * AppCallbacks — All FLTK widget callbacks
 *
 * Callbacks are grouped by concern:
 *   • Playback control    (play_btn_cb, pause_resume_cb, seek_cb, volume_cb …)
 *   • Search               (search_cb, progressive_fill_cb, load_more_search_results)
 *   • Language switching   (lang_btn_cb, apply_language)
 *   • Artist navigation    (artist_name_cb)
 *   • Equaliser            (eq_slider_cb, eq_toggle_cb, open_eq_window_cb)
 *   • Preferences          (open_prefs_window_cb, prefs_toggle_cb, …)
 *   • Download             (download_cb)
 *   • Sidebar / misc       (sidebar_playlist_cb, load_sidebar_playlists)
 *   • Category cards       (category_card_cb)
 *   • Playlist actions     (playlist_play_click_cb, playlist_delete_click_cb)
 *   • Playback state       (heart_btn_cb, shuffle_cb, repeat_cb)
 *   • Refresh              (refresh_current_view)
 */

#include <FL/Fl_Widget.H>
#include <string>

/* ── Playback ────────────────────────────────────── */
void play_selected_cb(Fl_Widget* w, void* data);
void play_btn_cb(Fl_Widget* w, void* data);
void pause_resume_cb(Fl_Widget* w, void* data);
void seek_cb(Fl_Widget* w, void* data);
void volume_cb(Fl_Widget* w, void* data);
void shuffle_cb(Fl_Widget* w, void* data);
void repeat_cb(Fl_Widget* w, void* data);
void heart_btn_cb(Fl_Widget* w, void* data);

/* ── Search ──────────────────────────────────────── */
void search_cb(Fl_Widget* w, void* data);
void progressive_fill_cb(void* data);
void load_more_search_results();
void yt_toggle_cb(Fl_Widget* w, void* data);
void twitch_toggle_cb(Fl_Widget* w, void* data);

/* ── Category cards (Home view) ──────────────────── */
void category_card_cb(Fl_Widget* w, void* data);

/* ── Sidebar ─────────────────────────────────────── */
void sidebar_playlist_cb(Fl_Widget* w, void* data);
void load_sidebar_playlists();

/* ── Language ────────────────────────────────────── */
void apply_language();
void lang_btn_cb(Fl_Widget* w, void* data);

/* ── Artist ──────────────────────────────────────── */
void artist_name_cb(Fl_Widget* w, void* data);

/* ── Equaliser ───────────────────────────────────── */
void eq_slider_cb(Fl_Widget* w, void* data);
void eq_toggle_cb(Fl_Widget* w, void* data);
void open_eq_window_cb(Fl_Widget* w, void* data);

/* ── Preferences ─────────────────────────────────── */
void open_prefs_window_cb(Fl_Widget* w, void* data);
void prefs_toggle_cb(Fl_Widget* w, void* data);
void status_toggle_cb(Fl_Widget* w, void* data);
void buffer_cb(Fl_Widget* w, void* data);
void fetch_size_cb(Fl_Widget* w, void* data);
void scroll_batch_cb(Fl_Widget* w, void* data);

/* ── Download ────────────────────────────────────── */
void download_cb(Fl_Widget* w, void* data);

/* ── Misc ────────────────────────────────────────── */
void refresh_current_view();
void playlist_play_click_cb(Fl_Widget* w, void* data);
void playlist_delete_click_cb(Fl_Widget* w, void* data);

/* ── Radio ────────────────────────────────────────── */
void radio_station_cb(Fl_Widget* w, void* data);
void radio_country_cb(Fl_Widget* w, void* data);
void radio_add_custom_cb(Fl_Widget* w, void* data);


#endif // APPCALLBACKS_H
