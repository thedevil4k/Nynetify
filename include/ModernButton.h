#ifndef MODERNBUTTON_H
#define MODERNBUTTON_H

#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include "Theme.h"

class ModernButton : public Fl_Button {
public:
    ModernButton(int x, int y, int w, int h, const char* l = 0) 
        : Fl_Button(x, y, w, h, l) {
        box(FL_FLAT_BOX);
        color(Theme::BACKGROUND);
        labelcolor(Theme::TEXT_PRIMARY);
        labelfont(FL_HELVETICA_BOLD);
        labelsize(14);
    }

protected:
    void draw() override {
        bool is_below = (Fl::belowmouse() == this);
        bool is_pushed = value() || (Fl::pushed() == this && is_below);

        // Draw an elegant rounded box
        // We use a custom drawing for a truly modern feel
        int r = Theme::BORDER_RADIUS;
        
        // Highlight logic
        Fl_Color bg = color();
        if (is_below) bg = Theme::HOVER;
        if (is_pushed) bg = Theme::ACCENT;
        if (color() == Theme::ACCENT) { // If it's a primary button
             bg = is_below ? fl_lighter(Theme::ACCENT) : Theme::ACCENT;
             if (is_pushed) bg = fl_darker(Theme::ACCENT);
        }

        // Draw background
        fl_color(bg);
        fl_draw_box(FL_RFLAT_BOX, x(), y(), w(), h(), bg); // RFLAT_BOX for rounded corners if supported or custom
        
        // Manual rounded rect drawing if RFLAT_BOX is not enough
        // For FLTK simplicity, we'll stick to a clean flat style with colored borders or highlights
        
        // Draw label
        fl_color(labelcolor());
        if (is_pushed) fl_color(fl_lighter(labelcolor()));
        fl_font(labelfont(), labelsize());
        fl_draw(label(), x(), y(), w(), h(), align(), 0, 0);
    }

    int handle(int event) override {
        int ret = Fl_Button::handle(event);
        if (event == FL_ENTER || event == FL_LEAVE || event == FL_PUSH || event == FL_RELEASE) {
            redraw();
        }
        return ret;
    }
};

#endif
