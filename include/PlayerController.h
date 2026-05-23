#ifndef PLAYERCONTROLLER_H
#define PLAYERCONTROLLER_H

#include <string>
#include <vector>
#include "YoutubeService.h"

/*
 * PlayerController — Playback queue management
 *
 * Manages the global play_queue, current index, repeat/shuffle
 * state, and the functions that start, advance, and rewind playback.
 * Also owns the periodic UI-update timer that polls mpv for
 * position / duration and auto-advances on EOF.
 */

/* Start playing track at the given queue index */
void play_index(int index);

/* Advance to next track (respects repeat, shuffle, wrap-around) */
void play_next();

/* Go back to previous track (with wrap-around) */
void play_prev();

/* Format seconds to MM:SS string */
std::string format_time(double seconds);

/*
 * Periodic 200ms timer — polls mpv for position / duration,
 * updates the progress bar, handles EOF.
 */
void update_ui_cb(void* data);

#endif // PLAYERCONTROLLER_H
