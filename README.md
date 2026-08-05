# WusicPlayer

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh.md">中文</a>
</p>

<p align="center">
  A modern local music player for Linux desktop.
</p>

<p align="center">
  <img alt="Platform" src="https://img.shields.io/badge/platform-Linux-2ea44f">
  <img alt="Language" src="https://img.shields.io/badge/language-C%2B%2B23-00599C">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6.5+-41CD52">
  <img alt="Build" src="https://img.shields.io/badge/build-CMake-064F8C">
  <img alt="License" src="https://img.shields.io/badge/license-GPLv3-blue">
  <img alt="Status" src="https://img.shields.io/badge/status-WIP-orange">
</p>

---

> [!WARNING]
> WusicPlayer is under active development. Stability and compatibility are not guaranteed.

---

## About

WusicPlayer is a personal Qt project with two goals:

1. A practical, modern local music player for the Linux desktop workflow.
2. A Linux-side experience inspired by **Foobar2000 + foobox (v6) themes** (without heavy pro-audio features).

It uses a modular `view / controller / service / model / core` layered architecture, with an FFmpeg + miniaudio playback backend, TagLib tag read/write, and SQLite FTS5 library search.

## Features

### Playback
- Local audio playback (FFmpeg decode + miniaudio output)
- Play / Pause / Stop / Previous / Next / Seek / Volume / Mute
- Play modes: in-order, shuffle, loop-one, loop-list
- Output device switching
- 10-band equalizer (custom presets)

### Playlists
- Create, rename (inline), copy, delete, save
- Import files / folders (configurable add-file policy)
- Multi-column table view with sorting and custom columns
- Flexible sort expressions (e.g. `%artist% %album% | %track_number%`)
- Drag-to-reorder within the list

### Music Library
- Directory-based library scan with incremental updates (file watcher + periodic reconcile)
- Library browser grouped by artist / album / genre / folder / year
- Metadata display and editing (written back to files)
- Cover art

### Search
- **Library search**: SQLite FTS5 with prefix and substring matching (CJK-friendly)
- **In-playlist search**: in-memory index, real-time filtering
- Search results play directly and sync to the main view

### Lyrics
- Synced lyrics (embedded / external LRC)
- Desktop lyrics floating window
- Online lyrics search (Netease Cloud Music source)
- Built-in lyrics / tag editor

### UI & Customization
- Theme system: system theme, built-in themes (dark/light), external plugin themes
- Custom QStyle-based rendering (no QSS)
- Configurable shortcuts
- Custom icons

### Data
- `WusicPlayer.json` config persistence
- Tag metadata written back to audio files
- Playback state restored on startup

## Screenshots

| Main window | Equalizer |
|---|---|
| ![Main Window](docs/screenshots/main_window.png) | ![EQ](docs/screenshots/eq.png) |

More screenshots in [`docs/screenshots/`](docs/screenshots/).

## Quick Start (Linux)

```bash
# 1. Install dependencies
sudo apt install qt6-base-dev qt6-multimedia-dev qt6-svg-dev qt6-sql-dev \
    libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev \
    libtag1-dev libssl-dev zlib1g-dev ninja-build pkgconf git

# 2. Clone (with submodules)
git clone --recurse-submodules <repository-url>
cd WusicPlayer

# 3. Configure & build
cmake --preset debug
cmake --build --preset debug

# 4. Run
./build/debug/src/WusicPlayer
```

Detailed build instructions (incl. Windows / macOS) in [`docs/BUILDING.md`](docs/BUILDING.md).

## Tests

```bash
cmake --preset debug
cd build/debug && ctest
```

Test modules (8):
- `lrc_parser_core_test` — LRC parsing
- `tb_playlist` — playlist model
- `tb_add_file_policy` — add-file policy
- `tb_library` — library / FTS5 search
- `tb_library_browse` — library browse model
- `tb_search_backend` — search backends
- `tb_playback_queue` — playback queue
- `utils_audio` — audio utilities

## Documentation

| Doc | Description |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Architecture: layers, modules, key mechanisms, naming |
| [`docs/USAGE.md`](docs/USAGE.md) | Usage guide: layout, context menus, drag & drop, search, shortcuts |
| [`docs/BUILDING.md`](docs/BUILDING.md) | Platform build instructions |
| [`docs/THEME_SYSTEM.md`](docs/THEME_SYSTEM.md) | Theme system (incl. writing plugins) |
| [`docs/history/`](docs/history/) | Historical refactor design & decision records |

## Roadmap

- [ ] Audio spectrum visualization
- [ ] Smart playlists (saved queries)
- [ ] Broader unit test coverage
- [ ] Packaging (AppImage / Flatpak / .deb)

## License

GPLv3. See [LICENSE](LICENSE).

## Credits

- [Foobar2000](https://www.foobar2000.org/) / [foobox](https://github.com/dream7180/foobox-en) — UI inspiration
- [FFmpeg](https://ffmpeg.org/) — audio decoding
- [miniaudio](https://miniaud.io/) — audio output
- [TagLib](https://taglib.org/) — metadata
- [magic_enum](https://github.com/Neargye/magic_enum) — enum reflection
- [Qt](https://www.qt.io/) — application framework
