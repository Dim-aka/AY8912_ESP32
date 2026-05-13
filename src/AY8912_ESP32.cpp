#include "AY8912_ESP32.h"

// Точная таблица DAC из ayumi (true-grue)
static const float AY_DAC[32] = {
  0.0f, 0.0f,
  0.00999465934234f, 0.00999465934234f,
  0.0144502937362f,  0.0144502937362f,
  0.0210574502174f,  0.0210574502174f,
  0.0307011520562f,  0.0307011520562f,
  0.0455481803616f,  0.0455481803616f,
  0.0644998855573f,  0.0644998855573f,
  0.107362478065f,   0.107362478065f,
  0.126588845655f,   0.126588845655f,
  0.20498970016f,    0.20498970016f,
  0.292210269322f,   0.292210269322f,
  0.372838941024f,   0.372838941024f,
  0.492530708782f,   0.492530708782f,
  0.635324635691f,   0.635324635691f,
  0.805584802014f,   0.805584802014f,
  1.0f, 1.0f
};

static const float PAN_L[3] = {0.30f, 0.50f, 0.70f};
static const float PAN_R[3] = {0.70f, 0.50f, 0.30f};

static const uint8_t ENV_SEG[16][2] = {
  {0, 2}, {0, 2}, {0, 2}, {0, 2},
  {1, 2}, {1, 2}, {1, 2}, {1, 2},
  {0, 0}, {0, 2}, {0, 1}, {0, 3},
  {1, 1}, {1, 3}, {1, 0}, {1, 2}
};

// Таблица периодов для октавы 3 (CLK=1773400)
const uint16_t AY8912::PERIOD_TABLE[12] = {
  0x1D5, // C  130.81 Hz
  0x1C2, // C# 138.59
  0x1B0, // D  146.83
  0x19E, // D# 155.56
  0x18D, // E  164.81
  0x17D, // F  174.61
  0x16D, // F# 185.00
  0x15E, // G  196.00
  0x14F, // G# 207.65
  0x141, // A  220.00
  0x134, // A# 233.08
  0x127  // B  246.94
};

AY8912::AY8912() {}

void AY8912::begin(uint32_t sampleRate, uint32_t clockRate) {
  _sampleRate = sampleRate;
  _clockRate  = clockRate;
  _toneAdv    = (float)_clockRate / (16.0f * (float)_sampleRate);
  _envAdvBase = (float)_clockRate / (256.0f * (float)_sampleRate);

  _panL[0] = 0.70f; _panR[0] = 0.30f;
  _panL[1] = 0.50f; _panR[1] = 0.50f;
  _panL[2] = 0.30f; _panR[2] = 0.70f;

  reset();
}

void AY8912::reset() {
  memset(&emu, 0, sizeof(emu));
  emu.noiseState = 1;
  for (int i = 0; i < 3; i++) {
    emu.tonePeriod[i] = 1;
    emu.toneOut[i]    = 1;
    emu.toneCount[i]  = 1.0f;
  }
  emu.noisePeriod = 1;
  emu.noiseCount  = 1.0f;
  emu.envPeriod   = 1;
  emu.envCount    = 1.0f;
  emu.envInc      = _envAdvBase;
  emu.envShape    = 0;
  emu.envSegment  = 0;
  _envResetSegment();
}

void AY8912::setPan(uint8_t channel, float pan) {
  if (channel > 2) return;
  if (pan < 0.0f) pan = 0.0f;
  if (pan > 1.0f) pan = 1.0f;
  _panL[channel] = 1.0f - pan;
  _panR[channel] = pan;
}

uint8_t AY8912::readRegister(uint8_t reg) const {
  if (reg > 15) return 0;
  return emu.reg[reg];
}

void AY8912::setMixerBit(uint8_t bit, bool value) {
  if (bit > 5) return;
  uint8_t mixer = emu.reg[7];
  if (value) mixer |= (1 << bit);
  else       mixer &= ~(1 << bit);
  writeRegister(7, mixer);
}

void AY8912::_envResetSegment() {
  uint8_t func = ENV_SEG[emu.envShape & 0xF][emu.envSegment & 1];
  emu.envLevel = (func == 0 || func == 3) ? 31 : 0;
}

void AY8912::_updateReg(int r) {
  switch (r) {
    case 0: case 1:
      emu.tonePeriod[0] = (emu.reg[0] | ((emu.reg[1] & 0x0F) << 8));
      if (emu.tonePeriod[0] == 0) emu.tonePeriod[0] = 1;
      break;
    case 2: case 3:
      emu.tonePeriod[1] = (emu.reg[2] | ((emu.reg[3] & 0x0F) << 8));
      if (emu.tonePeriod[1] == 0) emu.tonePeriod[1] = 1;
      break;
    case 4: case 5:
      emu.tonePeriod[2] = (emu.reg[4] | ((emu.reg[5] & 0x0F) << 8));
      if (emu.tonePeriod[2] == 0) emu.tonePeriod[2] = 1;
      break;
    case 6:
      emu.noisePeriod = emu.reg[6] & 0x1F;
      if (emu.noisePeriod == 0) emu.noisePeriod = 1;
      break;
    case 11: case 12:
      emu.envPeriod = (emu.reg[11] | (emu.reg[12] << 8));
      if (emu.envPeriod == 0) emu.envPeriod = 1;
      emu.envInc = _envAdvBase / (float)emu.envPeriod;
      break;
    case 13:
      emu.envShape   = emu.reg[13] & 0x0F;
      emu.envCount   = 0;
      emu.envSegment = 0;
      _envResetSegment();
      break;
  }
}

void AY8912::writeRegister(uint8_t reg, uint8_t value) {
  if (reg > 15) return;
  if (emu.reg[reg] == value) return;
  emu.reg[reg] = value;
  _updateReg(reg);
}

// === НОВЫЕ УДОБНЫЕ ФУНКЦИИ ===

uint16_t AY8912::_midiToPeriod(uint8_t midiNote) {
  int octave = (midiNote / 12) - 3;
  int note   = midiNote % 12;
  
  uint16_t period = PERIOD_TABLE[note];
  
  if (octave > 0) {
    period >>= octave;
  } else if (octave < 0) {
    period <<= (-octave);
  }
  
  if (period < 1) period = 1;
  if (period > 4095) period = 4095;
  return period;
}

void AY8912::setNote(uint8_t channel, uint8_t midiNote) {
  if (channel > 2 || midiNote > 127) return;
  uint16_t period = _midiToPeriod(midiNote);
  uint8_t r = channel * 2;
  writeRegister(r, period & 0xFF);
  writeRegister(r + 1, (period >> 8) & 0x0F);
}

void AY8912::setNote(uint8_t channel, uint8_t note, uint8_t octave) {
  if (note > 11 || octave > 7) return;
  uint8_t midiNote = octave * 12 + note;
  setNote(channel, midiNote);
}

void AY8912::enableTone(uint8_t channel, bool enable) {
  if (channel > 2) return;
  setMixerBit(channel, !enable);
}

void AY8912::enableNoise(uint8_t channel, bool enable) {
  if (channel > 2) return;
  setMixerBit(channel + 3, !enable);
}

void AY8912::setVolume(uint8_t channel, uint8_t volume) {
  if (channel > 2 || volume > 15) return;
  uint8_t vreg = emu.reg[8 + channel];
  vreg = (vreg & 0x10) | (volume & 0x0F);
  writeRegister(8 + channel, vreg);
}

void AY8912::setEnvelopePeriod(uint16_t period) {
  if (period < 1) period = 1;
  writeRegister(11, period & 0xFF);
  writeRegister(12, (period >> 8) & 0xFF);
}

void AY8912::setEnvelopeShape(uint8_t shape) {
  if (shape > 15) return;
  writeRegister(13, shape);
}

void AY8912::enableEnvelope(uint8_t channel, bool enable) {
  if (channel > 2) return;
  uint8_t vreg = emu.reg[8 + channel];
  if (enable) vreg |= 0x10;
  else        vreg &= ~0x10;
  writeRegister(8 + channel, vreg);
}

// === ГЕНЕРАЦИЯ ЗВУКА ===

void AY8912::process(float& outLeft, float& outRight) {
  const float TONE_ADV = (float)_clockRate / (16.0f * (float)_sampleRate);

  for (int ch = 0; ch < 3; ch++) {
    emu.toneCount[ch] -= TONE_ADV;
    while (emu.toneCount[ch] <= 0.0f) {
      emu.toneCount[ch] += emu.tonePeriod[ch];
      emu.toneOut[ch] ^= 1;
    }
  }

  emu.noiseCount -= TONE_ADV;
  while (emu.noiseCount <= 0.0f) {
    emu.noiseCount += emu.noisePeriod;
    uint32_t bit = ((emu.noiseState ^ (emu.noiseState >> 3)) & 1);
    emu.noiseState = (emu.noiseState >> 1) | (bit << 16);
  }

  emu.envCount -= emu.envInc;
  while (emu.envCount <= 0.0f) {
    emu.envCount += emu.envPeriod;
    uint8_t func = ENV_SEG[emu.envShape & 0x0F][emu.envSegment & 1];
    switch (func) {
      case 0:
        emu.envLevel -= 1;
        if (emu.envLevel < 0) {
          emu.envSegment ^= 1;
          _envResetSegment();
        }
        break;
      case 1:
        emu.envLevel += 1;
        if (emu.envLevel > 31) {
          emu.envSegment ^= 1;
          _envResetSegment();
        }
        break;
      case 2: case 3:
        break;
    }
  }

  int noise    = emu.noiseState & 1;
  int envelope = emu.envLevel;
  float mixL   = 0.0f;
  float mixR   = 0.0f;

  for (int ch = 0; ch < 3; ch++) {
    uint8_t mixer = emu.reg[7];
    int t_off = (mixer >> ch) & 1;
    int n_off = (mixer >> (ch + 3)) & 1;
    int out   = (emu.toneOut[ch] | t_off) & (noise | n_off);
    if (!out) continue;

    uint8_t vreg   = emu.reg[8 + ch];
    int volIdx     = (vreg & 0x10) ? envelope : ((vreg & 0x0F) * 2 + 1);
    float vol      = AY_DAC[volIdx];

    mixL += vol * _panL[ch];
    mixR += vol * _panR[ch];
  }

  // DC blocker
  emu.dcL += 0.002f * (mixL - emu.dcL);
  emu.dcR += 0.002f * (mixR - emu.dcR);
  mixL -= emu.dcL;
  mixR -= emu.dcR;

  outLeft  = tanhf(mixL * 0.8f);
  outRight = tanhf(mixR * 0.8f);
}