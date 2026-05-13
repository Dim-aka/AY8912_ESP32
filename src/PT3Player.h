#ifndef PT3_PLAYER_H
#define PT3_PLAYER_H

#include <Arduino.h>
#include <AY8912_ESP32.h>

#define PT3_MAX_PATTERNS    42
#define PT3_MAX_POSITIONS   128
#define PT3_PATTERN_ROWS    64
#define PT3_MAX_ORNAMENTS   16
#define PT3_MAX_SAMPLES     32
#define PT3_MAX_ENVELOPES   16

// Команды PT3
enum PT3Command {
  CMD_NONE = 0,
  CMD_SETORN = 1,      // 1xxx: установить огибающую/орнамент
  CMD_SLIDEUP = 2,     // 2xxx: плавное повышение тона
  CMD_SLIDEDN = 3,     // 3xxx: плавное понижение тона
  CMD_PORTAMENTO = 4,  // 4xxx: портаменто
  CMD_VIBRATO = 5,     // 5xxx: вибрато
  CMD_TONESLIDE = 6,   // 6xxx: слайд тона + вибрато
  CMD_DELAY = 8,       // 8xxx: задержка сэмпла
  CMD_DELAYEDORN = 9,  // 9xxx: отложенный орнамент
  CMD_SPEED = 15       // Fxxx: скорость
};

struct PT3Ornament {
  int8_t  data[32];
  uint8_t length;
  uint8_t loop;
};

struct PT3Sample {
  struct {
    int8_t  toneShift;
    uint8_t noise;
    bool    noiseEnable;
    uint8_t volume;      // 0-3, реальная = volume * 5 + additor
    bool    toneEnable;
  } lines[32];
  uint8_t length;
  uint8_t loop;
  uint8_t additor;       // смещение громкости
};

struct PT3Envelope {
  uint16_t period;
  uint8_t  shape;
};

struct PT3PatternRow {
  uint8_t note;        // 0=пауза, 1=C, 2=C#...
  uint8_t octave;      // 0-7
  uint8_t sample;      // 0=нет
  uint8_t ornament;    // 0=нет
  bool    envEnable;
  uint8_t command;
  uint8_t param;
};

struct PT3Pattern {
  PT3PatternRow rows[PT3_PATTERN_ROWS][3];
};

class PT3Player {
public:
  PT3Player();
  
  bool load(const uint8_t* data, size_t size);
  void attachAY(AY8912* ay);
  
  void play();
  void stop();
  bool isPlaying();
  
  void tick();
  
  const char* getTitle()   { return title; }
  const char* getAuthor()  { return author; }
  uint8_t getPosition()    { return currentPosition; }
  uint8_t getPattern()     { return currentPattern; }
  uint8_t getRow()         { return currentRow; }

private:
  AY8912* ay;
  
  char     title[43];
  char     author[43];
  uint8_t  version;
  
  uint8_t  positions[PT3_MAX_POSITIONS];
  uint8_t  numPositions;
  uint8_t  loopPosition;
  
  PT3Pattern   patterns[PT3_MAX_PATTERNS];
  uint8_t      numPatterns;
  
  PT3Ornament  ornaments[PT3_MAX_ORNAMENTS];
  PT3Sample    samples[PT3_MAX_SAMPLES];
  PT3Envelope  envelopes[PT3_MAX_ENVELOPES];
  
  uint16_t freqTable[96];
  uint8_t  freqTableType;
  
  bool     playing;
  uint8_t  currentPosition;
  uint8_t  currentPattern;
  uint8_t  currentRow;
  uint8_t  speed;
  uint8_t  speedCounter;
  
  // Глобальная огибающая
  uint16_t envPeriod;
  uint8_t  envShape;
  bool     envChanged;
  
  struct ChannelState {
    uint8_t  sampleNum;
    uint8_t  samplePos;
    uint8_t  ornamentNum;
    uint8_t  ornamentPos;
    uint16_t tonePeriod;
    uint16_t basePeriod;
    uint8_t  volume;
    bool     envEnabled;
    uint8_t  envNum;
    
    // Эффекты
    int16_t  toneSlide;      // текущее смещение слайда
    int16_t  toneSlideStep;  // шаг слайда
    uint8_t  toneSlideCount;
    uint8_t  toneSlideSpeed;
    uint16_t toneTarget;     // цель portamento
    
    uint8_t  vibratoPos;
    uint8_t  vibratoDepth;
    uint8_t  vibratoSpeed;
    uint8_t  vibratoDelay;
    
    uint8_t  delayedSample;  // отложенный сэмпл
    uint8_t  delayedOrnament;// отложенный орнамент
    uint8_t  delayCounter;
    
    bool     toneOn;
    bool     noiseOn;
    uint8_t  noiseVal;
  } channels[3];
  
  // Внутренние методы
  bool parseHeader(const uint8_t* data, size_t size);
  void parsePositions(const uint8_t* data, uint16_t offset);
  void parsePatterns(const uint8_t* data, uint16_t offset);
  void parseOrnaments(const uint8_t* data, uint16_t offset);
  void parseSamples(const uint8_t* data, uint16_t offset);
  void parseEnvelopes(const uint8_t* data, uint16_t offset);
  void initFreqTable(uint8_t type);
  
  void processRow();
  void processChannel(uint8_t ch, PT3PatternRow& row);
  void updateChannel(uint8_t ch);
  void applyMixer(uint8_t ch);
  
  uint16_t noteToPeriod(uint8_t note, uint8_t octave);
  int8_t getVibratoValue(uint8_t pos, uint8_t depth);
};

#endif