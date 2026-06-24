#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <cstdint>
#include <FL/Fl_Menu_Item.H>
#include "UIWidgets.h"
#include "Globals.h"
#include "PlayerEngine.h"
#include "Theme.h"
#include "ModernButton.h"
#include "PlaylistManager.h"
#include "ProgressSlider.h"

/* ================================================================
 * CircularButton — Play/Pause green circle
 * ================================================================ */
CircularButton::CircularButton(int x, int y, int w, int h, const char* label)
    : Fl_Button(x, y, w, h, label) {
    box(FL_NO_BOX);
    color(Theme::ACCENT);
    labelcolor(FL_BLACK);
}

void CircularButton::draw() {
    fl_antialias(settings.enableAntialiasing ? 1 : 0);
    bool is_below  = (Fl::belowmouse() == this);
    bool is_pushed = value() || (Fl::pushed() == this && is_below);

    Fl_Color bg = color();
    if (is_below)  bg = fl_lighter(bg);
    if (is_pushed) bg = fl_darker(bg);

    fl_color(bg);
    fl_pie(x(), y(), w(), h(), 0, 360);

    fl_color(labelcolor());
    int cx = x() + w() / 2;
    int cy = y() + h() / 2;
    std::string lbl = label() ? label() : "";
    if (lbl == "@>" || lbl == "PLAY" || lbl == "PAUSE" || lbl == "@||") {
        if (player && !player->is_paused()) {
            fl_rectf(cx - 5, cy - 8, 3, 16);
            fl_rectf(cx + 2, cy - 8, 3, 16);
        } else {
            int pts_x[3] = { cx - 5, cx - 5, cx + 8 };
            int pts_y[3] = { cy - 8, cy + 8, cy };
            fl_polygon(pts_x[0], pts_y[0], pts_x[1], pts_y[1], pts_x[2], pts_y[2]);
        }
    } else {
        fl_font(labelfont(), labelsize());
        fl_draw(label(), x(), y(), w(), h(), FL_ALIGN_CENTER, 0, 0);
    }
}

int CircularButton::handle(int event) {
    int ret = Fl_Button::handle(event);
    if (event == FL_ENTER || event == FL_LEAVE || event == FL_PUSH || event == FL_RELEASE)
        redraw();
    return ret;
}

/* ================================================================
 * HeartButton — Favourite toggle ♡ / ♥
 * ================================================================ */
HeartButton::HeartButton(int x, int y, int w, int h) : Fl_Button(x, y, w, h, "") {
    box(FL_NO_BOX);
}

void HeartButton::draw() {
    fl_antialias(settings.enableAntialiasing ? 1 : 0);
    bool is_below = (Fl::belowmouse() == this);
    fl_color(active ? Theme::ACCENT : (is_below ? Theme::TEXT_PRIMARY : Theme::TEXT_SECONDARY));
    fl_font(FL_HELVETICA, 20);
    const char* symbol = active ? "\xe2\x99\xa5" : "\xe2\x99\xa1";
    fl_draw(symbol, x(), y(), w(), h(), FL_ALIGN_CENTER, 0, 0);
}

int HeartButton::handle(int event) {
    int ret = Fl_Button::handle(event);
    if (event == FL_ENTER || event == FL_LEAVE || event == FL_PUSH || event == FL_RELEASE)
        redraw();
    return ret;
}

/* ================================================================
 * ResultsBrowser — Main track list with progressive loading
 * ================================================================ */
ResultsBrowser::ResultsBrowser(int x, int y, int w, int h, const char* l)
    : Fl_Browser(x, y, w, h, l) {}

int ResultsBrowser::handle(int event) {
    if (event == FL_MOUSEWHEEL && Fl::event_dy() > 0) {
        if ((current_category == "SEARCH" || current_category == "CHANNEL") &&
            total_loaded_results < (int)last_results.size()) {

            Fl::remove_timeout(progressive_fill_cb);

            int sz = size();
            if (sz > 0) {
                const char* t = text(sz);
                if (t && strstr(t, lang->show_more_text)) remove(sz);
            }

            int batch = std::min(settings.scrollBatchSize,
                                 (int)last_results.size() - total_loaded_results);
            for (int i = 0; i < batch; i++) {
                const auto& res = last_results[total_loaded_results + i];
                std::string prefix;
                if (res.is_channel) {
                    if (res.is_live)
                        add((std::string("\xe2\x97\x8f\t") + res.title + "\t" + res.author).c_str());
                    else
                        add((std::string("\t") + res.title + "\t" + res.author).c_str());
                } else if (res.is_video) {
                    add((std::string("\t") + res.title + "\t" + res.author).c_str());
                } else if (res.is_playlist) {
                    add((std::string("@C255\xe2\x96\xb6\t") + res.title + "\t" + res.author).c_str());
                } else {
                    bool is_fav = PlaylistManager::is_favorite(res.video_id);
                    std::string star = is_fav ? "@C7\xe2\x98\x85" : "@C255\xe2\x98\x86";
                    add((star + "\t" + res.title + "\t" + res.author).c_str());
                }
            }
            total_loaded_results += batch;

            if (current_category == "SEARCH" &&
                total_loaded_results < (int)last_results.size()) {
                std::string more = "@C150\xe2\x96\xb8  " + std::string(lang->show_more_prefix)
                                 + std::to_string((int)last_results.size() - total_loaded_results)
                                 + lang->remaining_suffix;
                add(more.c_str());
            }
            redraw();
            return 1;
        }
    }

    if (event == FL_PUSH) {
        int line = value();
        if (Fl::event_button() == 1) {
            if (current_category == "SEARCH") {
                const char* t = text(line);
                if (t && strstr(t, lang->show_more_text)) {
                    load_more_search_results();
                    return 1;
                }
            }
            if (Fl::event_x() >= x() && Fl::event_x() < x() + 30) {
                if (line > 0 && line <= (int)last_results.size()) {
                    if (last_results[line - 1].is_playlist || last_results[line - 1].is_channel)
                        return 0;
                    std::string video_id = last_results[line - 1].video_id;
                    if (video_id.find(".txt") == std::string::npos) {
                        static PlaylistSelectionWindow* win = nullptr;
                        if (win) { win->hide(); delete win; }
                        win = new PlaylistSelectionWindow(video_id);
                        win->set_modal();
                        if (Fl::first_window())
                            win->position(Fl::first_window()->x() + (Fl::first_window()->w() - win->w()) / 2,
                                          Fl::first_window()->y() + (Fl::first_window()->h() - win->h()) / 2);
                        win->show();
                        return 1;
                    }
                }
            }
        } else if (Fl::event_button() == 3) {
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

void ResultsBrowser::delete_entry_cb(Fl_Widget* w, void* data) {
    auto* rb = (ResultsBrowser*)w;
    int line = (int)(intptr_t)data;
    if (line > 0 && line <= (int)last_results.size()) {
        std::string video_id = last_results[line - 1].video_id;
        if (current_category == "MY FAVORITES")
            PlaylistManager::remove_from_favorites(video_id);
        else if (!current_playlist.empty())
            PlaylistManager::remove_song_from_playlist(current_playlist, video_id);
        refresh_current_view();
    }
}

/* ================================================================
 * CreatePlaylistWindow — Modal dialog
 * ================================================================ */
CreatePlaylistWindow::CreatePlaylistWindow() : Fl_Double_Window(300, 180, lang->new_playlist_title) {
    color(Theme::SIDEBAR);

    nameIn = new Fl_Input(100, 25, 180, 25, lang->name_label);
    nameIn->textcolor(Theme::TEXT_PRIMARY);
    nameIn->color(Theme::HOVER);
    nameIn->labelcolor(Theme::TEXT_SECONDARY);

    commentIn = new Fl_Input(100, 65, 180, 25, lang->comment_label);
    commentIn->textcolor(Theme::TEXT_PRIMARY);
    commentIn->color(Theme::HOVER);
    commentIn->labelcolor(Theme::TEXT_SECONDARY);

    auto* btn = new ModernButton(100, 115, 100, 35, lang->create_btn);
    btn->color(Theme::ACCENT);
    btn->callback(create_cb, this);
    end();
}

void CreatePlaylistWindow::clear_inputs() {
    if (nameIn)    nameIn->value("");
    if (commentIn) commentIn->value("");
}

void CreatePlaylistWindow::create_cb(Fl_Widget*, void* data) {
    auto* win = (CreatePlaylistWindow*)data;
    std::string name = win->nameIn->value();
    std::string comment = win->commentIn->value();
    if (!name.empty()) {
        PlaylistManager::create_playlist(name, comment);
        win->hide();
        load_sidebar_playlists();
    }
}

/* ================================================================
 * PlaylistSelectionWindow — Modal dialog
 * ================================================================ */
PlaylistSelectionWindow::PlaylistSelectionWindow(const std::string& video_id)
    : Fl_Double_Window(300, 350, lang->add_to_playlist_title), vid(video_id) {
    color(Theme::SIDEBAR);
    list = new Fl_Hold_Browser(10, 10, 280, 280);
    list->color(Theme::HOVER);
    list->textcolor(Theme::TEXT_PRIMARY);
    list->selection_color(Theme::ACCENT);

    list->add(lang->favorites_btn);
    auto playlists = PlaylistManager::get_all_playlists();
    for (const auto& p : playlists) {
        std::string name = p;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".txt")
            name = name.substr(0, name.size() - 4);
        list->add(name.c_str());
    }

    auto* btn = new ModernButton(100, 300, 100, 35, lang->add_btn);
    btn->callback(add_cb, this);

    end();
}

void PlaylistSelectionWindow::add_cb(Fl_Widget*, void* data) {
    auto* win = (PlaylistSelectionWindow*)data;
    int val = win->list->value();
    if (val > 0) {
        std::string choice = win->list->text(val);
        if (choice == lang->favorites_btn) {
            PlaylistManager::add_to_favorites(win->vid);
        } else {
            PlaylistManager::add_to_playlist(choice, win->vid);
        }
        win->hide();
        show_styled_message(lang->added_to_playlist);
    }
}

/* ================================================================
 * Styled message / choice dialogs
 * ================================================================ */
void show_styled_message(const char* msg) {
    Fl_Double_Window win(360, 120, lang->window_title);
    win.color(Theme::SIDEBAR);
    win.set_modal();

    Fl_Box text(20, 20, 320, 40, msg);
    text.labelcolor(Theme::TEXT_PRIMARY);
    text.box(FL_NO_BOX);
    text.align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

    auto* okBtn = new ModernButton(130, 75, 100, 30, lang->ok_btn);
    okBtn->color(Theme::ACCENT);
    okBtn->labelcolor(FL_BLACK);
    okBtn->callback([](Fl_Widget*, void* d) { ((Fl_Double_Window*)d)->hide(); }, &win);

    win.end();
    if (Fl::first_window())
        win.position(Fl::first_window()->x() + (Fl::first_window()->w() - win.w()) / 2,
                     Fl::first_window()->y() + (Fl::first_window()->h() - win.h()) / 2);
    win.show();
    while (win.shown()) Fl::wait();
}

bool show_styled_choice(const char* msg) {
    bool result = false;
    Fl_Double_Window win(360, 140, lang->window_title);
    win.color(Theme::SIDEBAR);
    win.set_modal();

    Fl_Box text(20, 20, 320, 50, msg);
    text.labelcolor(Theme::TEXT_PRIMARY);
    text.box(FL_NO_BOX);
    text.align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE);

    auto* yesBtn = new ModernButton(80, 90, 90, 30, lang->yes_btn);
    yesBtn->color(Theme::ACCENT);
    yesBtn->labelcolor(FL_BLACK);
    yesBtn->callback([](Fl_Widget* w, void* d) { *((bool*)d) = true; w->window()->hide(); }, &result);

    auto* noBtn = new ModernButton(190, 90, 90, 30, lang->no_btn);
    noBtn->color(Theme::HOVER);
    noBtn->labelcolor(Theme::TEXT_PRIMARY);
    noBtn->callback([](Fl_Widget*, void* d) { ((Fl_Double_Window*)d)->hide(); }, &win);

    win.end();
    if (Fl::first_window())
        win.position(Fl::first_window()->x() + (Fl::first_window()->w() - win.w()) / 2,
                     Fl::first_window()->y() + (Fl::first_window()->h() - win.h()) / 2);
    win.show();
    while (win.shown()) Fl::wait();
    return result;
}
