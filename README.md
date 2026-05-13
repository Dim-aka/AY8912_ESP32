# AY8912_ESP32

Fast float-based AY-3-8910 / AY-8912 emulator + PT3 player for ESP32.

No external dependencies. No `double` math (no CPU crunch, no crackling).  
Pure float logic — runs smoothly at 44100 Hz even on a single core.

---

## Features

- **Cycle-accurate emulation**: exact DAC table, 17-bit noise LFSR, 16 envelope shapes
- **Stereo panning**: per-channel left/right balance
- **I2S native**: works with any I2S DAC (PCM5102, MAX98357, etc.)
- **Zero crackling**: all math in `float` (ESP32-S3 hardware FPU)
- **Tiny footprint**: ~200 bytes RAM, no dynamic allocation
- **PT3 player built-in**: play ZX Spectrum Pro Tracker 3 music directly

---

## Requirements

- **Board**: any ESP32 (ESP32-S3 recommended)
- **Core**: Arduino-ESP32 3.x (IDF 5.x, new I2S API)
- **DAC**: any stereo I2S DAC (3 pins: BCK, WS/LRC, DIN)

---

## Quick Start

### 1. Wiring (example for PCM5102 / MAX98357)

| ESP32-S3 | DAC        |
|----------|------------|
| GPIO 4   | BCK        |
| GPIO 5   | WS / LRC   |
| GPIO 6   | DIN        |
| GND      | GND        |
| 3.3V     | 3.3V       |

> For PCM5102: tie **FMT** to GND (I2S mode), **XSMT** to 3.3V (unmute).

### 2. Install

Clone into your Arduino libraries folder:

 
cd ~/Documents/Arduino/libraries
git clone https://github.com/yourname/AY8912_ESP32.git
Or download as ZIP and add via Sketch → Include Library → Add .ZIP Library...
3. Examples
Basic Notes (no external files)
 
#include <AY8912_ESP32.h>

AY8912 ay;

void setup() {
  ay.begin(44100, 1773400);
  
  // Play A4 on channel 0
  ay.setNote(0, AY8912::NOTE_A, 4);
  ay.setVolume(0, 15);
  ay.enableTone(0, true);
  ay.writeRegister(7, 0x3E); // enable tone A
}

void loop() {
  float L, R;
  ay.process(L, R);
  // send to I2S...
}
Register Dump Player
See examples/Player/Player.ino — requires track.h with ay_track[] array.

PT3 Player
See examples/PT3_Player/PT3_Player.ino — requires converted PT3 file:
 
xxd -i music.pt3 > music_pt3.h

API Reference
AY8912 Class
| Method                         | Description                              |
| ------------------------------ | ---------------------------------------- |
| `begin(sampleRate, clockRate)` | Initialize (44100 Hz, 1773400 Hz for ZX) |
| `reset()`                      | Reset all registers                      |
| `writeRegister(reg, value)`    | Write to AY register 0..13               |
| `readRegister(reg)`            | Read AY register                         |
| `setPan(ch, pan)`              | Stereo position 0.0..1.0                 |
| `setMixerBit(bit, value)`      | Direct mixer control                     |
| `process(L, R)`                | Generate one stereo sample               |

Convenient Methods
| Method                       | Description          |
| ---------------------------- | -------------------- |
| `setNote(ch, midiNote)`      | MIDI note 0..127     |
| `setNote(ch, note, octave)`  | Note name + octave   |
| `enableTone(ch, enable)`     | Toggle tone          |
| `enableNoise(ch, enable)`    | Toggle noise         |
| `setVolume(ch, vol)`         | Volume 0..15         |
| `setEnvelopePeriod(period)`  | Envelope period      |
| `setEnvelopeShape(shape)`    | Envelope shape 0..15 |
| `enableEnvelope(ch, enable)` | Toggle envelope      |

PT3Player Class
| Method                              | Description              |
| ----------------------------------- | ------------------------ |
| `load(data, size)`                  | Load PT3 from byte array |
| `attachAY(&ay)`                     | Connect to AY emulator   |
| `play()` / `stop()` / `isPlaying()` | Control playback         |
| `tick()`                            | Call at 50 Hz            |
| `getTitle()` / `getAuthor()`        | Module info              |

How It Works
The original ayumi library by true-grue is mathematically perfect, but uses double (64-bit).
ESP32-S3 has no hardware FPU for double, causing I2S underruns and crackling.
AY8912_ESP32 re-implements the exact same logic in float (32-bit).
The ESP32-S3 hardware FPU handles float natively, leaving CPU headroom for your app.
License
MIT License. 
