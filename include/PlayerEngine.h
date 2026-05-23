#ifndef PLAYERENGINE_H
#define PLAYERENGINE_H

#include <mpv/client.h>
#include <string>
#include <stdexcept>
#include <iostream>

class PlayerEngine {
public:
    PlayerEngine() {
        handle = mpv_create();
        if (!handle) throw std::runtime_error("Failed to create MPV handle");
        
        // Settings for integrated audio playback
        mpv_set_option_string(handle, "vo", "null");       // No video
        // Removing explicit WASAPI to allow MPV to choose best processed path
        // mpv_set_option_string(handle, "ao", "wasapi");     
        mpv_set_option_string(handle, "tls-verify", "no"); // Ignore TLS errors
        
        // Use mpv's internal ytdl support (via yt-dlp)
        mpv_set_option_string(handle, "ytdl", "yes");
        mpv_set_option_string(handle, "ytdl-format", "bestaudio/best");
        
        // Point to the yt-dlp executable
        const char* ytdl_path = "yt-dlp";
        std::string script_opts = "ytdl_hook-ytdl_path=" + std::string(ytdl_path);
        mpv_set_option_string(handle, "script-opts", script_opts.c_str());
        
        // Fix for 403 errors: use android client extractor args
        mpv_set_option_string(handle, "ytdl-raw-options", "extractor-args=youtube:player_client=android");
        
        // Request log messages to debug
        mpv_request_log_messages(handle, "info");

        // Fast, low-latency audio streaming
        mpv_set_option_string(handle, "cache", "yes");
        mpv_set_option_string(handle, "cache-pause-initial", "no");       // Play as soon as first byte arrives
        mpv_set_option_string(handle, "cache-pause", "no");               // Don't stall on minor rebuffers
        mpv_set_option_string(handle, "demuxer-max-bytes", "512KiB");     // Smaller initial buffer (audio only)
        mpv_set_option_string(handle, "demuxer-max-back-bytes", "512KiB");
        mpv_set_option_string(handle, "demuxer-readahead-secs", "5");     // Only read 5s ahead
        mpv_set_option_string(handle, "network-timeout", "5");            // Fail fast on connection loss
        
        if (mpv_initialize(handle) < 0) 
            throw std::runtime_error("Failed to initialize MPV");
        
        std::cout << "[PLAYER] MPV Engine initialized." << std::endl;
    }

    ~PlayerEngine() {
        if (handle) mpv_terminate_destroy(handle);
    }

    void play(const std::string& url) {
        std::cout << "[PLAYER] Loading URL: " << url << std::endl;
        const char* cmd[] = {"loadfile", url.c_str(), "replace", NULL};
        int err = mpv_command(handle, cmd);
        if (err < 0) {
            std::cerr << "[PLAYER] Error loading file: " << mpv_error_string(err) << std::endl;
        }
    }

    void pause() {
        std::cout << "[PLAYER] Paused." << std::endl;
        int pause_val = 1;
        mpv_set_property(handle, "pause", MPV_FORMAT_FLAG, &pause_val);
    }

    void resume() {
        std::cout << "[PLAYER] Resumed." << std::endl;
        int pause_val = 0;
        mpv_set_property(handle, "pause", MPV_FORMAT_FLAG, &pause_val);
    }

    bool is_paused() {
        int paused = 0;
        mpv_get_property(handle, "pause", MPV_FORMAT_FLAG, &paused);
        return paused != 0;
    }

    double get_position() {
        double pos = 0;
        mpv_get_property(handle, "playback-time", MPV_FORMAT_DOUBLE, &pos);
        return pos;
    }

    double get_duration() {
        double dur = 0;
        mpv_get_property(handle, "duration", MPV_FORMAT_DOUBLE, &dur);
        return dur;
    }

    void set_position(double pos) {
        mpv_set_property(handle, "playback-time", MPV_FORMAT_DOUBLE, &pos);
    }

    void set_volume(double vol) {
        mpv_set_property(handle, "volume", MPV_FORMAT_DOUBLE, &vol);
    }

    double get_volume() {
        double vol = 100;
        mpv_get_property(handle, "volume", MPV_FORMAT_DOUBLE, &vol);
        return vol;
    }

    void set_eq_enabled(bool enabled) {
        eq_enabled = enabled;
        apply_eq();
    }

    bool is_eq_enabled() const { return eq_enabled; }

    void set_eq_gain(int band, double gain) {
        if (band < 0 || band >= 10) return;
        eq_gains[band] = gain;
        if (eq_enabled) apply_eq();
    }

    void apply_eq() {
        if (!eq_enabled) {
            mpv_set_property_string(handle, "af", "");
            std::cout << "[PLAYER] EQ Disabled" << std::endl;
            return;
        }

        // Frequencies for a standard 10-band EQ
        double freqs[] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
        
        // Construct chained filter string: equalizer=f=31:g=X,equalizer=f=62:g=Y...
        std::string filter = "";
        for (int i = 0; i < 10; ++i) {
            // Using width_type=o (octaves) with w=1 is standard for graphic EQ
            filter += "equalizer=f=" + std::to_string((int)freqs[i]) + 
                      ":width_type=o:w=1:g=" + std::to_string(eq_gains[i]);
            if (i < 9) filter += ",";
        }
        
        int err = mpv_set_property_string(handle, "af", filter.c_str());
        if (err < 0) {
            std::cerr << "[PLAYER] EQ Filter Error: " << mpv_error_string(err) << std::endl;
        } else {
            std::cout << "[PLAYER] Applied Filter: " << filter << std::endl;
        }
    }

    void set_buffer_size(int mb) {
        std::string bytes = std::to_string(mb * 1024 * 1024);
        mpv_set_option_string(handle, "demuxer-max-bytes", bytes.c_str());
        mpv_set_option_string(handle, "demuxer-max-back-bytes", bytes.c_str());
        std::cout << "[PLAYER] Buffer size set to " << mb << "MB" << std::endl;
    }

    double get_buffer_usage_mb() {
        int64_t bytes = 0;
        mpv_get_property(handle, "demuxer-cache-state/fw-bytes", MPV_FORMAT_INT64, &bytes);
        return (double)bytes / (1024.0 * 1024.0);
    }

    double get_cache_duration() {
        double cache_time = 0;
        mpv_get_property(handle, "demuxer-cache-duration", MPV_FORMAT_DOUBLE, &cache_time);
        return cache_time;
    }

    // Call this to process events and logs.
    // Returns: 0 = no event, 1 = EOF (track ended), 2 = error
    int update() {
        int status = 0;
        while (true) {
            mpv_event *event = mpv_wait_event(handle, 0);
            if (event->event_id == MPV_EVENT_NONE) break;
            
            if (event->event_id == MPV_EVENT_LOG_MESSAGE) {
                auto *log = (mpv_event_log_message *)event->data;
                std::cout << "[MPV LOG] " << log->text;
            } else if (event->event_id == MPV_EVENT_START_FILE) {
                std::cout << "[PLAYER] Start loading file..." << std::endl;
            } else if (event->event_id == MPV_EVENT_PLAYBACK_RESTART) {
                std::cout << "[PLAYER] Playback started/restarted." << std::endl;
            } else if (event->event_id == MPV_EVENT_END_FILE) {
                auto *end_ev = (mpv_event_end_file *)event->data;
                std::cout << "[PLAYER] Playback ended. Reason: " << end_ev->reason << std::endl;
                if (end_ev->reason == MPV_END_FILE_REASON_EOF) {
                    status = 1;
                } else if (end_ev->reason == MPV_END_FILE_REASON_ERROR) {
                    status = 2;
                }
            }
        }
        return status;
    }

private:
    mpv_handle* handle;
    bool eq_enabled = false;
    double eq_gains[10] = {0,0,0,0,0,0,0,0,0,0};
};

#endif
