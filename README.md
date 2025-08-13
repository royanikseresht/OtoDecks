# OtoDecks 🎛🎶

OtoDecks is a JUCE-based dual-deck DJ application that combines **real-time audio mixing**, **loop triggering**, **BPM analysis via Python**, and **custom playlist management** into a creative DJ/production tool.  
It’s built in **C++ with JUCE** for low-latency performance, and integrates **Python + Librosa** for advanced audio analysis.

---

## ✨ Features

### 🎵 Playback & Mixing
- **Dual Decks** with individual controls (play/pause, pitch, BPM sync, reverse playback).
- **Crossfader** for smooth transitions between decks.
- **WaveformDisplay** with moving playhead for visual cueing.
- **Loop library** (drum, guitar, piano, violin) embedded as resources for instant triggering.

### 🔄 Reverse Playback
- Tracks can be played backwards for creative effects.
- Reverse logic integrated into `TrackListComponent::proceedEndOfTrack()` and `DJAudioPlayer`.

### 📝 Playlist Management
- **PlaylistComponent** displays tracks with:
  - BPM
  - Musical key
  - Favorite toggle ❤️
  - Personal notes 📝
- **Favorites & All Tracks toggle** lets you filter the library instantly.
- CSV-based persistent storage via `CSVOperator`.

### 🎙 Recording System
- **RecordToggleSwitch** controls direct-to-disk WAV recording.
- `AudioRecorder` ensures sample-accurate writes to avoid glitches.
- Recording integrates with playlist metadata.

### 🎚 Loop & Cue Controls
- Embedded loops can be triggered directly in the decks.
- Potential for auto-loop overlays in waveforms.

### 📈 Python BPM Analysis
- `scripts/analyze_track.py` uses Librosa to:
  - Load audio
  - Perform beat tracking
  - Estimate BPM
- Auto-updates playlist CSV so new tracks instantly get accurate BPM.

---

## 🛠 Technical Architecture

### 4.1 C++ JUCE Components
| Component            | Purpose |
|----------------------|---------|
| **PlaylistComponent** | Displays tracks, supports search, favorites, notes, BPM. |
| **TrackListComponent** | Manages navigation, shuffle, repeat, and UI updates. |
| **WaveformDisplay**   | Renders audio waveform and playhead. |
| **RecordToggleSwitch**| Custom toggle for recording control. |
| **CSVOperator**       | Reads/writes CSV metadata (BPM, key, favorites, notes). |
| **DJAudioPlayer**     | Core playback engine (pitch, BPM sync, reverse). |
| **AudioRecorder**     | Handles WAV recording to disk. |
| **LookAndFeel**       | Custom UI theme. |

### 4.2 Reverse Playback
- Integrated at playback engine level.
- Can reverse entire tracks or reverse at loop boundaries.

### 4.3 Recording System
- Direct-to-disk via `AudioRecorder`.
- Controlled by `RecordToggleSwitch` in the UI.

### 4.4 Playlist Metadata Storage
- CSV holds BPM, key, favorite status, notes.
- Updated automatically after BPM analysis or UI changes.

### 4.5 Python BPM Analysis
Development journey:
1. Started with a YouTube tutorial on tempo estimation.
2. Learned Librosa beat tracking and FFT.
3. Built crude BPM detector, then refined using Librosa advanced examples.
4. Integrated into OtoDecks so adding a track auto-updates BPM.

---

## 📦 Project Structure

```
OtoDecks/
├── loops/
│   ├── drum/
│   ├── guitar/
│   ├── piano/
│   └── violin/
├── scripts/
│   └── analyze_track.py
├── Source/
│   ├── AudioRecorder.h
│   ├── CSVOperator.cpp/.h
│   ├── DeckGUI.cpp/.h
│   ├── DJAudioPlayer.cpp/.h
│   ├── LookAndFeel.cpp/.h
│   ├── Main.cpp
│   ├── MainComponent.cpp/.h
│   ├── MemoryAudioSource.h
│   ├── PlaylistComponent.cpp/.h
│   ├── RecordToggleSwitch.h
│   ├── TrackListComponent.cpp/.h
│   ├── WaveformDisplay.cpp/.h
└── OtoDecks.jucer
```

---

## ⚙ Building from `.jucer`

1. Open `OtoDecks.jucer` in **Projucer**.
2. Set the JUCE modules path to your JUCE installation.
3. Select **Exporters**:
   - Visual Studio 2022 (Windows)
   - Xcode (macOS)
4. Build the project.  
   *(Optional: update `targetName` to `OtoDecks` in `.jucer` for cleaner output naming)*

---

## 🚀 Future Improvements
- Two-take mode for recording.
- Enhanced EQ controls per deck.

---

## 📜 License
MIT License — feel free to remix and improve.
