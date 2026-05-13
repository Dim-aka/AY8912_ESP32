// AY8912_ESP32 Notes Example
// Игра нот с огибающей, без внешних файлов

#include <AY8912_ESP32.h>
#include <driver/i2s_std.h>

constexpr gpio_num_t PIN_BCLK = GPIO_NUM_4;
constexpr gpio_num_t PIN_LRC  = GPIO_NUM_5;
constexpr gpio_num_t PIN_DOUT = GPIO_NUM_6;

constexpr uint32_t SAMPLE_RATE = 44100;
constexpr size_t   BUFFER_SIZE = 256;

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
  Serial.println("AY8912 Notes Example");

  ay.begin(SAMPLE_RATE, 1773400);
  initI2S();

  // === Аккорд C-E-G с огибающей ===
  
  // Канал A — C4 (MIDI 60)
  ay.setNote(0, AY8912::NOTE_C, 4);
  ay.setVolume(0, 15);
  ay.enableTone(0, true);
  
  // Канал B — E4 (MIDI 64)
  ay.setNote(1, AY8912::NOTE_E, 4);
  ay.setVolume(1, 15);
  ay.enableTone(1, true);
  
  // Канал C — G4 (MIDI 67), используем огибающую
  ay.setNote(2, AY8912::NOTE_G, 4);
  ay.enableEnvelope(2, true);
  ay.enableTone(2, true);
  
  // Настройка огибающей: пилообразная вниз (форма 4)
  ay.setEnvelopePeriod(5000);
  ay.setEnvelopeShape(4);
  
  // Микшер: все тона включены, шум выключен
  ay.writeRegister(7, 0x38);
}

void loop() {
  processAudio();
}