#include "PT3Player.h"

// Таблицы частот
static const uint16_t FREQ_TABLE_VORTEX[96] = {
  0x0D1B,0x0C55,0x0B96,0x0ADE,0x0A2C,0x0980,0x08DA,0x0839,0x079D,0x0706,0x0674,0x05E6,
  0x068E,0x062A,0x05CB,0x056F,0x0516,0x04C0,0x046D,0x041C,0x03CF,0x0383,0x033A,0x02F3,
  0x0347,0x0315,0x02E6,0x02B8,0x028B,0x0260,0x0237,0x020E,0x01E8,0x01C2,0x019D,0x017A,
  0x01A4,0x018B,0x0173,0x015C,0x0146,0x0130,0x011C,0x0107,0x00F4,0x00E1,0x00CF,0x00BD,
  0x00D2,0x00C5,0x00B9,0x00AE,0x00A3,0x0098,0x008E,0x0084,0x007A,0x0071,0x0068,0x005F,
  0x0069,0x0063,0x005D,0x0057,0x0052,0x004C,0x0047,0x0042,0x003D,0x0039,0x0034,0x0030,
  0x0035,0x0031,0x002E,0x002B,0x0029,0x0026,0x0024,0x0021,0x001F,0x001D,0x001A,0x0018,
  0x001A,0x0019,0x0017,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000D,0x000C
};

static const uint16_t FREQ_TABLE_ASC[96] = {
  0x0ECD,0x0E07,0x0D47,0x0C8E,0x0BDB,0x0B2F,0x0A8A,0x09E9,0x094E,0x08B7,0x0825,0x0798,
  0x0767,0x0704,0x06A4,0x0647,0x05EE,0x0598,0x0545,0x04F5,0x04A7,0x045C,0x0413,0x03CC,
  0x03B4,0x0382,0x0352,0x0324,0x02F7,0x02CC,0x02A3,0x027B,0x0254,0x022E,0x020A,0x01E6,
  0x01DA,0x01C1,0x01A9,0x0192,0x017C,0x0166,0x0152,0x013E,0x012A,0x0117,0x0105,0x00F3,
  0x00ED,0x00E1,0x00D5,0x00C9,0x00BE,0x00B3,0x00A9,0x009F,0x0095,0x008C,0x0083,0x007A,
  0x0077,0x0070,0x006B,0x0065,0x005F,0x005A,0x0055,0x0050,0x004B,0x0046,0x0042,0x003D,
  0x003C,0x0038,0x0035,0x0033,0x0030,0x002D,0x002B,0x0028,0x0026,0x0023,0x0021,0x001F,
  0x001E,0x001C,0x001B,0x0019,0x0018,0x0017,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010
};

static const uint16_t FREQ_TABLE_PRO[96] = {
  0x0D1B,0x0C55,0x0B96,0x0ADE,0x0A2C,0x0980,0x08DA,0x0839,0x079D,0x0706,0x0674,0x05E6,
  0x068E,0x062A,0x05CB,0x056F,0x0516,0x04C0,0x046D,0x041C,0x03CF,0x0383,0x033A,0x02F3,
  0x0347,0x0315,0x02E6,0x02B8,0x028B,0x0260,0x0237,0x020E,0x01E8,0x01C2,0x019D,0x017A,
  0x01A4,0x018B,0x0173,0x015C,0x0146,0x0130,0x011C,0x0107,0x00F4,0x00E1,0x00CF,0x00BD,
  0x00D2,0x00C5,0x00B9,0x00AE,0x00A3,0x0098,0x008E,0x0084,0x007A,0x0071,0x0068,0x005F,
  0x0069,0x0063,0x005D,0x0057,0x0052,0x004C,0x0047,0x0042,0x003D,0x0039,0x0034,0x0030,
  0x0035,0x0031,0x002E,0x002B,0x0029,0x0026,0x0024,0x0021,0x001F,0x001D,0x001A,0x0018,
  0x001A,0x0019,0x0017,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010,0x000F,0x000D,0x000C
};

// Таблица вибрато (синус)
static const int8_t VIBRATO_TABLE[32] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 5, 4, 3, 2, 1,
  0, -1, -2, -3, -4, -5, -6, -7, -8, -7, -6, -5, -4, -3, -2, -1
};

PT3Player::PT3Player() {
  ay = nullptr;
  playing = false;
  memset(title, 0, sizeof(title));
  memset(author, 0, sizeof(author));
  envPeriod = 0;
  envShape = 0;
  envChanged = false;
}

bool PT3Player::load(const uint8_t* data, size_t size) {
  if (size < 200) return false;
  
  // Проверка сигнатуры
  bool hasSignature = (memcmp(data, "ProTracker 3.", 13) == 0);
  
  if (hasSignature) {
    version = data[13] - '0';
  } else {
    // Пробуем определить по структуре
    version = 3;
  }
  
  // Читаем заголовок
  int headerOffset = hasSignature ? 0 : 0;
  
  if (hasSignature) {
    memcpy(title, data + 15, 42);
    title[42] = '\0';
    memcpy(author, data + 57, 42);
    author[42] = '\0';
  }
  
  uint16_t envOffset  = data[headerOffset + 99] | (data[headerOffset + 100] << 8);
  uint16_t ornOffset  = data[headerOffset + 101] | (data[headerOffset + 102] << 8);
  uint16_t ptrnOffset = data[headerOffset + 103] | (data[headerOffset + 104] << 8);
  freqTableType       = data[headerOffset + 105];
  loopPosition        = data[headerOffset + 110];
  uint16_t posOffset  = data[headerOffset + 111] | (data[headerOffset + 112] << 8);
  uint16_t dataOffset = data[headerOffset + 113] | (data[headerOffset + 114] << 8);
  
  // Корректировка смещений для файлов без сигнатуры
  if (!hasSignature) {
    posOffset = 0x63; // 99
    dataOffset = data[99] | (data[100] << 8);
    ornOffset = data[101] | (data[102] << 8);
    ptrnOffset = data[103] | (data[104] << 8);
    envOffset = data[105] | (data[106] << 8);
    freqTableType = data[107];
  }
  
  initFreqTable(freqTableType);
  
  parsePositions(data, posOffset);
  
  // Паттерны начинаются после позиций
  uint16_t patternsStart = posOffset + numPositions + 1; // +1 для 0xFF
  
  parsePatterns(data, patternsStart);
  parseOrnaments(data, ornOffset);
  parseSamples(data, ptrnOffset);
  parseEnvelopes(data, envOffset);
  
  return numPositions > 0 && numPatterns > 0;
}

void PT3Player::initFreqTable(uint8_t type) {
  const uint16_t* src = FREQ_TABLE_VORTEX;
  if (type == 1) src = FREQ_TABLE_ASC;
  else if (type == 2) src = FREQ_TABLE_PRO;
  
  for (int i = 0; i < 96; i++) {
    freqTable[i] = src[i];
  }
}

void PT3Player::parsePositions(const uint8_t* data, uint16_t offset) {
  numPositions = 0;
  while (numPositions < PT3_MAX_POSITIONS) {
    uint8_t pos = data[offset++];
    if (pos == 0xFF) break;
    positions[numPositions++] = pos;
  }
}

void PT3Player::parsePatterns(const uint8_t* data, uint16_t offset) {
  memset(patterns, 0, sizeof(patterns));
  numPatterns = 0;
  
  uint16_t ptr = offset;
  
  // Определяем количество паттернов по позициям
  uint8_t maxPattern = 0;
  for (int i = 0; i < numPositions; i++) {
    if (positions[i] > maxPattern) maxPattern = positions[i];
  }
  
  for (int p = 0; p <= maxPattern && p < PT3_MAX_PATTERNS; p++) {
    for (int r = 0; r < PT3_PATTERN_ROWS; r++) {
      for (int ch = 0; ch < 3; ch++) {
        uint8_t b1 = data[ptr++];
        uint8_t b2 = data[ptr++];
        uint8_t b3 = data[ptr++];
        
        PT3PatternRow& row = patterns[p].rows[r][ch];
        
        // Распаковка PT3
        if (b1 == 0) {
          // Пустая строка или спец. случай
          row.note = 0;
          row.octave = 0;
        } else {
          row.note = ((b1 - 1) % 12) + 1;
          row.octave = (b1 - 1) / 12;
        }
        
        row.sample = b2 & 0x0F;
        row.envEnable = (b2 & 0x40) != 0;
        
        // Команда и параметр
        if (b2 & 0x80) {
          // Есть команда
          row.command = (b3 >> 4) & 0x0F;
          row.param = ((b2 >> 4) & 0x03) | ((b3 & 0x0F) << 2);
        } else {
          row.command = 0;
          row.param = 0;
        }
        
        // Орнамент
        if ((b2 & 0x80) == 0 || row.command == 0) {
          row.ornament = ((b2 >> 4) & 0x03) | ((b3 & 0x03) << 2);
        } else {
          row.ornament = 0;
        }
      }
    }
    numPatterns = p + 1;
  }
}

void PT3Player::parseOrnaments(const uint8_t* data, uint16_t offset) {
  memset(ornaments, 0, sizeof(ornaments));
  
  for (int o = 0; o < PT3_MAX_ORNAMENTS; o++) {
    uint8_t len = data[offset++];
    ornaments[o].length = len & 0x3F;
    ornaments[o].loop = (len >> 6) & 0x03;
    
    for (int i = 0; i < ornaments[o].length; i++) {
      int8_t val = (int8_t)data[offset++];
      ornaments[o].data[i] = val;
    }
  }
}

void PT3Player::parseSamples(const uint8_t* data, uint16_t offset) {
  memset(samples, 0, sizeof(samples));
  
  for (int s = 0; s < PT3_MAX_SAMPLES; s++) {
    uint8_t flags = data[offset++];
    samples[s].length = flags & 0x0F;
    samples[s].loop = (flags >> 4) & 0x0F;
    
    for (int i = 0; i < samples[s].length; i++) {
      uint8_t b1 = data[offset++];
      uint8_t b2 = data[offset++];
      
      samples[s].lines[i].toneShift = (b1 & 0x0F);
      if (b1 & 0x10) samples[s].lines[i].toneShift = -samples[s].lines[i].toneShift;
      samples[s].lines[i].toneEnable = (b1 & 0x20) == 0;
      samples[s].lines[i].noise = b2 & 0x1F;
      samples[s].lines[i].noiseEnable = (b2 & 0x20) != 0;
      samples[s].lines[i].volume = (b2 >> 6) & 0x03;
    }
    
    samples[s].additor = (flags >> 4) & 0x0F;
  }
}

void PT3Player::parseEnvelopes(const uint8_t* data, uint16_t offset) {
  memset(envelopes, 0, sizeof(envelopes));
  
  for (int e = 0; e < PT3_MAX_ENVELOPES; e++) {
    envelopes[e].period = data[offset] | (data[offset + 1] << 8);
    envelopes[e].shape = data[offset + 2];
    offset += 3;
  }
}

void PT3Player::attachAY(AY8912* ayChip) {
  ay = ayChip;
}

void PT3Player::play() {
  playing = true;
  currentPosition = 0;
  currentRow = 0;
  speed = 6;
  speedCounter = 0;
  envPeriod = 0;
  envShape = 0;
  envChanged = false;
  memset(channels, 0, sizeof(channels));
  
  // Включаем все каналы
  if (ay) {
    ay->writeRegister(7, 0x38); // тона вкл, шумы выкл
  }
}

void PT3Player::stop() {
  playing = false;
  if (ay) ay->reset();
}

bool PT3Player::isPlaying() {
  return playing;
}

void PT3Player::tick() {
  if (!playing || !ay) return;
  
  speedCounter++;
  if (speedCounter >= speed) {
    speedCounter = 0;
    processRow();
    
    currentRow++;
    if (currentRow >= PT3_PATTERN_ROWS) {
      currentRow = 0;
      currentPosition++;
      if (currentPosition >= numPositions) {
        currentPosition = loopPosition;
      }
    }
  }
  
  // Обновляем эффекты каждый тик
  for (int ch = 0; ch < 3; ch++) {
    updateChannel(ch);
  }
  
  // Глобальная огибающая
  if (envChanged) {
    ay->writeRegister(11, envPeriod & 0xFF);
    ay->writeRegister(12, (envPeriod >> 8) & 0xFF);
    ay->writeRegister(13, envShape);
    envChanged = false;
  }
}

void PT3Player::processRow() {
  if (currentPosition >= numPositions) return;
  
  currentPattern = positions[currentPosition];
  if (currentPattern >= numPatterns) return;
  
  for (int ch = 0; ch < 3; ch++) {
    PT3PatternRow& row = patterns[currentPattern].rows[currentRow][ch];
    processChannel(ch, row);
  }
}

void PT3Player::processChannel(uint8_t ch, PT3PatternRow& row) {
  ChannelState& state = channels[ch];
  
  // Новая нота
  if (row.note > 0) {
    state.basePeriod = noteToPeriod(row.note, row.octave);
    state.tonePeriod = state.basePeriod;
    state.samplePos = 0;
    state.ornamentPos = 0;
    state.vibratoPos = 0;
    state.vibratoDelay = 0;
    state.toneSlide = 0;
    state.toneSlideStep = 0;
    state.toneTarget = 0;
    
    // Сброс эффектов при новой ноте (кроме специальных случаев)
    if (row.command != CMD_PORTAMENTO && row.command != CMD_TONESLIDE) {
      state.toneSlideStep = 0;
    }
  }
  
  // Сэмпл
  if (row.sample > 0) {
    state.sampleNum = row.sample;
    state.samplePos = 0;
  }
  
  // Орнамент
  if (row.ornament > 0) {
    state.ornamentNum = row.ornament;
    state.ornamentPos = 0;
  }
  
  // Глобальная огибающая
  if (row.envEnable && row.ornament > 0) {
    envPeriod = envelopes[row.ornament].period;
    envShape = envelopes[row.ornament].shape;
    envChanged = true;
    
    // Включаем огибающую на канале
    state.envEnabled = true;
  }
  
  // Команды
  switch (row.command) {
    case CMD_SETORN: // 1xxx - установить орнамент
      state.ornamentNum = row.param & 0x0F;
      state.ornamentPos = 0;
      break;
      
    case CMD_SLIDEUP: // 2xxx - плавное повышение
      state.toneSlideStep = row.param;
      state.toneSlideSpeed = 1;
      state.toneSlideCount = 0;
      state.toneTarget = 0;
      break;
      
    case CMD_SLIDEDN: // 3xxx - плавное понижение
      state.toneSlideStep = -(int16_t)row.param;
      state.toneSlideSpeed = 1;
      state.toneSlideCount = 0;
      state.toneTarget = 0;
      break;
      
    case CMD_PORTAMENTO: // 4xxx - портаменто
      if (row.note > 0) {
        state.toneTarget = state.basePeriod;
        state.toneSlideStep = row.param;
        state.toneSlideSpeed = 1;
      }
      break;
      
    case CMD_VIBRATO: // 5xxx - вибрато
      state.vibratoDepth = row.param & 0x0F;
      state.vibratoSpeed = (row.param >> 4) & 0x0F;
      if (state.vibratoSpeed == 0) state.vibratoSpeed = 1;
      break;
      
    case CMD_TONESLIDE: // 6xxx - слайд + вибрато
      state.toneSlideStep = (int8_t)row.param;
      state.toneSlideSpeed = 1;
      break;
      
    case CMD_DELAY: // 8xxx - задержка сэмпла
      state.delayCounter = row.param;
      state.delayedSample = state.sampleNum;
      state.sampleNum = 0;
      break;
      
    case CMD_DELAYEDORN: // 9xxx - отложенный орнамент
      state.delayedOrnament = row.param;
      break;
      
    case CMD_SPEED: // Fxxx - скорость
      speed = row.param;
      if (speed < 1) speed = 1;
      if (speed > 31) speed = 31;
      break;
  }
}

void PT3Player::updateChannel(uint8_t ch) {
  ChannelState& state = channels[ch];
  if (state.basePeriod == 0 && state.tonePeriod == 0) return;
  
  uint16_t period = state.tonePeriod;
  uint8_t vol = 0;
  bool toneOn = true;
  bool noiseOn = false;
  uint8_t noiseVal = 0;
  
  // Обработка задержки сэмпла
  if (state.delayCounter > 0) {
    state.delayCounter--;
    if (state.delayCounter == 0 && state.delayedSample > 0) {
      state.sampleNum = state.delayedSample;
      state.samplePos = 0;
      state.delayedSample = 0;
    }
  }
  
  // Сэмпл
  if (state.sampleNum > 0 && state.sampleNum < PT3_MAX_SAMPLES) {
    PT3Sample& smp = samples[state.sampleNum];
    if (smp.length > 0 && state.samplePos < smp.length) {
      auto& line = smp.lines[state.samplePos];
      
      // Tone shift
      if (line.toneEnable) {
        period += line.toneShift;
      } else {
        toneOn = false;
      }
      
      // Volume: 0-3 -> 0-15 с additor
      vol = line.volume * 4 + smp.additor;
      if (vol > 15) vol = 15;
      
      // Noise
      if (line.noiseEnable) {
        noiseOn = true;
        noiseVal = line.noise;
      }
      
      // Следующая строка сэмпла
      state.samplePos++;
      if (state.samplePos >= smp.length) {
        state.samplePos = smp.loop;
      }
    }
  } else {
    // Нет сэмпла - используем громкость по умолчанию
    vol = state.volume;
  }
  
  // Орнамент
  if (state.ornamentNum > 0 && state.ornamentNum < PT3_MAX_ORNAMENTS) {
    PT3Ornament& orn = ornaments[state.ornamentNum];
    if (orn.length > 0 && state.ornamentPos < orn.length) {
      period += orn.data[state.ornamentPos];
      
      state.ornamentPos++;
      if (state.ornamentPos >= orn.length) {
        state.ornamentPos = orn.loop;
      }
    }
  }
  
  // Tone slide / Portamento
  if (state.toneSlideStep != 0) {
    state.toneSlideCount++;
    if (state.toneSlideCount >= state.toneSlideSpeed) {
      state.toneSlideCount = 0;
      
      if (state.toneTarget > 0) {
        // Portamento - движемся к цели
        if (state.tonePeriod > state.toneTarget) {
          state.tonePeriod -= state.toneSlideStep;
          if (state.tonePeriod <= state.toneTarget) {
            state.tonePeriod = state.toneTarget;
            state.toneTarget = 0;
          }
        } else if (state.tonePeriod < state.toneTarget) {
          state.tonePeriod += state.toneSlideStep;
          if (state.tonePeriod >= state.toneTarget) {
            state.tonePeriod = state.toneTarget;
            state.toneTarget = 0;
          }
        }
      } else {
        // Обычный слайд
        state.toneSlide += state.toneSlideStep;
      }
    }
  }
  
  // Применяем слайд
  period += state.toneSlide;
  
  // Vibrato
  if (state.vibratoDepth > 0) {
    state.vibratoDelay++;
    if (state.vibratoDelay >= state.vibratoSpeed) {
      state.vibratoDelay = 0;
      state.vibratoPos++;
      state.vibratoPos &= 0x1F; // 32 позиции
      
      int8_t vib = VIBRATO_TABLE[state.vibratoPos];
      period += (vib * state.vibratoDepth) >> 3;
    }
  }
  
  // Ограничения
  if (period < 1) period = 1;
  if (period > 4095) period = 4095;
  
  // Сохраняем текущий период
  state.tonePeriod = period;
  
  // Запись в AY
  uint8_t reg = ch * 2;
  ay->writeRegister(reg, period & 0xFF);
  ay->writeRegister(reg + 1, (period >> 8) & 0x0F);
  
  // Громкость + огибающая
  uint8_t vreg = vol & 0x0F;
  if (state.envEnabled) vreg |= 0x10;
  ay->writeRegister(8 + ch, vreg);
  
  // Микшер
  if (toneOn) {
    ay->setMixerBit(ch, false); // tone on (бит 0 = выкл)
  } else {
    ay->setMixerBit(ch, true);  // tone off
  }
  
  if (noiseOn) {
    ay->setMixerBit(ch + 3, false); // noise on
    ay->writeRegister(6, noiseVal);
  } else {
    ay->setMixerBit(ch + 3, true);  // noise off
  }
}

uint16_t PT3Player::noteToPeriod(uint8_t note, uint8_t octave) {
  if (note < 1 || note > 12) return 0;
  int idx = (octave * 12) + (note - 1);
  if (idx < 0) idx = 0;
  if (idx >= 96) idx = 95;
  return freqTable[idx];
}