#ifndef MODERNCHOICE_H
#define MODERNCHOICE_H

#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>
#include "Theme.h"
#include "AppSettings.h"

extern AppSettings settings;

class ModernChoice : public Fl_Choice {
public:
    ModernChoice(int X, int Y, int W, int H, const char* L = 0)
        : Fl_Choice(X, Y, W, H, L) {
        color(Theme::HOVER);
        textcolor(Theme::TEXT_PRIMARY);
        textsize(14);
        labelfont(FL_HELVETICA_BOLD);
        down_box(FL_FLAT_BOX);
    }

protected:
    void draw() override {
        fl_antialias(settings.enableAntialiasing ? 1 : 0);
        bool is_below = (Fl::belowmouse() == this);
        bool is_pushed = (Fl::pushed() == this);

        Fl_Color bg = color();
        if (is_pushed) bg = fl_darker(bg);
        else if (is_below) bg = fl_rgb_color(55, 55, 55);

        fl_color(bg);
        fl_draw_box(FL_RFLAT_BOX, x(), y(), w(), h(), bg);

        fl_color(textcolor());
        fl_font(textfont(), textsize());
        const char* txt = text();
        if (!txt) txt = "";
        fl_draw(txt, x() + 8, y(), w() - 28, h(), FL_ALIGN_LEFT | FL_ALIGN_INSIDE, 0, 0);

        int cx = x() + w() - 11;
        int cy = y() + h() / 2;
        int s = 4;
        fl_color(Theme::TEXT_SECONDARY);
        fl_polygon(cx - s, cy - s/2, cx + s, cy - s/2, cx, cy + s/2 + 1);
    }

    int handle(int event) override {
        int ret = Fl_Choice::handle(event);
        if (event == FL_ENTER || event == FL_LEAVE || event == FL_PUSH || event == FL_RELEASE)
            redraw();
        return ret;
    }
};

#endif
