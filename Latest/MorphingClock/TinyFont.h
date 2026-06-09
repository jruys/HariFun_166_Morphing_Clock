#ifndef TINYFONT_H
#define TINYFONT_H

#include <Arduino.h>
#include <PxMatrix.h>

#define TF_COLS 4
#define TF_ROWS 5

struct TFFace {
  char fface[TF_ROWS]; // 4 columns mapped onto a 5-row byte array structure
};

/**
 * @brief Draws a single micro-font character on an arbitrary coordinate matrix plane.
 */
void TFDrawChar(PxMATRIX* d, char value, char xo, char yo, int col);

/**
 * @brief Traverses strings and outputs proportional text structures to the matrix.
 * Optimized with 'const String&' to eliminate dynamic RAM object copying.
 */
void TFDrawText(PxMATRIX* d, const String& text, char xo, char yo, int col);

#endif // TINYFONT_H