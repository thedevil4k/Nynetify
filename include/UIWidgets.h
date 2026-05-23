#ifndef UIWIDGETS_H
#define UIWIDGETS_H

#include <FL/Fl_Button.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Hold_Browser.H>
#include <string>

class ProgressSlider;
class ModernButton;

/*
 * CircularButton — Play/Pause button with a green circle
 *
 * Draws a filled circle that toggles between a play-triangle
 * and pause-bars icons depending on the player state.  Hover
 * and press states are visualised with lighter/darker tints.
 */
class CircularButton : public Fl_Button {
public:
    CircularButton(int x, int y, int w, int h, const char* label = 0);
protected:
    void draw() override;
    int handle(int event) override;
};

/*
 * HeartButton — Favourite toggle (♡ / ♥)
 *
 * Displays an outlined heart when inactive and a filled,
 * accent-coloured heart when active.
 */
class HeartButton : public Fl_Button {
public:
    bool active = false;
    HeartButton(int x, int y, int w, int h);
protected:
    void draw() override;
    int handle(int event) override;
};

/*
 * ResultsBrowser — Main scrollable track list
 *
 * Handles mouse-wheel progressive loading, left-click on
 * the star column to add to playlists, right-click context
 * menu to delete entries.
 */
class ResultsBrowser : public Fl_Browser {
public:
    ResultsBrowser(int x, int y, int w, int h, const char* l = nullptr);
protected:
    int handle(int event) override;
    static void delete_entry_cb(Fl_Widget* w, void* data);
};

/*
 * CreatePlaylistWindow — Modal dialog to create a new playlist
 *
 * Contains a name field, an optional comment field, and a
 * "Create" button.  On submit it calls PlaylistManager.
 */
class CreatePlaylistWindow : public Fl_Double_Window {
    Fl_Input *nameIn, *commentIn;
public:
    CreatePlaylistWindow();
    void clear_inputs();
    static void create_cb(Fl_Widget*, void* data);
};

/*
 * PlaylistSelectionWindow — Modal dialog to add a song to a playlist
 *
 * Shows a list of existing playlists + "Favourites".  On
 * selection the video_id is added via PlaylistManager.
 */
class PlaylistSelectionWindow : public Fl_Double_Window {
public:
    Fl_Hold_Browser* list;
    std::string vid;
    PlaylistSelectionWindow(const std::string& video_id);
    static void add_cb(Fl_Widget*, void* data);
};

/* ── Styled info / confirm dialogs ───────────────── */
void show_styled_message(const char* msg);
bool show_styled_choice(const char* msg);

#endif // UIWIDGETS_H
