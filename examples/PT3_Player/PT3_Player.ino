// AY8912_ESP32 PT3 Player Example
// Воспроизведение PT3 модулей ZX Spectrum
// Конвертируйте .pt3 в .h: xxd -i music.pt3 > music_pt3.h

#include <AY8912_ESP32.h>
#include <PT3Player.h>
#include <driver/i2s_std.h>
#include "music_pt3.h"

constexpr gpio_num_t PIN_BCLK = GPIO_NUM_4;
constexpr gpio_num_t PIN_LRC  = GPIO_NUM_5;
constexpr gpio_num_t PIN_DOUT = GPIO_NUM_6;
constexpr uint8_t    PIN_BTN  = 10;
constexpr uint8_t    PIN_LED  = 48;

constexpr uint32_t SAMPLE_RATE = 44100;
constexpr size_t   BUFFER_SIZE = 256;

AY8912 ay;
PT3Player player;
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

  ay.begin(SAMPLE_RATE, 1773400);
  initI2S();

  if (!player.load(music_pt3, sizeof(music_pt3))) {
    Serial.println("PT3 load failed!");
    return;
  }
  
  Serial.print("Title: ");
  Serial.println(player.getTitle());
  Serial.print("Author: ");
  Serial.println(player.getAuthor());
  
  player.attachAY(&ay);
  player.play();
  
  Serial.println("PT3 playing...");
}

void loop() {
  static unsigned long lastTick = 0;
  unsigned long now = millis();
  
  // 50 Гц = 20 мс между тиками
  if (now - lastTick >= 20) {
    lastTick = now;
    player.tick();
  }
  
  processAudio();
}