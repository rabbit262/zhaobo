#ifndef _EXPRESSION_CONFIG_H_
#define _EXPRESSION_CONFIG_H_

#include "stm32_serial.h"
#include <cstring>

// ═══════════════════════════════════════════════════════════════
// HARDWARE PINS — which GPIOs connect ESP32 UART1 to the STM32
//
// Change these to match your PCB / wiring.
// Set RX to -1 if the STM32 never sends data back.
// ═══════════════════════════════════════════════════════════════
static const int kStm32UartTxPin = 17;
static const int kStm32UartRxPin = -1;   // one-way: ESP32 → STM32


// ═══════════════════════════════════════════════════════════════
// SYSTEM 1 — LLM emotion → physical action mapping
//
// The server sends {"type":"llm","emotion":"happy"}.
// This table maps each emotion string to a sequence of motor
// actions. Edit this table when:
//   - The server sends a new emotion you want to handle
//   - You want to change what "happy" physically does
//   - The STM32 gains new capabilities
//
// Available actions:
//   kActionBlink  kActionEyeRotate  kActionSmile  kActionTalk
//   kActionTurnHead  kActionLeftEyeBlink  kActionRightEyeBlink
// ═══════════════════════════════════════════════════════════════

struct EmotionMapping {
    const char* emotion;         // emotion string from server
    Stm32Action actions[3];      // up to 3 sequential actions
    uint8_t action_count;        // how many entries in actions[]
    uint16_t delay_between_ms;   // pause between sequential actions
};

static const EmotionMapping kEmotionMap[] = {
    //  emotion       actions                                     count  delay(ms)
    { "happy",      { kActionSmile,     kActionBlink            },  2,   300 },
    { "neutral",    { kActionBlink                              },  1,     0 },
    { "sad",        { kActionBlink,     kActionBlink            },  2,   500 },
    { "surprised",  { kActionEyeRotate, kActionBlink            },  2,   200 },
    { "angry",      { kActionTurnHead                           },  1,     0 },
    { "wink",       { kActionLeftEyeBlink                       },  1,     0 },
    { "thinking",   { kActionEyeRotate                          },  1,     0 },
    { "laughing",   { kActionSmile,     kActionSmile            },  2,   200 },
    { "confused",   { kActionEyeRotate, kActionRightEyeBlink    },  2,   300 },
};
static const int kEmotionMapSize = sizeof(kEmotionMap) / sizeof(kEmotionMap[0]);


// ═══════════════════════════════════════════════════════════════
// SYSTEM 2 — timed / state-driven automatic behaviors
//
// These run regardless of what the LLM says. They make the doll
// look "alive". All timing is in clock-tick units (1 tick = 1 s).
// ═══════════════════════════════════════════════════════════════

// Periodic idle actions (only fire when state is Idle or Listening)
static const int kBlinkIntervalTicks       = 10;   // blink roughly every 10 s
static const int kEyeRotateIntervalTicks   = 15;   // rotate eyes roughly every 15 s
static const int kIdleVariationTicks       = 3;    // ± random jitter in seconds

// What happens at TTS state transitions
static const bool kSmileOnSpeakingStart = true;    // smile when assistant starts talking
static const bool kTalkOnSpeakingStart  = true;    // mouth-move animation during speech
static const bool kBlinkOnSpeakingStop  = true;    // blink when assistant stops talking


// ═══════════════════════════════════════════════════════════════
// Helper — look up an emotion string in the table
// ═══════════════════════════════════════════════════════════════
inline const EmotionMapping* FindEmotion(const char* emotion) {
    if (!emotion) return nullptr;
    for (int i = 0; i < kEmotionMapSize; i++) {
        if (strcasecmp(kEmotionMap[i].emotion, emotion) == 0) {
            return &kEmotionMap[i];
        }
    }
    return nullptr;
}

#endif // _EXPRESSION_CONFIG_H_
