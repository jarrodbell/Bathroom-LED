#define FONT_HEIGHT 5

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void getHeatColor(float value, float *red, float *green, float *blue) {
  const int NUM_COLORS = 3;
  //static float color[NUM_COLORS][3] = { {0,0,1}, {0,1,0}, {1,1,0}, {1,0,0} };
  // A static array of 4 colors:  (blue,   green,  yellow,  red) using {r,g,b} for each.

  // A static array of 3 colors:  (blue,   yellow,  red) using {r,g,b} for each.
  static float color[NUM_COLORS][3] = { {0,0,1}, {1,1,0}, {1,0,0} };

  int idx1;        // |-- Our desired color will be between these two indexes in "color".
  int idx2;        // |
  float fractBetween = 0;  // Fraction between "idx1" and "idx2" where our value is.

  if (value <= 0) {
    idx1 = idx2 = 0;    // accounts for an input <=0
  } else if (value >= 1) {
    idx1 = idx2 = NUM_COLORS-1; // accounts for an input >=0
  } else  {
    value = value * (NUM_COLORS-1);        // Will multiply value by 3.
    idx1  = floor(value);                  // Our desired color will be after this index.
    idx2  = idx1+1;                        // ... and before this index (inclusive).
    fractBetween = value - float(idx1);    // Distance between the two indexes (0-1).
  }

  *red   = (color[idx2][0] - color[idx1][0])*fractBetween + color[idx1][0];
  *green = (color[idx2][1] - color[idx1][1])*fractBetween + color[idx1][1];
  *blue  = (color[idx2][2] - color[idx1][2])*fractBetween + color[idx1][2];
}

// Fetch font character bitmap from flash
byte charBuffer[FONT_HEIGHT] = {0};
void loadCharBuffer(byte character) {
  byte mappedCharacter = character;
  if (mappedCharacter >= 32 && mappedCharacter <= 95) {
    mappedCharacter -= 32; // subtract font array offset
  } else if (mappedCharacter >= 97 && mappedCharacter <= 122) {
    mappedCharacter -= 64; // subtract font array offset and convert lowercase to uppercase
  } else {
    mappedCharacter = 96; // unknown character block
  }

  for (byte i = 0; i < FONT_HEIGHT; i++) {
    charBuffer[i] = pgm_read_byte(Font[mappedCharacter]+i);
  }

}

// Fetch a character value from a text string in flash
char loadStringChar(char* string, byte character) {
  return (char)string[character];
}

#define NORMAL 0
#define RAINBOW 1
#define charSpacing 1
#define charRowOffset (MATRIX_HEIGHT/2-(FONT_HEIGHT/2))
int numLoops = 0;
// Scroll a text string
void scrollText(char* message, byte style, CRGB fgColor, CRGB bgColor) {
  static byte currentMessageChar = 0;
  static byte currentCharColumn = 0;
  static byte paletteCycle = 0;
  static CRGB currentColor;
  static byte bitBuffer[16] = {0};
  static byte bitBufferPointer = 0;

  // startup tasks
  if (effectInit == false) {
    effectInit = true;
    currentMessageChar = 0;
    currentCharColumn = 0;
    loadCharBuffer(loadStringChar(message, currentMessageChar));
    currentPalette = RainbowColors_p;
    for (byte i = 0; i < MATRIX_WIDTH; i++) bitBuffer[i] = 0;
  }

  paletteCycle += 15;

  if (currentCharColumn < FONT_HEIGHT) { // characters are 5 pixels wide
    bitBuffer[(bitBufferPointer + MATRIX_WIDTH - 1) % MATRIX_WIDTH] = charBuffer[currentCharColumn]; // character
  } else {
    bitBuffer[(bitBufferPointer + MATRIX_WIDTH - 1) % MATRIX_WIDTH] = 0; // space
  }

  CRGB pixelColor;

  FastLED.clear();
  for (byte x = 0; x < MATRIX_WIDTH; x++) {
    for (byte y = 0; y < FONT_HEIGHT; y++) { // characters are 5 pixels tall
      if (bitRead(bitBuffer[(bitBufferPointer + x) % MATRIX_WIDTH], y) == 1) {
        if (style == RAINBOW) {
          pixelColor = ColorFromPalette(currentPalette, paletteCycle+y*16, 255);
        } else {
          pixelColor = fgColor;
        }
      } else {
        pixelColor = bgColor;
      }
      leds[XY(x, y + charRowOffset)] = pixelColor;
    }
  }

  currentCharColumn++;
  if (currentCharColumn > (4 + charSpacing)) {
    currentCharColumn = 0;
    currentMessageChar++;
    char nextChar = loadStringChar(message, currentMessageChar);
    if (nextChar == 0) { // null character at end of string
      currentMessageChar = 0;
      Serial.println((String)(currentMessageChar));
      nextChar = loadStringChar(message, currentMessageChar);
    }
    loadCharBuffer(nextChar);
  }

  bitBufferPointer++;
  if (bitBufferPointer > 15) bitBufferPointer = 0;
}

uint16_t scrollTemp() {
  char result[5]; // Buffer big enough for 7-character float
  dtostrf(temp, 3, 1, result);
  sprintf(result,"%s ",result);
  float r, g, b;
  float mapped = mapfloat(constrain(temp, 15, 32), 15, 32, 0, 1);
  getHeatColor(mapped, &r, &g, &b); // Returns RGB values in 0.0 to 1.0 range
  //Serial.println((String)"Mapped Color = R: " + r + ", G: " + g + ", B: " + b + ", RAW: " + mapped);

  EVERY_N_MILLISECONDS(150) { // Use secondary timer rather than delaying too long in the main loop (causes button issues)
    scrollText(result, NORMAL, CRGB(r*255,g*255,b*255), CRGB::Black);
  }
  return 10;
}

uint16_t scrollHello() {
  //scrollText("HELLO ", RAINBOW, 0, CRGB::Black);
  EVERY_N_MILLISECONDS(100) { // Use secondary timer rather than delaying too long in the main loop (causes button issues)
    scrollText("HELLO ", NORMAL, CRGB::White, CRGB::Black);
  }
  return 10;
}

uint16_t scrollOff() {

  EVERY_N_MILLISECONDS(150) { // Use secondary timer rather than delaying too long in the main loop (causes button issues)
    if (millis() - lastScroll > 10000) {
      scrollText("GO ", NORMAL, CRGB::White, CRGB::Black);
      lastScroll = millis();
      effectInit = true;
    }
  }
  return 10;
}
