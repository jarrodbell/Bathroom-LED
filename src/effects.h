byte blackoutPos = 0;
byte blackoutWidth = 0;
bool checkBlackout(int i) {

  if (blackoutWidth == 0) return false;
  int blackoutStart = blackoutPos;
  int blackoutEnd = blackoutPos + blackoutWidth;

  // Wrap around
  if (blackoutStart < 0) blackoutStart = MATRIX_WIDTH + blackoutStart;
  if (blackoutEnd > MATRIX_WIDTH) blackoutEnd = blackoutEnd - MATRIX_WIDTH;

  if (blackoutStart < blackoutEnd) {
		if (i >= blackoutStart && i <= blackoutEnd) {
			return true;
		}
	} else {
		if (i >= blackoutStart || i <= blackoutEnd) {
			return true;
		}
	}

  return false;
}

uint16_t rainbow() {
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
  return 10;
}

void addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

uint16_t rainbowWithGlitter() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
  return 10;
}

uint16_t sparks() {
  //FastLED.clear();
  nscale8(leds, NUM_LEDS, 1);
  addGlitter(80);
  addGlitter(80);
  addGlitter(80);
  addGlitter(80);
  addGlitter(80);
  return 8;
}

// Definitions for twinkles.
#define PEAK_COLOR CRGB(MAXBRIGHTNESS,MAXBRIGHTNESS,MAXBRIGHTNESS)
#define TWINKLE_SPEED 10
int ledState[NUM_LEDS];
enum {SteadyDim, GettingBrighter, GettingDimmerAgain};

uint16_t twinkles() {
  fadeToBlackBy(leds, NUM_LEDS, 1);
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    if ( ledState[i] == SteadyDim) {
      // this pixels is currently: SteadyDim
      // so we randomly consider making it start getting brighter
      if ( random16(0, NUM_LEDS) < 1) {
        ledState[i] = GettingBrighter;
      }

    } else if ( ledState[i] == GettingBrighter ) {
      // this pixels is currently: GettingBrighter
      // so if it's at peak color, switch it to getting dimmer again
      if ( leds[i] >= PEAK_COLOR) {
        ledState[i] = GettingDimmerAgain;
      } else {
        // otherwise, just keep brightening it:
        leds[i] += CRGB(TWINKLE_SPEED, TWINKLE_SPEED, TWINKLE_SPEED);
      }

    } else { // getting dimmer again
      // this pixels is currently: GettingDimmerAgain
      // so if it's back to base color, switch it to steady dim
      if (leds[i] <= CRGB(0, 0, 0) ) {
        leds[i] = CRGB(0, 0, 0); // reset to exact base color, in case we overshot
        ledState[i] = SteadyDim;
      } else {
        // otherwise, just keep dimming it down:
        leds[i] -= CRGB(TWINKLE_SPEED/2, TWINKLE_SPEED/2, TWINKLE_SPEED/2);
      }
      //      mapTo(i, leds[i]);
    }
  }
  return 10;
}

// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING  50

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 60


const uint8_t kxMatrixWidth = 10;
const uint8_t kxMatrixHeight = 6;

uint16_t XY2( uint8_t x, uint8_t y) {
  return (y * kxMatrixWidth) + x;
}

uint16_t Fire2012() {

  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
    for( int i = 0; i < NUM_LEDS; i++) {
      heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
    }

    // Step 2.  Heat from each cell drifts 'up' and diffuses a little
    for( int k= NUM_LEDS - 1; k >= 2; k--) {
      heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
    }

    // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
    if( random8() < SPARKING ) {
      int y = random8(7);
      heat[y] = qadd8( heat[y], random8(160,255) );
    }

    // Step 4.  Map from heat cells to LED colors
    for( int j = 0; j < NUM_LEDS; j++) {
      // Determine if the pos is within a blackout area
      leds[j] = HeatColor( heat[j]);
    }

    return 2; // ms delay
}

uint16_t beacon() {
  if (beaconCounter == 0) {
    if (counter <= NUM_LEDS ) {
      for (int j = 0; j < 11 ; j++) {
        leds[NUM_LEDS - 1 - counter - j] = CRGB::White;
      }
    }
    if (counter >= 9 ) {
      for (int j = 1; j < 11 ; j++) {
        leds[NUM_LEDS - 1 - counter + j] = CRGB::Black;
      }
    }
    counter = counter + 9;
  } else {
    fadeToBlackBy(leds, NUM_LEDS, 5);
  }
  return 10;
}

uint16_t glitter() {
  //  // startup tasks
  //  if (effectInit == false) {
  //    effectInit = true;
  //    effectDelay = 15;
  //  }

  // Draw one frame of the animation into the LED array
  for (int x = 0; x < kxMatrixWidth; x++) {
    for (int y = 0; y < kxMatrixHeight; y++) {
      leds[XY2(x, y)] = CHSV(gHue, 255, random8(10) * 25);
    }
  }

  return 50;
}

void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(240);
  }
}

void allBlack() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(100);
  }
}

uint16_t crawl() {
  if (counter % 2 == 0) {
    int i = counter;
    int j;
    int halfLEDs = NUM_LEDS/2;
    counter < halfLEDs ? j = counter + halfLEDs : j = counter - halfLEDs;
    leds[NUM_LEDS - 1 - i] = leds[i] = leds[NUM_LEDS - 1 - j] = leds[j] = CHSV(gHue, 255, 255);
    fadeall();
  }
  return 15;
}

uint16_t crawl2() {
  allBlack();
  for (int p = 0; p < 19 ; p++) {
    int n = 17 * p + crawlCounter * 10 % 17;
    if (n < NUM_LEDS) {
      leds[n] = CHSV(gHue, 255, 255);
    }
    if (n + 10 < NUM_LEDS) {
      leds[n + 10] = CHSV(gHue, 255, 255);
    }
  }
  return 15;
}

void blurpattern2() {
  static uint8_t kBorderWidth = 0;
  static uint8_t kSquareWidth = 64;
  //  if (effectInit == false) {
  //    effectInit = true;
  //    effectDelay = 10;
  //    fadingActive = false;
  //  }

  // Apply some blurring to whatever's already on the matrix
  // Note that we never actually clear the matrix, we just constantly
  // blur it repeatedly.  Since the blurring is 'lossy', there's
  // an automatic trend toward black -- by design.
  //  uint8_t blurAmount = dim8_raw( beatsin8(3, 64, 64) );
  //  blur1d( leds, 320, blurAmount);

  // Use three out-of-sync sine waves
  uint8_t  i = beatsin16(  91 / 2, kBorderWidth, kSquareWidth - kBorderWidth);
  uint8_t  j = beatsin16( 109 / 2, kBorderWidth, kSquareWidth - kBorderWidth);
  uint8_t  k = beatsin16(  73 / 2, kBorderWidth, kSquareWidth - kBorderWidth);

  // The color of each point shifts over time, each at a different speed.
  uint16_t ms = millis();
  leds[XY2( i, j)] += CHSV( ms / 29, 200, 255);
  leds[XY2( j, k)] += CHSV( ms / 41, 200, 255);
  leds[XY2( k, i)] += CHSV( ms / 73, 200, 255);
}

uint16_t cylon() {
  int trail = 5;
  fadeToBlackBy(leds, NUM_LEDS, trail);
  uint16_t j = beatsin8(10, 0, 255);
  j = map(j, 0, 255, trail, NUM_LEDS);
  for (int pos = j - trail ; pos < j + trail ; pos++) {
    leds[constrain(pos, 0, NUM_LEDS - 1)] = CHSV(gHue, 255, 192);
  }

  return 10;
}

uint16_t cylon2() {
  int trail = 5;
  fadeToBlackBy(leds, NUM_LEDS, trail);
  uint16_t j = beatsin8(10, 0, 255);
  uint16_t k = 255-j;
  j = map(j, 0, 255, trail, NUM_LEDS);
  k = map(k, 0, 255, trail, NUM_LEDS);
  for (int pos = j - trail ; pos < j + trail ; pos++) {
    leds[constrain(pos, 0, NUM_LEDS - 1)] += CHSV(gHue, 255, 192);
  }
  for (int pos = k - trail ; pos < k + trail ; pos++) {
    leds[constrain(pos, 0, NUM_LEDS - 1)] += CHSV(gHue+90, 255, 192);
  }
  return 10;
}

uint16_t confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, NUM_LEDS, 10);
  leds[random16(NUM_LEDS)] += ColorFromPalette(palette, gHue + random8(64), 255); // CHSV(gHue + random8(64), 200, 255);
  return 8;
}

uint16_t bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  static uint8_t BeatsPerMinute = 62;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(palette, gHue + (i * 2), beat - gHue + (i * 10));
  }
  EVERY_N_SECONDS(10) {
    BeatsPerMinute = random8(100) + 15;
  }
  return 8;
}

uint16_t juggle() {
  // N colored dots, weaving in and out of sync with each other
  fadeToBlackBy(leds, NUM_LEDS, 20);
  byte dothue = 0;
  byte dotCount = 3;
  for (int i = 0; i < dotCount; i++) {
    leds[beatsin16(i + dotCount - 1, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
    dothue += 256 / dotCount;
  }
  return 8;
}

// An animation to play while the crowd goes wild after the big performance
uint16_t applause()
{
  static uint16_t lastPixel = 0;
  fadeToBlackBy(leds, NUM_LEDS, 32);
  leds[lastPixel] = CHSV(random8(HUE_BLUE, HUE_PURPLE), 255, 255);
  lastPixel = random16(NUM_LEDS);
  leds[lastPixel] = CRGB::White;
  return 8;
}

// An "animation" to just fade to black.  Useful as the last track
// in a non-looping performance-oriented playlist.
uint16_t fadeToBlack()
{
  fadeToBlackBy(leds, NUM_LEDS, 10);
  return 8;
}

uint16_t sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 20);
  uint16_t pos = beatsin16(13, 0, NUM_LEDS);
  static uint16_t prevpos = 0;
  if ( pos < prevpos ) {
    fill_solid( leds + pos, (prevpos - pos) + 1, CHSV(gHue, 220, 255));
  } else {
    fill_solid( leds + prevpos, (pos - prevpos) + 1, CHSV( gHue, 220, 255));
  }
  yield();
  prevpos = pos;

  return 8;
}

// Pride2015 by Mark Kriegsman
// https://gist.github.com/kriegsman/964de772d64c502760e5

// This function draws rainbows with an ever-changing,
// widely-varying set of parameters.
uint16_t pride() {
  static uint16_t sPseudotime = 0;
  static uint16_t sLastMillis = 0;
  static uint16_t sHue16 = 0;

  uint8_t sat8 = beatsin88(87, 220, 250);
  uint8_t brightdepth = beatsin88(341, 96, 224);
  uint16_t brightnessthetainc16 = beatsin88(203, (25 * 256), (40 * 256));
  uint8_t msmultiplier = beatsin88(147, 23, 60);

  uint16_t hue16 = sHue16;//gHue * 256;
  uint16_t hueinc16 = beatsin88(113, 1, 3000);

  uint16_t ms = millis();
  uint16_t deltams = ms - sLastMillis;
  sLastMillis = ms;
  sPseudotime += deltams * msmultiplier;
  sHue16 += deltams * beatsin88(400, 5, 9);
  uint16_t brightnesstheta16 = sPseudotime;

  for (int i = 0; i < NUM_LEDS; i++) {
    hue16 += hueinc16;
    uint8_t hue8 = hue16 / 256;

    brightnesstheta16 += brightnessthetainc16;
    uint16_t b16 = sin16(brightnesstheta16) + 32768;

    uint16_t bri16 = (uint32_t) ((uint32_t) b16 * (uint32_t) b16) / 65536;
    uint8_t bri8 = (uint32_t) (((uint32_t) bri16) * brightdepth) / 65536;
    bri8 += (255 - brightdepth);

    CRGB newcolor = CHSV(hue8, sat8, bri8);

    uint8_t pixelnumber = i;
    pixelnumber = (NUM_LEDS - 1) - pixelnumber;

    nblend(leds[pixelnumber], newcolor, 64);
  }

  return 0;
}

CRGB leadingRainColor = CRGB(128,255,128);
uint8_t trailGreen = 130;
CRGB trailRainColor = CRGB(27,trailGreen,39);
byte activeColumns[MATRIX_WIDTH];
byte activeSpeeds[MATRIX_WIDTH];
unsigned long activeTimes[MATRIX_WIDTH];
unsigned long currentMillis;
uint8_t fadeBy = trailGreen/MATRIX_HEIGHT;
uint16_t fallingCode() {

  EVERY_N_MILLISECONDS(75) {
    if (!effectInit) {
      // Clear LEDs first time the effect starts
      FastLED.clear();
      memset(activeColumns, 0, sizeof(activeColumns));
      memset(activeSpeeds, 0, sizeof(activeSpeeds));
      memset(activeTimes, 0, sizeof(activeTimes));
      effectInit = true;
    }
    // move code downward
    currentMillis = millis();
    // start with lowest row to allow proper overlapping on each column
    for (int8_t row=MATRIX_HEIGHT-1; row>=0; row--) {
      for (int8_t col=0; col<MATRIX_WIDTH; col++) {
        if (leds[XY(col, row)] == leadingRainColor && currentMillis - activeTimes[col] > activeSpeeds[col]) {
          activeTimes[col] = currentMillis;
          leds[XY(col, row)] = trailRainColor; // create trail
          if (row < MATRIX_HEIGHT-1) {
            leds[XY(col, row+1)] = leadingRainColor; // Move down
          } else {
            activeColumns[col] = 0; // End of col
          }
        }
      }
    }

    // fade all trail leds
    for (int i = 0; i < NUM_LEDS; i++) {
      if (leds[i].g != 255) leds[i] -= CRGB(fadeBy, fadeBy, fadeBy);
    }

    // check for empty screen to ensure code spawn
    bool emptyScreen = true;
    for(int i = 0; i < NUM_LEDS; i++) {
      if (leds[i]) {
        emptyScreen = false;
        break;
      }
    }

    // spawn new falling code
    // lower number == more frequent spawns
    if (random8(2) == 0 || emptyScreen) {
      int8_t spawnX = random8(MATRIX_WIDTH);
      if (!activeColumns[spawnX]) {
        leds[XY(spawnX, 0)] = leadingRainColor;
        activeColumns[spawnX] = 1;
        activeSpeeds[spawnX] = random8(100) + 25;
        activeTimes[spawnX] = millis();
      }
    }
  }

  return 10;
}

uint16_t solid() {
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  return 10;
}

uint16_t solidChanging() {
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  return 10;
}
