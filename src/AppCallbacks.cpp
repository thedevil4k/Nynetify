#include <iostream>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include "AppCallbacks.h"
#include "Globals.h"
#include "Theme.h"
#include "PlayerEngine.h"
#include "ModernButton.h"
#include "ProgressSlider.h"
#include "ModernSlider.h"
#include "ModernChoice.h"
#include "PlayerController.h"
#include "ViewManager.h"
#include "ArtistParser.h"
#include "AppSettings.h"
#include "UIWidgets.h"
#include "PlaylistManager.h"
#include "YoutubeService.h"

/* ================================================================
 * Greeting (time-of-day based)
 * ================================================================ */
std::string get_greeting() {
    time_t rawtime;
    time(&rawtime);
    auto* timeinfo = localtime(&rawtime);
    int hour = timeinfo->tm_hour;
    if (hour < 12) return lang->greeting_morning;
    if (hour < 18) return lang->greeting_afternoon;
    return lang->greeting_evening;
}

/* ================================================================
 * Playback callbacks
 * ================================================================ */

/* ── Browser double-click / single right-click play ─ */
void play_selected_cb(Fl_Widget* w, void* data) {
    auto* browser = (Fl_Browser*)w;
    int line = browser->value();

    if (current_category == "SEARCH") {
        const char* t = browser->text(line);
        if (t && strstr(t, lang->show_more_text)) {
            load_more_search_results();
            return;
        }
    }

    if (line <= 0 || line > (int)last_results.size()) return;
    if (!Fl::event_clicks() && data == nullptr) return;   /* require double-click */

    std::string video_id = last_results[line - 1].video_id;
    std::string title    = last_results[line - 1].title;
    std::string author   = last_results[line - 1].author;

    if (last_results[line - 1].is_playlist) {
        show_youtube_playlist_view(video_id, title, author);
        return;
    }
    if (last_results[line - 1].is_channel) {
        show_channel_view(video_id, title);
        return;
    }
    if (video_id.find(".txt") != std::string::npos) {
        std::string name = video_id;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            name = name.substr(0, name.size() - 4);
        show_playlist_view(name);
        return;
    }

    play_queue = last_results;
    current_queue_index = line - 1;
    play_index(current_queue_index);
}

/* ── Main play/pause button ──────────────────────── */
void play_btn_cb(Fl_Widget* w, void* data) {
    if (!player) return;
    if (player->is_paused()) {
        player->resume();
        w->redraw();
    } else if (current_queue_index >= 0) {
        player->pause();
        w->redraw();
    } else {
        int line = resultsBrowser ? resultsBrowser->value() : 0;
        if (line > 0 && line <= (int)last_results.size())
            play_selected_cb(resultsBrowser, (void*)1);
        else if (!last_results.empty()) {
            play_queue = last_results;
            current_queue_index = 0;
            play_index(0);
        }
    }
}

void pause_resume_cb(Fl_Widget* w, void* data) {
    if (!player) return;
    if (player->is_paused()) player->resume();
    else                     player->pause();
    if (playBtn) playBtn->redraw();
}

void seek_cb(Fl_Widget* w, void* data) {
    if (player) player->set_position(progressBar->value());
}

void volume_cb(Fl_Widget* w, void* data) {
    if (player) player->set_volume(volumeSlider->value());
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
    if (current_queue_index < 0 || current_queue_index >= (int)play_queue.size()) return;
    std::string video_id = play_queue[current_queue_index].video_id;
    if (PlaylistManager::is_favorite(video_id)) {
        PlaylistManager::remove_from_favorites(video_id);
        if (heartBtn) heartBtn->active = false;
    } else {
        PlaylistManager::add_to_favorites(video_id);
        if (heartBtn) heartBtn->active = true;
    }
    if (heartBtn) heartBtn->redraw();
    if (current_category == "MY FAVORITES") refresh_current_view();
}

/* ================================================================
 * Search callbacks
 * ================================================================ */

void search_cb(Fl_Widget* w, void* data) {
    current_category = "SEARCH";
    current_playlist = "";
    auto* input   = (Fl_Input*)w;
    auto* browser = (Fl_Browser*)data;
    browser->clear();

    std::string query = input->value();
    if (query.empty()) return;

    last_search_query  = query;
    if (searchFilter) last_search_filter = searchFilter->value();

    std::cout << "[UI] Searching for: " << query << "..." << std::endl;
    try {
        last_results = YoutubeService::search(query, user_region, last_search_filter, settings.initialFetchSize);
        std::cout << "[UI] Found " << last_results.size() << " results." << std::endl;

        total_loaded_results = 0;
        browser->clear();

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

        if (total_loaded_results < (int)last_results.size())
            Fl::add_timeout(0.05, progressive_fill_cb);
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Search failed: " << e.what() << std::endl;
        browser->add(lang->search_failed);
    }
}

void progressive_fill_cb(void* data) {
    if (!resultsBrowser) return;

    int batch = std::min(3, (int)last_results.size() - total_loaded_results);
    auto* browser = resultsBrowser;
    for (int i = total_loaded_results; i < total_loaded_results + batch; i++) {
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
    total_loaded_results += batch;
    browser->redraw();

    int row_h = browser->textsize() + 4;
    if (row_h < 14) row_h = 18;
    int viewport_cap = browser->h() / row_h;

    if (total_loaded_results >= viewport_cap || total_loaded_results >= (int)last_results.size()) {
        if (current_category == "SEARCH" && total_loaded_results < (int)last_results.size()) {
            std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix)
                             + std::to_string((int)last_results.size() - total_loaded_results)
                             + lang->remaining_suffix;
            browser->add(more.c_str());
            browser->redraw();
        }
        return;
    }
    Fl::repeat_timeout(0.05, progressive_fill_cb);
}

void load_more_search_results() {
    auto* browser = resultsBrowser;
    if (!browser) return;

    int last_line = browser->size();
    if (last_line > 0) {
        const char* t = browser->text(last_line);
        if (t && strstr(t, lang->show_more_text)) browser->remove(last_line);
    }

    int prev_total = (int)last_results.size();
    int new_limit  = prev_total + settings.initialFetchSize;
    auto fresh = YoutubeService::search(last_search_query, user_region, last_search_filter, new_limit);

    for (int i = prev_total; i < (int)fresh.size(); i++)
        last_results.push_back(fresh[i]);

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

    if (total_loaded_results < (int)last_results.size()) {
        std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix)
                         + std::to_string((int)last_results.size() - total_loaded_results)
                         + lang->remaining_suffix;
        browser->add(more.c_str());
    }
    browser->redraw();
}

/* ================================================================
 * Category cards (Home)
 * ================================================================ */
void category_card_cb(Fl_Widget* w, void* data) {
    int idx = (int)(intptr_t)data;
    if (idx == 5) {
        show_favorites_view();
    } else {
        show_search_view();
        if (searchBar) {
            searchBar->value(lang->card_cats[idx]);
            search_cb(searchBar, resultsBrowser);
        }
    }
}

/* ================================================================
 * Refresh current view
 * ================================================================ */
void refresh_current_view() {
    if (current_category == "MY FAVORITES") {
        show_favorites_view();
    } else if (!current_playlist.empty()) {
        std::string name = current_playlist;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            name = name.substr(0, name.size() - 4);
        show_playlist_view(name);
    }
}

/* ================================================================
 * Sidebar
 * ================================================================ */
void sidebar_playlist_cb(Fl_Widget* w, void* data) {
    int val = sidebarPlaylistList ? sidebarPlaylistList->value() : 0;
    if (val > 0) {
        std::string name = sidebarPlaylistList->text(val);
        show_playlist_view(name);
    }
}

void load_sidebar_playlists() {
    if (!sidebarPlaylistList) return;
    sidebarPlaylistList->clear();
    auto lists = PlaylistManager::get_all_playlists();
    for (const auto& list : lists) {
        std::string name = list;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            name = name.substr(0, name.size() - 4);
        sidebarPlaylistList->add(name.c_str());
    }
    sidebarPlaylistList->redraw();
}

/* ================================================================
 * Language switching
 * ================================================================ */
void apply_language() {
    if (sidebarHomeBtn)       sidebarHomeBtn->copy_label(lang->home);
    if (sidebarSearchBtn)     sidebarSearchBtn->copy_label(lang->search);
    if (sidebarLibHeading)    sidebarLibHeading->copy_label(lang->your_library);
    if (sidebarLikedBtn)      sidebarLikedBtn->copy_label(lang->liked_songs);
    if (sidebarNewPlaylistBtn) sidebarNewPlaylistBtn->copy_label(lang->create_playlist);
    if (sidebarPrefsBtn)      sidebarPrefsBtn->copy_label(lang->settings);
    if (langToggleBtn)        langToggleBtn->copy_label(lang->language_btn);

    if (homeGreetingBox) homeGreetingBox->copy_label(get_greeting().c_str());
    if (homeBrowseTitle)  homeBrowseTitle->copy_label(lang->browse_categories);
    if (homeFeaturedTitle) homeFeaturedTitle->copy_label(lang->now_playing_featured);
    if (homeFeatDesc)     homeFeatDesc->copy_label(lang->feat_desc);
    for (int i = 0; i < 6; i++)
        if (homeCardButtons[i])
            homeCardButtons[i]->copy_label(lang->card_cats[i]);

    if (searchFilter) {
        int prev = searchFilter->value();
        searchFilter->clear();
        searchFilter->add(lang->everything);
        searchFilter->add(lang->songs_filter);
        searchFilter->add(lang->playlists_filter);
        searchFilter->add(lang->channels_filter);
        searchFilter->value(prev >= 0 && prev < 4 ? prev : 0);
    }
    if (searchBar) searchBar->tooltip(lang->search_tooltip);

    if (playlistNameBox && (current_category.empty() || current_category == "Top Hits"))
        playlistNameBox->copy_label(lang->playlist_name_placeholder);
    if (playlistDescBox) playlistDescBox->copy_label(lang->playlist_desc_placeholder);
    if (playlistDeleteBtn) playlistDeleteBtn->copy_label(lang->delete_playlist);

    if (current_queue_index < 0 && nowPlayingBox)
        nowPlayingBox->copy_label(lang->not_playing);
    if (nowPlayingArtistBox) nowPlayingArtistBox->tooltip(lang->view_channel);

    if (Fl::first_window())
        Fl::first_window()->copy_label(lang->window_title);

    Fl::redraw();
}

void lang_btn_cb(Fl_Widget*, void*) {
    lang = (lang == &LANG_EN) ? &LANG_ES : &LANG_EN;
    if (eqWin)   { eqWin->hide();   eqWin   = nullptr; }
    if (prefWin) { prefWin->hide(); prefWin = nullptr; }
    apply_language();
}

/* ================================================================
 * Artist navigation
 * ================================================================ */
void artist_name_cb(Fl_Widget* w, void* data) {
    if (current_queue_index < 0 || current_queue_index >= (int)play_queue.size()) return;

    auto& entry = play_queue[current_queue_index];
    std::string cleaned_uploader = clean_artist_name(entry.author);

    /* Start with the uploader */
    std::vector<std::string> artists;
    if (!cleaned_uploader.empty()) artists.push_back(cleaned_uploader);

    /* Extract title-based artists & merge deduped */
    auto title_artists = extract_artists_from_title(entry.title, cleaned_uploader);
    for (const auto& ta : title_artists) {
        bool found = false;
        for (const auto& a : artists) {
            std::string a_low = a, ta_low = ta;
            std::transform(a_low.begin(), a_low.end(), a_low.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            std::transform(ta_low.begin(), ta_low.end(), ta_low.begin(),
                           [](unsigned char c){ return std::tolower(c); });
            if (a_low == ta_low) { found = true; break; }
        }
        if (!found && is_valid_artist_name(ta)) artists.push_back(ta);
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
        std::string cid = entry.channel_id;
        if (cid.empty()) {
            cid = YoutubeService::get_channel_id(entry.video_id);
            if (!cid.empty()) entry.channel_id = cid;
        }
        if (!cid.empty())
            show_channel_view(cid, name);
        else
            show_styled_message(lang->channel_id_unavailable);
    } else {
        auto results = YoutubeService::search(name, user_region, 3, 5);
        for (const auto& r : results) {
            if (r.is_channel) { show_channel_view(r.video_id, name); return; }
        }
        show_styled_message(lang->channel_id_unavailable);
    }
}

/* ================================================================
 * Equaliser
 * ================================================================ */
void eq_slider_cb(Fl_Widget* w, void* data) {
    intptr_t band = (intptr_t)data;
    auto* slider = (Fl_Slider*)w;
    if (player) player->set_eq_gain((int)band, slider->value());
}

void eq_toggle_cb(Fl_Widget* w, void* data) {
    auto* check = (Fl_Check_Button*)w;
    if (player) player->set_eq_enabled(check->value() != 0);
}

void open_eq_window_cb(Fl_Widget* w, void* data) {
    if (eqWin) { eqWin->show(); return; }

    eqWin = new Fl_Double_Window(500, 320, lang->eq_title);
    eqWin->color(Theme::SIDEBAR);
    if (Fl::first_window())
        eqWin->position(Fl::first_window()->x() + (Fl::first_window()->w() - eqWin->w()) / 2,
                        Fl::first_window()->y() + (Fl::first_window()->h() - eqWin->h()) / 2);

    auto* eqToggle = new Fl_Check_Button(20, 10, 120, 25, lang->enable_eq);
    eqToggle->labelcolor(Theme::TEXT_PRIMARY);
    eqToggle->callback(eq_toggle_cb);
    if (player) eqToggle->value(player->is_eq_enabled() ? 1 : 0);

    { auto* b = new Fl_Box(465, 45, 30, 20, "+12"); b->labelcolor(Theme::TEXT_SECONDARY); b->labelsize(10); }
    { auto* b = new Fl_Box(465, 137, 30, 20, "0");  b->labelcolor(Theme::TEXT_SECONDARY); b->labelsize(10); }
    { auto* b = new Fl_Box(465, 230, 30, 20, "-12"); b->labelcolor(Theme::TEXT_SECONDARY); b->labelsize(10); }

    const char* labels[] = {"31", "62", "125", "250", "500", "1k", "2k", "4k", "8k", "16k"};
    for (int i = 0; i < 10; ++i) {
        auto* s = new ModernSlider(20 + i * 45, 50, 25, 200, labels[i]);
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

/* ================================================================
 * Preferences
 * ================================================================ */
void prefs_toggle_cb(Fl_Widget* w, void* data) {
    auto* btn = (Fl_Check_Button*)w;
    settings.loadThumbnails = btn->value() != 0;
    save_settings();
}

void status_toggle_cb(Fl_Widget* w, void* data) {
    auto* btn = (Fl_Check_Button*)w;
    settings.showStatusBar = btn->value() != 0;
    if (statusBar) {
        if (settings.showStatusBar) statusBar->show(); else statusBar->hide();
    }
    save_settings();
}

void buffer_cb(Fl_Widget* w, void* data) {
    auto* slider = (Fl_Slider*)w;
    auto* label  = (Fl_Box*)data;
    settings.bufferSizeMB = (int)slider->value();
    char buf[256];
    snprintf(buf, sizeof(buf), lang->max_buffer_label, settings.bufferSizeMB);
    label->copy_label(buf);
    label->redraw();
    if (player) player->set_buffer_size(settings.bufferSizeMB);
    save_settings();
}

void fetch_size_cb(Fl_Widget* w, void* data) {
    auto* slider = (Fl_Slider*)w;
    auto* label  = (Fl_Box*)data;
    settings.initialFetchSize = (int)slider->value();
    char buf[256];
    snprintf(buf, sizeof(buf), lang->fetch_size_label, settings.initialFetchSize);
    label->copy_label(buf);
    label->redraw();
    save_settings();
}

void scroll_batch_cb(Fl_Widget* w, void* data) {
    auto* slider = (Fl_Slider*)w;
    auto* label  = (Fl_Box*)data;
    settings.scrollBatchSize = (int)slider->value();
    char buf[256];
    snprintf(buf, sizeof(buf), lang->batch_label, settings.scrollBatchSize);
    label->copy_label(buf);
    label->redraw();
    save_settings();
}

/* ── Preferences window ──────────────────────────── */
void open_prefs_window_cb(Fl_Widget* w, void* data) {
    if (prefWin) { prefWin->show(); return; }

    prefWin = new Fl_Double_Window(300, 400, lang->settings_title);
    prefWin->color(Theme::SIDEBAR);
    if (Fl::first_window())
        prefWin->position(Fl::first_window()->x() + (Fl::first_window()->w() - prefWin->w()) / 2,
                          Fl::first_window()->y() + (Fl::first_window()->h() - prefWin->h()) / 2);

    auto* thumbToggle = new Fl_Check_Button(20, 20, 260, 30, lang->load_thumbnails);
    thumbToggle->labelcolor(Theme::TEXT_PRIMARY);
    thumbToggle->value(settings.loadThumbnails ? 1 : 0);
    thumbToggle->callback(prefs_toggle_cb);

    auto* statusToggle = new Fl_Check_Button(20, 60, 260, 30, lang->show_status_bar);
    statusToggle->labelcolor(Theme::TEXT_PRIMARY);
    statusToggle->value(settings.showStatusBar ? 1 : 0);
    statusToggle->callback(status_toggle_cb);

    auto* bufLabel = new Fl_Box(20, 110, 260, 20);
    bufLabel->labelcolor(Theme::TEXT_SECONDARY);
    bufLabel->labelsize(12);
    bufLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    char initBuf[256];
    snprintf(initBuf, sizeof(initBuf), lang->max_buffer_label, settings.bufferSizeMB);
    bufLabel->copy_label(initBuf);

    auto* bufSlider = new ModernSlider(20, 135, 260, 20);
    bufSlider->type(FL_HOR_SLIDER);
    bufSlider->bounds(1, 100);
    bufSlider->step(1);
    bufSlider->value((double)settings.bufferSizeMB);
    bufSlider->callback(buffer_cb, bufLabel);

    auto* fetchLabel = new Fl_Box(20, 175, 260, 20);
    fetchLabel->labelcolor(Theme::TEXT_SECONDARY);
    fetchLabel->labelsize(12);
    fetchLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    snprintf(initBuf, sizeof(initBuf), lang->fetch_size_label, settings.initialFetchSize);
    fetchLabel->copy_label(initBuf);

    auto* fetchSlider = new ModernSlider(20, 200, 260, 20);
    fetchSlider->type(FL_HOR_SLIDER);
    fetchSlider->bounds(20, 200);
    fetchSlider->step(10);
    fetchSlider->value((double)settings.initialFetchSize);
    fetchSlider->callback(fetch_size_cb, fetchLabel);

    auto* batchLabel = new Fl_Box(20, 240, 260, 20);
    batchLabel->labelcolor(Theme::TEXT_SECONDARY);
    batchLabel->labelsize(12);
    batchLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    snprintf(initBuf, sizeof(initBuf), lang->batch_label, settings.scrollBatchSize);
    batchLabel->copy_label(initBuf);

    auto* batchSlider = new ModernSlider(20, 265, 260, 20);
    batchSlider->type(FL_HOR_SLIDER);
    batchSlider->bounds(1, 10);
    batchSlider->step(1);
    batchSlider->value((double)settings.scrollBatchSize);
    batchSlider->callback(scroll_batch_cb, batchLabel);

    auto* dlLabel = new Fl_Box(20, 305, 260, 20, lang->download_path);
    dlLabel->labelcolor(Theme::TEXT_SECONDARY);
    dlLabel->labelsize(12);
    dlLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    auto* dlInput = new Fl_Input(20, 325, 205, 25);
    dlInput->value(settings.downloadPath.c_str());
    dlInput->textcolor(Theme::TEXT_PRIMARY);
    dlInput->color(Theme::HOVER);
    dlInput->textsize(11);
    dlInput->callback([](Fl_Widget* w, void*) {
        auto* in = (Fl_Input*)w;
        settings.downloadPath = in->value();
        save_settings();
    });

    auto* browseBtn = new ModernButton(230, 325, 50, 25, lang->browse);
    browseBtn->color(Theme::HOVER);
    browseBtn->labelcolor(Theme::TEXT_PRIMARY);
    browseBtn->labelsize(11);
    browseBtn->callback([](Fl_Widget*, void* d) {
        auto* inp = (Fl_Input*)d;
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

/* ================================================================
 * Download
 * ================================================================ */
void download_cb(Fl_Widget* w, void* data) {
    if (current_queue_index < 0 || current_queue_index >= (int)play_queue.size()) return;

    auto& entry = play_queue[current_queue_index];
    std::string video_id = entry.video_id;
    std::string title    = entry.title;
    std::string author   = entry.author;

    auto sanitise = [](std::string& s) {
        const char* illegal = "\\/:*?\"<>|";
        for (char& c : s)
            for (const char* p = illegal; *p; ++p)
                if (c == *p) { c = '_'; break; }
    };
    std::string safe_author = author;
    std::string safe_title  = title;
    sanitise(safe_author);
    sanitise(safe_title);

    std::string dlPath = settings.downloadPath;
    if (dlPath.empty()) dlPath = get_default_downloads_path();
    std::string filename = dlPath + NYN_PATH_SEP + safe_author + " - " + safe_title + ".mp3";
    std::string url = "https://www.youtube.com/watch?v=" + video_id;
    std::string cmd = std::string(YT_DLP) + " -x --audio-format mp3 --no-playlist -o \""
                      + filename + "\" \"" + url + "\"" + NYN_NULL_REDIRECT;

    char status[512];
    snprintf(status, sizeof(status), lang->downloading, title.c_str());
    if (statusBar) { statusBar->copy_label(status); statusBar->redraw(); }

    auto* titleCopy = new std::string(title);
    auto* filenameCopy = new std::string(filename);
    std::thread([cmd, titleCopy, filenameCopy]() {
        std::system(cmd.c_str());
        
        // Clean up the original webm file after conversion
        std::string webm_file = filenameCopy->substr(0, filenameCopy->length() - 4) + ".webm";
        if (std::filesystem::exists(webm_file)) {
            std::filesystem::remove(webm_file);
        }
        
        Fl::awake([](void* d) {
            auto* t = (std::string*)d;
            if (statusBar) {
                char buf[512];
                snprintf(buf, sizeof(buf), lang->download_completed, t->c_str());
                statusBar->copy_label(buf);
                statusBar->redraw();
            }
            delete t;
        }, titleCopy);
        delete filenameCopy;
    }).detach();
}

/* ================================================================
 * Playlist action callbacks
 * ================================================================ */
void playlist_play_click_cb(Fl_Widget* w, void* data) {
    if (!last_results.empty()) {
        play_queue = last_results;
        current_queue_index = 0;
        play_index(0);
    }
}

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
