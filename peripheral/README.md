# ESP32 → STM32 Expression Command System

Sends motor/expression commands from the ESP32 to an STM32 motor
controller over UART, making a physical doll blink, smile, rotate eyes,
and move its mouth in sync with the AI assistant's speech.

## Architecture

```
                     xiaozhi-server
                          │
           WebSocket JSON │ binary audio
                          ▼
┌──────────────────────────────────────────────────┐
│ ESP32 (application.cc)                           │
│                                                  │
│  System 1                       System 2         │
│  LLM emotion → action          Timed behaviors   │
│                                                  │
│  server sends                   clock_ticks_     │
│  {"type":"llm",                 fires every 1 s  │
│   "emotion":"happy"}                             │
│       │                              │           │
│       ▼                              ▼           │
│  FindEmotion("happy")          if idle for 10s:  │
│  → kEmotionMap lookup            → blink         │
│  → [smile, blink]              if idle for 15s:  │
│       │                          → eye_rotate    │
│       │                              │           │
│       │    TTS state events:         │           │
│       │    start → smile + talk      │           │
│       │    stop  → blink             │           │
│       │              │               │           │
│       └──────────────┴───────────────┘           │
│                      │                           │
│              stm32_serial_                       │
│              .SendExpression()                    │
│                      │ UART1 @ 115200            │
└──────────────────────┼───────────────────────────┘
                       │
              ┌────────┴────────┐
              │ STM32           │
              │ receives frame: │
              │ 0x80 0x00 0x00  │
              │ <action> 0xFF   │
              │ → moves motors  │
              └─────────────────┘
```

## UART Frame Protocol (from command.md)

```
Byte:   [0]    [1]    [2]    [3]     [4]
        0x80   cmd    CH     data    0xFF
        head   ────   ────   ────    tail
```

Baud rate: **115200**, 8N1.

### Expression commands (cmd = 0x00)

| data | Action           | Constant             |
|------|------------------|----------------------|
| 0x00 | Blink            | `kActionBlink`       |
| 0x01 | Eye rotate       | `kActionEyeRotate`   |
| 0x02 | Smile            | `kActionSmile`       |
| 0x03 | Talk (mouth)     | `kActionTalk`        |
| 0x04 | Turn head        | `kActionTurnHead`    |
| 0x05 | Left eye blink   | `kActionLeftEyeBlink`|
| 0x06 | Right eye blink  | `kActionRightEyeBlink`|

### Speed commands (cmd = 0x01)

| CH   | Target       | data                       |
|------|--------------|----------------------------|
| 0x00 | Eye speed    | signed int8, ±128 °/s      |
| 0x01 | Neck speed   | signed int8, ±128 °/s      |

## Two systems, one config file

All behavior is configured in **`expression_config.h`**. Edit it and
reflash — no other files need to change.

### System 1 — LLM emotion → physical action

The server already sends `{"type":"llm","emotion":"happy"}` with each
LLM response. The `kEmotionMap[]` table maps each emotion string to a
sequence of physical actions:

```cpp
// expression_config.h
static const EmotionMapping kEmotionMap[] = {
    //  emotion       actions                          count  delay(ms)
    { "happy",      { kActionSmile,  kActionBlink },     2,   300 },
    { "surprised",  { kActionEyeRotate, kActionBlink },  2,   200 },
    { "thinking",   { kActionEyeRotate },                1,     0 },
    // ...
};
```

**How to add a new emotion**: add a row to the table. Example:

```cpp
{ "excited", { kActionSmile, kActionEyeRotate, kActionBlink }, 3, 200 },
```

**How to handle an emotion the hardware can't do**: map it to the
closest approximation. Example: we can't do "surprised" directly, so
it's mapped to eye_rotate + blink.

**How to disable System 1**: remove all rows from `kEmotionMap[]`
(or set `kEmotionMapSize` to 0).

### System 2 — Timed / state-driven automatic behavior

Constants in `expression_config.h`:

```cpp
// Periodic idle behaviors (seconds)
static const int kBlinkIntervalTicks       = 10;  // blink every ~10 s
static const int kEyeRotateIntervalTicks   = 15;  // rotate eyes every ~15 s
static const int kIdleVariationTicks       = 3;   // ±3 s random jitter

// TTS state transitions
static const bool kSmileOnSpeakingStart = true;   // smile when bot starts talking
static const bool kTalkOnSpeakingStart  = true;   // mouth-move during speech
static const bool kBlinkOnSpeakingStop  = true;   // blink when bot stops talking
```

**How to change blink frequency**: change `kBlinkIntervalTicks` (e.g.
`5` for every 5 seconds, `30` for every 30 seconds).

**How to disable mouth movement during speech**: set
`kTalkOnSpeakingStart = false`.

**How to disable all timed behavior**: set all `k*IntervalTicks` to
`9999` and all `k*OnSpeaking*` to `false`.
How to configure everything from one file

  Open expression_config.h and edit:

  // Change what "happy" does physically:
  { "happy", { kActionSmile, kActionBlink }, 2, 300 },

  // Change blink frequency:
  static const int kBlinkIntervalTicks = 10;    // ← change to 5 for faster

  // Change eye rotate frequency:
  static const int kEyeRotateIntervalTicks = 15;  // ← change to 20 for slower

  // Disable mouth-move during speech:
  static const bool kTalkOnSpeakingStart = false;

  // Change UART pins:
  static const int kStm32UartTxPin = 17;   // ← your GPIO

  Edit → reflash → done. No other files need changing for behavior tweaks.
## Hardware wiring

```cpp
// expression_config.h
static const int kStm32UartTxPin = 17;   // ESP32 GPIO → STM32 RX
static const int kStm32UartRxPin = -1;   // not connected (one-way)
```

Change these to match your PCB. The ESP32-S3 can map UART1 to any
GPIO, so pick any free pin. Common choices:

| Board | Suggested TX | Suggested RX | Notes |
|-------|-------------|-------------|-------|
| Custom PCB | Per schematic | Per schematic | Check for conflicts with I2S/SPI |
| Dev board | GPIO 17 | GPIO 18 | Typically free on most S3 devkits |

## Files added / modified

| File | Action | What it does |
|------|--------|-------------|
| `peripheral/stm32_serial.h` | **NEW** | UART driver class, frame builder |
| `peripheral/stm32_serial.cc` | **NEW** | UART init, `SendFrame()`, `SendExpression()` |
| `peripheral/expression_config.h` | **NEW** | All configurable behavior: emotion map, timing, pins |
| `peripheral/README.md` | **NEW** | This file |
| `application.h` | **MODIFIED** | Added `Stm32Serial` member + tick counters |
| `application.cc` | **MODIFIED** | Added 3 hooks: init, clock-tick, json-callback |
| `CMakeLists.txt` | **MODIFIED** | Added `peripheral/stm32_serial.cc` to SOURCES |

## How to test without the STM32 connected

Connect a USB-to-serial adapter to the TX pin (GPIO 17 by default) and
GND, then monitor at 115200 baud:

```bash
# On your PC (Linux)
screen /dev/ttyUSB0 115200
# or
picocom -b 115200 /dev/ttyUSB0
```

Trigger a test:
1. Say something to the doll → it enters Speaking state → you should
   see `0x80 0x00 0x00 0x02 0xFF` (smile) and `0x80 0x00 0x00 0x03
   0xFF` (talk) on the serial monitor.
2. Wait 10 seconds idle → you should see `0x80 0x00 0x00 0x00 0xFF`
   (blink).
3. Wait 15 seconds idle → you should see `0x80 0x00 0x00 0x01 0xFF`
   (eye rotate).

## Build

No new ESP-IDF components or dependencies. Standard build:

```bash
cd /home/xxz1277/xz/xiaozhi-esp32-410
idf.py build
idf.py flash monitor
```

The UART driver uses `driver/uart.h` which is part of ESP-IDF's core
`driver` component — already included in every ESP32 project.

## Future extensions

- **Speed control**: `stm32_serial_.SendEyeSpeed(50)` sends a signed
  int8 speed command. Wire this to a slider in the web UI or a voice
  command for real-time servo speed control.
- **Bidirectional**: set `kStm32UartRxPin` to a real GPIO and read
  sensor data back from the STM32 (e.g. touch sensor, proximity
  sensor) → trigger voice responses based on physical touch.
- **LLM action tags**: modify the system prompt to include tags like
  `[smile]` in the LLM output text, parse them on the server before
  TTS, and send them as a dedicated WebSocket message type. This gives
  the LLM per-sentence control over expressions (covered in detail in
  `tts/REPORT.md`).
