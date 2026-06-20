# Nynetify

> A desktop YouTube audio player built with FLTK and mpv.

Search YouTube, browse playlists and channels, create local playlists, and download songs as MP3 — all in a lightweight native app.

## Features

- **YouTube search** — songs, playlists, and channels via yt-dlp
- **Playback** — queue management, shuffle, repeat, seek, volume
- **Local playlists** — create, manage, and persist your own playlists
- **Download** — save any track as MP3 via yt-dlp
- **Channel view** — browse channel content, view artist pages
- **Equalizer** — 10-band EQ with toggle
- **Multi-language** — English / Spanish interface with runtime toggle
- **Custom dark theme** — dark UI with rounded modern widgets
- **Cross-platform** — Windows x64, Windows ARM64, Linux x86-64 and i386 (DEB/RPM)

## Architecture

Nynetify is a C++20 desktop application organized in four layers:

```
┌────────────────────────────────────────────────────────────────┐
│                        Widget Layer (FLTK)                      │
│  ModernButton  ModernChoice  ModernSlider  ProgressSlider       │
│  CircularButton  HeartButton  ResultsBrowser  CreatePlaylistWin │
└──────────────────────────┬─────────────────────────────────────┘
                           │ callbacks
┌──────────────────────────▼─────────────────────────────────────┐
│                      Controller Layer                           │
│  AppCallbacks.cpp  (search, playback, prefs, download, EQ)     │
│  PlayerController.cpp  (queue, play next/prev, UI timer)       │
└──────────────────────────┬─────────────────────────────────────┘
                           │ view switching
┌──────────────────────────▼─────────────────────────────────────┐
│                        View Layer                               │
│  ViewManager.cpp  (home/search/playlist/channel views,         │
│                    async cover art, region detection)           │
└──────────────────────────┬─────────────────────────────────────┘
                           │ services
┌──────────────────────────▼─────────────────────────────────────┐
│                      Service Layer                              │
│  YoutubeService.h   →  InvidiousClient (curl + JSON)            │
│                    ↘  yt-dlp CLI (subprocess)                   │
│  PlayerEngine.h     →  libmpv (audio playback)                  │
│  PlaylistManager.h  →  File I/O (FAVS/ + PLAYLIST/)            │
│  ArtistParser.cpp   →  Multi-artist extraction heuristics       │
└──────────────────────────┬─────────────────────────────────────┘
                           │ data
┌──────────────────────────▼─────────────────────────────────────┐
│                        Data Layer                               │
│  SearchResult.h   AppSettings.h/.cpp   Lang.h   Theme.h        │
│  settings.cfg (persist)   FAVS/*.txt   PLAYLIST/*.txt          │
└────────────────────────────────────────────────────────────────┘
```

### Search Pipeline

1. User types query + Enter → `search_cb()` in `AppCallbacks.cpp`
2. `YoutubeService::search()` routes to **Invidious API** first (if enabled & filter is `Everything` or `Songs`):
   - `InvidiousClient::search()` → HTTP GET to `iv.datura.dev/api/v1/search` → JSON parse → `vector<SearchResult>`
3. Falls back to **yt-dlp CLI** for empty results or other filter types (`Playlists`/`Channels`)
4. Browser populated with first 2 results, then 3 more every 50ms via `progressive_fill_cb`
5. "Show more" mock-item appended once viewport is filled

### Playback Pipeline

1. Double-click or Play button → `play_selected_cb()` / `play_btn_cb()`
2. `PlayerController::play_index()` updates now-playing labels, heart state, starts async cover art
3. `PlayerEngine::play(url)` feeds the YouTube URL to **mpv** (which resolves via its built-in ytdl hook)
4. `update_ui_cb` fires every 200ms: polls mpv position/duration, updates progress bar + time labels
5. On EOF: auto-advance to next track (respecting shuffle, repeat, and wrap-around)

### UI Views

- **Home** — 6 category cards (2×3 grid) + "Now Playing" featured section with cover art
- **Search** — Input bar + filter dropdown + `ResultsBrowser` (full height)
- **Playlist / Channel** — Header banner (cover, name, description) + `ResultsBrowser` below
- **Player Bar** — Fixed at bottom: cover art, track title, heart, shuffle/prev/play/next/repeat, progress, volume, EQ, download
- **Status Bar** — RAM, buffer, region, clock

### Persistence

- `AppSettings` → `settings.cfg` (key=value format next to the executable)
- `PlaylistManager` → `FAVS/favs.txt` (favorites) + `PLAYLIST/*.txt` (custom playlists) — both plain text
- `Lang.h` stores all UI strings inline for English and Spanish; a global `lang` pointer toggles between them

### System Tray (Windows)

The `SystemTray` class intercepts `WM_CLOSE` via window subclassing to minimize to tray instead of exiting. Tray icon is created with `Shell_NotifyIconA`; double-click restores the window, right-click shows a context menu (Show / Exit). On Linux, the class compiles to empty stubs.

## Downloads
| Windows | x86-64 | [Installer](https://github.com/anomalyco/Nynetify/releases/latest) / [Portable ZIP](https://github.com/anomalyco/Nynetify/releases/latest) |
| Windows | ARM64 | [Installer](https://github.com/anomalyco/Nynetify/releases/latest) / [Portable ZIP](https://github.com/anomalyco/Nynetify/releases/latest) |
| Linux | x86-64 | [.deb](https://github.com/anomalyco/Nynetify/releases/latest) / [.rpm](https://github.com/anomalyco/Nynetify/releases/latest) |
| Linux | i386 | [.deb](https://github.com/anomalyco/Nynetify/releases/latest) / [.rpm](https://github.com/anomalyco/Nynetify/releases/latest) |

Grab the latest build from the [Releases page](https://github.com/anomalyco/Nynetify/releases/latest) or the Actions tab.

## Screenshots

*(Coming soon)*

## Dependencies

### Runtime
- [mpv](https://mpv.io/) — playback engine
- [yt-dlp](https://github.com/yt-dlp/yt-dlp) — YouTube metadata & audio extraction

### Build
- C++20 compiler (GCC, Clang, MSVC)
- [CMake](https://cmake.org/) ≥ 3.10
- [FLTK](https://www.fltk.org/) 1.4+
- libmpv (development headers)
- Python 3 + Pillow (for icon generation)

## Build

### Windows (MSYS2)

```bash
# Install dependencies
pacman -S mingw-w64-x86_64-{cmake,ninja,fltk,mpv,python-pillow}

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/Nynetify.exe
```

For ARM64, replace `x86_64` with `clang-aarch64` and use `CLANGARM64` MSYS2 environment.

### Linux (Ubuntu/Debian)

```bash
sudo apt install cmake build-essential libfltk1.3-dev libmpv-dev python3-pip python3-pil
pip3 install yt-dlp --break-system-packages

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build

# Package as .deb
cd build && cpack -G DEB
```

### Linux (Fedora/RHEL)

```bash
sudo dnf install cmake gcc-c++ fltk-devel mpv-libs-devel python3-pip python3-pillow rpm-build
pip3 install yt-dlp --break-system-packages

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build

# Package as .rpm
cd build && cpack -G RPM
```

## License

MIT
