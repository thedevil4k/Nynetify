#ifndef PROGRESSSLIDER_H
#define PROGRESSSLIDER_H

#include <FL/Fl_Slider.H>
#include <FL/fl_draw.H>
#include "Theme.h"

class ProgressSlider : public Fl_Slider {
    double buffered_value = 0;

public:
    ProgressSlider(int X, int Y, int W, int H, const char* L = 0)
        : Fl_Slider(X, Y, W, H, L) {
        type(FL_HOR_SLIDER);
        box(FL_FLAT_BOX);
        color(Theme::HOVER);
        selection_color(Theme::ACCENT);
    }

    void set_buffered(double v) {
        if (v != buffered_value) {
            buffered_value = v;
            redraw();
        }
    }

    void draw() override {
        int x_val = x(), y_val = y(), w_val = w(), h_val = h();

        double max_val = maximum();
        double handle_ratio = (max_val > 0) ? (value() - minimum()) / (max_val - minimum()) : 0;
        if (handle_ratio < 0) handle_ratio = 0;
        if (handle_ratio > 1) handle_ratio = 1;

        // Background trough (rounded)
        fl_color(color());
        fl_draw_box(FL_RFLAT_BOX, x_val, y_val, w_val, h_val, color());

        if (max_val > 0) {
            // Buffered Range (Light Grey) — rounded with clip
            double b_ratio = buffered_value / max_val;
            if (b_ratio > 1.0) b_ratio = 1.0;
            int bw = (int)(w_val * b_ratio);
            if (bw > 0) {
                fl_push_clip(x_val, y_val, bw, h_val);
                fl_draw_box(FL_RFLAT_BOX, x_val, y_val, w_val, h_val, fl_rgb_color(100, 100, 100));
                fl_pop_clip();
            }

            // Played Range (Green) — rounded with clip
            int pw = (int)(w_val * handle_ratio);
            if (pw > 0) {
                fl_push_clip(x_val, y_val, pw, h_val);
                fl_draw_box(FL_RFLAT_BOX, x_val, y_val, w_val, h_val, selection_color());
                fl_pop_clip();
            }
        }

        // Handle (small circle)
        int hd = h_val + 6;
        if (hd > 14) hd = 14;
        int hr = hd / 2;
        int hx = x_val + (int)(w_val * handle_ratio);
        int hy = y_val + h_val / 2;
        fl_color(FL_WHITE);
        fl_pie(hx - hr, hy - hr, hd, hd, 0, 360);
    }
};

#endif
