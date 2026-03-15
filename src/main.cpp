#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_ask.H>
#include <filesystem>
#include <fstream>
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Vista or higher
#endif
#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <time.h>
#include <iomanip>
#include "Theme.h"
#include "ModernButton.h"
#include "YoutubeService.h"
#include "PlayerEngine.h"
#include "ProgressSlider.h"
#include "PlaylistManager.h"
#include <cstdint>
#include <sstream> // Added for std::ostringstream

PlayerEngine* player = nullptr;
std::vector<SearchResult> last_results;
std::string user_region = "US"; // Default

ProgressSlider* progressBar = nullptr;
Fl_Slider* volumeSlider = nullptr;
Fl_Box* currentTimeBox = nullptr;
Fl_Box* totalTimeBox = nullptr;
Fl_Input* searchBar = nullptr;

// Forward declarations
void search_cb(Fl_Widget* w, void* data);
void category_cb(Fl_Widget* w, void* data);
void refresh_current_view();

std::string current_category = "Top Hits";
std::string current_playlist = ""; // To track if we're viewing a playlist

class CreatePlaylistWindow : public Fl_Double_Window {
    Fl_Input *nameIn, *commentIn;
public:
    CreatePlaylistWindow() : Fl_Double_Window(300, 180, "New Playlist") {
        color(Theme::SIDEBAR);
        nameIn = new Fl_Input(100, 20, 180, 25, "Name:");
        nameIn->textcolor(Theme::TEXT_PRIMARY);
        nameIn->color(Theme::HOVER);
        nameIn->labelcolor(Theme::TEXT_PRIMARY);

        commentIn = new Fl_Input(100, 55, 180, 25, "Comment:");
        commentIn->textcolor(Theme::TEXT_PRIMARY);
        commentIn->color(Theme::HOVER);
        commentIn->labelcolor(Theme::TEXT_PRIMARY);

        ModernButton* btn = new ModernButton(100, 100, 100, 35, "CREATE");
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
        }
    }
};

class PlaylistSelectionWindow : public Fl_Double_Window {
public:
    Fl_Hold_Browser* list;
    std::string vid;
    PlaylistSelectionWindow(const std::string& video_id) : Fl_Double_Window(300, 350, "Add to Playlist"), vid(video_id) {
        color(Theme::SIDEBAR);
        list = new Fl_Hold_Browser(10, 10, 280, 280);
        list->color(Theme::HOVER);
        list->textcolor(Theme::TEXT_PRIMARY);
        list->selection_color(Theme::ACCENT);
        
        list->add("FAVORITES");
        auto playlists = PlaylistManager::get_all_playlists();
        for (const auto& p : playlists) {
            list->add(p.c_str());
        }

        ModernButton* btn = new ModernButton(100, 300, 100, 35, "ADD");
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
        }
    }
};

class ResultsBrowser : public Fl_Browser {
public:
    ResultsBrowser(int x, int y, int w, int h, const char* l = 0) : Fl_Browser(x, y, w, h, l) {}

    int handle(int event) override {
        if (event == FL_PUSH) {
            int wx = Fl::event_x();
            int wy = Fl::event_y();
            int line = value(); // value() is updated on PUSH if within list box

            if (Fl::event_button() == 1) { // Left Click
                if (wx >= x() && wx < x() + 30) {
                    if (line > 0 && line <= (int)last_results.size()) {
                        std::string video_id = last_results[line - 1].video_id;
                        if (video_id.find(".txt") == std::string::npos) {
                            PlaylistSelectionWindow* win = new PlaylistSelectionWindow(video_id);
                            win->set_modal();
                            win->show();
                            // Refresh star state might be needed after window closes, 
                            // but for now we let the user re-search or re-load category.
                            return 1;
                        }
                    }
                }
            } else if (Fl::event_button() == 3) { // Right Click
                if (line > 0 && line <= (int)last_results.size()) {
                    Fl_Menu_Item rclick_menu[] = {
                        { "Delete", 0, delete_entry_cb, (void*)(intptr_t)line },
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
            } else if (current_category == "MY PLAYLISTS") {
                // If we are looking at the list of playlists
                PlaylistManager::delete_playlist(video_id); // In this view, video_id is filename
            }
            // Trigger refresh
            refresh_current_view();
        }
    }
};

ResultsBrowser* resultsBrowser = nullptr;
Fl_Browser* categoryBrowser = nullptr;
Fl_Box* nowPlayingBox = nullptr;
Fl_Choice* searchFilter = nullptr;

struct AppSettings {
    bool loadThumbnails = true;
    bool showStatusBar = true;
    int bufferSizeMB = 2;
} settings;

Fl_Box* coverArtBox = nullptr;
Fl_JPEG_Image* currentImage = nullptr;

// Status Bar Elements
Fl_Box* statusBar = nullptr;
HANDLE self;

void detect_region() {
    std::cout << "[UI] Detecting region..." << std::endl;
    std::string temp_file = "region_temp.txt";
    // Use ipapi.co for simple country code retrieval
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

std::vector<std::string> categories = {
    "Classic Rock",
    "Hip Hop",
    "Flamenco",
    "Electro Swing",
    "Corrido Tumbado",
    "Reggaetón",
    "Pop",
    "Synthwave",
    "Electro House",
    "Deep House",
    "Progressive House",
    "Drum & Bass",
    "Breakbeat",
    "Techno",
    "Phonk",
    "Rock",
    "Lo-fi Beats",
    "MIX",
    "ASMR",
    "Podcast",
    "---",
    "MY FAVORITES",
    "MY PLAYLISTS"
};

std::string format_time(double seconds) {
    if (seconds < 0) seconds = 0;
    int m = (int)(seconds / 60);
    int s = (int)seconds % 60;
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << m << ":"
        << std::setw(2) << std::setfill('0') << s;
    return oss.str();
}

void update_ui_cb(void* data) {
    if (player && progressBar) {
        player->update(); // Process MPV events
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
    Fl::repeat_timeout(0.2, update_ui_cb); // Increased frequency for smoother progress bar
}

void update_cover_art(const std::string& video_id) {
    if (!settings.loadThumbnails || !coverArtBox) return;

    std::string url = YoutubeService::get_thumbnail_url(video_id);
    std::string temp_file = "cover_temp.jpg";
    
    // Simple download using curl (standard on Win10+)
    std::string cmd = "curl -s -L -o " + temp_file + " " + url;
    system(cmd.c_str());

    if (std::filesystem::exists(temp_file)) {
        if (currentImage) delete currentImage;
        currentImage = new Fl_JPEG_Image(temp_file.c_str());
        if (currentImage && currentImage->w() > 0) {
            // Scale image to fit box while maintaining aspect ratio
            int bw = coverArtBox->w();
            int bh = coverArtBox->h();
            Fl_Image* scaled = currentImage->copy(bw, bh);
            coverArtBox->image(scaled);
            coverArtBox->redraw();
        }
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

void play_selected_cb(Fl_Widget* w, void* data) {
    Fl_Browser* browser = (Fl_Browser*)w;
    int line = browser->value();
    if (line > 0 && line <= (int)last_results.size()) {
        std::string video_id = last_results[line - 1].video_id;
        std::string title = last_results[line - 1].title;

        // Check if it's a playlist reference (ends in .txt and it's from PLAYLIST categoriy)
        if (video_id.find(".txt") != std::string::npos) {
            browser->clear();
            browser->add("Loading playlist metadata...");
            browser->redraw();
            Fl::check();

            current_playlist = video_id; // Store current filename
            auto ids = PlaylistManager::get_playlist_songs(video_id);
            last_results = YoutubeService::get_metadata(ids);
            browser->clear();
            for (const auto& res : last_results) {
                bool is_fav = PlaylistManager::is_favorite(res.video_id);
                std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
                browser->add((star + "\t" + res.title + "\t" + res.author).c_str());
            }
            return;
        }
        current_playlist = "";

        std::cout << "[UI] Selected: " << title << " (" << video_id << ")" << std::endl;
        
        if (nowPlayingBox) {
            std::string np = title + " - " + last_results[line - 1].author;
            nowPlayingBox->copy_label(np.c_str());
            nowPlayingBox->redraw();
        }

        std::string stream_url = YoutubeService::get_audio_url(video_id);
        if (!stream_url.empty()) {
            player->play(stream_url);
            update_cover_art(video_id);
        } else {
            std::cerr << "[ERROR] Could not get stream URL" << std::endl;
        }
    }
}

void play_btn_cb(Fl_Widget* w, void* data) {
    Fl_Browser* browser = (Fl_Browser*)data;
    play_selected_cb(browser, nullptr);
}

void add_to_favs_cb(Fl_Widget* w, void* data) {
    if (!resultsBrowser) return;
    int line = resultsBrowser->value();
    if (line > 0 && line <= (int)last_results.size()) {
        std::string video_id = last_results[line - 1].video_id;
        PlaylistManager::add_to_favorites(video_id);
        std::cout << "[UI] Added to Favorites: " << video_id << std::endl;
        fl_message("Added to Favorites!");
    } else {
        fl_message("Please select a song first.");
    }
}

void refresh_current_view() {
    if (current_category == "MY FAVORITES") {
        category_cb(categoryBrowser, nullptr); // Re-trigger category selection
    } else if (!current_playlist.empty()) {
        // Special case for playlist content
        resultsBrowser->clear();
        resultsBrowser->add("Refreshing...");
        resultsBrowser->redraw();
        Fl::check();

        auto ids = PlaylistManager::get_playlist_songs(current_playlist);
        last_results = YoutubeService::get_metadata(ids);
        resultsBrowser->clear();
        for (const auto& res : last_results) {
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            resultsBrowser->add((star + "\t" + res.title + "\t" + res.author).c_str());
        }
    } else if (current_category == "MY PLAYLISTS") {
        category_cb(categoryBrowser, nullptr);
    } else {
        search_cb(searchBar, resultsBrowser);
    }
}

void search_cb(Fl_Widget* w, void* data) {
    current_category = "SEARCH";
    current_playlist = "";
    Fl_Input* input = (Fl_Input*)w;
    Fl_Browser* browser = (Fl_Browser*)data;
    browser->clear();
    
    std::string query = input->value();
    if (query.empty()) return;

    std::cout << "[UI] Searching for: " << query << "..." << std::endl;
    
    try {
        int filter = 0;
        if (searchFilter) filter = searchFilter->value();

        last_results = YoutubeService::search(query, user_region, filter);
        std::cout << "[UI] Found " << last_results.size() << " results." << std::endl;
        
        for (const auto& res : last_results) {
            bool is_fav = PlaylistManager::is_favorite(res.video_id);
            std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
            std::string line_item = star + "\t" + res.title + "\t" + res.author;
            browser->add(line_item.c_str());
        }
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Search failed: " << e.what() << std::endl;
        browser->add("Error: Search failed. Check console for details.");
    }
}

void category_cb(Fl_Widget* w, void* data) {
    Fl_Browser* cb = (Fl_Browser*)w;
    int line = cb->value();
    if (line > 0 && line <= (int)categories.size()) {
        std::string cat = categories[line - 1];
        if (cat == "---") return;
        current_category = cat;
        current_playlist = "";
        
        if (cat == "MY FAVORITES") {
            resultsBrowser->clear();
            resultsBrowser->add("Loading metadata... Please wait.");
            resultsBrowser->redraw();
            Fl::check(); // Process events to show the loading message

            last_results.clear();
            auto items = PlaylistManager::get_favorites();
            std::vector<std::string> ids;
            for (const auto& item : items) {
                if (!item.is_playlist) ids.push_back(item.value);
            }

            if (!ids.empty()) {
                last_results = YoutubeService::get_metadata(ids);
                resultsBrowser->clear();
                for (const auto& res : last_results) {
                    bool is_fav = PlaylistManager::is_favorite(res.video_id);
                    std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
                    std::string line_item = star + "\t" + res.title + "\t" + res.author;
                    resultsBrowser->add(line_item.c_str());
                }
            } else {
                resultsBrowser->clear();
                resultsBrowser->add("No favorites found.");
            }
            return;
        }

        if (cat == "MY PLAYLISTS") {
            resultsBrowser->clear();
            last_results.clear();
            auto lists = PlaylistManager::get_all_playlists();
            if (lists.empty()) {
                resultsBrowser->add("No playlists found.");
            } else {
                for (const auto& list : lists) {
                    resultsBrowser->add(("@C255\xe2\x98\x86\t" + list + "\t(Playlist)").c_str());
                    SearchResult r; r.title = list; r.video_id = list; // Flag it's a playlist
                    last_results.push_back(r);
                }
            }
            return;
        }

        if (searchBar && resultsBrowser) {
            searchBar->value(cat.c_str());
            if (searchFilter) searchFilter->value(0); // Reset to default for category clicks
            search_cb(searchBar, resultsBrowser);
        }
    }
}

void pause_resume_cb(Fl_Widget* w, void* data) {
    static bool paused = false;
    if (paused) {
        player->resume();
        w->label("PAUSE");
    } else {
        player->pause();
        w->label("RESUME");
    }
    paused = !paused;
    w->redraw();
}

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
        std::cout << "[UI] EQ " << (check->value() ? "Enabled" : "Disabled") << std::endl;
    }
}

void open_eq_window_cb(Fl_Widget* w, void* data) {
    Fl_Double_Window* eqWin = new Fl_Double_Window(500, 320, "Equalizer (10 Bands)");
    eqWin->color(Theme::SIDEBAR);
    
    Fl_Check_Button* eqToggle = new Fl_Check_Button(20, 10, 120, 25, " ENABLE EQ");
    eqToggle->labelcolor(Theme::TEXT_PRIMARY);
    eqToggle->callback(eq_toggle_cb);
    if (player) eqToggle->value(player->is_eq_enabled() ? 1 : 0);

    // dB Markers
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
        Fl_Slider* s = new Fl_Slider(20 + i * 45, 50, 25, 200, labels[i]);
        s->type(FL_VERT_SLIDER);
        s->bounds(12, -12); // +12dB to -12dB
        s->value(0);
        s->color(Theme::HOVER);
        s->selection_color(Theme::ACCENT);
        s->callback(eq_slider_cb, (void*)(intptr_t)i);
        s->labelsize(10);
        s->labelcolor(Theme::TEXT_PRIMARY);
    }
    
    eqWin->end();
    eqWin->set_non_modal();
    eqWin->show();
}

void prefs_toggle_cb(Fl_Widget* w, void* data) {
    Fl_Check_Button* btn = (Fl_Check_Button*)w;
    settings.loadThumbnails = btn->value();
}

void status_toggle_cb(Fl_Widget* w, void* data) {
    Fl_Check_Button* btn = (Fl_Check_Button*)w;
    settings.showStatusBar = btn->value();
    if (statusBar) {
        if (settings.showStatusBar) statusBar->show();
        else statusBar->hide();
    }
}


void buffer_cb(Fl_Widget* w, void* data) {
    Fl_Slider* slider = (Fl_Slider*)w;
    Fl_Box* label = (Fl_Box*)data;
    settings.bufferSizeMB = (int)slider->value();
    
    char buf[64];
    snprintf(buf, sizeof(buf), "Max Buffer Size: %d MB", settings.bufferSizeMB);
    label->copy_label(buf);
    label->redraw();

    if (player) {
        player->set_buffer_size(settings.bufferSizeMB);
    }
}

void open_prefs_window_cb(Fl_Widget* w, void* data) {
    Fl_Double_Window* prefWin = new Fl_Double_Window(300, 260, "Settings");
    prefWin->color(Theme::SIDEBAR);
    
    Fl_Check_Button* thumbToggle = new Fl_Check_Button(20, 20, 260, 30, " Load Thumbnails");
    thumbToggle->labelcolor(Theme::TEXT_PRIMARY);
    thumbToggle->value(settings.loadThumbnails ? 1 : 0);
    thumbToggle->callback(prefs_toggle_cb);

    Fl_Check_Button* statusToggle = new Fl_Check_Button(20, 60, 260, 30, " Show Status Bar");
    statusToggle->labelcolor(Theme::TEXT_PRIMARY);
    statusToggle->value(settings.showStatusBar ? 1 : 0);
    statusToggle->callback(status_toggle_cb);

    Fl_Box* bufLabel = new Fl_Box(20, 110, 260, 20);
    bufLabel->labelcolor(Theme::TEXT_SECONDARY);
    bufLabel->labelsize(12);
    bufLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    char initBuf[64];
    snprintf(initBuf, sizeof(initBuf), "Max Buffer Size: %d MB", settings.bufferSizeMB);
    bufLabel->copy_label(initBuf);

    Fl_Slider* bufSlider = new Fl_Slider(20, 135, 260, 20);
    bufSlider->type(FL_HOR_SLIDER);
    bufSlider->bounds(1, 100);
    bufSlider->step(1);
    bufSlider->value(settings.bufferSizeMB);
    bufSlider->color(Theme::HOVER);
    bufSlider->selection_color(Theme::ACCENT);
    bufSlider->callback(buffer_cb, (void*)bufLabel);
    
    prefWin->end();
    prefWin->set_non_modal();
    prefWin->show();
}

void update_status_bar_cb(void* data) {
    if (!statusBar) return;

    // --- RAM ---
    PROCESS_MEMORY_COUNTERS_EX pmc;
    GetProcessMemoryInfo(self, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
    double ram_mb = pmc.WorkingSetSize / (1024.0 * 1024.0);

    // --- Network (D/U) ---
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

    // --- Time & Buffer ---
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
             " YTfy Beta | Region: %s | RAM: %.1f MB | BUF: %.1f MB | Net D: %.1f KB/s U: %.1f KB/s | %s",
             user_region.c_str(), ram_mb, buf_usage, down_kb, up_kb, time_str);
    
    statusBar->copy_label(final_status);
    statusBar->redraw();

    Fl::repeat_timeout(1.0, update_status_bar_cb);
}

int main(int argc, char **argv) {
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
    Fl_Double_Window *window = new Fl_Double_Window(1000, 750, "YTfy");
    window->color(Theme::BACKGROUND);
    fl_register_images();

    // Sidebar
    Fl_Box *sidebarBox = new Fl_Box(0, 0, 200, 750);
    sidebarBox->box(FL_FLAT_BOX);
    sidebarBox->color(Theme::SIDEBAR);

    // Cover Art Box
    coverArtBox = new Fl_Box(10, 20, 180, 180);
    coverArtBox->box(FL_NO_BOX);

    Fl_Box *catLabel = new Fl_Box(10, 210, 180, 30, "CATEGORIES");
    catLabel->labelcolor(Theme::TEXT_PRIMARY);
    catLabel->labelsize(16);
    catLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    categoryBrowser = new Fl_Browser(10, 250, 180, 400);
    categoryBrowser->type(FL_HOLD_BROWSER);
    categoryBrowser->box(FL_FLAT_BOX);
    categoryBrowser->color(Theme::SIDEBAR);
    categoryBrowser->textcolor(Theme::TEXT_SECONDARY);
    categoryBrowser->selection_color(Theme::HOVER);
    categoryBrowser->callback(category_cb);
    for (const auto& cat : categories) {
        categoryBrowser->add(cat.c_str());
    }

    ModernButton *prefsBtn = new ModernButton(10, 660, 180, 35, "PREFERENCES");
    prefsBtn->callback(open_prefs_window_cb);

    ModernButton *newPlaylistBtn = new ModernButton(10, 620, 180, 35, "NEW PLAYLIST");
    newPlaylistBtn->callback([](Fl_Widget*, void*){
        CreatePlaylistWindow* win = new CreatePlaylistWindow();
        win->set_modal();
        win->show();
    });

    // Search Bar
    searchBar = new Fl_Input(220, 20, 450, 35, "Search:");
    searchBar->align(FL_ALIGN_LEFT);
    searchBar->textcolor(Theme::TEXT_PRIMARY);
    searchBar->color(Theme::HOVER);

    searchFilter = new Fl_Choice(680, 20, 120, 35);
    searchFilter->add("Default");
    searchFilter->add("Only Songs");
    searchFilter->add("Playlist");
    searchFilter->value(0);
    searchFilter->color(Theme::HOVER);
    searchFilter->textcolor(Theme::TEXT_PRIMARY);
    searchFilter->selection_color(Theme::ACCENT);
    
    // Results List
    resultsBrowser = new ResultsBrowser(220, 70, 750, 500);
    resultsBrowser->color(Theme::SIDEBAR);
    resultsBrowser->textcolor(Theme::TEXT_PRIMARY);
    resultsBrowser->callback(play_selected_cb);
    resultsBrowser->type(FL_HOLD_BROWSER);

    // Now Playing Indicator
    nowPlayingBox = new Fl_Box(220, 575, 750, 30, "Ready to Play");
    nowPlayingBox->labelcolor(Theme::TEXT_PRIMARY);
    nowPlayingBox->labelsize(14);
    nowPlayingBox->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    
    // Column configuration
    static int browser_widths[] = {30, 500, 200, 0};
    resultsBrowser->column_widths(browser_widths);
    resultsBrowser->column_char('\t');

    searchBar->callback(search_cb, resultsBrowser);
    searchBar->when(FL_WHEN_ENTER_KEY);

    // Time Indicators
    currentTimeBox = new Fl_Box(220, 610, 50, 20, "00:00");
    currentTimeBox->labelcolor(Theme::TEXT_SECONDARY);
    currentTimeBox->labelsize(12);

    totalTimeBox = new Fl_Box(920, 610, 50, 20, "00:00");
    totalTimeBox->labelcolor(Theme::TEXT_SECONDARY);
    totalTimeBox->labelsize(12);

    // Progress Bar
    progressBar = new ProgressSlider(220, 635, 750, 15);
    progressBar->callback(seek_cb);

    // Player controls
    ModernButton *playBtn = new ModernButton(420, 680, 80, 40, "PLAY");
    playBtn->color(Theme::ACCENT);
    playBtn->labelcolor(FL_WHITE);
    playBtn->callback(play_btn_cb, resultsBrowser);

    ModernButton *pauseBtn = new ModernButton(510, 680, 80, 40, "PAUSE");
    pauseBtn->callback(pause_resume_cb);

    // Volume Control
    Fl_Box* volLabel = new Fl_Box(800, 680, 40, 40, "VOL");
    volLabel->labelcolor(Theme::TEXT_SECONDARY);
    volLabel->labelsize(12);

    volumeSlider = new Fl_Slider(840, 690, 120, 20);
    volumeSlider->type(FL_HOR_SLIDER);
    volumeSlider->color(Theme::HOVER);
    volumeSlider->selection_color(Theme::ACCENT);
    volumeSlider->bounds(0, 130);
    volumeSlider->value(100);
    volumeSlider->callback(volume_cb);

    ModernButton *eqBtn = new ModernButton(730, 680, 50, 40, "EQ");
    eqBtn->callback(open_eq_window_cb);

    ModernButton *favBtn = new ModernButton(640, 680, 80, 40, "FAV+");
    favBtn->callback(add_to_favs_cb);

    Fl::add_timeout(0.2, update_ui_cb);

    // Status Bar Background & Label
    statusBar = new Fl_Box(0, 730, 1000, 20);
    statusBar->box(FL_FLAT_BOX);
    statusBar->color(Theme::SIDEBAR);
    statusBar->labelcolor(Theme::TEXT_SECONDARY);
    statusBar->labelsize(11);
    statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    if (!settings.showStatusBar) statusBar->hide();

    window->end();
    window->show(argc, argv);

    // Initial Search (Pre-load)
    if (searchBar && resultsBrowser) {
        searchBar->value("Top Hits");
        search_cb(searchBar, resultsBrowser);
    }

    Fl::add_timeout(0.5, update_status_bar_cb);

    return Fl::run();
}
