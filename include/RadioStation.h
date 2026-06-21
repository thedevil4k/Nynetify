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
};

inline std::vector<RadioStation> bundled_stations = {
    // ── Spain ──
    {0,  "Loca FM Drum & Bass",  "https://locafm-dnce.uniqcast.com/locafm_dnce",   "Drum & Bass", "ES", "Spain", "MP3", 192},
    {1,  "Loca FM Hardstyle",    "https://locafm-we.uniqcast.com/locafm_we",       "Hardstyle",    "ES", "Spain", "MP3", 192},
    {2,  "Loca FM Techno",       "https://locafm-we.uniqcast.com/locafm_ww",       "Techno",       "ES", "Spain", "MP3", 192},
    {3,  "Loca FM House",        "https://locafm-we.uniqcast.com/locafm_hot",      "House",        "ES", "Spain", "MP3", 192},
    {4,  "Loca FM Trance",       "https://locafm-we.uniqcast.com/locafm_trance",   "Trance",       "ES", "Spain", "MP3", 192},
    {5,  "Loca FM Chill",        "https://locafm-we.uniqcast.com/locafm_xx",       "Chill",        "ES", "Spain", "MP3", 192},
    {6,  "Los 40 Principales",   "https://playerservices.streamtheworld.com/api/livestream-redirect/LOS40.mp3",   "Pop",    "ES", "Spain", "MP3", 128},
    {7,  "Cadena SER",           "https://playerservices.streamtheworld.com/api/livestream-redirect/CADENASER.mp3", "Talk", "ES", "Spain", "MP3", 128},
    {8,  "Radio 3 (RNE)",        "https://rtvelivestream.rtve.es/radio3/radio3_main.m3u8", "Cultural", "ES", "Spain", "AAC", 192},
    {9,  "Europa FM",            "https://ns100.emisionlocal.com:9030/stream",      "Pop",    "ES", "Spain", "MP3", 128},
    {10, "Flaix FM",             "https://flaixfm.streaming-pro.com:8006/flaixfm.mp3", "Dance", "ES", "Spain", "MP3", 128},
    {11, "RAC 105",              "https://streaming.rac105.cat/rac105-128-mp3",     "Pop",    "ES", "Spain", "MP3", 128},
    {12, "Radiolé",              "https://playerservices.streamtheworld.com/api/livestream-redirect/RADIOLE.mp3", "Spanish", "ES", "Spain", "MP3", 128},
    {13, "Máxima FM",            "https://playerservices.streamtheworld.com/api/livestream-redirect/MAXIMA.mp3", "Dance", "ES", "Spain", "MP3", 128},
    {14, "Kiss FM",              "https://kissfm.kissfmradio.cires21.com/kissfm.mp3","Pop",    "ES", "Spain", "MP3", 128},
    // ── United Kingdom ──
    {15, "BBC Radio 1",          "https://stream.live.vc.bbcmedia.co.uk/bbc_radio_one",   "Pop",       "UK", "United Kingdom", "MP3", 128},
    {16, "BBC Radio 6 Music",    "https://stream.live.vc.bbcmedia.co.uk/bbc_6music",      "Indie",     "UK", "United Kingdom", "MP3", 128},
    {17, "Jazz FM",              "https://edge-bauerall-01-gos2.sharp-stream.com/jazzfm.mp3", "Jazz",  "UK", "United Kingdom", "MP3", 128},
    {18, "Rinse FM",             "https://streamer-uk.rinse.fm:8443/stream",               "Electronic","UK", "United Kingdom", "MP3", 192},
    {19, "NTS Radio",            "https://stream-relay-geo.ntslive.net/stream",             "Underground","UK","United Kingdom", "MP3", 128},
    // ── United States ──
    {20, "Radio Paradise",       "https://stream.radioparadise.com/ogg-192",               "Eclectic",  "US", "United States", "OGG", 192},
    {21, "KEXP",                 "https://kexp.streamguys1.com/kexp160.aac",                "Indie",     "US", "United States", "AAC", 160},
    {22, "NPR News",             "https://npr.streamguys1.com/npr-mp3-128",                 "Talk",      "US", "United States", "MP3", 128},
    {23, "SomaFM Groove Salad",  "https://ice.somafm.com/groovesalad",                      "Chill",     "US", "United States", "MP3", 128},
    {24, "SomaFM Bassdrive",     "https://ice.somafm.com/bassdrive",                        "Drum & Bass","US","United States", "MP3", 128},
    {25, "WNYC",                 "https://fm939.wnyc.org/wnycfm-web",                       "Talk",      "US", "United States", "MP3", 128},
    // ── France ──
    {26, "FIP",                  "https://icecast.radiofrance.fr/fip-midfi.mp3",            "Eclectic",  "FR", "France", "MP3", 128},
    {27, "France Inter",         "https://icecast.radiofrance.fr/franceinter-midfi.mp3",    "Talk",      "FR", "France", "MP3", 128},
    // ── Germany ──
    {28, "N-JOY",                "https://ndr-edge-30a8-dus-lg-cdn.cast.addradio.de/ndr/njoy/live/mp3/128/stream.mp3", "Pop", "DE", "Germany", "MP3", 128},
    {29, "Deutschlandfunk",      "https://st01.sslstream.dlf.de/dlf/01/high/aac/stream.aac", "Talk",    "DE", "Germany", "AAC", 128},
    // ── Italy ──
    {30, "Radio Deejay",         "https://sphera.fluidstream.net/rdeejay.mp3",              "Pop",       "IT", "Italy", "MP3", 128},
    {31, "Virgin Radio Italy",   "https://icecast.radioitalia.it/radio/8013/virginradio.mp3",  "Pop",       "IT", "Italy", "MP3", 128},
    // ── Japan ──
    {32, "NHK FM",               "https://radio-stream.nhk.jp/hls/live/2023223/nhkradiruakfm/master.m3u8", "Pop", "JP", "Japan", "AAC", 128},
    // ── Australia ──
    {33, "Triple J",             "https://live-radio02.mediahubaustralia.com/2TJW/mp3/",    "Indie",     "AU", "Australia", "MP3", 128},
    // ── Netherlands ──
    {34, "Radio 538",            "https://playerservices.streamtheworld.com/api/livestream-redirect/RADIO538.mp3", "Pop", "NL", "Netherlands", "MP3", 128},
    // ── Canada ──
    {35, "CBC Music",            "https://cbcradiolive.akamaized.net/hls/live/2045973/ES_R2_EER/master.m3u8",    "Pop", "CA", "Canada", "AAC", 128},
};

#endif
