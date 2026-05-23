/*
 * main.cpp — Application entry point & full UI layout
 *
 * This file builds every widget in the Nynetify window: sidebar,
 * home/search/playlist panels, player bar, progress bar, status
 * bar.  All callback logic lives in separate modules (AppCallbacks,
 * PlayerController, ViewManager, …) included via Globals.h.
 *
 * The global variable definitions are here (the one translation
 * unit that provides storage for the `extern` declarations made
 * in Globals.h).
 */

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>

#include <windows.h>
#include <psapi.h>
#include <iphlpapi.h>

#include "Globals.h"
#include "AppSettings.h"
#include "PlayerEngine.h"
#include "Theme.h"
#include "ModernButton.h"
#include "ProgressSlider.h"
#include "ModernSlider.h"
#include "ModernChoice.h"
#include "UIWidgets.h"
#include "AppCallbacks.h"
#include "PlayerController.h"
#include "ViewManager.h"

/* ================================================================
 * Global variable definitions (storage for extern declarations)
 * ================================================================ */

PlayerEngine* player = nullptr;
std::vector<SearchResult> last_results;
int total_loaded_results = 0;
std::string last_search_query;
int last_search_filter = 0;
std::string user_region = "US";

std::vector<SearchResult> play_queue;
int current_queue_index = -1;
bool is_shuffle = false;
bool is_repeat = false;

std::string current_category = "Top Hits";
std::string current_playlist;

ProgressSlider* progressBar = nullptr;
ModernSlider* volumeSlider = nullptr;
Fl_Box* currentTimeBox = nullptr;
Fl_Box* totalTimeBox = nullptr;
Fl_Input* searchBar = nullptr;
ModernChoice* searchFilter = nullptr;
Fl_Box* statusBar = nullptr;
Fl_Double_Window* eqWin = nullptr;
Fl_Double_Window* prefWin = nullptr;
void* self = nullptr;   /* HANDLE from windows.h */

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

CircularButton* playBtn = nullptr;
HeartButton* heartBtn = nullptr;
ResultsBrowser* resultsBrowser = nullptr;

/* ================================================================
 * main()
 * ================================================================ */
int main(int argc, char **argv) {
    srand((unsigned int)time(nullptr));

    /* Initialise player & region detection (async) */
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
    auto* window = new Fl_Double_Window(1000, 750, lang->window_title);
    window->color(Theme::BACKGROUND);
    fl_register_images();

    /* ── 1. LEFT SIDEBAR (0–200) ──────────────────── */
    auto* sidebarBox = new Fl_Box(0, 0, 200, 660);
    sidebarBox->box(FL_FLAT_BOX);
    sidebarBox->color(Theme::SIDEBAR);

    auto* logoLabel = new Fl_Box(15, 15, 170, 30, "\xe2\x99\xab Nynetify");
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
        if (Fl::first_window())
            win->position(Fl::first_window()->x() + (Fl::first_window()->w() - win->w()) / 2,
                          Fl::first_window()->y() + (Fl::first_window()->h() - win->h()) / 2);
        win->show();
    });

    langToggleBtn = new ModernButton(10, 595, 180, 30, lang->language_btn);
    langToggleBtn->color(Theme::HOVER);
    langToggleBtn->callback(lang_btn_cb);

    sidebarPrefsBtn = new ModernButton(10, 630, 180, 30, lang->settings);
    sidebarPrefsBtn->callback(open_prefs_window_cb);

    /* ── 2. HOME VIEW PANEL ───────────────────────── */
    homeGroup = new Fl_Group(200, 0, 800, 660);
    auto* homeBg = new Fl_Box(200, 0, 800, 660);
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

    for (int i = 0; i < 6; ++i) {
        int col = i % 2, row = i / 2;
        homeCardButtons[i] = new ModernButton(220 + col * 380, 105 + row * 95, 365, 80, lang->card_cats[i]);
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

    /* ── 3. SEARCH VIEW PANEL ─────────────────────── */
    searchGroup = new Fl_Group(200, 0, 800, 660);
    auto* searchBg = new Fl_Box(200, 0, 800, 660);
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

    /* ── 4. PLAYLIST VIEW PANEL ───────────────────── */
    playlistGroup = new Fl_Group(200, 0, 800, 660);
    auto* playlistBg = new Fl_Box(200, 0, 800, 660);
    playlistBg->box(FL_FLAT_BOX);
    playlistBg->color(Theme::BACKGROUND);

    auto* playlistHeaderBg = new Fl_Box(200, 0, 800, 200);
    playlistHeaderBg->box(FL_FLAT_BOX);
    playlistHeaderBg->color(fl_rgb_color(22, 45, 30));

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

    auto* playlistPlayBtn = new CircularButton(380, 130, 50, 50, "@>");
    playlistPlayBtn->color(Theme::ACCENT);
    playlistPlayBtn->labelcolor(FL_BLACK);
    playlistPlayBtn->callback(playlist_play_click_cb);

    playlistDeleteBtn = new ModernButton(450, 138, 110, 34, lang->delete_playlist);
    playlistDeleteBtn->color(Theme::BACKGROUND);
    playlistDeleteBtn->labelcolor(fl_rgb_color(220, 53, 69));
    playlistDeleteBtn->callback(playlist_delete_click_cb);

    playlistGroup->end();

    /* ── 5. RESULTS BROWSER (shared, repositioned) ── */
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

    /* ── 6. BOTTOM PLAYER BAR (660–730) ───────────── */
    auto* playerBarBox = new Fl_Box(0, 660, 1000, 70);
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

    auto* shuffleBtn = new ModernButton(380, 670, 30, 30, "S");
    shuffleBtn->color(Theme::SIDEBAR);
    shuffleBtn->labelsize(10);
    shuffleBtn->labelcolor(Theme::TEXT_SECONDARY);
    shuffleBtn->callback(shuffle_cb);

    auto* prevBtn = new ModernButton(420, 670, 30, 30, "<");
    prevBtn->color(Theme::SIDEBAR);
    prevBtn->labelcolor(Theme::TEXT_PRIMARY);
    prevBtn->callback([](Fl_Widget*, void*){ play_prev(); });

    playBtn = new CircularButton(460, 665, 40, 40, "PLAY");
    playBtn->callback(play_btn_cb, resultsBrowser);

    auto* nextBtn = new ModernButton(510, 670, 30, 30, ">");
    nextBtn->color(Theme::SIDEBAR);
    nextBtn->labelcolor(Theme::TEXT_PRIMARY);
    nextBtn->callback([](Fl_Widget*, void*){ play_next(); });

    auto* repeatBtn = new ModernButton(550, 670, 30, 30, "R");
    repeatBtn->color(Theme::SIDEBAR);
    repeatBtn->labelsize(10);
    repeatBtn->labelcolor(Theme::TEXT_SECONDARY);
    repeatBtn->callback(repeat_cb);

    currentTimeBox = new Fl_Box(330, 708, 40, 15, "00:00");
    currentTimeBox->labelcolor(Theme::TEXT_SECONDARY);
    currentTimeBox->labelsize(10);

    progressBar = new ProgressSlider(380, 712, 240, 8);
    progressBar->callback(seek_cb);

    totalTimeBox = new Fl_Box(630, 708, 40, 15, "00:00");
    totalTimeBox->labelcolor(Theme::TEXT_SECONDARY);
    totalTimeBox->labelsize(10);

    auto* volIcon = new Fl_Box(760, 680, 30, 30, "VOL");
    volIcon->labelcolor(Theme::TEXT_SECONDARY);
    volIcon->labelsize(10);

    volumeSlider = new ModernSlider(790, 685, 100, 20);
    volumeSlider->type(FL_HOR_SLIDER);
    volumeSlider->bounds(0, 130);
    volumeSlider->value(100);
    volumeSlider->callback(volume_cb);

    auto* eqBtn = new ModernButton(900, 675, 40, 40, "EQ");
    eqBtn->color(Theme::SIDEBAR);
    eqBtn->labelcolor(Theme::TEXT_SECONDARY);
    eqBtn->callback(open_eq_window_cb);

    auto* downloadBtn = new ModernButton(945, 675, 40, 40, "\xe2\x86\x93");
    downloadBtn->color(Theme::SIDEBAR);
    downloadBtn->labelcolor(Theme::TEXT_SECONDARY);
    downloadBtn->callback(download_cb);
    downloadBtn->tooltip(lang->download_song);

    /* ── 7. STATUS BAR (730–750) ──────────────────── */
    statusBar = new Fl_Box(0, 730, 1000, 20);
    statusBar->box(FL_FLAT_BOX);
    statusBar->color(Theme::SIDEBAR);
    statusBar->labelcolor(Theme::TEXT_SECONDARY);
    statusBar->labelsize(11);
    statusBar->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    if (!settings.showStatusBar) statusBar->hide();

    /* ── Initial state ────────────────────────────── */
    show_home_view();
    load_sidebar_playlists();

    Fl::add_timeout(0.2, update_ui_cb);
    Fl::add_timeout(0.5, update_status_bar_cb);

    load_settings();
    apply_language();

    window->end();
    window->show(argc, argv);
    return Fl::run();
}
