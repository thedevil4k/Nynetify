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
- **Cross-platform** — Windows x64, Windows ARM64, Linux (DEB/RPM)

## Downloads

| Platform | Architecture | Format |
|---|---|---|
| Windows | x86-64 | [Installer](https://github.com/anomalyco/Nynetify/releases/latest) / [Portable ZIP](https://github.com/anomalyco/Nynetify/releases/latest) |
| Windows | ARM64 | [Installer](https://github.com/anomalyco/Nynetify/releases/latest) / [Portable ZIP](https://github.com/anomalyco/Nynetify/releases/latest) |
| Linux | x86-64 | [.deb](https://github.com/anomalyco/Nynetify/releases/latest) / [.rpm](https://github.com/anomalyco/Nynetify/releases/latest) |

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
