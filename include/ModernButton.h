#ifndef MODERNBUTTON_H
#define MODERNBUTTON_H

#include <FL/Fl_Button.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Image.H>
#include "Theme.h"
#include "AppSettings.h"

extern AppSettings settings;

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

    void set_image(Fl_Image* img) {
        m_image = img;
        if (m_image) {
            int iw = m_image->w();
            int ih = m_image->h();
            int bw = w() - 8;
            int bh = h() - 8;
            int scale = bw < bh ? bw : bh;
            if (iw > scale || ih > scale) {
                int nw = scale;
                int nh = scale * ih / iw;
                if (nh > bh) { nh = bh; nw = bw * iw / ih; }
                m_scaled = m_image->copy(nw, nh);
            } else {
                m_scaled = m_image->copy(iw, ih);
            }
        }
        redraw();
    }

protected:
    Fl_Image* m_image  = nullptr;
    Fl_Image* m_scaled = nullptr;

    void draw() override {
        fl_antialias(settings.enableAntialiasing ? 1 : 0);
        bool is_below = (Fl::belowmouse() == this);
        bool is_pushed = value() || (Fl::pushed() == this && is_below);

        // Highlight logic
        Fl_Color bg = color();
        if (is_below) bg = Theme::HOVER;
        if (is_pushed) bg = Theme::ACCENT;
        if (color() == Theme::ACCENT) {
             bg = is_below ? fl_lighter(Theme::ACCENT) : Theme::ACCENT;
             if (is_pushed) bg = fl_darker(Theme::ACCENT);
        }

        // Draw background
        fl_color(bg);
        fl_draw_box(FL_RFLAT_BOX, x(), y(), w(), h(), bg);

        if (m_scaled) {
            // Draw image centered
            int ix = x() + (w() - m_scaled->w()) / 2;
            int iy = y() + (h() - m_scaled->h()) / 2;
            m_scaled->draw(ix, iy);
        } else {
            // Draw label
            fl_color(labelcolor());
            if (is_pushed) fl_color(fl_lighter(labelcolor()));
            fl_font(labelfont(), labelsize());
            fl_draw(label(), x(), y(), w(), h(), align(), 0, 0);
        }
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
