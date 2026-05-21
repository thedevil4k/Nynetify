#ifndef MODERNSLIDER_H
#define MODERNSLIDER_H

#include <FL/Fl_Slider.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <algorithm>
#include "Theme.h"

class ModernSlider : public Fl_Slider {
public:
    ModernSlider(int X, int Y, int W, int H, const char* L = 0)
        : Fl_Slider(X, Y, W, H, L) {
        box(FL_FLAT_BOX);
        color(Theme::HOVER);
        selection_color(Theme::ACCENT);
    }

    int handle(int event) override {
        if (event == FL_MOUSEWHEEL) {
            int dy = Fl::event_dy();
            double step_val = step();
            if (step_val == 0) {
                double r = maximum() - minimum();
                if (r < 0) r = -r;
                step_val = !is_horiz() ? 1.0 : (r / 20.0);
            }

            double newVal = value();
            if (dy < 0) {
                newVal += step_val;
            } else if (dy > 0) {
                newVal -= step_val;
            }

            double min_b = minimum();
            double max_b = maximum();
            if (min_b > max_b) std::swap(min_b, max_b);
            if (newVal < min_b) newVal = min_b;
            if (newVal > max_b) newVal = max_b;

            if (newVal != value()) {
                value(newVal);
                do_callback();
                redraw();
            }
            return 1;
        }
        return Fl_Slider::handle(event);
    }

protected:
    bool is_horiz() {
        int t = type();
        return t == FL_HOR_SLIDER || t == FL_HOR_FILL_SLIDER || t == FL_HOR_NICE_SLIDER;
    }

    void draw() override {
        int sx = x(), sy = y(), sw = w(), sh = h();
        bool horiz = is_horiz();

        double min = minimum();
        double max = maximum();
        double val = value();
        bool inverted = min > max;
        if (inverted) std::swap(min, max);
        double range = max - min;
        double ratio = (range > 0) ? (val - min) / range : 0;
        if (ratio < 0) ratio = 0;
        if (ratio > 1) ratio = 1;

        bool invert_ratio = horiz ? inverted : !inverted;
        if (invert_ratio) ratio = 1.0 - ratio;

        int hd = (horiz ? sh : sw) + 4;
        if (hd < 12) hd = 12;
        if (hd > 22) hd = 22;
        int hr = hd / 2;

        fl_color(color());
        fl_draw_box(FL_RFLAT_BOX, sx, sy, sw, sh, color());

        if (horiz) {
            int fw = (int)(sw * ratio);
            if (fw > 0) {
                fl_push_clip(sx, sy, fw, sh);
                fl_draw_box(FL_RFLAT_BOX, sx, sy, sw, sh, selection_color());
                fl_pop_clip();
            }
        } else {
            int fh = (int)(sh * ratio);
            if (fh > 0) {
                fl_push_clip(sx, sy + sh - fh, sw, fh);
                fl_draw_box(FL_RFLAT_BOX, sx, sy, sw, sh, selection_color());
                fl_pop_clip();
            }
        }

        int hx, hy;
        if (horiz) {
            hx = sx + (int)((sw - 1) * ratio);
            hy = sy + sh / 2;
        } else {
            hx = sx + sw / 2;
            hy = sy + (int)((sh - 1) * (1.0 - ratio));
        }

        bool is_below = (Fl::belowmouse() == this);
        fl_color(is_below ? FL_WHITE : fl_rgb_color(200, 200, 200));
        fl_pie(hx - hr, hy - hr, hd, hd, 0, 360);

        draw_label();
    }
};

#endif