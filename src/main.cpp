#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include "ModernChoice.h"
#include "ModernSlider.h"
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Item.H>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <thread>
#include <cstdlib>

#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>

#include "PlayerEngine.h"
#include "Theme.h"
#include "ModernButton.h"
#include "ProgressSlider.h"
#include "PlaylistManager.h"
#include "YoutubeService.h"
#include "Lang.h"

// Forward Declarations
void search_cb(Fl_Widget* w, void* data);
void progressive_fill_cb(void* data);
void load_more_search_results();
void refresh_current_view();
void show_home_view();
void show_search_view();
void show_playlist_view(const std::string& playlist_name);
void show_youtube_playlist_view(const std::string& playlist_id, const std::string& playlist_name, const std::string& uploader);
void show_channel_view(const std::string& channel_id, const std::string& channel_name);
void show_favorites_view();
void load_sidebar_playlists();
void update_cover_art(const std::string& video_id);
void play_index(int index);
void play_next();
void play_prev();
void artist_name_cb(Fl_Widget* w, void* data);
void apply_language();
void lang_btn_cb(Fl_Widget* w, void* data);
void download_cb(Fl_Widget* w, void* data);
void save_settings();
void load_settings();

// Globals
PlayerEngine* player = nullptr;
std::vector<SearchResult> last_results;
int total_loaded_results = 0;
std::string last_search_query;
int last_search_filter = 0;
std::string user_region = "US"; // Default

ProgressSlider* progressBar = nullptr;
ModernSlider* volumeSlider = nullptr;
Fl_Box* currentTimeBox = nullptr;
Fl_Box* totalTimeBox = nullptr;
Fl_Input* searchBar = nullptr;
ModernChoice* searchFilter = nullptr;
Fl_Box* statusBar = nullptr;
Fl_Double_Window* eqWin = nullptr;
Fl_Double_Window* prefWin = nullptr;
HANDLE self;

// Spotify Clone Globals
std::vector<SearchResult> play_queue;
int current_queue_index = -1;
bool is_shuffle = false;
bool is_repeat = false;

std::string current_category = "Top Hits";
std::string current_playlist = ""; // To track if we're viewing a playlist

// View Groups & Widgets
Fl_Group* homeGroup = nullptr;
Fl_Group* searchGroup = nullptr;
Fl_Group* playlistGroup = nullptr;
Fl_Hold_Browser* sidebarPlaylistList = nullptr;

Fl_Box* playlistCoverBox = nullptr;
Fl_Box* playlistNameBox = nullptr;
Fl_Box* playlistDescBox = nullptr;
ModernButton* playlistDeleteBtn = nullptr;
Fl_Box* coverArtBox = nullptr;
Fl_Box* nowPlayingBox = nullptr;
Fl_Button* nowPlayingArtistBox = nullptr;
Fl_Box* miniCoverArtBox = nullptr;

Fl_JPEG_Image* currentImage = nullptr;
Fl_JPEG_Image* miniCurrentImage = nullptr;

// Language-aware widget pointers (for apply_language)
ModernButton* sidebarHomeBtn = nullptr;
ModernButton* sidebarSearchBtn = nullptr;
Fl_Box* sidebarLibHeading = nullptr;
ModernButton* sidebarLikedBtn = nullptr;
ModernButton* sidebarNewPlaylistBtn = nullptr;
ModernButton* sidebarPrefsBtn = nullptr;
ModernButton* langToggleBtn = nullptr;

Fl_Box* homeGreetingBox = nullptr;
Fl_Box* homeBrowseTitle = nullptr;
Fl_Box* homeFeaturedTitle = nullptr;
Fl_Box* homeFeatDesc = nullptr;
ModernButton* homeCardButtons[6] = {};

// Custom Circular Button for Spotify Green Play/Pause
class CircularButton : public Fl_Button {
public:
    CircularButton(int x, int y, int w, int h, const char* label = 0)
        : Fl_Button(x, y, w, h, label) {
        box(FL_NO_BOX);
        color(Theme::ACCENT);
        labelcolor(FL_BLACK);
    }
protected:
    void draw() override {
        bool is_below = (Fl::belowmouse() == this);
        bool is_pushed = value() || (Fl::pushed() == this && is_below);

        Fl_Color bg = color();
        if (is_below) bg = fl_lighter(bg);
        if (is_pushed) bg = fl_darker(bg);

        // Draw filled circle
        fl_color(bg);
        fl_pie(x(), y(), w(), h(), 0, 360);

        // Draw the play/pause icon in the middle
        fl_color(labelcolor());
        int cx = x() + w() / 2;
        int cy = y() + h() / 2;
        std::string lbl = label() ? label() : "";
        if (lbl == "@>" || lbl == "PLAY" || lbl == "PAUSE" || lbl == "@||") {
            if (player && !player->is_paused()) {
                // Draw pause bars
                fl_rectf(cx - 5, cy - 8, 3, 16);
                fl_rectf(cx + 2, cy - 8, 3, 16);
            } else {
                // Draw a play triangle
                int pts_x[3] = { cx - 5, cx - 5, cx + 8 };
                int pts_y[3] = { cy - 8, cy + 8, cy };
                fl_polygon(pts_x[0], pts_y[0], pts_x[1], pts_y[1], pts_x[2], pts_y[2]);
            }
        } else {
            fl_font(labelfont(), labelsize());
            fl_draw(label(), x(), y(), w(), h(), FL_ALIGN_CENTER, 0, 0);
        }
    }
    int handle(int event) override {
        int ret = Fl_Button::handle(event);
        if (event == FL_ENTER || event == FL_LEAVE || event == FL_PUSH || event == FL_RELEASE) {
            redraw();
        }
        return ret;
    }
};

// Custom Heart Button for Favoriting
class HeartButton : public Fl_Button {
public:
    bool active = false;
    HeartButton(int x, int y, int w, int h) : Fl_Button(x, y, w, h, "") {
        box(FL_NO_BOX);
    }
protected:
    void draw() override {
        bool is_below = (Fl::belowmouse() == this);
        fl_color(active ? Theme::ACCENT : (is_below ? Theme::TEXT_PRIMARY : Theme::TEXT_SECONDARY));
        fl_font(FL_HELVETICA, 20);
        const char* symbol = active ? "\xe2\x99\xa5" : "\xe2\x99\xa1"; // filled vs empty heart
        fl_draw(symbol, x(), y(), w(), h(), FL_ALIGN_CENTER, 0, 0);
    }
    int handle(int event) override {
        int ret = Fl_Button::handle(event);
        if (event == FL_ENTER || event == FL_LEAVE || event == FL_PUSH || event == FL_RELEASE) {
            redraw();
        }
        return ret;
    }
};

CircularButton* playBtn = nullptr;
HeartButton* heartBtn = nullptr;

// Settings structure
struct AppSettings {
    bool loadThumbnails = true;
    bool showStatusBar = true;
    int bufferSizeMB = 2;
    int initialFetchSize = 50;
    int scrollBatchSize = 2;
    std::string downloadPath;
} settings;

// Helper to get time-based greeting
std::string get_greeting() {
    time_t rawtime;
    struct tm * timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    int hour = timeinfo->tm_hour;
    if (hour < 12) return lang->greeting_morning;
    else if (hour < 18) return lang->greeting_afternoon;
    else return lang->greeting_evening;
}

std::string get_default_downloads_path() {
    const char* user = getenv("USERPROFILE");
    if (user) {
        std::string path = std::string(user) + "\\Downloads";
        return path;
    }
    return "C:\\Downloads";
}

void save_settings() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string settingsPath(exePath);
    size_t pos = settingsPath.find_last_of("\\/");
    if (pos != std::string::npos) settingsPath = settingsPath.substr(0, pos + 1);
    settingsPath += "settings.cfg";
    std::ofstream f(settingsPath);
    if (!f) return;
    f << "loadThumbnails=" << (settings.loadThumbnails ? 1 : 0) << "\n";
    f << "showStatusBar=" << (settings.showStatusBar ? 1 : 0) << "\n";
    f << "bufferSizeMB=" << settings.bufferSizeMB << "\n";
    f << "initialFetchSize=" << settings.initialFetchSize << "\n";
    f << "scrollBatchSize=" << settings.scrollBatchSize << "\n";
    f << "downloadPath=" << settings.downloadPath << "\n";
}

void load_settings() {
    settings.downloadPath = get_default_downloads_path();
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string settingsPath(exePath);
    size_t pos = settingsPath.find_last_of("\\/");
    if (pos != std::string::npos) settingsPath = settingsPath.substr(0, pos + 1);
    settingsPath += "settings.cfg";
    std::ifstream f(settingsPath);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "loadThumbnails") settings.loadThumbnails = (val == "1");
        else if (key == "showStatusBar") settings.showStatusBar = (val == "1");
        else if (key == "bufferSizeMB") settings.bufferSizeMB = std::stoi(val);
        else if (key == "initialFetchSize") settings.initialFetchSize = std::stoi(val);
        else if (key == "scrollBatchSize") settings.scrollBatchSize = std::stoi(val);
        else if (key == "downloadPath" && !val.empty()) settings.downloadPath = val;
    }
}

// Modal Windows (Singletons)

// Forward declarations for styled dialogs
void show_styled_message(const char* msg);
bool show_styled_choice(const char* msg);

class CreatePlaylistWindow : public Fl_Double_Window {
    Fl_Input *nameIn, *commentIn;
public:
    void clear_inputs() {
        if (nameIn) nameIn->value("");
        if (commentIn) commentIn->value("");
    }

    CreatePlaylistWindow() : Fl_Double_Window(300, 180, lang->new_playlist_title) {
        color(Theme::SIDEBAR);
        nameIn = new Fl_Input(100, 25, 180, 25, lang->name_label);
        nameIn->textcolor(Theme::TEXT_PRIMARY);
        nameIn->color(Theme::HOVER);
        nameIn->labelcolor(Theme::TEXT_SECONDARY);

        commentIn = new Fl_Input(100, 65, 180, 25, lang->comment_label);
        commentIn->textcolor(Theme::TEXT_PRIMARY);
        commentIn->color(Theme::HOVER);
        commentIn->labelcolor(Theme::TEXT_SECONDARY);

        ModernButton* btn = new ModernButton(100, 115, 100, 35, lang->create_btn);
        btn->color(Theme::ACCENT);
        btn->callback(create_cb, this);
        end();
    }

    static void create_cb(Fl_Widget*, void* data) {
        CreatePlaylistWindow* win = (CreatePlaylistWindow*)data;
        std::string name = win->nameIn->value();
        std::string comment = win->commentIn->value();
        if (!name.empty()) {
            PlaylistManager::create_playlist(name, comment);
            win->hide();
            load_sidebar_playlists();
        }
    }
};

class PlaylistSelectionWindow : public Fl_Double_Window {
public:
    Fl_Hold_Browser* list;
    std::string vid;
    PlaylistSelectionWindow(const std::string& video_id) : Fl_Double_Window(300, 350, lang->add_to_playlist_title), vid(video_id) {
        color(Theme::SIDEBAR);
        list = new Fl_Hold_Browser(10, 10, 280, 280);
        list->color(Theme::HOVER);
        list->textcolor(Theme::TEXT_PRIMARY);
        list->selection_color(Theme::ACCENT);
        
        list->add(lang->favorites_btn);
        auto playlists = PlaylistManager::get_all_playlists();
        for (const auto& p : playlists) {
            // Strip .txt for UI
            std::string name = p;
            if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
                name = name.substr(0, name.size() - 4);
            }
            list->add(name.c_str());
        }

        ModernButton* btn = new ModernButton(100, 300, 100, 35, lang->add_btn);
        btn->callback(add_cb, this);
        end();
    }

    static void add_cb(Fl_Widget*, void* data) {
        PlaylistSelectionWindow* win = (PlaylistSelectionWindow*)data;
        int val = win->list->value();
        if (val > 0) {
            std::string choice = win->list->text(val);
            if (choice == "FAVORITES") {
                PlaylistManager::add_to_favorites(win->vid);
            } else {
                PlaylistManager::add_to_playlist(choice, win->vid);
            }
            win->hide();
            show_styled_message(lang->added_to_playlist);
        }
    }
};

// Styled message dialogs
void show_styled_message(const char* msg) {
    Fl_Double_Window win(360, 120, " ");
    win.color(Theme::SIDEBAR);
    win.set_modal();

    Fl_Box text(20, 20, 320, 40, msg);
    text.labelcolor(Theme::TEXT_PRIMARY);
    text.box(FL_NO_BOX);
    text.align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

    ModernButton* okBtn = new ModernButton(130, 75, 100, 30, lang->ok_btn);
    okBtn->color(Theme::ACCENT);
    okBtn->labelcolor(FL_BLACK);
    okBtn->callback([](Fl_Widget*, void* d) { ((Fl_Double_Window*)d)->hide(); }, &win);

    win.end();
    if (Fl::first_window()) {
        win.position(Fl::first_window()->x() + (Fl::first_window()->w() - win.w()) / 2,
                     Fl::first_window()->y() + (Fl::first_window()->h() - win.h()) / 2);
    }
    win.show();
    while (win.shown()) Fl::wait();
}

bool show_styled_choice(const char* msg) {
    bool result = false;
    Fl_Double_Window win(360, 140, " ");
    win.color(Theme::SIDEBAR);
    win.set_modal();

    Fl_Box text(20, 20, 320, 50, msg);
    text.labelcolor(Theme::TEXT_PRIMARY);
    text.box(FL_NO_BOX);
    text.align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

    ModernButton* yesBtn = new ModernButton(80, 90, 90, 30, lang->yes_btn);
    yesBtn->color(Theme::ACCENT);
    yesBtn->labelcolor(FL_BLACK);
    yesBtn->callback([](Fl_Widget* w, void* d) {
        *((bool*)d) = true;
        w->window()->hide();
    }, &result);

    ModernButton* noBtn = new ModernButton(190, 90, 90, 30, lang->no_btn);
    noBtn->color(Theme::HOVER);
    noBtn->labelcolor(Theme::TEXT_PRIMARY);
    noBtn->callback([](Fl_Widget*, void* d) { ((Fl_Double_Window*)d)->hide(); }, &win);

    win.end();
    if (Fl::first_window()) {
        win.position(Fl::first_window()->x() + (Fl::first_window()->w() - win.w()) / 2,
                     Fl::first_window()->y() + (Fl::first_window()->h() - win.h()) / 2);
    }
    win.show();
    while (win.shown()) Fl::wait();
    return result;
}

// Main List Browser
class ResultsBrowser : public Fl_Browser {
public:
    ResultsBrowser(int x, int y, int w, int h, const char* l = 0) : Fl_Browser(x, y, w, h, l) {}

    int handle(int event) override {
        if (event == FL_MOUSEWHEEL && Fl::event_dy() > 0) { // Scroll down
            if ((current_category == "SEARCH" || current_category == "CHANNEL") &&
                total_loaded_results < (int)last_results.size()) {

                // Cancel progressive fill if running
                Fl::remove_timeout(progressive_fill_cb);

                // Remove "Show more" line if present (last line)
                int sz = size();
                if (sz > 0) {
                    const char* t = text(sz);
                    if (t && strstr(t, lang->show_more_text)) remove(sz);
                }

                // Add next batch from buffer
                int batch = std::min(settings.scrollBatchSize, (int)last_results.size() - total_loaded_results);
                for (int i = 0; i < batch; i++) {
                    const auto& res = last_results[total_loaded_results + i];
                    if (res.is_channel)
                        add((std::string("@C255\x40\t") + res.title + "\t" + res.author).c_str());
                    else if (res.is_playlist)
                        add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
                    else {
                        bool is_fav = PlaylistManager::is_favorite(res.video_id);
                        std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
                        add((star + "\t" + res.title + "\t" + res.author).c_str());
                    }
                }
                total_loaded_results += batch;

                // Add "Show more" back if buffer still has items (SEARCH only)
                if (current_category == "SEARCH") {
                    if (total_loaded_results < (int)last_results.size()) {
                        std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix) + std::to_string((int)last_results.size() - total_loaded_results) + lang->remaining_suffix;
                        add(more.c_str());
                    }
                }

                redraw();
                return 1;
            }
        }
        if (event == FL_PUSH) {
            int wx = Fl::event_x();
            int wy = Fl::event_y();
            int line = value();

            if (Fl::event_button() == 1) { // Left Click
                // Check if clicked on "Show more" line (text-based, works regardless of buffer state)
                if (current_category == "SEARCH") {
                    const char* t = text(line);
                    if (t && strstr(t, lang->show_more_text)) {
                        load_more_search_results();
                        return 1;
                    }
                }
                // Click on the heart icon or star column to open playlist adder
                if (wx >= x() && wx < x() + 30) {
                    if (line > 0 && line <= (int)last_results.size()) {
                        if (last_results[line - 1].is_playlist || last_results[line - 1].is_channel) return 0;
                        std::string video_id = last_results[line - 1].video_id;
                        if (video_id.find(".txt") == std::string::npos) {
                            static PlaylistSelectionWindow* win = nullptr;
                            if (win) {
                                win->hide();
                                delete win;
                            }
                            win = new PlaylistSelectionWindow(video_id);
                            win->set_modal();
                            if (Fl::first_window()) {
                                win->position(Fl::first_window()->x() + (Fl::first_window()->w() - win->w())/2,
                                              Fl::first_window()->y() + (Fl::first_window()->h() - win->h())/2);
                            }
                            win->show();
                            return 1;
                        }
                    }
                }
            } else if (Fl::event_button() == 3) { // Right Click
                if (line > 0 && line <= (int)last_results.size()) {
                    Fl_Menu_Item rclick_menu[] = {
                        { lang->delete_from_playlist, 0, delete_entry_cb, (void*)(intptr_t)line },
                        { 0 }
                    };
                    const Fl_Menu_Item* m = rclick_menu->popup(Fl::event_x(), Fl::event_y());
                    if (m) m->do_callback(this, m->user_data());
                    return 1;
                }
            }
        }
        return Fl_Browser::handle(event);
    }

    static void delete_entry_cb(Fl_Widget* w, void* data) {
        ResultsBrowser* rb = (ResultsBrowser*)w;
        int line = (int)(intptr_t)data;
        if (line > 0 && line <= (int)last_results.size()) {
            std::string video_id = last_results[line - 1].video_id;
            if (current_category == "MY FAVORITES") {
                PlaylistManager::remove_from_favorites(video_id);
            } else if (!current_playlist.empty()) {
                PlaylistManager::remove_song_from_playlist(current_playlist, video_id);
            }
            refresh_current_view();
        }
    }
};

ResultsBrowser* resultsBrowser = nullptr;

// Time utilities
std::string format_time(double seconds) {
    if (seconds < 0) seconds = 0;
    int m = (int)(seconds / 60);
    int s = (int)seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << m << ":"
        << std::setw(2) << std::setfill('0') << s;
    return oss.str();
}

// Queue Player Traversal Functions
void play_index(int index) {
    if (index >= 0 && index < (int)play_queue.size()) {
        current_queue_index = index;
        std::string video_id = play_queue[index].video_id;
        std::string title = play_queue[index].title;
        std::string author = play_queue[index].author;

        std::cout << "[UI] Queue Play Index " << index << ": " << title << " (" << video_id << ")" << std::endl;
        
        // Match selection in browser if list matches
        resultsBrowser->select(index + 1);

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

        if (heartBtn) {
            heartBtn->active = PlaylistManager::is_favorite(video_id);
            heartBtn->redraw();
        }

        update_cover_art(video_id);

        std::string stream_url = YoutubeService::get_audio_url(video_id);
        if (!stream_url.empty()) {
            player->play(stream_url);
            if (playBtn) {
                playBtn->redraw();
            }
        } else {
            std::cerr << "[ERROR] Could not get stream URL" << std::endl;
        }
    }
}

void play_next() {
    if (play_queue.empty()) return;
    if (is_repeat) {
        play_index(current_queue_index);
        return;
    }
    if (is_shuffle) {
        int idx = rand() % play_queue.size();
        play_index(idx);
        return;
    }
    int next_idx = current_queue_index + 1;
    if (next_idx >= (int)play_queue.size()) {
        next_idx = 0; // wrap around
    }
    play_index(next_idx);
}

void play_prev() {
    if (play_queue.empty()) return;
    int prev_idx = current_queue_index - 1;
    if (prev_idx < 0) {
        prev_idx = (int)play_queue.size() - 1; // wrap around
    }
    play_index(prev_idx);
}

std::vector<std::string> parse_artists(const std::string& author) {
    std::vector<std::string> result;
    if (author.empty()) return result;

    std::string s = author;
    const char* patterns[] = {" ft. ", " feat. ", " Ft. ", " Feat. ", " & ", " x ", " X ", ", "};
    const char* delim = " | ";

    for (const char* pat : patterns) {
        std::string p(pat);
        size_t pos = 0;
        while ((pos = s.find(p, pos)) != std::string::npos) {
            s.replace(pos, p.length(), delim);
            pos += 3;
        }
    }

    std::string d = " | ";
    size_t start = 0, pos;
    while ((pos = s.find(d, start)) != std::string::npos) {
        std::string tok = s.substr(start, pos - start);
        while (!tok.empty() && tok.front() == ' ') tok.erase(0, 1);
        while (!tok.empty() && tok.back() == ' ') tok.pop_back();
        if (!tok.empty()) result.push_back(tok);
        start = pos + d.length();
    }
    std::string tok = s.substr(start);
    while (!tok.empty() && tok.front() == ' ') tok.erase(0, 1);
    while (!tok.empty() && tok.back() == ' ') tok.pop_back();
    if (!tok.empty()) result.push_back(tok);

    return result;
}

std::string clean_artist_name(std::string name) {
    size_t pos = name.find(" - Topic");
    if (pos != std::string::npos) {
        name = name.substr(0, pos);
    }
    // Strip " and [digits] more" or " & [digits] more"
    pos = name.find(" and ");
    if (pos != std::string::npos) {
        std::string tail = name.substr(pos + 5); // after " and "
        size_t more_pos = tail.find(" more");
        if (more_pos != std::string::npos && more_pos > 0) {
            bool all_digits = true;
            for (size_t j = 0; j < more_pos; ++j) {
                if (!std::isdigit(static_cast<unsigned char>(tail[j]))) { all_digits = false; break; }
            }
            if (all_digits) {
                name = name.substr(0, pos);
            }
        }
    }
    pos = name.find(" & ");
    if (pos != std::string::npos) {
        std::string tail = name.substr(pos + 3);
        size_t more_pos = tail.find(" more");
        if (more_pos != std::string::npos && more_pos > 0) {
            bool all_digits = true;
            for (size_t j = 0; j < more_pos; ++j) {
                if (!std::isdigit(static_cast<unsigned char>(tail[j]))) { all_digits = false; break; }
            }
            if (all_digits) {
                name = name.substr(0, pos);
            }
        }
    }
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) name.erase(0, 1);
    while (!name.empty() && (name.back() == ' ' || name.back() == '\t')) name.pop_back();
    return name;
}

bool is_valid_artist_name(const std::string& name) {
    if (name.empty() || name.length() > 40) return false;
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return std::tolower(c); });
    const char* bad_words[] = {
        "video", "oficial", "official", "lyrics", "letra", "audio",
        "visualizer", "videoclip", "clip", "prod", "remix", "mashup",
        "karaoke", "sub", "subtitulado", "subtitles", "reaccion",
        "official video", "video oficial"
    };
    for (const char* bw : bad_words) {
        if (lower.find(bw) != std::string::npos) return false;
    }
    if (lower.find('(') != std::string::npos || lower.find(')') != std::string::npos ||
        lower.find('[') != std::string::npos || lower.find(']') != std::string::npos) {
        return false;
    }
    return true;
}

std::vector<std::string> extract_artists_from_title(const std::string& title, const std::string& cleaned_uploader) {
    std::vector<std::string> results;
    size_t dash_pos = title.find(" - ");
    if (dash_pos == std::string::npos) return results;

    std::string partA = title.substr(0, dash_pos);
    std::string partB = title.substr(dash_pos + 3);
    while (!partA.empty() && partA.front() == ' ') partA.erase(0, 1);
    while (!partA.empty() && partA.back() == ' ') partA.pop_back();
    while (!partB.empty() && partB.front() == ' ') partB.erase(0, 1);
    while (!partB.empty() && partB.back() == ' ') partB.pop_back();

    std::string partA_lower = partA;
    std::string partB_lower = partB;
    std::string uploader_lower = cleaned_uploader;
    std::transform(partA_lower.begin(), partA_lower.end(), partA_lower.begin(), [](unsigned char c){ return std::tolower(c); });
    std::transform(partB_lower.begin(), partB_lower.end(), partB_lower.begin(), [](unsigned char c){ return std::tolower(c); });
    std::transform(uploader_lower.begin(), uploader_lower.end(), uploader_lower.begin(), [](unsigned char c){ return std::tolower(c); });
    while (!uploader_lower.empty() && uploader_lower.back() == ' ') uploader_lower.pop_back();

    bool partA_is_artist = false;
    bool partB_is_artist = false;

    if (!uploader_lower.empty()) {
        if (partA_lower.find(uploader_lower) != std::string::npos) {
            partA_is_artist = true;
        } else if (partB_lower.find(uploader_lower) != std::string::npos) {
            partB_is_artist = true;
        }
    }

    if (!partA_is_artist && !partB_is_artist) {
        const char* keywords[] = {"video", "oficial", "official", "lyrics", "letra", "audio", "visualizer", "videoclip", "remix", "mashup"};
        bool partA_has = false;
        bool partB_has = false;
        for (const char* kw : keywords) {
            if (partA_lower.find(kw) != std::string::npos) partA_has = true;
            if (partB_lower.find(kw) != std::string::npos) partB_has = true;
        }
        if (partA_has && !partB_has) {
            partB_is_artist = true;
        } else if (partB_has && !partA_has) {
            partA_is_artist = true;
        }
    }

    if (!partA_is_artist && !partB_is_artist) partA_is_artist = true;

    std::string artist_str = partA_is_artist ? partA : partB;
    std::string title_str  = partA_is_artist ? partB : partA;

    std::vector<std::string> parsed = parse_artists(artist_str);
    for (const auto& p : parsed) {
        std::string cleaned = clean_artist_name(p);
        if (is_valid_artist_name(cleaned)) results.push_back(cleaned);
    }

    const char* ft_keywords[] = {" ft. ", " feat. ", " Ft. ", " Feat. "};
    for (const char* ft : ft_keywords) {
        size_t feat_pos = title_str.find(ft);
        if (feat_pos != std::string::npos) {
            std::string feat_part = title_str.substr(feat_pos + strlen(ft));
            while (!feat_part.empty() && feat_part.front() == ' ') feat_part.erase(0, 1);
            if (!feat_part.empty() && feat_part.back() == ')') feat_part.pop_back();
            if (!feat_part.empty() && feat_part.back() == ']') feat_part.pop_back();
            std::vector<std::string> feat_parsed = parse_artists(feat_part);
            for (const auto& p : feat_parsed) {
                std::string cleaned = clean_artist_name(p);
                if (is_valid_artist_name(cleaned)) results.push_back(cleaned);
            }
            break;
        }
    }

    return results;
}

int show_artist_selector(const std::vector<std::string>& artists) {
    int result = -1;
    int h = 70 + (int)artists.size() * 42;
    int height = std::max(150, std::min(h, 400));
    Fl_Double_Window win(300, height, lang->view_channel);
    win.color(Theme::SIDEBAR);

    struct SelData { int idx; int* res; Fl_Window* w; };

    for (size_t i = 0; i < artists.size(); i++) {
        ModernButton* btn = new ModernButton(20, 20 + (int)i * 42, 260, 34, artists[i].c_str());
        btn->color(Theme::HOVER);
        btn->labelcolor(Theme::TEXT_PRIMARY);
        btn->labelsize(13);
        SelData* sd = new SelData{(int)i, &result, &win};
        btn->callback([](Fl_Widget* w, void* d) {
            SelData* sd = (SelData*)d;
            *sd->res = sd->idx;
            sd->w->hide();
            delete sd;
        }, sd);
    }

    win.end();
    if (Fl::first_window()) {
        win.position(Fl::first_window()->x() + (Fl::first_window()->w() - win.w()) / 2,
                     Fl::first_window()->y() + (Fl::first_window()->h() - win.h()) / 2);
    }
    win.show();
    while (win.shown()) Fl::wait();
    return result;
}

void artist_name_cb(Fl_Widget* w, void* data) {
    if (current_queue_index < 0 || current_queue_index >= (int)play_queue.size()) return;

    std::string author_str = play_queue[current_queue_index].author;
    std::string cleaned_uploader = clean_artist_name(author_str);

    // Start with the main uploader artist
    std::vector<std::string> artists;
    if (!cleaned_uploader.empty()) {
        artists.push_back(cleaned_uploader);
    }

    // Extract collaborators from the title
    std::string title_str = play_queue[current_queue_index].title;
    std::vector<std::string> title_artists = extract_artists_from_title(title_str, cleaned_uploader);

    // Merge and dedup (case-insensitive)
    for (const auto& ta : title_artists) {
        bool found = false;
        for (const auto& a : artists) {
            std::string a_lower = a, ta_lower = ta;
            std::transform(a_lower.begin(), a_lower.end(), a_lower.begin(), [](unsigned char c){ return std::tolower(c); });
            std::transform(ta_lower.begin(), ta_lower.end(), ta_lower.begin(), [](unsigned char c){ return std::tolower(c); });
            if (a_lower == ta_lower) { found = true; break; }
        }
        if (!found && is_valid_artist_name(ta)) {
            artists.push_back(ta);
        }
    }

    if (artists.empty()) {
        show_styled_message(lang->channel_id_unavailable);
        return;
    }

    int selected = 0;
    if (artists.size() > 1) {
        selected = show_artist_selector(artists);
        if (selected < 0 || selected >= (int)artists.size()) return;
    }

    std::string name = artists[selected];

    if (selected == 0) {
        std::string cid = play_queue[current_queue_index].channel_id;
        if (cid.empty()) {
            cid = YoutubeService::get_channel_id(play_queue[current_queue_index].video_id);
            if (!cid.empty()) play_queue[current_queue_index].channel_id = cid;
        }
        if (!cid.empty()) {
            show_channel_view(cid, name);
        } else {
            show_styled_message(lang->channel_id_unavailable);
        }
    } else {
        auto results = YoutubeService::search(name, user_region, 3, 5);
        for (const auto& r : results) {
            if (r.is_channel) {
                show_channel_view(r.video_id, name);
                return;
            }
        }
        show_styled_message(lang->channel_id_unavailable);
    }
}

void apply_language() {
    // Sidebar
    if (sidebarHomeBtn) sidebarHomeBtn->copy_label(lang->home);
    if (sidebarSearchBtn) sidebarSearchBtn->copy_label(lang->search);
    if (sidebarLibHeading) sidebarLibHeading->copy_label(lang->your_library);
    if (sidebarLikedBtn) sidebarLikedBtn->copy_label(lang->liked_songs);
    if (sidebarNewPlaylistBtn) sidebarNewPlaylistBtn->copy_label(lang->create_playlist);
    if (sidebarPrefsBtn) sidebarPrefsBtn->copy_label(lang->settings);
    if (langToggleBtn) langToggleBtn->copy_label(lang->language_btn);

    // Home
    if (homeGreetingBox) homeGreetingBox->copy_label(get_greeting().c_str());
    if (homeBrowseTitle) homeBrowseTitle->copy_label(lang->browse_categories);
    if (homeFeaturedTitle) homeFeaturedTitle->copy_label(lang->now_playing_featured);
    if (homeFeatDesc) homeFeatDesc->copy_label(lang->feat_desc);
    for (int i = 0; i < 6; i++) {
        if (homeCardButtons[i]) homeCardButtons[i]->copy_label(lang->card_cats[i]);
    }

    // Search filter
    if (searchFilter) {
        int prev_val = searchFilter->value();
        searchFilter->clear();
        searchFilter->add(lang->everything);
        searchFilter->add(lang->songs_filter);
        searchFilter->add(lang->playlists_filter);
        searchFilter->add(lang->channels_filter);
        searchFilter->value(prev_val >= 0 && prev_val < 4 ? prev_val : 0);
    }
    if (searchBar) searchBar->tooltip(lang->search_tooltip);

    // Playlist view placeholders
    if (playlistNameBox && (current_category.empty() || current_category == "Top Hits"))
        playlistNameBox->copy_label(lang->playlist_name_placeholder);
    if (playlistDescBox) playlistDescBox->copy_label(lang->playlist_desc_placeholder);
    if (playlistDeleteBtn) playlistDeleteBtn->copy_label(lang->delete_playlist);

    // Player
    if (current_queue_index < 0 && nowPlayingBox)
        nowPlayingBox->copy_label(lang->not_playing);
    if (nowPlayingArtistBox) nowPlayingArtistBox->tooltip(lang->view_channel);

    // Window title
    if (Fl::first_window())
        Fl::first_window()->copy_label(lang->window_title);

    Fl::redraw();
}

void lang_btn_cb(Fl_Widget*, void*) {
    if (lang == &LANG_EN)
        lang = &LANG_ES;
    else
        lang = &LANG_EN;
    if (eqWin) { eqWin->hide(); eqWin = nullptr; }
    if (prefWin) { prefWin->hide(); prefWin = nullptr; }
    apply_language();
}

// Global player callbacks
void play_selected_cb(Fl_Widget* w, void* data) {
    Fl_Browser* browser = (Fl_Browser*)w;
    int line = browser->value();
    // "Show more" line → load more results (text-based detection)
    if (current_category == "SEARCH") {
        const char* t = browser->text(line);
        if (t && strstr(t, lang->show_more_text)) {
            load_more_search_results();
            return;
        }
    }
    if (line > 0 && line <= (int)last_results.size()) {
        // If not double-clicked and not triggered via button click, skip play
        if (!Fl::event_clicks() && data == nullptr) {
            return;
        }

        std::string video_id = last_results[line - 1].video_id;
        std::string title = last_results[line - 1].title;

        // If it's a YouTube playlist, open playlist view
        if (last_results[line - 1].is_playlist) {
            std::string author = last_results[line - 1].author;
            show_youtube_playlist_view(video_id, title, author);
            return;
        }

        // If it's a YouTube channel, open channel view
        if (last_results[line - 1].is_channel) {
            show_channel_view(video_id, title);
            return;
        }

        // If it's a local playlist reference (ends in .txt), switch to playlist view
        if (video_id.find(".txt") != std::string::npos) {
            std::string name = video_id;
            if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
                name = name.substr(0, name.size() - 4);
            }
            show_playlist_view(name);
            return;
        }

        play_queue = last_results;
        current_queue_index = line - 1;
        play_index(current_queue_index);
    }
}

void play_btn_cb(Fl_Widget* w, void* data) {
    if (player) {
        if (player->is_paused()) {
            player->resume();
            w->redraw();
        } else {
            // Check if anything is currently loaded
            if (current_queue_index >= 0) {
                player->pause();
                w->redraw();
            } else {
                // If nothing playing, play first song in browser if selected
                int line = resultsBrowser->value();
                if (line > 0 && line <= (int)last_results.size()) {
                    play_selected_cb(resultsBrowser, (void*)1);
                } else if (!last_results.empty()) {
                    play_queue = last_results;
                    current_queue_index = 0;
                    play_index(0);
                }
            }
        }
    }
}

void pause_resume_cb(Fl_Widget* w, void* data) {
    if (player) {
        if (player->is_paused()) {
            player->resume();
        } else {
            player->pause();
        }
        if (playBtn) playBtn->redraw();
    }
}

void seek_cb(Fl_Widget* w, void* data) {
    if (player) {
        player->set_position(progressBar->value());
    }
}

void volume_cb(Fl_Widget* w, void* data) {
    if (player) {
        player->set_volume(volumeSlider->value());
    }
}

void shuffle_cb(Fl_Widget* w, void* data) {
    is_shuffle = !is_shuffle;
    w->selection_color(is_shuffle ? Theme::ACCENT : Theme::HOVER);
    w->labelcolor(is_shuffle ? Theme::ACCENT : Theme::TEXT_SECONDARY);
    w->redraw();
}

void repeat_cb(Fl_Widget* w, void* data) {
    is_repeat = !is_repeat;
    w->selection_color(is_repeat ? Theme::ACCENT : Theme::HOVER);
    w->labelcolor(is_repeat ? Theme::ACCENT : Theme::TEXT_SECONDARY);
    w->redraw();
}

void heart_btn_cb(Fl_Widget* w, void* data) {
    if (current_queue_index >= 0 && current_queue_index < (int)play_queue.size()) {
        std::string video_id = play_queue[current_queue_index].video_id;
        if (PlaylistManager::is_favorite(video_id)) {
            PlaylistManager::remove_from_favorites(video_id);
            heartBtn->active = false;
        } else {
            PlaylistManager::add_to_favorites(video_id);
            heartBtn->active = true;
        }
        heartBtn->redraw();
        if (current_category == "MY FAVORITES") {
            refresh_current_view();
        }
    }
}

// Category cards clicking on home
void category_card_cb(Fl_Widget* w, void* data) {
    int idx = (int)(intptr_t)data;
    if (idx == 5) { // "Liked Songs" / "Me gusta" card
        show_favorites_view();
    } else {
        show_search_view();
        if (searchBar) {
            searchBar->value(lang->card_cats[idx]);
            search_cb(searchBar, resultsBrowser);
        }
    }
}

// Dynamic refresh logic
void refresh_current_view() {
    if (current_category == "MY FAVORITES") {
        show_favorites_view();
    } else if (!current_playlist.empty()) {
        std::string name = current_playlist;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
            name = name.substr(0, name.size() - 4);
        }
        show_playlist_view(name);
    }
}

// Search execution
void search_cb(Fl_Widget* w, void* data) {
    current_category = "SEARCH";
    current_playlist = "";
    Fl_Input* input = (Fl_Input*)w;
    Fl_Browser* browser = (Fl_Browser*)data;
    browser->clear();

    std::string query = input->value();
    if (query.empty()) return;

    last_search_query = query;
    if (searchFilter) last_search_filter = searchFilter->value();

    std::cout << "[UI] Searching for: " << query << "..." << std::endl;

    try {
        last_results = YoutubeService::search(query, user_region, last_search_filter, settings.initialFetchSize);
        std::cout << "[UI] Found " << last_results.size() << " results." << std::endl;

        total_loaded_results = 0;
        browser->clear();

        // Show first 2 items immediately (seeds the progressive fill)
        int to_show = std::min(2, (int)last_results.size());
        for (int i = 0; i < to_show; i++) {
            const auto& res = last_results[i];
            if (res.is_channel)
                browser->add((std::string("@C255\x40\t") + res.title + "\t" + res.author).c_str());
            else if (res.is_playlist)
                browser->add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
            else {
                bool is_fav = PlaylistManager::is_favorite(res.video_id);
                std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
                browser->add((star + "\t" + res.title + "\t" + res.author).c_str());
            }
        }
        total_loaded_results = to_show;

        // Start progressive fill timer (only if there are more items to show)
        if (total_loaded_results < (int)last_results.size()) {
            Fl::add_timeout(0.05, progressive_fill_cb);
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Search failed: " << e.what() << std::endl;
        browser->add(lang->search_failed);
    }
}

void progressive_fill_cb(void* data) {
    if (!resultsBrowser) return;

    // Add 3 items (or remaining)
    int batch = std::min(3, (int)last_results.size() - total_loaded_results);
    for (int i = total_loaded_results; i < total_loaded_results + batch; i++) {
        const auto& res = last_results[i];
        if (res.is_channel)
            resultsBrowser->add((std::string("@C255\x40\t") + res.title + "\t" + res.author).c_str());
        else if (res.is_playlist)
            resultsBrowser->add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
        else {
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            resultsBrowser->add((star + "\t" + res.title + "\t" + res.author).c_str());
        }
    }
    total_loaded_results += batch;
    resultsBrowser->redraw();

    // Estimate viewport capacity
    int row_h = resultsBrowser->textsize() + 4;
    if (row_h < 14) row_h = 18;
    int viewport_cap = resultsBrowser->h() / row_h;

    // Check whether to stop
    if (total_loaded_results >= viewport_cap || total_loaded_results >= (int)last_results.size()) {
        // Add "Show more" if viewport full and buffer still has items (SEARCH only)
        if (current_category == "SEARCH" && total_loaded_results < (int)last_results.size()) {
            std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix) + std::to_string((int)last_results.size() - total_loaded_results) + lang->remaining_suffix;
            resultsBrowser->add(more.c_str());
            resultsBrowser->redraw();
        }
        return; // stop timer
    }

    // Continue filling
    Fl::repeat_timeout(0.05, progressive_fill_cb);
}

void load_more_search_results() {
    Fl_Browser* browser = resultsBrowser;
    if (!browser) return;

    // Remove the "Show more" line if present
    int last_line = browser->size();
    if (last_line > 0) {
        const char* t = browser->text(last_line);
        if (t && strstr(t, lang->show_more_text)) browser->remove(last_line);
    }

    int prev_total = (int)last_results.size();
    int new_limit = prev_total + settings.initialFetchSize;

    // Refetch with larger limit to get more results
    auto fresh = YoutubeService::search(last_search_query, user_region, last_search_filter, new_limit);

    // Append new items (those beyond prev_total)
    for (int i = prev_total; i < (int)fresh.size(); i++) {
        last_results.push_back(fresh[i]);
    }

    // Show next batch from the buffer
    int to_add = std::min(settings.scrollBatchSize, (int)last_results.size() - total_loaded_results);
    for (int i = total_loaded_results; i < total_loaded_results + to_add; i++) {
        const auto& res = last_results[i];
        if (res.is_channel)
            browser->add((std::string("@C255\x40\t") + res.title + "\t" + res.author).c_str());
        else if (res.is_playlist)
            browser->add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
        else {
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            browser->add((star + "\t" + res.title + "\t" + res.author).c_str());
        }
    }
    total_loaded_results += to_add;

    // If buffer still has items, add Show more back
    if (total_loaded_results < (int)last_results.size()) {
        std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix) + std::to_string((int)last_results.size() - total_loaded_results) + lang->remaining_suffix;
        browser->add(more.c_str());
    }
    browser->redraw();
}

// Sidebar Playlist Callback
void sidebar_playlist_cb(Fl_Widget* w, void* data) {
    int val = sidebarPlaylistList->value();
    if (val > 0) {
        std::string playlist_name = sidebarPlaylistList->text(val);
        show_playlist_view(playlist_name);
    }
}

// Sidebar loader
void load_sidebar_playlists() {
    if (!sidebarPlaylistList) return;
    sidebarPlaylistList->clear();
    auto lists = PlaylistManager::get_all_playlists();
    for (const auto& list : lists) {
        std::string name = list;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt") {
            name = name.substr(0, name.size() - 4);
        }
        sidebarPlaylistList->add(name.c_str());
    }
    sidebarPlaylistList->redraw();
}

// Equalizer sliders callback
void eq_slider_cb(Fl_Widget* w, void* data) {
    intptr_t band = (intptr_t)data;
    Fl_Slider* slider = (Fl_Slider*)w;
    if (player) {
        player->set_eq_gain((int)band, slider->value());
    }
}

void eq_toggle_cb(Fl_Widget* w, void* data) {
    Fl_Check_Button* check = (Fl_Check_Button*)w;
    if (player) {
        player->set_eq_enabled(check->value());
    }
}

void open_eq_window_cb(Fl_Widget* w, void* data) {
    if (eqWin) {
        eqWin->show();
        return;
    }
    eqWin = new Fl_Double_Window(500, 320, lang->eq_title);
    eqWin->color(Theme::SIDEBAR);
    
    if (Fl::first_window()) {
        eqWin->position(Fl::first_window()->x() + (Fl::first_window()->w() - eqWin->w())/2,
                        Fl::first_window()->y() + (Fl::first_window()->h() - eqWin->h())/2);
    }
    
    Fl_Check_Button* eqToggle = new Fl_Check_Button(20, 10, 120, 25, lang->enable_eq);
    eqToggle->labelcolor(Theme::TEXT_PRIMARY);
    eqToggle->callback(eq_toggle_cb);
    if (player) eqToggle->value(player->is_eq_enabled() ? 1 : 0);

    Fl_Box* topDb = new Fl_Box(465, 45, 30, 20, "+12");
    topDb->labelcolor(Theme::TEXT_SECONDARY);
    topDb->labelsize(10);

    Fl_Box* midDb = new Fl_Box(465, 137, 30, 20, "0");
    midDb->labelcolor(Theme::TEXT_SECONDARY);
    midDb->labelsize(10);

    Fl_Box* botDb = new Fl_Box(465, 230, 30, 20, "-12");
    botDb->labelcolor(Theme::TEXT_SECONDARY);
    botDb->labelsize(10);

    const char* labels[] = {"31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
    
    for (int i = 0; i < 10; ++i) {
        ModernSlider* s = new ModernSlider(20 + i * 45, 50, 25, 200, labels[i]);
        s->type(FL_VERT_SLIDER);
        s->bounds(12, -12);
        s->value(0);
        s->callback(eq_slider_cb, (void*)(intptr_t)i);
        s->labelsize(10);
        s->labelcolor(Theme::TEXT_PRIMARY);
    }
    
    eqWin->end();
    eqWin->set_non_modal();
    eqWin->show();
}

// Preferences Callbacks
void prefs_toggle_cb(Fl_Widget* w, void* data) {
    Fl_Check_Button* btn = (Fl_Check_Button*)w;
    settings.loadThumbnails = btn->value();
    save_settings();
}

void status_toggle_cb(Fl_Widget* w, void* data) {
    Fl_Check_Button* btn = (Fl_Check_Button*)w;
    settings.showStatusBar = btn->value();
    if (statusBar) {
        if (settings.showStatusBar) statusBar->show();
        else statusBar->hide();
    }
    save_settings();
}

void buffer_cb(Fl_Widget* w, void* data) {
    Fl_Slider* slider = (Fl_Slider*)w;
    Fl_Box* label = (Fl_Box*)data;
    settings.bufferSizeMB = (int)slider->value();
    
    char buf[256];
    snprintf(buf, sizeof(buf), lang->max_buffer_label, settings.bufferSizeMB);
    label->copy_label(buf);
    label->redraw();

    if (player) {
        player->set_buffer_size(settings.bufferSizeMB);
    }
    save_settings();
}

void fetch_size_cb(Fl_Widget* w, void* data) {
    Fl_Slider* slider = (Fl_Slider*)w;
    Fl_Box* label = (Fl_Box*)data;
    settings.initialFetchSize = (int)slider->value();
    char buf[256];
    snprintf(buf, sizeof(buf), lang->fetch_size_label, settings.initialFetchSize);
    label->copy_label(buf);
    label->redraw();
    save_settings();
}

void scroll_batch_cb(Fl_Widget* w, void* data) {
    Fl_Slider* slider = (Fl_Slider*)w;
    Fl_Box* label = (Fl_Box*)data;
    settings.scrollBatchSize = (int)slider->value();
    char buf[256];
    snprintf(buf, sizeof(buf), lang->batch_label, settings.scrollBatchSize);
    label->copy_label(buf);
    label->redraw();
    save_settings();
}

void download_cb(Fl_Widget* w, void* data) {
    if (current_queue_index < 0 || current_queue_index >= (int)play_queue.size()) return;
    
    std::string video_id = play_queue[current_queue_index].video_id;
    std::string title = play_queue[current_queue_index].title;
    std::string author = play_queue[current_queue_index].author;
    
    // Sanitize filename
    std::string safe_author = author;
    std::string safe_title = title;
    const char* illegal = "\\/:*?\"<>|";
    for (char& c : safe_author) {
        for (const char* p = illegal; *p; ++p) { if (c == *p) { c = '_'; break; } }
    }
    for (char& c : safe_title) {
        for (const char* p = illegal; *p; ++p) { if (c == *p) { c = '_'; break; } }
    }
    
    // Get download path
    std::string dlPath = settings.downloadPath;
    if (dlPath.empty()) dlPath = get_default_downloads_path();
    
    std::string filename = dlPath + "\\" + safe_author + " - " + safe_title + ".mp3";
    
    std::string url = "https://www.youtube.com/watch?v=" + video_id;
    std::string cmd = std::string(YT_DLP) + " -x --audio-format mp3 --no-playlist -o \"" + filename + "\" \"" + url + "\" 2>NUL";
    
    // Show status
    char status[512];
    snprintf(status, sizeof(status), lang->downloading, title.c_str());
    if (statusBar) { statusBar->copy_label(status); statusBar->redraw(); }
    
    // Allocate a copy of the title string for the completion callback
    std::string* titleCopy = new std::string(title);
    
    std::thread([cmd, titleCopy]() {
        std::system(cmd.c_str());
        Fl::awake([](void* d) {
            std::string* t = (std::string*)d;
            if (statusBar) {
                char buf[512];
                snprintf(buf, sizeof(buf), lang->download_completed, t->c_str());
                statusBar->copy_label(buf);
                statusBar->redraw();
            }
            delete t;
        }, titleCopy);
    }).detach();
}

void open_prefs_window_cb(Fl_Widget* w, void* data) {
    if (prefWin) {
        prefWin->show();
        return;
    }
    prefWin = new Fl_Double_Window(300, 400, lang->settings_title);
    prefWin->color(Theme::SIDEBAR);

    if (Fl::first_window()) {
        prefWin->position(Fl::first_window()->x() + (Fl::first_window()->w() - prefWin->w())/2,
                          Fl::first_window()->y() + (Fl::first_window()->h() - prefWin->h())/2);
    }
    
    Fl_Check_Button* thumbToggle = new Fl_Check_Button(20, 20, 260, 30, lang->load_thumbnails);
    thumbToggle->labelcolor(Theme::TEXT_PRIMARY);
    thumbToggle->value(settings.loadThumbnails ? 1 : 0);
    thumbToggle->callback(prefs_toggle_cb);

    Fl_Check_Button* statusToggle = new Fl_Check_Button(20, 60, 260, 30, lang->show_status_bar);
    statusToggle->labelcolor(Theme::TEXT_PRIMARY);
    statusToggle->value(settings.showStatusBar ? 1 : 0);
    statusToggle->callback(status_toggle_cb);

    Fl_Box* bufLabel = new Fl_Box(20, 110, 260, 20);
    bufLabel->labelcolor(Theme::TEXT_SECONDARY);
    bufLabel->labelsize(12);
    bufLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    char initBuf[256];
    snprintf(initBuf, sizeof(initBuf), lang->max_buffer_label, settings.bufferSizeMB);
    bufLabel->copy_label(initBuf);

    ModernSlider* bufSlider = new ModernSlider(20, 135, 260, 20);
    bufSlider->type(FL_HOR_SLIDER);
    bufSlider->bounds(1, 100);
    bufSlider->step(1);
    bufSlider->value(settings.bufferSizeMB);
    bufSlider->callback(buffer_cb, (void*)bufLabel);

    // Fetch size (yt-dlp limit per call)
    Fl_Box* fetchLabel = new Fl_Box(20, 175, 260, 20);
    fetchLabel->labelcolor(Theme::TEXT_SECONDARY);
    fetchLabel->labelsize(12);
    fetchLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    snprintf(initBuf, sizeof(initBuf), lang->fetch_size_label, settings.initialFetchSize);
    fetchLabel->copy_label(initBuf);

    ModernSlider* fetchSlider = new ModernSlider(20, 200, 260, 20);
    fetchSlider->type(FL_HOR_SLIDER);
    fetchSlider->bounds(20, 200);
    fetchSlider->step(10);
    fetchSlider->value(settings.initialFetchSize);
    fetchSlider->callback(fetch_size_cb, (void*)fetchLabel);

    // Scroll batch size
    Fl_Box* batchLabel = new Fl_Box(20, 240, 260, 20);
    batchLabel->labelcolor(Theme::TEXT_SECONDARY);
    batchLabel->labelsize(12);
    batchLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    snprintf(initBuf, sizeof(initBuf), lang->batch_label, settings.scrollBatchSize);
    batchLabel->copy_label(initBuf);

    ModernSlider* batchSlider = new ModernSlider(20, 265, 260, 20);
    batchSlider->type(FL_HOR_SLIDER);
    batchSlider->bounds(1, 10);
    batchSlider->step(1);

    batchSlider->value(settings.scrollBatchSize);
    batchSlider->callback(scroll_batch_cb, (void*)batchLabel);

    // Download path
    Fl_Box* dlLabel = new Fl_Box(20, 305, 260, 20, lang->download_path);
    dlLabel->labelcolor(Theme::TEXT_SECONDARY);
    dlLabel->labelsize(12);
    dlLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    Fl_Input* dlInput = new Fl_Input(20, 325, 205, 25);
    dlInput->value(settings.downloadPath.c_str());
    dlInput->textcolor(Theme::TEXT_PRIMARY);
    dlInput->color(Theme::HOVER);
    dlInput->textsize(11);
    dlInput->callback([](Fl_Widget* w, void*) {
        Fl_Input* in = (Fl_Input*)w;
        settings.downloadPath = in->value();
        save_settings();
    });

    ModernButton* browseBtn = new ModernButton(230, 325, 50, 25, lang->browse);
    browseBtn->color(Theme::HOVER);
    browseBtn->labelcolor(Theme::TEXT_PRIMARY);
    browseBtn->labelsize(11);
    browseBtn->callback([](Fl_Widget*, void* d) {
        Fl_Input* inp = (Fl_Input*)d;
        Fl_Native_File_Chooser nfc;
        nfc.directory(inp->value());
        nfc.title("Select download folder");
        nfc.type(Fl_Native_File_Chooser::BROWSE_DIRECTORY);
        if (nfc.show() == 0) {
            inp->value(nfc.filename());
            settings.downloadPath = nfc.filename();
            save_settings();
        }
    }, dlInput);

    prefWin->end();
    prefWin->set_non_modal();
    prefWin->show();
}

// Cover Art Image Loader
void update_cover_art(const std::string& video_id) {
    if (!settings.loadThumbnails) return;

    std::string url = YoutubeService::get_thumbnail_url(video_id);
    std::string temp_file = "cover_temp.jpg";
    
    // Download using curl
    std::string cmd = "curl -s -L -o " + temp_file + " " + url;
    system(cmd.c_str());

    if (std::filesystem::exists(temp_file)) {
        // Large cover (Home view)
        if (coverArtBox) {
            if (currentImage) delete currentImage;
            currentImage = new Fl_JPEG_Image(temp_file.c_str());
            if (currentImage && currentImage->w() > 0) {
                int bw = coverArtBox->w();
                int bh = coverArtBox->h();
                Fl_Image* scaled = currentImage->copy(bw, bh);
                coverArtBox->image(scaled);
                coverArtBox->redraw();
            }
        }
        
        // Mini cover (Player bar)
        if (miniCoverArtBox) {
            if (miniCurrentImage) delete miniCurrentImage;
            miniCurrentImage = new Fl_JPEG_Image(temp_file.c_str());
            if (miniCurrentImage && miniCurrentImage->w() > 0) {
                int bw = miniCoverArtBox->w();
                int bh = miniCoverArtBox->h();
                Fl_Image* scaled = miniCurrentImage->copy(bw, bh);
                miniCoverArtBox->image(scaled);
                miniCoverArtBox->redraw();
            }
        }
    }
}

// View-switching Functions
void show_home_view() {
    homeGroup->show();
    searchGroup->hide();
    playlistGroup->hide();
    resultsBrowser->hide();
    if (sidebarPlaylistList) sidebarPlaylistList->value(0);
}

void show_search_view() {
    homeGroup->hide();
    searchGroup->show();
    playlistGroup->hide();
    
    resultsBrowser->position(220, 70);
    resultsBrowser->size(760, 570);
    resultsBrowser->show();
    if (sidebarPlaylistList) sidebarPlaylistList->value(0);
}

void show_playlist_view(const std::string& playlist_name) {
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();
    
    resultsBrowser->position(220, 220);
    resultsBrowser->size(760, 420);
    resultsBrowser->show();
    
    playlistNameBox->copy_label(playlist_name.c_str());
    
    std::string comment = PlaylistManager::get_playlist_comment(playlist_name + ".txt");
    if (comment.empty()) {
        comment = lang->playlist_desc_placeholder;
    }
    playlistDescBox->copy_label(comment.c_str());
    
    // Set current active playlist name
    current_playlist = playlist_name + ".txt";
    current_category = "MY PLAYLISTS";

    // Show delete button for local playlists
    playlistDeleteBtn->show();

    // Load songs
    resultsBrowser->clear();
    resultsBrowser->add(lang->loading_playlist);
    resultsBrowser->redraw();
    Fl::check();

    auto ids = PlaylistManager::get_playlist_songs(current_playlist);
    last_results = YoutubeService::get_metadata(ids);
    resultsBrowser->clear();
    if (last_results.empty()) {
        resultsBrowser->add(lang->no_tracks);
    } else {
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
    
    resultsBrowser->position(220, 220);
    resultsBrowser->size(760, 420);
    resultsBrowser->show();
    
    playlistNameBox->copy_label("Liked Songs");
    playlistDescBox->copy_label(lang->favorites_desc);
    
    current_playlist = "";
    current_category = "MY FAVORITES";

    // Hide delete button for favorites
    playlistDeleteBtn->hide();

    resultsBrowser->clear();
    resultsBrowser->add(lang->loading_favorites);
    resultsBrowser->redraw();
    Fl::check();

    auto items = PlaylistManager::get_favorites();
    std::vector<std::string> ids;
    for (const auto& item : items) {
        if (!item.is_playlist) ids.push_back(item.value);
    }
    
    last_results.clear();
    if (!ids.empty()) {
        last_results = YoutubeService::get_metadata(ids);
        resultsBrowser->clear();
        for (const auto& res : last_results) {
            resultsBrowser->add(("@C7\xe2\x98\x85\t" + res.title + "\t" + res.author).c_str());
        }
    } else {
        resultsBrowser->clear();
        resultsBrowser->add(lang->no_favorites);
    }
    resultsBrowser->redraw();
}

// YouTube Playlist View
void show_youtube_playlist_view(const std::string& playlist_id, const std::string& playlist_name, const std::string& uploader) {
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();

    resultsBrowser->position(220, 220);
    resultsBrowser->size(760, 420);
    resultsBrowser->show();

    playlistNameBox->copy_label(playlist_name.c_str());
    std::string desc = std::string(lang->yt_playlist_by) + uploader;
    playlistDescBox->copy_label(desc.c_str());

    playlistDeleteBtn->hide();

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

// Channel View
void show_channel_view(const std::string& channel_id, const std::string& channel_name) {
    homeGroup->hide();
    searchGroup->hide();
    playlistGroup->show();

    resultsBrowser->position(220, 220);
    resultsBrowser->size(760, 420);
    resultsBrowser->show();

    playlistNameBox->copy_label(channel_name.c_str());
    std::string desc = lang->yt_channel;
    playlistDescBox->copy_label(desc.c_str());

    playlistDeleteBtn->hide();

    current_playlist = "";
    current_category = "CHANNEL";

    resultsBrowser->clear();
    resultsBrowser->add(lang->loading_channel);
    resultsBrowser->redraw();
    Fl::check();

    last_results = YoutubeService::get_channel_content(channel_id, channel_name);

    // Load channel avatar
    std::string avatar_url = YoutubeService::get_channel_avatar_url(channel_id);
    if (!avatar_url.empty()) {
        std::string temp_file = "cover_temp.jpg";
        std::string dl_cmd = "curl -s -L -o " + temp_file + " \"" + avatar_url + "\"";
        system(dl_cmd.c_str());
        if (std::filesystem::exists(temp_file)) {
            if (playlistCoverBox) {
                Fl_JPEG_Image* img = new Fl_JPEG_Image(temp_file.c_str());
                if (img && img->w() > 0) {
                    int bw = playlistCoverBox->w();
                    int bh = playlistCoverBox->h();
                    Fl_Image* scaled = img->copy(bw, bh);
                    playlistCoverBox->image(scaled);
                    playlistCoverBox->redraw();
                }
                delete img;
            }
            std::filesystem::remove(temp_file);
        }
    }

    resultsBrowser->clear();
    if (last_results.empty()) {
        resultsBrowser->add(lang->no_channel_content);
    } else {
        // Show separator header for playlists
        bool has_playlists = false;
        for (const auto& res : last_results) {
            if (res.is_playlist) {
                if (!has_playlists) {
                    resultsBrowser->add((std::string("@C7") + lang->channel_playlists).c_str());
                    has_playlists = true;
                }
                resultsBrowser->add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
            }
        }
        // Show separator header for videos
        bool has_videos = false;
        for (const auto& res : last_results) {
            if (!res.is_playlist) {
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
    }
    resultsBrowser->redraw();
}

// Timer loops
void update_ui_cb(void* data) {
    if (player && progressBar) {
        // Run MPV event loop and check for EOF
        int ev = player->update();
        if (ev == 1) { // EOF reached
            play_next();
        }

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
            if (currentTimeBox) {
                currentTimeBox->copy_label("00:00");
                currentTimeBox->redraw();
            }
            if (totalTimeBox) {
                totalTimeBox->copy_label("00:00");
                totalTimeBox->redraw();
            }
        }
    }
    Fl::repeat_timeout(0.2, update_ui_cb);
}

void update_status_bar_cb(void* data) {
    if (!statusBar) return;

    // RAM
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(self, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    double ram_mb = pmc.WorkingSetSize / (1024.0 * 1024.0);

    // Network
    static ULONG lastIn = 0, lastOut = 0;
    static bool first_net = true;
    double down_kb = 0, up_kb = 0;
    
    ULONG dwSize = 0;
    GetIfTable(NULL, &dwSize, FALSE);
    MIB_IFTABLE *pIfTable = (MIB_IFTABLE *) malloc(dwSize);
    
    if (GetIfTable(pIfTable, &dwSize, FALSE) == NO_ERROR) {
        ULONG currentIn = 0, currentOut = 0;
        for (DWORD i = 0; i < pIfTable->dwNumEntries; i++) {
            currentIn += pIfTable->table[i].dwInOctets;
            currentOut += pIfTable->table[i].dwOutOctets;
        }
        if (!first_net) {
            if (currentIn >= lastIn) down_kb = (double)(currentIn - lastIn) / 1024.0;
            if (currentOut >= lastOut) up_kb = (double)(currentOut - lastOut) / 1024.0;
        }
        lastIn = currentIn; lastOut = currentOut;
        first_net = false;
    }
    if (pIfTable) free(pIfTable);

    // Local time
    time_t rawtime;
    struct tm * timeinfo;
    char time_str[80];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);

    double buf_usage = 0;
    if (player) buf_usage = player->get_buffer_usage_mb();

    char final_status[256];
    snprintf(final_status, sizeof(final_status), 
             " %s | Region: %s | RAM: %.1f MB | BUF: %.1f MB | Net D: %.1f KB/s U: %.1f KB/s | %s", lang->status_prefix,
             user_region.c_str(), ram_mb, buf_usage, down_kb, up_kb, time_str);
    
    statusBar->copy_label(final_status);
    statusBar->redraw();

    Fl::repeat_timeout(1.0, update_status_bar_cb);
}

void detect_region() {
    std::cout << "[UI] Detecting region..." << std::endl;
    std::string temp_file = "region_temp.txt";
    std::string cmd = "curl -s -L -o " + temp_file + " https://ipapi.co/country/";
    system(cmd.c_str());

    if (std::filesystem::exists(temp_file)) {
        std::ifstream ifs(temp_file);
        std::string region;
        if (ifs >> region && region.length() == 2) {
            user_region = region;
            std::cout << "[UI] Region detected: " << user_region << std::endl;
        }
        ifs.close();
        std::filesystem::remove(temp_file);
    }
}

// Playlist view Play Click
void playlist_play_click_cb(Fl_Widget* w, void* data) {
    if (!last_results.empty()) {
        play_queue = last_results;
        current_queue_index = 0;
        play_index(0);
    }
}

// Playlist view Delete Click
void playlist_delete_click_cb(Fl_Widget* w, void* data) {
    if (current_category == "MY PLAYLISTS" && !current_playlist.empty()) {
        if (show_styled_choice(lang->delete_confirm)) {
            PlaylistManager::delete_playlist(current_playlist);
            current_playlist = "";
            show_home_view();
            load_sidebar_playlists();
        }
    } else if (current_category == "MY FAVORITES") {
        show_styled_message(lang->cannot_delete_favorites);
    } else if (current_category == "YOUTUBE_PLAYLIST") {
        show_styled_message(lang->cannot_delete_yt_playlist);
    } else if (current_category == "CHANNEL") {
        show_styled_message(lang->cannot_delete_channel);
    }
}

int main(int argc, char **argv) {
    srand((unsigned int)time(nullptr));
    try {
        player = new PlayerEngine();
        self = GetCurrentProcess();
        PlaylistManager::ensure_directories();
        detect_region();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    Fl::background(18, 18, 18);
    Fl_Double_Window *window = new Fl_Double_Window(1000, 750, lang->window_title);
    window->color(Theme::BACKGROUND);
    fl_register_images();

    // 1. LEFT SIDEBAR
    Fl_Box *sidebarBox = new Fl_Box(0, 0, 200, 660);
    sidebarBox->box(FL_FLAT_BOX);
    sidebarBox->color(Theme::SIDEBAR);

    Fl_Box* logoLabel = new Fl_Box(15, 15, 170, 30, "\xe2\x99\xab Nynetify");
    logoLabel->labelcolor(Theme::ACCENT);
    logoLabel->labelsize(22);
    logoLabel->labelfont(FL_HELVETICA_BOLD);
    logoLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    sidebarHomeBtn = new ModernButton(10, 55, 180, 35, lang->home);
    sidebarHomeBtn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    sidebarHomeBtn->callback([](Fl_Widget*, void*){ show_home_view(); });

    sidebarSearchBtn = new ModernButton(10, 95, 180, 35, lang->search);
    sidebarSearchBtn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    sidebarSearchBtn->callback([](Fl_Widget*, void*){ show_search_view(); });

    sidebarLibHeading = new Fl_Box(15, 140, 170, 20, lang->your_library);
    sidebarLibHeading->labelcolor(Theme::TEXT_SECONDARY);
    sidebarLibHeading->labelsize(11);
    sidebarLibHeading->labelfont(FL_HELVETICA_BOLD);
    sidebarLibHeading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    sidebarLikedBtn = new ModernButton(10, 165, 180, 35, lang->liked_songs);
    sidebarLikedBtn->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    sidebarLikedBtn->callback([](Fl_Widget*, void*){ show_favorites_view(); });

    sidebarPlaylistList = new Fl_Hold_Browser(10, 205, 180, 345);
    sidebarPlaylistList->box(FL_FLAT_BOX);
    sidebarPlaylistList->color(Theme::SIDEBAR);
    sidebarPlaylistList->textcolor(Theme::TEXT_SECONDARY);
    sidebarPlaylistList->selection_color(Theme::HOVER);
    sidebarPlaylistList->callback(sidebar_playlist_cb);

    sidebarNewPlaylistBtn = new ModernButton(10, 555, 180, 35, lang->create_playlist);
    sidebarNewPlaylistBtn->callback([](Fl_Widget*, void*){
        static CreatePlaylistWindow* win = nullptr;
        if (!win) {
            win = new CreatePlaylistWindow();
            win->set_modal();
        } else {
            win->clear_inputs();
        }
        if (Fl::first_window()) {
            win->position(Fl::first_window()->x() + (Fl::first_window()->w() - win->w())/2,
                          Fl::first_window()->y() + (Fl::first_window()->h() - win->h())/2);
        }
        win->show();
    });

    langToggleBtn = new ModernButton(10, 595, 180, 30, lang->language_btn);
    langToggleBtn->color(Theme::HOVER);
    langToggleBtn->callback(lang_btn_cb);

    sidebarPrefsBtn = new ModernButton(10, 630, 180, 30, lang->settings);
    sidebarPrefsBtn->callback(open_prefs_window_cb);

    // 2. MAIN VIEW PANELS (using Fl_Group)
    
    // HOME VIEW PANEL
    homeGroup = new Fl_Group(200, 0, 800, 660);
    Fl_Box* homeBg = new Fl_Box(200, 0, 800, 660);
    homeBg->box(FL_FLAT_BOX);
    homeBg->color(Theme::BACKGROUND);

    homeGreetingBox = new Fl_Box(220, 20, 500, 40);
    homeGreetingBox->copy_label(get_greeting().c_str());
    homeGreetingBox->labelcolor(Theme::TEXT_PRIMARY);
    homeGreetingBox->labelsize(28);
    homeGreetingBox->labelfont(FL_HELVETICA_BOLD);
    homeGreetingBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    homeBrowseTitle = new Fl_Box(220, 70, 200, 25, lang->browse_categories);
    homeBrowseTitle->labelcolor(Theme::TEXT_PRIMARY);
    homeBrowseTitle->labelsize(16);
    homeBrowseTitle->labelfont(FL_HELVETICA_BOLD);
    homeBrowseTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    // 6 Cards on Home
    for (int i = 0; i < 6; ++i) {
        int col = i % 2;
        int row = i / 2;
        int cx = 220 + col * 380;
        int cy = 105 + row * 95;
        homeCardButtons[i] = new ModernButton(cx, cy, 365, 80, lang->card_cats[i]);
        homeCardButtons[i]->color(Theme::CARD_BG);
        homeCardButtons[i]->labelcolor(Theme::TEXT_PRIMARY);
        homeCardButtons[i]->labelsize(16);
        homeCardButtons[i]->align(FL_ALIGN_CENTER);
        homeCardButtons[i]->callback(category_card_cb, (void*)(intptr_t)i);
    }

    homeFeaturedTitle = new Fl_Box(220, 395, 300, 25, lang->now_playing_featured);
    homeFeaturedTitle->labelcolor(Theme::TEXT_PRIMARY);
    homeFeaturedTitle->labelsize(16);
    homeFeaturedTitle->labelfont(FL_HELVETICA_BOLD);
    homeFeaturedTitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    coverArtBox = new Fl_Box(220, 430, 200, 200);
    coverArtBox->box(FL_FLAT_BOX);
    coverArtBox->color(Theme::HOVER);

    homeFeatDesc = new Fl_Box(440, 430, 520, 200);
    homeFeatDesc->box(FL_NO_BOX);
    homeFeatDesc->labelcolor(Theme::TEXT_SECONDARY);
    homeFeatDesc->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    homeFeatDesc->copy_label(lang->feat_desc);

    homeGroup->end();

    // SEARCH VIEW PANEL
    searchGroup = new Fl_Group(200, 0, 800, 660);
    Fl_Box* searchBg = new Fl_Box(200, 0, 800, 660);
    searchBg->box(FL_FLAT_BOX);
    searchBg->color(Theme::BACKGROUND);

    searchBar = new Fl_Input(220, 20, 420, 35);
    searchBar->textcolor(Theme::TEXT_PRIMARY);
    searchBar->color(Theme::HOVER);
    searchBar->box(FL_RFLAT_BOX);
    searchBar->tooltip(lang->search_tooltip);

    searchFilter = new ModernChoice(655, 20, 120, 35);
    searchFilter->add(lang->everything);
    searchFilter->add(lang->songs_filter);
    searchFilter->add(lang->playlists_filter);
    searchFilter->add(lang->channels_filter);
    searchFilter->value(0);
    searchFilter->selection_color(Theme::ACCENT);

    searchGroup->end();

    // PLAYLIST VIEW PANEL
    playlistGroup = new Fl_Group(200, 0, 800, 660);
    Fl_Box* playlistBg = new Fl_Box(200, 0, 800, 660);
    playlistBg->box(FL_FLAT_BOX);
    playlistBg->color(Theme::BACKGROUND);

    Fl_Box* playlistHeaderBg = new Fl_Box(200, 0, 800, 200);
    playlistHeaderBg->box(FL_FLAT_BOX);
    playlistHeaderBg->color(fl_rgb_color(22, 45, 30)); // Dark Spotify-Green tone

    playlistCoverBox = new Fl_Box(220, 30, 140, 140);
    playlistCoverBox->box(FL_FLAT_BOX);
    playlistCoverBox->color(Theme::HOVER);

    playlistNameBox = new Fl_Box(380, 40, 600, 40, lang->playlist_name_placeholder);
    playlistNameBox->labelcolor(Theme::TEXT_PRIMARY);
    playlistNameBox->labelsize(28);
    playlistNameBox->labelfont(FL_HELVETICA_BOLD);
    playlistNameBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    playlistDescBox = new Fl_Box(380, 85, 600, 30, lang->playlist_desc_placeholder);
    playlistDescBox->labelcolor(Theme::TEXT_SECONDARY);
    playlistDescBox->labelsize(13);
    playlistDescBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    CircularButton* playlistPlayBtn = new CircularButton(380, 130, 50, 50, "@>");
    playlistPlayBtn->color(Theme::ACCENT);
    playlistPlayBtn->labelcolor(FL_BLACK);
    playlistPlayBtn->callback(playlist_play_click_cb);

    playlistDeleteBtn = new ModernButton(450, 138, 110, 34, lang->delete_playlist);
    playlistDeleteBtn->color(Theme::BACKGROUND);
    playlistDeleteBtn->labelcolor(fl_rgb_color(220, 53, 69)); // red tone
    playlistDeleteBtn->callback(playlist_delete_click_cb);

    playlistGroup->end();

    // 3. RESULTS BROWSER (Shared, repositioned dynamically)
    resultsBrowser = new ResultsBrowser(220, 70, 760, 570);
    resultsBrowser->color(Theme::BACKGROUND);
    resultsBrowser->box(FL_FLAT_BOX);
    resultsBrowser->textcolor(Theme::TEXT_PRIMARY);
    resultsBrowser->callback(play_selected_cb);
    resultsBrowser->type(FL_HOLD_BROWSER);
    resultsBrowser->selection_color(Theme::HOVER);

    static int browser_widths[] = {40, 480, 200, 0};
    resultsBrowser->column_widths(browser_widths);
    resultsBrowser->column_char('\t');

    searchBar->callback(search_cb, resultsBrowser);
    searchBar->when(FL_WHEN_ENTER_KEY);

    // 4. BOTTOM PLAYER BAR (y=660 to y=730)
    Fl_Box* playerBarBox = new Fl_Box(0, 660, 1000, 70);
    playerBarBox->box(FL_FLAT_BOX);
    playerBarBox->color(Theme::SIDEBAR);

    miniCoverArtBox = new Fl_Box(15, 665, 60, 60);
    miniCoverArtBox->box(FL_FLAT_BOX);
    miniCoverArtBox->color(Theme::HOVER);

    nowPlayingBox = new Fl_Box(85, 672, 180, 20, lang->not_playing);
    nowPlayingBox->labelcolor(Theme::TEXT_PRIMARY);
    nowPlayingBox->labelfont(FL_HELVETICA_BOLD);
    nowPlayingBox->labelsize(13);
    nowPlayingBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    nowPlayingArtistBox = new Fl_Button(85, 695, 180, 18, "");
    nowPlayingArtistBox->box(FL_NO_BOX);
    nowPlayingArtistBox->labelcolor(Theme::TEXT_SECONDARY);
    nowPlayingArtistBox->labelsize(11);
    nowPlayingArtistBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    nowPlayingArtistBox->callback(artist_name_cb);
    nowPlayingArtistBox->tooltip(lang->view_channel);

    heartBtn = new HeartButton(275, 680, 30, 30);
    heartBtn->callback(heart_btn_cb);

    // Controls Center
    ModernButton* shuffleBtn = new ModernButton(380, 670, 30, 30, "S");
    shuffleBtn->color(Theme::SIDEBAR);
    shuffleBtn->labelsize(10);
    shuffleBtn->labelcolor(Theme::TEXT_SECONDARY);
    shuffleBtn->callback(shuffle_cb);

    ModernButton* prevBtn = new ModernButton(420, 670, 30, 30, "<");
    prevBtn->color(Theme::SIDEBAR);
    prevBtn->labelcolor(Theme::TEXT_PRIMARY);
    prevBtn->callback([](Fl_Widget*, void*){ play_prev(); });

    playBtn = new CircularButton(460, 665, 40, 40, "PLAY");
    playBtn->callback(play_btn_cb, resultsBrowser);

    ModernButton* nextBtn = new ModernButton(510, 670, 30, 30, ">");
    nextBtn->color(Theme::SIDEBAR);
    nextBtn->labelcolor(Theme::TEXT_PRIMARY);
    nextBtn->callback([](Fl_Widget*, void*){ play_next(); });

    ModernButton* repeatBtn = new ModernButton(550, 670, 30, 30, "R");
    repeatBtn->color(Theme::SIDEBAR);
    repeatBtn->labelsize(10);
    repeatBtn->labelcolor(Theme::TEXT_SECONDARY);
    repeatBtn->callback(repeat_cb);

    // Progress
    currentTimeBox = new Fl_Box(330, 708, 40, 15, "00:00");
    currentTimeBox->labelcolor(Theme::TEXT_SECONDARY);
    currentTimeBox->labelsize(10);

    progressBar = new ProgressSlider(380, 712, 240, 8);
    progressBar->callback(seek_cb);

    totalTimeBox = new Fl_Box(630, 708, 40, 15, "00:00");
    totalTimeBox->labelcolor(Theme::TEXT_SECONDARY);
    totalTimeBox->labelsize(10);

    // Right side
    Fl_Box* volIcon = new Fl_Box(760, 680, 30, 30, "VOL");
    volIcon->labelcolor(Theme::TEXT_SECONDARY);
    volIcon->labelsize(10);

    volumeSlider = new ModernSlider(790, 685, 100, 20);
    volumeSlider->type(FL_HOR_SLIDER);
    volumeSlider->bounds(0, 130);
    volumeSlider->value(100);
    volumeSlider->callback(volume_cb);

    ModernButton* eqBtn = new ModernButton(900, 675, 40, 40, "EQ");
    eqBtn->color(Theme::SIDEBAR);
    eqBtn->labelcolor(Theme::TEXT_SECONDARY);
    eqBtn->callback(open_eq_window_cb);

    ModernButton* downloadBtn = new ModernButton(945, 675, 40, 40, "\xe2\x86\x93");
    downloadBtn->color(Theme::SIDEBAR);
    downloadBtn->labelcolor(Theme::TEXT_SECONDARY);
    downloadBtn->callback(download_cb);
    downloadBtn->tooltip(lang->download_song);

    // 5. STATUS BAR (y=730 to y=750)
    statusBar = new Fl_Box(0, 730, 1000, 20);
    statusBar->box(FL_FLAT_BOX);
    statusBar->color(Theme::SIDEBAR);
    statusBar->labelcolor(Theme::TEXT_SECONDARY);
    statusBar->labelsize(11);
    statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    if (!settings.showStatusBar) statusBar->hide();

    // Show initial Home view
    show_home_view();
    load_sidebar_playlists();

    // Init Timeout loops
    Fl::add_timeout(0.2, update_ui_cb);
    Fl::add_timeout(0.5, update_status_bar_cb);

    load_settings();

    apply_language();

    window->end();
    window->show(argc, argv);

    return Fl::run();
}
