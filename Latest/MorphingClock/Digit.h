#ifndef DIGIT_H
#define DIGIT_H

#include <Arduino.h>
#include <PxMatrix.h> // LED Matrix driver: https://github.com/2dom/PxMatrix

class Digit {
  public:
    /**
     * @brief Constructs a Digit representation on a PxMATRIX screen.
     * @param d Pointer to the display instance.
     * @param value Initial digit value (0-9).
     * @param xo X coordinate offset for the digit's top-left corner.
     * @param yo Y coordinate offset for the digit's bottom edge (adjusted by inverse height coordinate).
     * @param color Default 16-bit color (RGB565) of the active segments.
     */
    Digit(PxMATRIX* d, byte value, uint16_t xo, uint16_t yo, uint16_t color);

    void Draw(byte value);       // Instantly draws a static digit without any transition
    void Morph(void);            // Manages incremental state machine frames during morphing transitions
    void SetValue(byte value);   // Targets a new value to morph towards
    void SetColor(uint16_t color);
    void DrawColon(uint16_t c);  // Draws a generic column separator dot pattern adjacent to the digit

  private:
    PxMATRIX* _display;          // Screen reference
    byte _oldvalue;              // Source value for morph transition state machine
    byte _value;                 // Target value for morph transition state machine
    byte _morphcnt;              // Current animation frame step/index counter
    uint16_t _color;             // Foreground 16-bit active segment color
    uint16_t xOffset;            // Local translation X coordinate
    uint16_t yOffset;            // Local translation Y coordinate

    // Display drawing wrappers with custom translation/invert metrics
    void drawPixel(uint16_t x, uint16_t y, uint16_t c);
    void drawFillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t c);
    void drawLine(uint16_t x, uint16_t y, uint16_t x2, uint16_t y2, uint16_t c);
    void drawSeg(byte seg);      // Renders an individual standard 7-segment piece

    // State machine frames for specific target numbers
    void Morph0();
    void Morph1();
    void Morph2();
    void Morph3();
    void Morph4();
    void Morph5();
    void Morph6();
    void Morph7();
    void Morph8();
    void Morph9();
};

#endif // DIGIT_H