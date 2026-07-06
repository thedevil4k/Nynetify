# Nynetify

> A desktop YouTube, Twitch, SoundCloud & radio audio player built with FLTK and mpv.

## What is Nynetify?

Nynetify is a lightweight native desktop audio player that lets you search and play audio from YouTube, Twitch, SoundCloud, and online radio stations — all without a browser. It also allows downloading tracks as MP3, creating local playlists, and managing favorites across all sources.

## Features

- **YouTube** — search songs, playlists, and channels via Invidious API or yt-dlp
- **Twitch** — search live streams, videos, and channels via Twitch GQL API
- **SoundCloud** — search and play tracks via the embedded SoundCloud widget
- **Online radio** — 52 built-in stations from 12 countries with country filter, favorites, and custom station support
- **Local playlists** — create, organize, and persist playlists as JSON
- **Favorites** — separate favorites for YouTube, Twitch, SoundCloud, and Radio
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
| Linux    | x86-64      | .deb / .rpm / AppImage |

## How It Works

### Data Retrieval (Search & Discovery)

Each audio source uses a different method to obtain its data:

| Source | Method | Library | Details |
|--------|--------|---------|---------|
| **YouTube** | Invidious API (preferred) | **libcurl** + **nlohmann/json** | HTTP GET to public Invidious instance → JSON response → deserialized into `SearchResult` structs. Falls back to yt-dlp CLI if the API returns no results. |
| **YouTube** | yt-dlp CLI (fallback) | **yt-dlp** subprocess via `Spawn.h` | For queries that Invidious doesn't handle well (e.g., playlist/channel filters) |
| **Twitch** | Twitch GQL API | **libcurl** + **nlohmann/json** | HTTP POST to the undocumented Twitch GraphQL endpoint using the website's Client-ID. Returns streams, videos, and channels. |
| **SoundCloud** | Embedded widget (loading) | **yt-dlp** subprocess via `Spawn.h` | SoundCloud URLs are parsed and resolved via yt-dlp's SoundCloud extractor |
| **Radio** | radio-browser.info API | **libcurl** + **nlohmann/json** | HTTP GET to `de1.api.radio-browser.info` for search and URL refresh. Local stations are cached in `stations.json` |

### Audio Playback

All audio playback goes through **mpv**, used as a C API library via `libmpv`:

| Source | URL Resolution | mpv Mode |
|--------|---------------|----------|
| **YouTube** | Pre-resolved via hidden `yt-dlp -g` → direct audio URL | `ytdl=no` (no console window) |
| **Twitch** | Channel/video URL passed directly | `ytdl=yes` (mpv resolves internally) |
| **SoundCloud** | Pre-resolved via hidden `yt-dlp -g` → direct audio URL | `ytdl=no` |
| **Radio** | Stream URL from `stations.json` or radio-browser.info | Direct URL, no resolution needed |

A 200ms UI timer polls mpv for position/duration and updates the progress bar and time labels. On EOF, the player auto-advances to the next track (respecting shuffle, repeat, and wrap-around).

### Download

Clicking the download button invokes `yt-dlp -x --audio-format mp3 <url>` in a background `std::thread`. The MP3 file is saved to the user's Downloads folder. yt-dlp is bundled with the Windows installer and declared as a dependency on Linux.

### Radio System

- **52 bundled stations** loaded from `stations.json` at startup (path resolved via `get_asset_path()` relative to the executable)
- Stations are organized by country; the user's detected country is sorted first
- A country filter dropdown lets users browse stations from a specific region
- Favorites are stored in `radio_favs.json`
- Custom stations can be added via the UI and are saved to `stations.json`
- Station URLs can be refreshed from radio-browser.info to get the best available bitrate
- Station logos are loaded from `assets/` folder (local PNG files)

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
│  YoutubeService / InvidiousClient — YouTube via API or yt-dlp   │
│  TwitchClient — Twitch search via GQL API                       │
│  SoundCloudClient — SoundCloud search via yt-dlp                │
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
│  soundcloud_favs.json, twitch_favs.json, playlists.json,        │
│  settings.cfg                                                    │
└─────────────────────────────────────────────────────────────────┘
```

### Search Pipeline

1. User types query → `search_cb()` spawns a `std::thread` (UI stays responsive)
2. Dispatches to the correct backend based on the selected platform toggle (YouTube/Twitch/SoundCloud)
3. HTTP request to Invidious or Twitch GQL API via libcurl, or yt-dlp subprocess for SoundCloud
4. JSON or parsed response → `vector<SearchResult>`
5. Results delivered to main thread via `Fl::awake()`
6. Browser populated progressively: 2 items initially, +3 every 50ms until viewport full, then "Show more" appears
7. Stale results from old searches are discarded via sequence numbers

### Data Persistence

All user data is stored as JSON files relative to the executable:

| File | Contents |
|------|----------|
| `stations.json` | All radio stations (bundled + custom) |
| `radio_favs.json` | Radio station favorite IDs |
| `youtube_favs.json` | YouTube video/playlist favorites |
| `soundcloud_favs.json` | SoundCloud track favorites |
| `twitch_favs.json` | Twitch channel favorites |
| `playlists.json` | Local playlists with tracks and comments |
| `settings.cfg` | Application settings (key=value) |

All files are in the `<exe_dir>/assets/` directory and can be edited while the app is closed.

## Technologies

### Core Frameworks & Libraries

- **[FLTK](https://www.fltk.org/) 1.4+** — Cross-platform GUI toolkit. All UI widgets, windows, and layout are built on FLTK.
- **[mpv](https://mpv.io/)** — Media player engine. Used as a C API library via `libmpv` for audio playback, seeking, volume control, and metadata queries.
- **[yt-dlp](https://github.com/yt-dlp/yt-dlp)** — CLI tool for YouTube, SoundCloud and general audio stream resolution. Invoked as a hidden subprocess for URL resolution and downloads.
- **[libcurl](https://curl.se/)** — HTTP client used for Invidious API, Twitch GQL API, and radio-browser.info API requests.
- **[nlohmann/json](https://github.com/nlohmann/json)** — JSON parsing library for all data persistence and API responses. Bundled as a single header in `include/nlohmann/json.hpp`.
- **[CMake](https://cmake.org/) 3.10+** — Build system.

### APIs

- **Invidious API** — YouTube data (search, channel info, playlist info) via public Invidious instances
- **Twitch GQL API** — Twitch search (streams, videos, channels) using the Twitch website's Client-ID
- **radio-browser.info** — Open database of internet radio stations with metadata and streaming URLs

### Build Tools

- **GCC 15+** (MinGW on Windows) or Clang
- **Ninja** (preferred) or Make

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
