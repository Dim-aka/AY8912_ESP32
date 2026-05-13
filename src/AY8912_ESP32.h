#ifndef AY8912_ESP32_H
#define AY8912_ESP32_H

#include <Arduino.h>
#include <math.h>

class AY8912 {
public:
  AY8912();
  
  // sampleRate: 44100, 22050 и т.д.
  // clockRate:  1773400 (ZX Spectrum 128K) или 1000000 (AY-3-8910 стандарт)
  void begin(uint32_t sampleRate = 44100, uint32_t clockRate = 1773400);
  
  void reset();
  
  // Запись в регистр AY (0..13). R14/R15 игнорируются (порт ввода/вывода).
  void writeRegister(uint8_t reg, uint8_t value);
  
  // Чтение регистра AY (для PT3 player и других нужд)
  uint8_t readRegister(uint8_t reg) const;
  
  // Панорама канала: 0.0 = полностью слева, 1.0 = полностью справа, 0.5 = центр
  void setPan(uint8_t channel, float pan);
  
  // Установить бит микшера напрямую
  // bit: 0-2 = tone off для канала A/B/C, 3-5 = noise off для канала A/B/C
  // value: true = выключить, false = включить
  void setMixerBit(uint8_t bit, bool value);
  
  // Сгенерировать один стерео-сэмпл. Вызывать в аудио-цикле.
  void process(float& outLeft, float& outRight);

  // === НОВОЕ: удобные функции ===
  
  // Установить ноту по MIDI-номеру (0=C-1, 60=C4, 69=A4=440Hz)
  // channel: 0, 1, 2
  // midiNote: 0..127
  void setNote(uint8_t channel, uint8_t midiNote);
  
  // Установить ноту по имени и октаве
  // note: 0=C, 1=C#, 2=D, 3=D#, 4=E, 5=F, 6=F#, 7=G, 8=G#, 9=A, 10=A#, 11=B
  // octave: 0..7 (октава 3 = стандартная для AY)
  void setNote(uint8_t channel, uint8_t note, uint8_t octave);
  
  // Быстрое включение/выключение канала
  void enableTone(uint8_t channel, bool enable);
  void enableNoise(uint8_t channel, bool enable);
  void setVolume(uint8_t channel, uint8_t volume); // 0..15
  
  // Огибающая
  void setEnvelopePeriod(uint16_t period); // R11/R12
  void setEnvelopeShape(uint8_t shape);    // R13 (0..15)
  void enableEnvelope(uint8_t channel, bool enable); // бит 4 громкости

  // Константы нот для удобства
  static constexpr uint8_t NOTE_C  = 0;
  static constexpr uint8_t NOTE_CS = 1;
  static constexpr uint8_t NOTE_D  = 2;
  static constexpr uint8_t NOTE_DS = 3;
  static constexpr uint8_t NOTE_E  = 4;
  static constexpr uint8_t NOTE_F  = 5;
  static constexpr uint8_t NOTE_FS = 6;
  static constexpr uint8_t NOTE_G  = 7;
  static constexpr uint8_t NOTE_GS = 8;
  static constexpr uint8_t NOTE_A  = 9;
  static constexpr uint8_t NOTE_AS = 10;
  static constexpr uint8_t NOTE_B  = 11;

  // Дружественный доступ для PT3Player
  friend class PT3Player;
  friend class STCPlayer;

private:
  struct AYEmu {
    uint8_t  reg[16];
    int      tonePeriod[3];
    int      toneOut[3];
    float    toneCount[3];
    int      noisePeriod;
    uint32_t noiseState;
    float    noiseCount;
    int      envPeriod;
    int      envLevel;
    int      envSegment;
    uint8_t  envShape;
    float    envCount;
    float    envInc;
    float    dcL, dcR;
  } emu;

  uint32_t _sampleRate;
  uint32_t _clockRate;
  float    _panL[3];
  float    _panR[3];
  float    _toneAdv;
  float    _envAdvBase;

  void _envResetSegment();
  void _updateReg(int r);
  
  // Таблица периодов для октавы 3 при CLK=1773400
  // period = CLK / 16 / freq
  static const uint16_t PERIOD_TABLE[12];
  
  // Пересчёт MIDI-ноты в период
  uint16_t _midiToPeriod(uint8_t midiNote);
};

#endif