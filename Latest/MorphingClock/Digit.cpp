#include "Digit.h"

// 7-Segment structural elements map
enum { sA, sB, sC, sD, sE, sF, sG };

#define segHeight 6
#define segWidth  segHeight
#define height    31u
#define width     63u
#define _black    0

// Layout flags for numbers mapped onto a 7-segment bit pattern byte
byte digitBits[] = {
  B11111100, // 0: ABCDEF--
  B01100000, // 1: -BC-----
  B11011010, // 2: AB-DE-G-
  B11110010, // 3: ABCD--G-
  B01100110, // 4: -BC--FG-
  B10110110, // 5: A-CD-FG-
  B10111110, // 6: A-CDEFG-
  B11100000, // 7: ABC-----
  B11111110, // 8: ABCDEFG-
  B11110110, // 9: ABCD_FG-
};

Digit::Digit(PxMATRIX* d, byte value, uint16_t xo, uint16_t yo, uint16_t color) {
  _display = d;
  _value = value;
  _oldvalue = 10; // Trigger redraw initially
  _morphcnt = 0;
  xOffset = xo;
  yOffset = yo;
  _color = color;
}

void Digit::SetValue(byte value) {
  _value = value;
  _morphcnt = 0;
}

void Digit::SetColor(uint16_t color) {
  _color = color;
}

// Coordinate systems inside draw commands map back to custom bottom-up orientation matrix math
void Digit::drawPixel(uint16_t x, uint16_t y, uint16_t c) {
  _display->drawPixel(xOffset + x, height - (y + yOffset), c);
}

void Digit::drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t c) {
  _display->drawLine(xOffset + x, height - (y + yOffset), xOffset + x2, height - (y2 + yOffset), c);
}

void Digit::drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c) {
  _display->fillRect(xOffset + x, height - (y + yOffset), w, h, c);
}

void Digit::DrawColon(uint16_t c) {
  drawFillRect(-3, segHeight - 1, 2, 2, c);
  drawFillRect(-3, segHeight + 1 + 3, 2, 2, c);
}

void Digit::drawSeg(byte seg) {
  switch (seg) {
    case sA: drawLine(1, segHeight * 2 + 2, segWidth, segHeight * 2 + 2, _color); break;
    case sB: drawLine(segWidth + 1, segHeight * 2 + 1, segWidth + 1, segHeight + 2, _color); break;
    case sC: drawLine(segWidth + 1, 1, segWidth + 1, segHeight, _color); break;
    case sD: drawLine(1, 0, segWidth, 0, _color); break;
    case sE: drawLine(0, 1, 0, segHeight, _color); break;
    case sF: drawLine(0, segHeight * 2 + 1, 0, segHeight + 2, _color); break;
    case sG: drawLine(1, segHeight + 1, segWidth, segHeight + 1, _color); break;
  }
}

void Digit::Draw(byte value) {
  byte pattern = digitBits[value];
  if (bitRead(pattern, 7)) drawSeg(sA);
  if (bitRead(pattern, 6)) drawSeg(sB);
  if (bitRead(pattern, 5)) drawSeg(sC);
  if (bitRead(pattern, 4)) drawSeg(sD);
  if (bitRead(pattern, 3)) drawSeg(sE);
  if (bitRead(pattern, 2)) drawSeg(sF);
  if (bitRead(pattern, 1)) drawSeg(sG);
  _value = value;
  _oldvalue = value;
  _morphcnt = segWidth + 2;
}

// Morph frames definitions
void Digit::Morph2() {
  if (_morphcnt <= segWidth) {
    int i = _morphcnt;
    if (i < segWidth) {
      drawPixel(segWidth - i, segHeight * 2 + 2, _color);
      drawPixel(segWidth - i, segHeight + 1, _color);
      drawPixel(segWidth - i, 0, _color);
    }
    drawLine(segWidth + 1 - i, 1, segWidth + 1 - i, segHeight, _black);
    drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph3() {
  if (_morphcnt <= segWidth) {
    int i = _morphcnt;
    drawLine(0 + i, 1, 0 + i, segHeight, _black);
    drawLine(1 + i, 1, 1 + i, segHeight, _color);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph4() {
  if (_morphcnt < segWidth) {
    int i = _morphcnt;
    drawPixel(segWidth - i, segHeight * 2 + 2, _black);
    drawPixel(0, segHeight * 2 + 1 - i, _color);
    drawPixel(1 + i, 0, _black);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph5() {
  if (_morphcnt < segWidth) {
    int i = _morphcnt;
    drawPixel(segWidth + 1, segHeight + 2 + i, _black);
    drawPixel(segWidth - i, segHeight * 2 + 2, _color);
    drawPixel(segWidth - i, 0, _color);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph6() {
  if (_morphcnt <= segWidth) {
    int i = _morphcnt;
    drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
    if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _black);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph7() {
  if (_morphcnt <= (segWidth + 1)) {
    int i = _morphcnt;
    drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, _black);
    drawLine(0 + i, 1, 0 + i, segHeight, _color);
    drawLine(0 + i - 1, segHeight * 2 + 1, 0 + i - 1, segHeight + 2, _black);
    drawLine(0 + i, segHeight * 2 + 1, 0 + i, segHeight + 2, _color);
    drawPixel(1 + i, 0, _black);
    drawPixel(1 + i, segHeight + 1, _black);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph8() {
  if (_morphcnt <= segWidth) {
    int i = _morphcnt;
    drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
    if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _black);
    drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
    if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _black);
    if (i < segWidth) {
      drawPixel(segWidth - i, 0, _color);
      drawPixel(segWidth - i, segHeight + 1, _color);
    }
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph9() {
  if (_morphcnt <= (segWidth + 1)) {
    int i = _morphcnt;
    drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, _black);
    drawLine(0 + i, 1, 0 + i, segHeight, _color);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph0() {
  if (_morphcnt <= segWidth) {
    int i = _morphcnt;
    if (_oldvalue == 1) {
      drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
      if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _black);
      drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
      if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _black);
      if (i < segWidth) drawPixel(segWidth - i, segHeight * 2 + 2, _color);
      if (i < segWidth) drawPixel(segWidth - i, 0, _color);
    }
    if (_oldvalue == 2) {
      drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
      if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _black);
      drawPixel(1 + i, segHeight + 1, _black);
      if (i < segWidth) drawPixel(segWidth + 1, segHeight + 1 - i, _color);
    }
    if (_oldvalue == 3) {
      drawLine(segWidth - i, segHeight * 2 + 1, segWidth - i, segHeight + 2, _color);
      if (i > 0) drawLine(segWidth - i + 1, segHeight * 2 + 1, segWidth - i + 1, segHeight + 2, _black);
      drawLine(segWidth - i, 1, segWidth - i, segHeight, _color);
      if (i > 0) drawLine(segWidth - i + 1, 1, segWidth - i + 1, segHeight, _black);
      drawPixel(segWidth - i, segHeight + 1, _black);
    }
    if (_oldvalue == 5) {
      if (i < segWidth) {
        if (i > 0) drawLine(1 + i, segHeight * 2 + 1, 1 + i, segHeight + 2, _black);
        drawLine(2 + i, segHeight * 2 + 1, 2 + i, segHeight + 2, _color);
      }
    }
    if (_oldvalue == 5 || _oldvalue == 9) {
      if (i < segWidth) drawPixel(segWidth - i, segHeight + 1, _black);
      if (i < segWidth) drawPixel(0, segHeight - i, _color);
    }
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph1() {
  if (_morphcnt <= (segWidth + 1)) {
    int i = _morphcnt;
    drawLine(0 + i - 1, 1, 0 + i - 1, segHeight, _black);
    drawLine(0 + i, 1, 0 + i, segHeight, _color);
    drawLine(0 + i - 1, segHeight * 2 + 1, 0 + i - 1, segHeight + 2, _black);
    drawLine(0 + i, segHeight * 2 + 1, 0 + i, segHeight + 2, _color);
    drawPixel(1 + i, segHeight * 2 + 2, _black);
    drawPixel(1 + i, 0, _black);
    drawPixel(1 + i, segHeight + 1, _black);
    _morphcnt++;
  } else {
    _oldvalue = _value;
  }
}

void Digit::Morph(void) {
  if (_value != _oldvalue) {
    switch (_value) {
      case 0: Morph0(); break;
      case 1: Morph1(); break;
      case 2: Morph2(); break;
      case 3: Morph3(); break;
      case 4: Morph4(); break;
      case 5: Morph5(); break;
      case 6: Morph6(); break;
      case 7: Morph7(); break;
      case 8: Morph8(); break;
      case 9: Morph9(); break;
      default: break;
    }
  }
}