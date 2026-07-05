#include <iostream>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <cctype>
#include <cstdint>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Native_File_Chooser.H>
#include "AppCallbacks.h"
#include "Globals.h"
#include "Theme.h"
#include "PlayerEngine.h"
#include "ModernButton.h"
#include "Spawn.h"
#include "ProgressSlider.h"
#include "ModernSlider.h"
#include "ModernChoice.h"
#include "PlayerController.h"
#include "ViewManager.h"
#include "ArtistParser.h"
#include "AppSettings.h"
#include "UIWidgets.h"
#include "PlaylistManager.h"
#include "RadioManager.h"
#include "RadioStation.h"
#include "YoutubeService.h"
#include "TwitchClient.h"

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

    if (line <= 0) return;
    if (!Fl::event_clicks() && data == nullptr) return;   /* require double-click */

    /* Channel view: lines contain headers/separators without '\t'.
     * Skip them and compute the real index into last_results. */
    if (current_category == "CHANNEL") {
        const char* t = browser->text(line);
        if (!t || !strchr(t, '\t')) return;  /* header / separator → ignore */

        int offset = 0;
        for (int i = 1; i < line; i++) {
            const char* ti = browser->text(i);
            if (!ti || !strchr(ti, '\t')) offset++;
        }
        int idx = line - 1 - offset;
        if (idx < 0 || idx >= (int)last_results.size()) return;

        if (last_results[idx].is_playlist) {
            show_youtube_playlist_view(last_results[idx].video_id,
                                       last_results[idx].title,
                                       last_results[idx].author);
            return;
        }

        /* Build a video-only queue */
        play_queue.clear();
        int queue_idx = 0;
        for (int i = 0; i <= idx; i++) {
            if (!last_results[i].is_playlist && !last_results[i].is_channel) {
                play_queue.push_back(last_results[i]);
                if (i < idx) queue_idx++;
            }
        }
        if (play_queue.empty()) return;
        current_queue_index = queue_idx;
        play_index(current_queue_index);
        return;
    }

    if (line > (int)last_results.size()) return;

    std::string video_id = last_results[line - 1].video_id;
    std::string title    = last_results[line - 1].title;
    std::string author   = last_results[line - 1].author;

    if (last_results[line - 1].is_playlist) {
        show_youtube_playlist_view(video_id, title, author);
        return;
    }
    /* Twitch/SoundCloud channels play directly — don't navigate to channel view */
    if (last_results[line - 1].is_channel && !last_results[line - 1].is_twitch
        && !last_results[line - 1].is_soundcloud) {
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
    if (RadioManager::get_radio_mode()) {
        if (RadioManager::current_radio_index >= 0) {
            int id = RadioManager::stations[RadioManager::current_radio_index].id;
            RadioManager::toggle_favorite(id);
            if (heartBtn) {
                heartBtn->active = RadioManager::is_favorite(id);
                heartBtn->redraw();
            }
        }
        return;
    }
    if (current_queue_index < 0 || current_queue_index >= (int)play_queue.size()) return;
    std::string video_id = play_queue[current_queue_index].video_id;
    if (play_queue[current_queue_index].is_soundcloud) {
        if (PlaylistManager::is_soundcloud_favorite(video_id)) {
            PlaylistManager::remove_soundcloud_favorite(video_id);
            if (heartBtn) heartBtn->active = false;
        } else {
            PlaylistManager::add_soundcloud_favorite(video_id);
            if (heartBtn) heartBtn->active = true;
        }
    } else if (PlaylistManager::is_favorite(video_id)) {
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

/* ── Platform toggle callbacks ──────────────────── */
void yt_toggle_cb(Fl_Widget* w, void* data) {
    searchPlatform = 0;
    settings.searchPlatform = 0;
    save_settings();
    if (ytToggleBtn)  ytToggleBtn->color(Theme::ACCENT);
    if (twitchToggleBtn) twitchToggleBtn->color(Theme::HOVER);
    if (soundcloudToggleBtn) soundcloudToggleBtn->color(Theme::HOVER);
    if (ytToggleBtn)  ytToggleBtn->redraw();
    if (twitchToggleBtn) twitchToggleBtn->redraw();
    if (soundcloudToggleBtn) soundcloudToggleBtn->redraw();
    /* Re-run search with debounce if there's a query */
    if (searchBar && searchBar->value()[0] != '\0')
        search_cb(searchBar, resultsBrowser);
}

void twitch_toggle_cb(Fl_Widget* w, void* data) {
    searchPlatform = 1;
    settings.searchPlatform = 1;
    save_settings();
    if (ytToggleBtn)  ytToggleBtn->color(Theme::HOVER);
    if (twitchToggleBtn) twitchToggleBtn->color(Theme::ACCENT);
    if (soundcloudToggleBtn) soundcloudToggleBtn->color(Theme::HOVER);
    if (ytToggleBtn)  ytToggleBtn->redraw();
    if (twitchToggleBtn) twitchToggleBtn->redraw();
    if (soundcloudToggleBtn) soundcloudToggleBtn->redraw();
    /* Re-run search with debounce if there's a query */
    if (searchBar && searchBar->value()[0] != '\0')
        search_cb(searchBar, resultsBrowser);
}

void soundcloud_toggle_cb(Fl_Widget* w, void* data) {
    searchPlatform = 2;
    settings.searchPlatform = 2;
    save_settings();
    if (ytToggleBtn) ytToggleBtn->color(Theme::HOVER);
    if (twitchToggleBtn) twitchToggleBtn->color(Theme::HOVER);
    if (soundcloudToggleBtn) soundcloudToggleBtn->color(Theme::ACCENT);
    if (ytToggleBtn) ytToggleBtn->redraw();
    if (twitchToggleBtn) twitchToggleBtn->redraw();
    if (soundcloudToggleBtn) soundcloudToggleBtn->redraw();
    if (searchBar && searchBar->value()[0] != '\0')
        search_cb(searchBar, resultsBrowser);
}

/* Async search helpers */
struct SearchTask {
    std::string query;
    std::string region;
    int filter_type = 0;
    int platform = 0;
    int max_results = 0;
    int sequence = 0;
    std::vector<SearchResult> results;
};

static int search_sequence = 0;

void search_completed_cb(void* data);

/* ── Search debounce timer callback ─────────────── */
static void debounced_search_cb(void* data) {
    auto* input = static_cast<Fl_Input*>(data);
    std::string query = input->value();
    if (query.empty()) return;

    current_category = "SEARCH";
    current_playlist = "";

    last_search_query  = query;
    if (searchFilter) last_search_filter = searchFilter->value();

    std::cout << "[UI] Searching for: " << query << "..." << std::endl;

    /* Show immediate loading indicator */
    if (resultsBrowser) {
        resultsBrowser->clear();
        resultsBrowser->add((std::string("@C150\xe2\x8f\xb3 ") + lang->searching + "...").c_str());
        resultsBrowser->redraw();
    }
    if (statusBar) {
        statusBar->copy_label("Searching...");
        statusBar->redraw();
    }

    auto* task = new SearchTask();
    task->query = query;
    task->region = user_region;
    task->filter_type = last_search_filter;
    task->platform = searchPlatform;
    task->max_results = settings.initialFetchSize;
    task->sequence = ++search_sequence;

    std::thread([task]() {
        try {
            if (task->platform == 1)
                task->results = TwitchClient::search(task->query, task->max_results);
            else if (task->platform == 2)
                task->results = SoundCloudClient::search(task->query, task->max_results);
            else
                task->results = YoutubeService::search(task->query, task->region, task->filter_type, task->max_results);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Search failed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Search failed: unknown exception" << std::endl;
        }
        Fl::awake(search_completed_cb, task);
    }).detach();
}

static void add_result_row(Fl_Browser* browser, const SearchResult& res) {
    if (res.is_channel) {
        if (res.is_live)
            browser->add((std::string("\xe2\x97\x8f\t") + res.title + "\t" + res.author).c_str());
        else
            browser->add((std::string("\t") + res.title + "\t" + res.author).c_str());
    } else if (res.is_video) {
        browser->add((std::string("\t") + res.title + "\t" + res.author).c_str());
    } else if (res.is_playlist) {
        browser->add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
    } else {
        bool is_fav;
        if (res.is_soundcloud)
            is_fav = PlaylistManager::is_soundcloud_favorite(res.video_id);
        else
            is_fav = PlaylistManager::is_favorite(res.video_id);
        std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
        browser->add((star + "\t" + res.title + "\t" + res.author).c_str());
    }
}

void search_completed_cb(void* data) {
    auto* task = (SearchTask*)data;
    if (!resultsBrowser) { delete task; return; }
    if (task->sequence != search_sequence) { delete task; return; }
    std::cout << "[UI] Found " << task->results.size() << " results." << std::endl;

    last_results = std::move(task->results);
    delete task;
    auto* browser = resultsBrowser;

    total_loaded_results = 0;
    browser->clear();

    if (last_results.empty()) {
        browser->add(lang->search_failed);
        browser->redraw();
        return;
    }

    /* Show first batch immediately (15 items or all if fewer) */
    int to_show = std::min(15, (int)last_results.size());
    for (int i = 0; i < to_show; i++)
        add_result_row(browser, last_results[i]);
    total_loaded_results = to_show;

    if (total_loaded_results < (int)last_results.size())
        Fl::add_timeout(0.03, progressive_fill_cb);
    browser->redraw();

    if (statusBar) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%zu results found", last_results.size());
        statusBar->copy_label(buf);
        statusBar->redraw();
    }
}

void search_cb(Fl_Widget* w, void* data) {
    auto* input = (Fl_Input*)w;
    std::string query = input->value();
    if (query.empty()) return;

    /* Debounce: cancel any pending search, wait 300ms */
    Fl::remove_timeout(debounced_search_cb);
    Fl::add_timeout(0.3, debounced_search_cb, input);
}

void progressive_fill_cb(void* data) {
    if (!resultsBrowser) return;

    int batch = std::min(15, (int)last_results.size() - total_loaded_results);
    auto* browser = resultsBrowser;
    for (int i = total_loaded_results; i < total_loaded_results + batch; i++)
        add_result_row(browser, last_results[i]);
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
    Fl::repeat_timeout(0.03, progressive_fill_cb);
}

void load_more_completed_cb(void* data) {
    auto* task = (SearchTask*)data;
    if (!resultsBrowser) { delete task; return; }
    if (task->sequence != search_sequence) { delete task; return; }
    auto* browser = resultsBrowser;

    if (task->results.empty()) {
        delete task;
        browser->redraw();
        return;
    }

    int prev_total = (int)last_results.size();
    for (int i = prev_total; i < (int)task->results.size(); i++)
        last_results.push_back(std::move(task->results[i]));
    delete task;

    int to_add = std::min(settings.scrollBatchSize, (int)last_results.size() - total_loaded_results);
    for (int i = total_loaded_results; i < total_loaded_results + to_add; i++)
        add_result_row(browser, last_results[i]);
    total_loaded_results += to_add;

    if (total_loaded_results < (int)last_results.size()) {
        std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix)
                         + std::to_string((int)last_results.size() - total_loaded_results)
                         + lang->remaining_suffix;
        browser->add(more.c_str());
    }
    browser->redraw();
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

    auto* task = new SearchTask();
    task->query = last_search_query;
    task->filter_type = last_search_filter;
    task->region = user_region;
    task->platform = searchPlatform;
    task->max_results = new_limit;
    task->sequence = search_sequence;

    std::thread([task]() {
        try {
            if (task->platform == 1)
                task->results = TwitchClient::search(task->query, task->max_results);
            else if (task->platform == 2)
                task->results = SoundCloudClient::search(task->query, task->max_results);
            else
                task->results = YoutubeService::search(task->query, task->region, task->filter_type, task->max_results);
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] Load more failed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] Load more failed: unknown exception" << std::endl;
        }
        Fl::awake(load_more_completed_cb, task);
    }).detach();
}

/* ================================================================
 * Category cards (Home)
 * ================================================================ */
void category_card_cb(Fl_Widget* w, void* data) {
    int idx = (int)(intptr_t)data;

    /* Immediate visual feedback: flash the card accent */
    Fl_Color original = w->color();
    w->color(Theme::ACCENT);
    w->redraw();
    Fl::check();  // force immediate redraw

    if (idx == 5) {
        w->color(original);
        w->redraw();
        show_favorites_view();
    } else {
        show_search_view();
        if (searchBar) {
            searchBar->value(lang->card_cats[idx]);
            search_cb(searchBar, resultsBrowser);
        }
        /* Restore original color after a short delay */
        Fl::add_timeout(0.2, [](void* d) {
            auto* btn = static_cast<Fl_Widget*>(d);
            btn->color(Theme::CARD_BG);
            btn->redraw();
        }, (void*)w);
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
    if (sidebarRadioBtn)      sidebarRadioBtn->copy_label(lang->radio);
    if (sidebarPrefsBtn)      sidebarPrefsBtn->copy_label(lang->settings);
    if (langToggleBtn)        langToggleBtn->copy_label(lang->language_btn);

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

    prefWin = new Fl_Double_Window(300, 460, lang->settings_title);
    prefWin->color(Theme::SIDEBAR);
    if (Fl::first_window())
        prefWin->position(Fl::first_window()->x() + (Fl::first_window()->w() - prefWin->w()) / 2,
                          Fl::first_window()->y() + (Fl::first_window()->h() - prefWin->h()) / 2);

    auto* thumbToggle = new Fl_Check_Button(20, 15, 260, 25, lang->load_thumbnails);
    thumbToggle->labelcolor(Theme::TEXT_PRIMARY);
    thumbToggle->value(settings.loadThumbnails ? 1 : 0);
    thumbToggle->callback(prefs_toggle_cb);

    auto* statusToggle = new Fl_Check_Button(20, 45, 260, 25, lang->show_status_bar);
    statusToggle->labelcolor(Theme::TEXT_PRIMARY);
    statusToggle->value(settings.showStatusBar ? 1 : 0);
    statusToggle->callback(status_toggle_cb);

    auto* aaToggle = new Fl_Check_Button(20, 75, 260, 25, lang->enable_antialiasing);
    aaToggle->labelcolor(Theme::TEXT_PRIMARY);
    aaToggle->value(settings.enableAntialiasing ? 1 : 0);
    aaToggle->callback([](Fl_Widget* w, void*) {
        auto* btn = (Fl_Check_Button*)w;
        settings.enableAntialiasing = btn->value() != 0;
        save_settings();
        if (Fl::first_window()) Fl::first_window()->redraw();
    });

    auto* debugToggle = new Fl_Check_Button(20, 105, 260, 25, lang->enable_debug_mode);
    debugToggle->labelcolor(Theme::TEXT_PRIMARY);
    debugToggle->value(settings.debugMode ? 1 : 0);
    debugToggle->callback([](Fl_Widget* w, void*) {
        auto* btn = (Fl_Check_Button*)w;
        settings.debugMode = btn->value() != 0;
        save_settings();
    });

    auto* openLogsBtn = new ModernButton(20, 135, 120, 28, lang->open_logs_btn);
    openLogsBtn->color(Theme::HOVER);
    openLogsBtn->labelcolor(Theme::TEXT_PRIMARY);
    openLogsBtn->callback([](Fl_Widget*, void*) { openDebugLogViewer(); });

    auto* providerLabel = new Fl_Box(20, 175, 260, 15, lang->search_provider_label);
    providerLabel->labelcolor(Theme::TEXT_SECONDARY);
    providerLabel->labelsize(12);
    providerLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    auto* providerChoice = new Fl_Choice(20, 193, 260, 27);
    providerChoice->color(Theme::HOVER);
    providerChoice->textcolor(Theme::TEXT_PRIMARY);
    providerChoice->add(lang->provider_invidious);
    providerChoice->add(lang->provider_ytdlp);
    providerChoice->value(static_cast<int>(settings.searchProvider));
    providerChoice->callback([](Fl_Widget* w, void*) {
        auto* c = (Fl_Choice*)w;
        settings.searchProvider = c->value() == 0 ?
            AppSettings::SearchProvider::Invidious : AppSettings::SearchProvider::YTDLP;
        YoutubeService::setUseInvidious(settings.searchProvider == AppSettings::SearchProvider::Invidious);
        save_settings();
    });

    auto* bufLabel = new Fl_Box(20, 237, 260, 15);
    bufLabel->labelcolor(Theme::TEXT_SECONDARY);
    bufLabel->labelsize(12);
    bufLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    char initBuf[256];
    snprintf(initBuf, sizeof(initBuf), lang->max_buffer_label, settings.bufferSizeMB);
    bufLabel->copy_label(initBuf);

    auto* bufSlider = new ModernSlider(20, 255, 260, 20);
    bufSlider->type(FL_HOR_SLIDER);
    bufSlider->bounds(1, 100);
    bufSlider->step(1);
    bufSlider->value((double)settings.bufferSizeMB);
    bufSlider->callback(buffer_cb, bufLabel);

    auto* fetchLabel = new Fl_Box(20, 292, 260, 15);
    fetchLabel->labelcolor(Theme::TEXT_SECONDARY);
    fetchLabel->labelsize(12);
    fetchLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    snprintf(initBuf, sizeof(initBuf), lang->fetch_size_label, settings.initialFetchSize);
    fetchLabel->copy_label(initBuf);

    auto* fetchSlider = new ModernSlider(20, 310, 260, 20);
    fetchSlider->type(FL_HOR_SLIDER);
    fetchSlider->bounds(20, 200);
    fetchSlider->step(10);
    fetchSlider->value((double)settings.initialFetchSize);
    fetchSlider->callback(fetch_size_cb, fetchLabel);

    auto* batchLabel = new Fl_Box(20, 347, 260, 15);
    batchLabel->labelcolor(Theme::TEXT_SECONDARY);
    batchLabel->labelsize(12);
    batchLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    snprintf(initBuf, sizeof(initBuf), lang->batch_label, settings.scrollBatchSize);
    batchLabel->copy_label(initBuf);

    auto* batchSlider = new ModernSlider(20, 365, 260, 20);
    batchSlider->type(FL_HOR_SLIDER);
    batchSlider->bounds(1, 10);
    batchSlider->step(1);
    batchSlider->value((double)settings.scrollBatchSize);
    batchSlider->callback(scroll_batch_cb, batchLabel);

    auto* dlLabel = new Fl_Box(20, 402, 260, 15, lang->download_path);
    dlLabel->labelcolor(Theme::TEXT_SECONDARY);
    dlLabel->labelsize(12);
    dlLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    auto* dlInput = new Fl_Input(20, 420, 205, 25);
    dlInput->value(settings.downloadPath.c_str());
    dlInput->textcolor(Theme::TEXT_PRIMARY);
    dlInput->color(Theme::HOVER);
    dlInput->textsize(11);
    dlInput->callback([](Fl_Widget* w, void*) {
        auto* in = (Fl_Input*)w;
        settings.downloadPath = in->value();
        save_settings();
    });

    auto* browseBtn = new ModernButton(230, 420, 50, 25, lang->browse);
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
    std::string url;
    if (entry.is_soundcloud)
        url = video_id;
    else if (entry.is_twitch)
        url = "https://www.twitch.tv/videos/" + video_id;
    else
        url = "https://www.youtube.com/watch?v=" + video_id;
    std::string cmd = std::string(YT_DLP) + " -x --audio-format mp3 --no-playlist -o \""
                      + filename + "\" \"" + url + "\"" + NYN_NULL_REDIRECT;

    char status[512];
    snprintf(status, sizeof(status), lang->downloading, title.c_str());
    if (statusBar) { statusBar->copy_label(status); statusBar->redraw(); }

    auto* titleCopy = new std::string(title);
    auto* filenameCopy = new std::string(filename);
    std::thread([cmd, titleCopy, filenameCopy]() {
        run_hidden(cmd, settings.debugMode, false);
        
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

/* ================================================================
 * Radio callbacks
 * ================================================================ */
void radio_station_cb(Fl_Widget* w, void* data) {
    auto* browser = (Fl_Browser*)w;
    int line = browser->value();
    if (line <= 0) return;
    if (!Fl::event_clicks() && data == nullptr) return;

    auto list = RadioManager::filtered();
    if (line > (int)list.size()) return;

    const auto& station = list[line - 1];
    RadioManager::radio_mode = true;

    // Find global index
    for (int i = 0; i < (int)RadioManager::stations.size(); i++) {
        if (RadioManager::stations[i].id == station.id) {
            RadioManager::current_radio_index = i;
            break;
        }
    }

    std::cout << "[RADIO] Playing: " << station.name << " (" << station.stream_url << ")" << std::endl;

    // Update UI
    if (nowPlayingBox) {
        std::string np = station.name;
        if (np.size() > 32) np = np.substr(0, 29) + "...";
        nowPlayingBox->copy_label(np.c_str());
        nowPlayingBox->redraw();
    }
    if (nowPlayingArtistBox) {
        nowPlayingArtistBox->copy_label(std::string(lang->radio_live).c_str());
        nowPlayingArtistBox->redraw();
    }
    if (heartBtn) {
        heartBtn->active = RadioManager::is_favorite(station.id);
        heartBtn->redraw();
    }

    update_radio_cover(station.logo);

    player->play(station.stream_url);
    if (playBtn) playBtn->redraw();
}

void radio_country_cb(Fl_Widget* w, void* data) {
    auto* choice = (Fl_Choice*)w;
    int idx = choice->value();
    if (idx <= 0) {
        RadioManager::current_country_filter = "";
    } else {
        auto countries = RadioManager::all_countries();
        if (idx - 1 < (int)countries.size())
            RadioManager::current_country_filter = countries[idx - 1];
    }
    show_radio_view();
}

void radio_add_custom_cb(Fl_Widget* w, void* data) {
    auto* win = new Fl_Double_Window(400, 200, lang->radio_add_custom);
    win->color(Theme::SIDEBAR);
    if (Fl::first_window())
        win->position(Fl::first_window()->x() + (Fl::first_window()->w() - win->w()) / 2,
                      Fl::first_window()->y() + (Fl::first_window()->h() - win->h()) / 2);

    auto* nameLabel = new Fl_Box(20, 20, 100, 25, lang->radio_custom_name);
    nameLabel->labelcolor(Theme::TEXT_PRIMARY);
    nameLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    auto* nameInput = new Fl_Input(130, 20, 250, 25);
    nameInput->textcolor(Theme::TEXT_PRIMARY);
    nameInput->color(Theme::HOVER);

    auto* urlLabel = new Fl_Box(20, 55, 100, 25, lang->radio_custom_url);
    urlLabel->labelcolor(Theme::TEXT_PRIMARY);
    urlLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);

    auto* urlInput = new Fl_Input(130, 55, 250, 25);
    urlInput->textcolor(Theme::TEXT_PRIMARY);
    urlInput->color(Theme::HOVER);

    auto* addBtn = new ModernButton(150, 110, 100, 30, lang->add_btn);
    addBtn->color(Theme::ACCENT);
    addBtn->labelcolor(FL_BLACK);
    addBtn->callback([](Fl_Widget* btn, void* d) {
        auto** inputs = (Fl_Input**)d;
        std::string name = inputs[0]->value();
        std::string url = inputs[1]->value();
        if (!name.empty() && !url.empty()) {
            RadioManager::add_custom(name, url, "Custom", "INT");
            RadioManager::sort_by_country();
            show_radio_view();
        }
        btn->parent()->hide();
        delete[] (Fl_Input**)d;
    }, new Fl_Input*[2]{nameInput, urlInput});

    win->end();
    win->set_non_modal();
    win->show();
}

void radio_search_online_cb(Fl_Widget* w, void* data) {
    std::string query = "";
    if (searchBar && searchBar->value()[0]) {
        query = searchBar->value();
    }
    if (query.empty()) {
        show_styled_message(lang->search_tooltip);
        return;
    }

    auto results = RadioManager::search_online(query);
    if (results.empty()) {
        show_styled_message(lang->radio_no_stations);
        return;
    }

    // Add to stations list
    for (auto& s : results) {
        RadioManager::stations.push_back(s);
    }
    RadioManager::sort_by_country();
    show_radio_view();
}

void radio_refresh_url_cb(Fl_Widget* w, void* data) {
    if (!radioBrowser) return;
    int line = radioBrowser->value();
    if (line <= 0) {
        show_styled_message(lang->radio_no_stations);
        return;
    }
    auto list = RadioManager::filtered();
    if (line > (int)list.size()) return;

    const auto& station = list[line - 1];
    if (station.is_custom) {
        show_styled_message("Cannot refresh custom station URL");
        return;
    }

    // Find global index
    int global_idx = -1;
    for (int i = 0; i < (int)RadioManager::stations.size(); i++) {
        if (RadioManager::stations[i].id == station.id) {
            global_idx = i;
            break;
        }
    }
    if (global_idx < 0) return;

    bool updated = RadioManager::refresh_station_url(global_idx);
    if (updated) {
        std::string msg = std::string("URL updated for: ") + RadioManager::stations[global_idx].name;
        show_styled_message(msg.c_str());
    } else {
        show_styled_message("No better URL found");
    }
    refresh_radio_browser();
}
