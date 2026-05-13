// AY8912_ESP32 Player Example
// Воспроизведение регистровых дампов (raw AY data)
// Файл track.h с массивом ay_track[] должен лежать рядом

#include <AY8912_ESP32.h>
#include <driver/i2s_std.h>
#include "track.h"

constexpr gpio_num_t PIN_BCLK = GPIO_NUM_4;
constexpr gpio_num_t PIN_LRC  = GPIO_NUM_5;
constexpr gpio_num_t PIN_DOUT = GPIO_NUM_6;
constexpr uint8_t    PIN_BTN  = 10;
constexpr uint8_t    PIN_LED  = 48;

constexpr uint32_t SAMPLE_RATE = 44100;
constexpr size_t   BUFFER_SIZE = 256;
constexpr uint8_t  TICK_MS     = 20;

AY8912 ay;
i2s_chan_handle_t i2s_tx;

void initI2S() {
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  chan_cfg.auto_clear = true;
  i2s_new_channel(&chan_cfg, &i2s_tx, NULL);

  i2s_std_config_t std_cfg = {
    .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
                    I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
    .gpio_cfg = {
      .mclk = I2S_GPIO_UNUSED,
      .bclk = PIN_BCLK,
      .ws   = PIN_LRC,
      .dout = PIN_DOUT,
      .din  = I2S_GPIO_UNUSED,
      .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
    },
  };
  i2s_channel_init_std_mode(i2s_tx, &std_cfg);
  i2s_channel_enable(i2s_tx);
}

void processAudio() {
  static int16_t buffer[BUFFER_SIZE * 2];
  for (size_t i = 0; i < BUFFER_SIZE; i++) {
    float L, R;
    ay.process(L, R);
    buffer[i * 2]     = (int16_t)(L * 32000.0f);
    buffer[i * 2 + 1] = (int16_t)(R * 32000.0f);
  }
  size_t bw;
  i2s_channel_write(i2s_tx, buffer, sizeof(buffer), &bw, portMAX_DELAY);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("AY8912 Player");

  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);

  ay.begin(SAMPLE_RATE, 1773400);
  initI2S();
}

bool playing = true;
size_t trackPos = 0;
unsigned long nextEventMs = 0;

void loop() {
  unsigned long now = millis();

  // --- Парсер трека ---
  if (playing) {
    if (nextEventMs == 0) nextEventMs = now;

    while (now >= nextEventMs) {
      if (trackPos >= sizeof(ay_track)) {
        trackPos = 0;
        ay.reset();
        nextEventMs += 300;
        break;
      }

      uint8_t packet[16];
      bool    pktMask[16] = {false};
      bool    gotDelay = false;
      uint8_t delayTicks = 0;

      while (trackPos < sizeof(ay_track) && !gotDelay) {
        uint8_t reg = ay_track[trackPos++];
        if (reg == 0xFF) {
          if (trackPos < sizeof(ay_track)) delayTicks = ay_track[trackPos++];
          if (trackPos < sizeof(ay_track)) trackPos++;
          gotDelay = true;
        } else {
          uint8_t val = 0;
          if (trackPos < sizeof(ay_track)) val = ay_track[trackPos++];
          if (reg < 16) {
            packet[reg] = val;
            pktMask[reg] = true;
          }
        }
      }

      if (gotDelay || trackPos >= sizeof(ay_track)) {
        for (int i = 0; i < 16; i++) {
          if (pktMask[i]) ay.writeRegister(i, packet[i]);
        }
        if (gotDelay) {
          nextEventMs += (unsigned long)delayTicks * TICK_MS;
          break;
        }
      }
    }
  }

  // --- Кнопка play/pause ---
  static bool lastBtn = HIGH, confBtn = HIGH;
  static unsigned long debounceMs = 0;
  bool btn = digitalRead(PIN_BTN);
  if (btn != lastBtn) debounceMs = now;
  if ((now - debounceMs) > 50 && btn != confBtn) {
    confBtn = btn;
    if (confBtn == LOW) {
      playing = !playing;
      if (!playing) ay.reset();
    }
  }
  lastBtn = btn;

  digitalWrite(PIN_LED, playing ? HIGH : LOW);

  // --- Аудио ---
  if (playing) {
    processAudio();
  } else {
    static int16_t silence[BUFFER_SIZE * 2] = {};
    size_t bw;
    i2s_channel_write(i2s_tx, silence, sizeof(silence), &bw, pdMS_TO_TICKS(10));
    delay(5);
  }
}