#ifndef THEME_H
#define THEME_H

#include <FL/Enumerations.H>

namespace Theme {
    // Spotify-inspired palette
    const Fl_Color BACKGROUND = fl_rgb_color(18, 18, 18);    // Charcoal
    const Fl_Color SIDEBAR = fl_rgb_color(0, 0, 0);          // Pure black
    const Fl_Color ACCENT = fl_rgb_color(29, 185, 84);       // Spotify Green
    const Fl_Color TEXT_PRIMARY = fl_rgb_color(255, 255, 255);
    const Fl_Color TEXT_SECONDARY = fl_rgb_color(179, 179, 179);
    const Fl_Color HOVER = fl_rgb_color(40, 40, 40);
    const Fl_Color FAVORITE_YELLOW = fl_rgb_color(255, 215, 0); // Gold

    const int BORDER_RADIUS = 8;
}

#endif
