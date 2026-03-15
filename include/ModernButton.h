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
        // Draw background with rounded corners
        fl_color(value() ? Theme::HOVER : (Fl::belowmouse() == this ? Theme::HOVER : Theme::BACKGROUND));
        fl_draw_box(FL_FLAT_BOX, x(), y(), w(), h(), Theme::BACKGROUND);
        
        // Custom drawing for a more premium look
        int r = Theme::BORDER_RADIUS;
        fl_begin_complex_polygon();
        fl_color(Theme::ACCENT);
        // Simplified rounded rect drawing
        fl_rectf(x(), y(), w(), h(), r); 
        fl_end_complex_polygon();

        // Draw label
        fl_color(labelcolor());
        fl_font(labelfont(), labelsize());
        fl_draw(label(), x(), y(), w(), h(), align(), 0, 0);
    }

    int handle(int event) override {
        int ret = Fl_Button::handle(event);
        if (event == FL_ENTER || event == FL_LEAVE) {
            redraw();
        }
        return ret;
    }
};

#endif
