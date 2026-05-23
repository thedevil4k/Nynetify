#include <algorithm>
#include <cctype>
#include <cstring>
#include "ArtistParser.h"
#include "Globals.h"
#include "Theme.h"
#include "ModernButton.h"

/* ================================================================
 * Artist string utilities
 * ================================================================ */

/* ── Split on collaboration separators ────────────── */
std::vector<std::string> parse_artists(const std::string& author) {
    std::vector<std::string> result;
    if (author.empty()) return result;

    std::string s = author;
    const char* patterns[] = {" ft. ", " feat. ", " Ft. ", " Feat. ", " & ", " x ", " X ", ", "};
    const char* delim = " | ";

    /* Normalise all separators to " | " */
    for (const char* pat : patterns) {
        std::string p(pat);
        size_t pos = 0;
        while ((pos = s.find(p, pos)) != std::string::npos) {
            s.replace(pos, p.length(), delim);
            pos += 3;
        }
    }

    /* Split on " | " */
    std::string d = " | ";
    size_t start = 0, pos;
    while ((pos = s.find(d, start)) != std::string::npos) {
        std::string tok = s.substr(start, pos - start);
        while (!tok.empty() && tok.front() == ' ') tok.erase(0, 1);
        while (!tok.empty() && tok.back()  == ' ') tok.pop_back();
        if (!tok.empty()) result.push_back(tok);
        start = pos + d.length();
    }
    std::string tok = s.substr(start);
    while (!tok.empty() && tok.front() == ' ') tok.erase(0, 1);
    while (!tok.empty() && tok.back()  == ' ') tok.pop_back();
    if (!tok.empty()) result.push_back(tok);

    return result;
}

/* ── Clean YouTube-artifact suffixes ─────────────── */
std::string clean_artist_name(std::string name) {
    /* Strip " — Topic" */
    size_t pos = name.find(" - Topic");
    if (pos != std::string::npos) name = name.substr(0, pos);

    /* Strip " and N more" */
    pos = name.find(" and ");
    if (pos != std::string::npos) {
        std::string tail = name.substr(pos + 5);
        size_t more_pos = tail.find(" more");
        if (more_pos != std::string::npos && more_pos > 0) {
            bool all_digits = true;
            for (size_t j = 0; j < more_pos; ++j)
                if (!std::isdigit(static_cast<unsigned char>(tail[j]))) { all_digits = false; break; }
            if (all_digits) name = name.substr(0, pos);
        }
    }
    /* Strip " & N more" */
    pos = name.find(" & ");
    if (pos != std::string::npos) {
        std::string tail = name.substr(pos + 3);
        size_t more_pos = tail.find(" more");
        if (more_pos != std::string::npos && more_pos > 0) {
            bool all_digits = true;
            for (size_t j = 0; j < more_pos; ++j)
                if (!std::isdigit(static_cast<unsigned char>(tail[j]))) { all_digits = false; break; }
            if (all_digits) name = name.substr(0, pos);
        }
    }
    /* Trim */
    while (!name.empty() && (name.front() == ' ' || name.front() == '\t')) name.erase(0, 1);
    while (!name.empty() && (name.back()  == ' ' || name.back()  == '\t')) name.pop_back();
    return name;
}

/* ── Reject unwanted strings ─────────────────────── */
bool is_valid_artist_name(const std::string& name) {
    if (name.empty() || name.length() > 40) return false;

    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    const char* bad_words[] = {
        "video", "oficial", "official", "lyrics", "letra", "audio",
        "visualizer", "videoclip", "clip", "prod", "remix", "mashup",
        "karaoke", "sub", "subtitulado", "subtitles", "reaccion",
        "official video", "video oficial"
    };
    for (const char* bw : bad_words)
        if (lower.find(bw) != std::string::npos) return false;

    if (lower.find('(') != std::string::npos ||
        lower.find(')') != std::string::npos ||
        lower.find('[') != std::string::npos ||
        lower.find(']') != std::string::npos)
        return false;

    return true;
}

/* ── Extract artists from music-video title ──────── */
std::vector<std::string> extract_artists_from_title(
    const std::string& title,
    const std::string& cleaned_uploader)
{
    std::vector<std::string> results;
    size_t dash_pos = title.find(" - ");
    if (dash_pos == std::string::npos) return results;

    std::string partA = title.substr(0, dash_pos);
    std::string partB = title.substr(dash_pos + 3);
    auto trim = [](std::string& s) {
        while (!s.empty() && s.front() == ' ') s.erase(0, 1);
        while (!s.empty() && s.back()  == ' ') s.pop_back();
    };
    trim(partA);
    trim(partB);

    std::string partA_lower = partA;
    std::string partB_lower = partB;
    std::string uploader_lower = cleaned_uploader;
    auto to_lower = [](std::string& s) {
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c){ return std::tolower(c); });
    };
    to_lower(partA_lower);
    to_lower(partB_lower);
    to_lower(uploader_lower);
    while (!uploader_lower.empty() && uploader_lower.back() == ' ') uploader_lower.pop_back();

    /* Heuristic: whichever side contains the uploader is the artist side */
    bool partA_is_artist = false;
    bool partB_is_artist = false;
    if (!uploader_lower.empty()) {
        if (partA_lower.find(uploader_lower) != std::string::npos)
            partA_is_artist = true;
        else if (partB_lower.find(uploader_lower) != std::string::npos)
            partB_is_artist = true;
    }

    /* Fallback: whichever side lacks video keywords is the artist side */
    if (!partA_is_artist && !partB_is_artist) {
        const char* keywords[] = {"video", "oficial", "official", "lyrics", "letra",
                                  "audio", "visualizer", "videoclip", "remix", "mashup"};
        bool partA_has = false, partB_has = false;
        for (const char* kw : keywords) {
            if (partA_lower.find(kw) != std::string::npos) partA_has = true;
            if (partB_lower.find(kw) != std::string::npos) partB_has = true;
        }
        if (partA_has && !partB_has)      partB_is_artist = true;
        else if (partB_has && !partA_has) partA_is_artist = true;
    }

    if (!partA_is_artist && !partB_is_artist) partA_is_artist = true; /* default */

    std::string artist_str = partA_is_artist ? partA : partB;
    std::string title_str  = partA_is_artist ? partB : partA;

    /* Parse the artist side for collaborations */
    std::vector<std::string> parsed = parse_artists(artist_str);
    for (const auto& p : parsed) {
        std::string cleaned = clean_artist_name(p);
        if (is_valid_artist_name(cleaned)) results.push_back(cleaned);
    }

    /* Extract "ft." / "feat." features from the song side */
    const char* ft_keywords[] = {" ft. ", " feat. ", " Ft. ", " Feat. "};
    for (const char* ft : ft_keywords) {
        size_t feat_pos = title_str.find(ft);
        if (feat_pos != std::string::npos) {
            std::string feat_part = title_str.substr(feat_pos + strlen(ft));
            while (!feat_part.empty() && feat_part.front() == ' ') feat_part.erase(0, 1);
            if (!feat_part.empty() && feat_part.back() == ')') feat_part.pop_back();
            if (!feat_part.empty() && feat_part.back() == ']') feat_part.pop_back();
            std::vector<std::string> feat_parsed = parse_artists(feat_part);
            for (const auto& p : feat_parsed) {
                std::string cleaned = clean_artist_name(p);
                if (is_valid_artist_name(cleaned)) results.push_back(cleaned);
            }
            break;
        }
    }
    return results;
}

/* ── Artist selector dialog ──────────────────────── */
int show_artist_selector(const std::vector<std::string>& artists) {
    int result = -1;
    int h = 70 + (int)artists.size() * 42;
    int height = std::max(150, std::min(h, 400));

    Fl_Double_Window win(300, height, lang->view_channel);
    win.color(Theme::SIDEBAR);

    struct SelData { int idx; int* res; Fl_Window* w; };

    for (size_t i = 0; i < artists.size(); i++) {
        auto* btn = new ModernButton(20, 20 + (int)i * 42, 260, 34, artists[i].c_str());
        btn->color(Theme::HOVER);
        btn->labelcolor(Theme::TEXT_PRIMARY);
        btn->labelsize(13);
        auto* sd = new SelData{(int)i, &result, &win};
        btn->callback([](Fl_Widget* w, void* d) {
            auto* sd = (SelData*)d;
            *sd->res = sd->idx;
            sd->w->hide();
            delete sd;
        }, sd);
    }

    win.end();
    if (Fl::first_window())
        win.position(Fl::first_window()->x() + (Fl::first_window()->w() - win.w()) / 2,
                     Fl::first_window()->y() + (Fl::first_window()->h() - win.h()) / 2);
    win.show();
    while (win.shown()) Fl::wait();
    return result;
}
