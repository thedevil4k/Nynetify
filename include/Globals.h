#ifndef GLOBALS_H
#define GLOBALS_H

#include <string>
#include <vector>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_Double_Window.H>
#include "Lang.h"
#include "Theme.h"
#include "YoutubeService.h"
#include "PlaylistManager.h"
#include "AppSettings.h"
#include "ProgressSlider.h"
#include "ModernSlider.h"
#include "ModernChoice.h"
#include "ModernButton.h"

/* ── Forward Declarations ────────────────────────── */
class PlayerEngine;
class SystemTray;
struct SearchResult;
struct CoverArtTask;
struct ChannelLoadTask;
class CircularButton;
class HeartButton;
class ResultsBrowser;
class ProgressSlider;
class ModernSlider;
class ModernButton;
class ModernChoice;

/* ── Globals ──────────────────────────────────────── */

// Player engine instance
extern PlayerEngine* player;

// System tray
extern SystemTray* systemTray;

// Search / result state
extern std::vector<SearchResult> last_results;
extern int total_loaded_results;
extern std::string last_search_query;
extern int last_search_filter;
extern std::string user_region;

// Playback queue state
extern std::vector<SearchResult> play_queue;
extern int current_queue_index;
extern bool is_shuffle;
extern bool is_repeat;

// Current view tracking
extern std::string current_category;
extern std::string current_playlist;

// UI widgets — player bar
extern ProgressSlider* progressBar;
extern ModernSlider* volumeSlider;
extern Fl_Box* currentTimeBox;
extern Fl_Box* totalTimeBox;
extern Fl_Input* searchBar;
extern ModernChoice* searchFilter;
extern Fl_Box* statusBar;
extern Fl_Double_Window* eqWin;
extern Fl_Double_Window* prefWin;
extern void* self; // HANDLE from windows.h

// View groups
extern Fl_Group* homeGroup;
extern Fl_Group* searchGroup;
extern Fl_Group* playlistGroup;

// Playlist / channel view widgets
extern Fl_Hold_Browser* sidebarPlaylistList;
extern Fl_Box* playlistCoverBox;
extern Fl_Box* playlistNameBox;
extern Fl_Box* playlistDescBox;
extern ModernButton* playlistDeleteBtn;

// Cover art widgets
extern Fl_Box* coverArtBox;
extern Fl_Box* nowPlayingBox;
extern Fl_Button* nowPlayingArtistBox;
extern Fl_Box* miniCoverArtBox;
extern Fl_JPEG_Image* currentImage;
extern Fl_JPEG_Image* miniCurrentImage;

// Language-sensitive sidebar widgets
extern ModernButton* sidebarHomeBtn;
extern ModernButton* sidebarSearchBtn;
extern Fl_Box* sidebarLibHeading;
extern ModernButton* sidebarLikedBtn;
extern ModernButton* sidebarNewPlaylistBtn;
extern ModernButton* sidebarPrefsBtn;
extern ModernButton* langToggleBtn;

// Home view widgets
extern Fl_Box* homeGreetingBox;
extern Fl_Box* homeBrowseTitle;
extern Fl_Box* homeFeaturedTitle;
extern Fl_Box* homeFeatDesc;
extern ModernButton* homeCardButtons[6];

// Circular & heart buttons in player bar
extern CircularButton* playBtn;
extern HeartButton* heartBtn;

// Results browser & settings
extern ResultsBrowser* resultsBrowser;
extern AppSettings settings;

/* ── Function Declarations ───────────────────────── */

// Global player callbacks
void play_selected_cb(Fl_Widget* w, void* data);
void play_btn_cb(Fl_Widget* w, void* data);
void pause_resume_cb(Fl_Widget* w, void* data);
void seek_cb(Fl_Widget* w, void* data);
void volume_cb(Fl_Widget* w, void* data);
void shuffle_cb(Fl_Widget* w, void* data);
void repeat_cb(Fl_Widget* w, void* data);
void heart_btn_cb(Fl_Widget* w, void* data);

// Queue management
void play_index(int index);
void play_next();
void play_prev();

// Search
void search_cb(Fl_Widget* w, void* data);
void progressive_fill_cb(void* data);
void load_more_search_results();
void category_card_cb(Fl_Widget* w, void* data);

// View switching
void show_home_view();
void show_search_view();
void show_playlist_view(const std::string& playlist_name);
void show_youtube_playlist_view(const std::string& playlist_id, const std::string& playlist_name, const std::string& uploader);
void show_channel_view(const std::string& channel_id, const std::string& channel_name);
void show_favorites_view();

// Sidebar
void sidebar_playlist_cb(Fl_Widget* w, void* data);
void load_sidebar_playlists();

// Language
void apply_language();
void lang_btn_cb(Fl_Widget* w, void* data);

// Artist
void artist_name_cb(Fl_Widget* w, void* data);

// EQ
void eq_slider_cb(Fl_Widget* w, void* data);
void eq_toggle_cb(Fl_Widget* w, void* data);
void open_eq_window_cb(Fl_Widget* w, void* data);

// Download
void download_cb(Fl_Widget* w, void* data);

// Preferences
void open_prefs_window_cb(Fl_Widget* w, void* data);
void prefs_toggle_cb(Fl_Widget* w, void* data);
void status_toggle_cb(Fl_Widget* w, void* data);
void buffer_cb(Fl_Widget* w, void* data);
void fetch_size_cb(Fl_Widget* w, void* data);
void scroll_batch_cb(Fl_Widget* w, void* data);

// Utilities
void refresh_current_view();
std::string get_greeting();
std::string format_time(double seconds);
void update_cover_art(const std::string& video_id);
void detect_region();

// Async callbacks
void cover_art_completed_cb(void* data);
void channel_content_completed_cb(void* data);
void region_detected_cb(void* data);

// Settings persistence
void save_settings();
void load_settings();
std::string get_default_downloads_path();

// Styled dialogs
void show_styled_message(const char* msg);
bool show_styled_choice(const char* msg);

// Artist parsing
std::vector<std::string> parse_artists(const std::string& author);
std::string clean_artist_name(std::string name);
bool is_valid_artist_name(const std::string& name);
std::vector<std::string> extract_artists_from_title(const std::string& title, const std::string& cleaned_uploader);
int show_artist_selector(const std::vector<std::string>& artists);

// Timer callbacks
void update_ui_cb(void* data);
void update_status_bar_cb(void* data);

// Playlist management callbacks
void playlist_play_click_cb(Fl_Widget* w, void* data);
void playlist_delete_click_cb(Fl_Widget* w, void* data);

#endif // GLOBALS_H
