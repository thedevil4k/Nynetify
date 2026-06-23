#ifndef RADIOSTATION_H
#define RADIOSTATION_H

#include <string>
#include <vector>

struct RadioStation {
    int id;
    std::string name;
    std::string stream_url;
    std::string genre;
    std::string country_code;
    std::string country_name;
    std::string codec;
    int bitrate;
    bool is_custom = false;
    std::string logo;  // path to local PNG, empty if none
};

inline std::vector<RadioStation> bundled_stations = {
    // ── Spain — Loca FM ──
    {0,  "Loca FM Live",          "https://s3.we4stream.com:2020/stream/locafm",              "Live",        "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {1,  "Loca FM House",         "https://s2.we4stream.com/listen/loca_house/live",           "House",       "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {2,  "Loca FM Techno",        "https://s2.we4stream.com/listen/loca_techo/live",           "Techno",      "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {3,  "Loca FM Trance",        "https://s2.we4stream.com/listen/loca_trance/live",          "Trance",      "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {4,  "Loca FM Dance",         "https://s2.we4stream.com/listen/loca_dance/live",           "Dance",       "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {5,  "Loca FM Chill Out",     "https://s2.we4stream.com/listen/loca_chill_out/live",       "Chill Out",   "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {6,  "Loca FM Deep House",    "https://s2.we4stream.com/listen/loca_deep_house/live",      "Deep House",  "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {7,  "Loca FM Drum & Bass",   "https://s2.we4stream.com/listen/loca_drum__bass/live",      "Drum & Bass", "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {8,  "Loca FM Hard",          "https://s2.we4stream.com/listen/loca_hard/live",            "Hard",        "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {9,  "Loca FM Remember",      "https://s2.we4stream.com/listen/loca_remember/live",        "Remember",    "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {10, "Loca FM Tech House",    "https://s2.we4stream.com/listen/loca_tech_house/live",      "Tech House",  "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {11, "Loca FM Big Room",      "https://s2.we4stream.com/listen/loca_big_room/live",        "Big Room",    "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {12, "Loca FM Sessions",      "https://s2.we4stream.com/listen/loca_sessions/live",        "Sessions",    "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {13, "Loca FM Ambient",       "https://s2.we4stream.com/listen/loca_ambient/live",         "Ambient",     "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {14, "Loca FM 80's",          "https://s2.we4stream.com/listen/loca_80s/live",             "80's",        "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {15, "Loca FM 90's",          "https://s2.we4stream.com/listen/loca_90s/live",             "90's",        "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {16, "Loca FM Melodic House", "https://s2.we4stream.com/listen/loca_melodic_house/live",   "Melodic House","ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {17, "Loca FM Melodic Techno","https://s2.we4stream.com/listen/loca_melodic_techno/live",  "Melodic Techno","ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {18, "Loca FM Hard Techno",   "https://s2.we4stream.com/listen/loca_hard_techno/live",    "Hard Techno", "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {19, "Loca FM Industrial",    "https://s2.we4stream.com/listen/loca_industrial/live",      "Industrial",  "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    {20, "Loca FM Urban",         "https://s1.we4stream.com:2020/stream/locaurban",             "Urban",       "ES", "Spain", "MP3", 192, false, "Loca_FM.png"},
    // ── Spain — Other ──
    {21, "Los 40 Principales",   "https://playerservices.streamtheworld.com/api/livestream-redirect/LOS40.mp3",   "Pop",    "ES", "Spain", "MP3", 128, false, ""},
    {22, "Cadena SER",           "https://playerservices.streamtheworld.com/api/livestream-redirect/CADENASER.mp3", "Talk", "ES", "Spain", "MP3", 128, false, "Cadena_SER.png"},
    {23, "Radio 3 (RNE)",        "https://rtvelivestream.rtve.es/radio3/radio3_main.m3u8", "Cultural", "ES", "Spain", "AAC", 192, false, ""},
    {24, "Europa FM",            "https://ns100.emisionlocal.com:9030/stream",      "Pop",    "ES", "Spain", "MP3", 128, false, "Europa_FM.png"},
    {25, "Flaix FM",             "https://stream.flaixfm.cat/icecast",                                            "Dance", "ES", "Spain", "AAC", 125, false, "flaixfm.png"},
    {26, "RAC 105",              "https://streaming.rac105.cat/rac105-128-mp3",     "Pop",    "ES", "Spain", "MP3", 128, false, "RAC 105.png"},
    {27, "Radiolé",              "https://playerservices.streamtheworld.com/api/livestream-redirect/RADIOLE.mp3", "Spanish", "ES", "Spain", "MP3", 128, false, "Radiole.png"},
    {28, "Kiss FM",              "https://kissfm.kissfmradio.cires21.com/kissfm.mp3","Pop",    "ES", "Spain", "MP3", 128, false, "Kiss_FM.png"},
    // ── United Kingdom ──
    {30, "BBC Radio 1",          "https://stream.live.vc.bbcmedia.co.uk/bbc_radio_one",   "Pop",       "UK", "United Kingdom", "MP3", 128, false, ""},
    {31, "BBC Radio 6 Music",    "https://stream.live.vc.bbcmedia.co.uk/bbc_6music",      "Indie",     "UK", "United Kingdom", "MP3", 128, false, ""},
    {32, "Jazz FM",              "https://edge-bauerall-01-gos2.sharp-stream.com/jazzfm.mp3", "Jazz",  "UK", "United Kingdom", "MP3", 128, false, ""},
    {33, "Rinse FM",             "https://streamer-uk.rinse.fm:8443/stream",               "Electronic","UK", "United Kingdom", "MP3", 192, false, ""},
    {34, "NTS Radio",            "https://stream-relay-geo.ntslive.net/stream",             "Underground","UK","United Kingdom", "MP3", 128, false, ""},
    // ── United States ──
    {35, "Radio Paradise",       "https://stream.radioparadise.com/ogg-192",               "Eclectic",  "US", "United States", "OGG", 192, false, ""},
    {36, "KEXP",                 "https://kexp.streamguys1.com/kexp160.aac",                "Indie",     "US", "United States", "AAC", 160, false, ""},
    {37, "NPR News",             "https://npr.streamguys1.com/npr-mp3-128",                 "Talk",      "US", "United States", "MP3", 128, false, ""},
    {38, "SomaFM Groove Salad",  "https://ice.somafm.com/groovesalad",                      "Chill",     "US", "United States", "MP3", 128, false, ""},
    {39, "SomaFM Bassdrive",     "https://ice.somafm.com/bassdrive",                        "Drum & Bass","US","United States", "MP3", 128, false, ""},
    {40, "WNYC",                 "https://fm939.wnyc.org/wnycfm-web",                       "Talk",      "US", "United States", "MP3", 128, false, ""},
    // ── France ──
    {41, "FIP",                  "https://icecast.radiofrance.fr/fip-midfi.mp3",            "Eclectic",  "FR", "France", "MP3", 128, false, ""},
    {42, "France Inter",         "https://icecast.radiofrance.fr/franceinter-midfi.mp3",    "Talk",      "FR", "France", "MP3", 128, false, ""},
    // ── Germany ──
    {43, "N-JOY",                "https://ndr-edge-30a8-dus-lg-cdn.cast.addradio.de/ndr/njoy/live/mp3/128/stream.mp3", "Pop", "DE", "Germany", "MP3", 128, false, ""},
    {44, "Deutschlandfunk",      "https://st01.sslstream.dlf.de/dlf/01/high/aac/stream.aac", "Talk",    "DE", "Germany", "AAC", 128, false, ""},
    // ── Italy ──
    {45, "Radio Deejay",         "https://sphera.fluidstream.net/rdeejay.mp3",              "Pop",       "IT", "Italy", "MP3", 128, false, ""},
    {46, "Virgin Radio Italy",   "https://icecast.radioitalia.it/radio/8013/virginradio.mp3",  "Pop",       "IT", "Italy", "MP3", 128, false, ""},
    // ── Japan ──
    {47, "NHK FM",               "https://radio-stream.nhk.jp/hls/live/2023223/nhkradiruakfm/master.m3u8", "Pop", "JP", "Japan", "AAC", 128, false, ""},
    // ── Australia ──
    {48, "Triple J",             "https://live-radio02.mediahubaustralia.com/2TJW/mp3/",    "Indie",     "AU", "Australia", "MP3", 128, false, ""},
    // ── Netherlands ──
    {49, "Radio 538",            "https://playerservices.streamtheworld.com/api/livestream-redirect/RADIO538.mp3", "Pop", "NL", "Netherlands", "MP3", 128, false, ""},
    // ── Canada ──
    {50, "CBC Music",            "https://cbcradiolive.akamaized.net/hls/live/2045973/ES_R2_EER/master.m3u8",    "Pop", "CA", "Canada", "AAC", 128, false, ""},
    // ── Spain — Added ──
    {51, "Onda Cero",            "https://playerservices.streamtheworld.com/api/livestream-redirect/ONDACERO.mp3",  "Talk", "ES", "Spain", "MP3", 128, false, "ondacero.png"},
    {52, "Cadena COPE",          "https://playerservices.streamtheworld.com/api/livestream-redirect/COPE.mp3",      "Talk", "ES", "Spain", "MP3", 128, false, "cope.png"},
    {53, "Los 40 Classic",       "https://playerservices.streamtheworld.com/api/livestream-redirect/LOS40_CLASSIC.mp3", "Pop", "ES", "Spain", "MP3", 128, false, ""},
    {55, "Los 40 Dance",         "https://playerservices.streamtheworld.com/api/livestream-redirect/LOS40_DANCE.mp3",  "Dance","ES", "Spain", "MP3", 128, false, ""},
};

#endif
