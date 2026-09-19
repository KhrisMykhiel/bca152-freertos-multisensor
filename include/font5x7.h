#pragma once
#include <stdint.h>

// Minimal 5x7 bitmap font, columns-major (5 bytes per glyph, 7 rows used
// of each byte's low bits). Covers space, '.', '-', '%', ':', digits 0-9,
// and uppercase A-Z — enough for this project's OLED strings.
// Index into FONT5x7 with font5x7_index(c).
extern const uint8_t FONT5x7[][5];
int font5x7_index(char c);
