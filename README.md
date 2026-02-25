# Ace Step 1.5 Dataset Manager

A desktop utility for preparing and editing music datasets for **ACE Step 1.5 / LoRA training**.

Built with **Qt 6 (Qt Widgets / Qt Multimedia)**.

## Overview

This tool helps you manage a dataset of audio tracks and edit per-track metadata in a fast, structured UI:

- Open a dataset from a **folder** or a specific **`.json` file**
- Edit dataset-level metadata (`name`, `custom_tag`, `tag_position`, `all_instrumental`, `genre_ratio`)
- Edit per-track fields (`Caption`, `Lyrics`, `Genre`, `BPM`, `Key`, `Time Sig`, `Duration`, `Language`, etc.)
- Preview audio directly inside each track card
- Save in a JSON format compatible with ACE Step-style dataset workflows

## Features

### Dataset Editing UI

- Scrollable list of track cards
- Audio player per track (play/pause + seek slider)
- Sticky player/actions panels inside each card (remain visible while scrolling long lyrics)
- Separate expand/collapse buttons for `Caption` and `Lyrics`
- `Prompt Override` per track:
  - `Use Global Ratio` -> `null`
  - `Caption`
  - `Genre`

### Workflow

- `Save` and `Save As`
- `Make backup` (stores backups in `_Backup`)
- `Reload` (reloads current folder/json to pick up external changes)
- `Merge paragraphs` for captions
- `Expand all / Collapse all`
- Unsaved-change tracking and field highlighting
- Close protection when there are unsaved changes

### Writing-Focused Features

- Optional **Caption/Lyrics-only mode** (reduces visual noise)
- Focus/Zen mode (hide top/right panels)
- Customizable font size for `Caption` / `Lyrics`
- Plain-text paste behavior (pasting from browser strips rich formatting)

### Tutorials

- Built-in tutorial windows for **Caption** and **Lyrics**
- Loads content from external Markdown files (`Help/*.md`)

## Hotkeys (Customizable)

Hotkeys are configurable in **Settings**:

- Focus mode toggle
- Save
- Make backup
- Play/Pause
- Seek backward
- Seek forward

Default media hotkeys:

- `Pause` -> Play/Pause
- `Alt+Left` -> Seek backward
- `Alt+Right` -> Seek forward

Seek step is configurable in Settings (seconds).

## JSON Behavior

- JSON fields are saved in a **fixed explicit order** (not alphabetical)
- `created_at` is generated on save in this format:
  - `YYYY-MM-DDTHH:MM:SS.ffffff`
- `raw_lyrics` is always written as:
  - `""`
- `tag_position` values:
  - `prepend`
  - `append`
  - `replace`

## UI / Theme

- The app forces a dark Qt theme (`Fusion` + custom dark palette) for consistent appearance across systems

## Help Markdown Files

Tutorial windows load Markdown files from `Help/` next to the executable.

Example layout:

```text
build/Debug/MusicDatasetManager.exe
build/Debug/Help/About Caption - The Most Important Input.md
build/Debug/Help/About Lyrics - The Temporal Script.md
```

## Build (Qt / CMake)

### Requirements

- Qt 6 (`Widgets`, `Multimedia`)
- CMake
- C++17 compiler (MSVC / clang / gcc)

### Example (Windows / MSVC)

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH="C:\Qt\6.10.2\msvc2022_64"
cmake --build build --config Debug
```

Run:

```powershell
.\build\Debug\MusicDatasetManager.exe
```

If CMake cannot find Qt, set `CMAKE_PREFIX_PATH` to your Qt installation.

## License / Notice

This software uses **Qt 6 (Qt Widgets / Qt Multimedia), licensed under LGPL v3**.

