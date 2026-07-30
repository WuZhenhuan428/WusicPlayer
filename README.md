# WusicPlayer

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh.md">中文</a>
</p>

<p align="center">
  A modern local music player for Linux desktop environments.
</p>

<p align="center">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Linux-2ea44f">
  <img alt="Language" src="https://img.shields.io/badge/language-C%2B%2B17-00599C">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6.5+-41CD52">
  <img alt="Build" src="https://img.shields.io/badge/build-CMake-064F8C">
  <img alt="License" src="https://img.shields.io/badge/license-GPLv3-blue">
  <img alt="Status" src="https://img.shields.io/badge/status-WIP-orange">
</p>

---

> [!WARNING]
> WusicPlayer is currently under development.\
> Stability and compatibility are not guaranteed.

---

## Overview

**WusicPlayer** is a personal Qt project with two practical goals:

1. Build a usable, modern local music player for Linux desktop workflows.
2. Provide a Linux-side alternative experience inspired by **Foobar2000 + foobox (v6) theme**, without aiming for heavy professional audio features.

This project is under active refactoring and feature iteration.

---

## Project Status

> **Work in Progress**

- Core playback and playlist features are available.
- Architecture follows a modular `view` / `controller` / `service` / `model` / `core` split.
- Packaging is **not provided yet** (e.g., `.deb` / `.rpm` / `.AppImage`).
- Bugs are possible; feedback is welcome.
- Some features (e.g., custom title bar) rely on X11/XCB and may not work under Wayland.

---

## Features

### Playback
- Local audio file playback (powered by FFmpeg + miniaudio)
- Play, pause, stop, next/previous, seek, volume control, mute
- Multiple play modes (sequential, shuffle, repeat one, repeat all)
- Audio device switching (output device selection)
- Ten-band equalizer with customizable presets

### Playlist Management
- Create, rename, copy, remove playlists
- Import/export playlists
- Multi-column track list with sortable headers
- Flexible sort expressions (`%artist% %album% | %track_number%`)

### Music Library
- Directory-based library scanning and management
- Track metadata display (artist, album, genre, year, bitrate, etc.)
- Cover art display

### Lyrics
- Synchronized lyrics display (embedded and external LRC files)
- Desktop lyrics overlay (floating window, work in progress)
- Lyrics search (Netease Cloud Music backend)
- Built-in lyrics/tag editor

### Search
- Full-text search across library and playlists
- In-memory search backend with real-time filtering

### UI & Customization
- Theme system supporting system themes, built-in themes (dark/light), and external plugin themes
- Custom QStyle-based rendering (no QSS)
- Configurable shortcut keys
- Custom icons

### Data Management
- Config persistence via `WusicPlayer.json`
- Tag metadata writeback to audio files
- Playback state restore on startup

---

## Screenshots
Main window:
![Main Window](docs/screenshots/main_window.png)

Network lyrics search:
![NetWork Lyrics Search](docs/screenshots/lyrics_search.png)

Search in playlist:
![Search in playlist](docs/screenshots/search_window.png)

Tag view and lyrics editor:
![tag viewer and lyrics editor](docs/screenshots/tag_viewer_and_lyrics_editor.png)

Equalizer and custom icons:
![qqualizer and custom icons](docs/screenshots/eq.png)

---

## Architecture

The project follows a modular **MVC-inspired** layered architecture with separate concerns:

```text
src/
├── app_controller.cpp/h    # Application-level orchestration (AppController)
├── main.cpp                 # Entry point
├── controller/              # Controllers — bridge between view and model
│   ├── PlaybackController   # Playback orchestration
│   ├── PlaylistController   # Playlist CRUD & manipulation
│   ├── shortcuts_controller # Keyboard shortcut management
│   └── search_backend/      # Search query processing
├── model/                   # Data models & view models
│   ├── playlist/            # Playlist data model
│   ├── search_model/        # Search data model
│   └── ShortcutsViewModel/  # Shortcut configuration model
├── view/                    # UI components (Qt Widgets)
│   ├── MainWindow           # Main window shell (UI container)
│   ├── WControlBar/         # Playback control bar
│   ├── LibraryWidget/       # Music library browser
│   ├── playlist/            # Playlist panel
│   ├── search_panel/        # Search UI
│   ├── DesktopLyricsWidget/ # Desktop lyrics overlay
│   ├── eq_widget/           # Equalizer panel
│   ├── tag_edit_widget/     # Metadata/tag editor
│   ├── SettingsPanel/       # Settings pages
│   ├── SidePanel/           # Navigation sidebar
│   └── dialogs/             # Modal dialogs
├── service/                 # Services — cross-cutting business logic
│   ├── playback_service     # Audio playback lifecycle
│   ├── playback_restore_service  # Resume playback on startup
│   ├── library_interaction_service # Library file operations
│   ├── tag_writeback_service     # Metadata writeback to files
│   └── theme_service        # Theme application & management
├── core/                    # Core infrastructure
│   ├── types.h              # Shared data types (TrackMetaData, SortRule, etc.)
│   ├── player_types.h       # Player-specific types
│   ├── search_types.h       # Search-specific types
│   ├── hsv_types.h          # Color space types
│   ├── player/              # Audio engine (FFmpeg + miniaudio)
│   ├── ConfigManager/       # JSON-based configuration persistence
│   ├── LyricsFetcher/       # Network lyrics fetching (Netease API)
│   ├── theme/               # Theme engine (QStyle-based, plugin system)
│   └── utils/               # Audio utilities, path helpers
└── static/                  # Qt resource files (icons, images)
```

### Design Principles

- **AppController** acts as the top-level coordinator, wiring together controllers, services, and views.
- **MainWindow** is a thin UI shell; business logic lives in controllers and services.
- **Services** encapsulate cross-cutting concerns (playback, library, tags, themes).
- **Models** hold data and provide view-friendly interfaces (e.g., `ShortcutsViewModel`).
- **Core** provides shared types, the audio engine, configuration, and the theme system.

---

## Dependencies

| Dependency | Version     | Purpose                                                          |
|------------|-------------|------------------------------------------------------------------|
| Qt 6       | ≥ 6.5       | Core, Widgets, Multimedia, Network, SVG                          |
| FFmpeg     | ≥ 4.x       | Audio decoding (libavcodec, libavformat, libavutil, libavfilter) |
| TagLib     | ≥ 1.x       | Audio metadata reading/writing                                   |
| OpenSSL    | ≥ 1.1       | HTTPS for network lyrics fetching                                |
| ZLIB       |             | Data compression                                                 |
| magic_enum | header-only | Static reflection for C++ enums (vendored submodule)             |
| lrc-parser | header-only | LRC lyrics format parser (vendored)                              |

---

## Building

See [BUILDING.md](docs/BUILDING.md) for detailed build instructions on Linux, Windows (MSVC), and Windows (MSYS2/MinGW).

### Quick Start (Linux)

```bash
# 1. Install dependencies
sudo apt install qt6-base-dev qt6-multimedia-dev qt6-svg-dev \
    libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev \
    libtag1-dev libssl-dev zlib1g-dev ninja-build pkgconf git

# 2. Clone with submodules
git clone --recurse-submodules https://github.com/your/wusicplayer.git
cd wusicplayer

# 3. Build
cmake --preset debug
cmake --build --preset debug
```

---

## Testing

> Test coverage is currently limited and evolving.

`WUSIC_BUILD_TESTS` is enabled by default. Tests use CMake's CTest.

```bash
# Build with tests
cmake --preset debug

# Run tests
cd build/debug && ctest
```

Test modules:
- `test/playlist/` — Playlist model unit tests
- `test/AudioScanner/` — Audio file scanning tests

---

## TODO

- [x] Custom FFmpeg + miniaudio playback backend
- [x] Controller-layer migration (decouple MainWindow)
- [x] Audio device switching
- [x] Independent search backend
- [x] Shortcut key binding
- [x] Network lyrics search (Netease Cloud Music)
- [x] GUI equalizer (10-band)
- [x] Theme system (QStyle-based, plugin architecture)
- [ ] Audio spectrum visualization
- [ ] Media library manager
- [ ] Expand unit test coverage
- [ ] Distribution packaging (AppImage / Flatpak / .deb)
- [ ] ...

---

## Contributing

Issues and PRs are welcome!

- Keep commits small and focused.
- Ensure the project builds and tests pass before opening a PR.
- Follow the existing code style and architecture patterns.

---

## License

This project is licensed under the **GPLv3** License. See [LICENSE](LICENSE) for details.

---

## Acknowledgements

- [Foobar2000](https://www.foobar2000.org/) + foobox — UI inspiration
- [FFmpeg](https://ffmpeg.org/) — Audio decoding
- [miniaudio](https://miniaud.io/) — Audio output
- [TagLib](https://taglib.org/) — Metadata handling
- [magic_enum](https://github.com/Neargye/magic_enum) — C++ enum reflection
- [Qt](https://www.qt.io/) — Application framework
- Prefer behavior-preserving refactors in separate commits

---

## License

This project is licensed under **GPL-3.0**.  
See the `LICENSE` file for details.

---

## Acknowledgements

- Qt
- [TagLib](https://github.com/taglib/taglib)
- UX inspiration from [Foobar2000](https://www.foobar2000.org) / [foobox](https://github.com/dream7180/foobox-en) v6 (no official affiliation)
- [miniaudio](https://miniaud.io)
- [ffmpeg](https://ffmpeg.org)
- [lrc-parser](https://github.com/WuZhenhuan428/lrc-parser)
- [audio-player-core](https://github.com/WuZhenhuan428/audio-player-core) (not sync with this proj)
