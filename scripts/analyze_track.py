# scripts/analyze_track.py

import sys
import librosa

if len(sys.argv) < 2:
    print("Usage: python analyze_track.py <path_to_audio_file>")
    sys.exit(1)

filename = sys.argv[1]

try:
    # Load audio file
    y, sr = librosa.load(filename, sr=None)

    # Estimate tempo (BPM)
    tempo, _ = librosa.beat.beat_track(y=y, sr=sr)

    # Convert tempo to float if it's a NumPy array
    if hasattr(tempo, '__len__'):
        tempo = float(tempo[0])  # in case it's an array
    else:
        tempo = float(tempo)

    # Print result in format: bpm,label
    print(f"{tempo:.1f},Unknown")

except Exception as e:
    print(f"Error analyzing track: {e}")
    sys.exit(1)