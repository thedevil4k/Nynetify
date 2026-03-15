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
        
        // Background
        fl_draw_box(FL_FLAT_BOX, x_val, y_val, w_val, h_val, color());

        double max_val = maximum();
        if (max_val > 0) {
            // Draw Buffered Range (Light Grey)
            double b_ratio = buffered_value / max_val;
            if (b_ratio > 1.0) b_ratio = 1.0;
            int bw = (int)(w_val * b_ratio);
            if (bw > 0) {
                fl_draw_box(FL_FLAT_BOX, x_val, y_val, bw, h_val, fl_rgb_color(100, 100, 100));
            }

            // Draw Played Range (Green)
            double p_ratio = (value() - minimum()) / (max_val - minimum());
            if (p_ratio > 1.0) p_ratio = 1.0;
            int pw = (int)(w_val * p_ratio);
            if (pw > 0) {
                fl_draw_box(FL_FLAT_BOX, x_val, y_val, pw, h_val, selection_color());
            }
        }

        // Draw handle (small white line)
        double handle_ratio = (value() - minimum()) / (max_val - minimum());
        int hx = x_val + (int)(w_val * handle_ratio) - 1;
        if (hx < x_val) hx = x_val;
        if (hx > x_val + w_val - 2) hx = x_val + w_val - 2;
        fl_draw_box(FL_FLAT_BOX, hx, y_val, 2, h_val, FL_WHITE);
    }
};

#endif
