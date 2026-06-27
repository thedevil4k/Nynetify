# Nynetify

> A desktop YouTube, Twitch & radio audio player built with FLTK and mpv.

## What is Nynetify?

Nynetify is a lightweight native audio player for desktop. It lets you search and play audio from YouTube and Twitch, listen to online radio stations, create local playlists, and download tracks as MP3 — all without a browser.

## Features

- Search and play audio from **YouTube** (via Invidious API or yt-dlp) and **Twitch** (via GQL API)
- **52 online radio stations** from 12 countries with country filter, favorites, and custom station support
- **Local playlists** — create, organize, and persist playlists as JSON
- **Favorites** — separate favorites for YouTube, Twitch, and Radio
- **Download** — save any track as MP3 via yt-dlp
- **Equalizer** — 10-band EQ with toggle
- **Multi-language** — English / Spanish interface with runtime toggle
- **System tray** — minimizes to tray on close with context menu (Windows)
- **Debug log viewer** — open real-time logs from Settings
- **Dark theme** — custom UI with rounded modern widgets

## Supported Platforms

| Platform | Architecture | Package Format |
|----------|-------------|----------------|
| Windows  | x86-64      | NSIS Installer / Portable ZIP |
| Windows  | ARM64       | NSIS Installer / Portable ZIP |
| Linux    | x86-64      | .deb / .rpm |

## Technologies

### Core Frameworks & Libraries

- **[FLTK](https://www.fltk.org/) 1.4+** — Cross-platform GUI toolkit. All UI widgets, windows, and layout are built on FLTK.
- **[mpv](https://mpv.io/)** — Media player engine. Used as a library via `libmpv` for audio playback, seeking, volume control, and metadata queries.
- **[yt-dlp](https://github.com/yt-dlp/yt-dlp)** — CLI tool for YouTube metadata extraction and audio stream resolution. Invoked as a subprocess.
- **[libcurl](https://curl.se/)** — HTTP client used for Invidious API, Twitch GQL API, and radio-browser.info API requests.
- **[nlohmann/json](https://github.com/nlohmann/json)** — JSON parsing library for all data persistence (favorites, playlists, radio stations, settings).
- **[CMake](https://cmake.org/) 3.10+** — Build system.

### APIs

- **Invidious API** — YouTube data (search, channel info, playlist info) via public Invidious instances
- **Twitch GQL API** — Twitch search (streams, videos, channels) using Twitch website's Client-ID
- **radio-browser.info** — Open database of internet radio stations with metadata

### Build Tools

- **GCC 15+** (MinGW on Windows) or Clang
- **Ninja** (preferred) or Make

## Architecture

Nynetify follows a layered architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────────┐
│                     Widget Layer (FLTK)                          │
│  Custom widgets: ModernButton, ModernChoice, ModernSlider,      │
│  ProgressSlider, CircularButton, HeartButton, ResultsBrowser    │
└───────────────────────────┬─────────────────────────────────────┘
                            │ callbacks
┌───────────────────────────▼─────────────────────────────────────┐
│                      Controller Layer                            │
│  AppCallbacks.cpp — search, playback, prefs, download, EQ       │
│  PlayerController.cpp — queue, play next/prev, UI timer         │
└───────────────────────────┬─────────────────────────────────────┘
                            │ view switching
┌───────────────────────────▼─────────────────────────────────────┐
│                        View Layer                                │
│  ViewManager.cpp — home, search, playlist, channel, radio,      │
│  credits views, async cover art, region detection                │
└───────────────────────────┬─────────────────────────────────────┘
                            │ services
┌───────────────────────────▼─────────────────────────────────────┐
│                       Service Layer                              │
│  YoutubeService / InvidiousClient — YouTube data via API/yt-dlp │
│  TwitchClient — Twitch data via GQL API                          │
│  PlayerEngine — libmpv audio playback wrapper                   │
│  RadioManager — radio-browser.info API + station management     │
│  PlaylistManager — favorites, playlists, persistence (JSON)     │
│  ArtistParser — multi-artist extraction heuristics              │
│  Spawn.h — cross-platform subprocess execution (no console)     │
└───────────────────────────┬─────────────────────────────────────┘
                            │ data
┌───────────────────────────▼─────────────────────────────────────┐
│                        Data Layer                                │
│  SearchResult.h, RadioStation.h, AppSettings.h, Lang.h, Theme.h │
│  JSON files: stations.json, radio_favs.json, youtube_favs.json, │
│  twitch_favs.json, playlists.json, settings.cfg                 │
└─────────────────────────────────────────────────────────────────┘
```

### Search Pipeline

1. User types query → `search_cb()` spawns a `std::thread` (UI stays responsive)
2. HTTP request to Invidious API or Twitch GQL API via libcurl
3. JSON response parsed into `vector<SearchResult>`
4. Results delivered to main thread via `Fl::awake()`
5. Browser populated progressively: 2 items initially, +3 every 50ms until viewport full, then "Show more" appears
6. Stale results discarded via sequence numbers

### Playback Pipeline

1. Double-click or Play button → `PlayerController::play_index()`
2. URL resolved depending on source:
   - **YouTube**: pre-resolved to direct audio URL via hidden `yt-dlp -g` call, fed to mpv with `ytdl=no` (no console window per song)
   - **Twitch**: channel/video URL passed to mpv with `ytdl=yes`
   - **Radio**: stream URL passed directly to mpv
3. UI timer fires every 200ms: polls mpv position/duration, updates progress bar + time labels
4. On EOF: auto-advance to next track (respecting shuffle, repeat, wrap-around)

### Subprocess Management

All subprocess calls (yt-dlp) go through `Spawn.h`:
- **Windows**: `CreateProcess` with `CREATE_NO_WINDOW` flag — no console windows ever appear
- **Linux**: `popen` with stderr redirected to null
- **Output capture**: uses temp files instead of pipes to avoid deadlocks on large output

### Data Persistence

All user data stored as JSON files next to the executable:

| File | Contents |
|------|----------|
| `stations.json` | All radio stations (bundled + custom) |
| `radio_favs.json` | Radio station favorite IDs |
| `youtube_favs.json` | YouTube video/playlist favorites |
| `twitch_favs.json` | Twitch channel favorites |
| `playlists.json` | Local playlists with tracks and comments |
| `settings.cfg` | Application settings (key=value) |

JSON files can be manually edited while the app is closed.

## Building from Source

### Windows (MSYS2)

```bash
# Install dependencies
pacman -S mingw-w64-x86_64-{cmake,ninja,fltk,mpv,curl}

# Build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run
./build/Nynetify.exe
```

For ARM64, replace `x86_64` with `clang-aarch64` and use the `CLANGARM64` MSYS2 environment.

### Linux (Ubuntu/Debian)

```bash
sudo apt install cmake build-essential libfltk1.3-dev libmpv-dev libcurl4-openssl-dev

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build

# Install
sudo cmake --install build

# Package as .deb
cd build && cpack -G DEB
```

### Linux (Fedora/RHEL)

```bash
sudo dnf install cmake gcc-c++ fltk-devel mpv-libs-devel libcurl-devel

cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build

# Package as .rpm
cd build && cpack -G RPM
```

## Downloads

Download the latest release from the [Releases page](https://github.com/thedevil4k/Nynetify/releases/latest) or the Actions tab.

## License

MIT
