#define FASTLED_ALLOW_INTERRUPTS 0
//#define FASTLED_INTERRUPT_RETRY_COUNT 3

// BELOW IS TO DISABLE WIFI AND REDUCE POWER CONSUMPTION
#define FREQUENCY    80                  // valid 80, 160
#include "ESP8266WiFi.h"
extern "C" {
#include "user_interface.h"
}

//#include <Arduino.h>
#include <FastLED.h>
#include <EEPROM.h>

//#include <WEMOS_DHT12.h>

#include "ClickButton.h"

// #include <LEDMatrix.h>
// #include <LEDText.h>
// #include <FontMatrise.h>

#define BUTTON_PIN D3
#define LED_PIN D5  // Data pin where the LED strip is connected to.
#define MIC_PIN A0

//DHT12 dht12;

ClickButton button1(BUTTON_PIN, LOW, CLICKBTN_PULLUP);

// Fade variables
int fadeValue = 64;
boolean fadeUp = false;    // false means fade down
boolean blackoutMoveUp = false;    // false means fade down
boolean blackoutSizeUp = false;    // false means fade down
boolean oldFadeUp = fadeUp;
boolean oldBlackoutMoveUp = blackoutMoveUp;
boolean oldBlackoutSizeUp = blackoutSizeUp;
const long fadeDelay = 30; // Time in milliseconds between brightness adjustment steps
long adjustFaderTime = 0;  // Time to adjust the fader
const long blackoutDelay = 200; // Time in milliseconds between blackout adjustment steps

// other
long currentTime;
int lastClicks = 0;
bool depressed = false;
long lastScroll = 0;
long lastTap = 0;
long numTaps = 0;

// How many leds in your strip?
//#define NUM_LEDS 62
#define MAX_LEDS 295
#define NUM_LEDS 295
//#define NUM_LEDS 61
#define LED_PER_WRAP 16

#define STARTBRIGHTNESS    96
#define MINBRIGHTNESS 3
#define MAXBRIGHTNESS 200
#define FRAMES_PER_SECOND  120

// Torch variables
const uint8_t MATRIX_WIDTH = LED_PER_WRAP;
const uint8_t MATRIX_HEIGHT = 18;
const int MATRIX_CENTER_X = MATRIX_WIDTH / 2;
const int MATRIX_CENTER_Y = MATRIX_HEIGHT / 2;
const byte MATRIX_CENTRE_X = MATRIX_CENTER_X - 1;
const byte MATRIX_CENTRE_Y = MATRIX_CENTER_Y - 1;

/*
#define MATRIX_TYPE    HORIZONTAL_ZIGZAG_MATRIX
cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds_text;
cLEDText ScrollingMsg;
const unsigned char TxtDemo[] = { EFFECT_FRAME_RATE "\x00"
                                  EFFECT_HSV_AH "\x00\xff\xff\xff\xff\xff"
                                  EFFECT_SCROLL_LEFT "   The "
                                  EFFECT_SCROLL_UP "Quick "
                                  EFFECT_SCROLL_LEFT "Brown "
                                  EFFECT_SCROLL_DOWN "Fox"
                                  EFFECT_SCROLL_LEFT "Jumps "
                                  EFFECT_SCROLL_UP "Over  "
                                  EFFECT_SCROLL_LEFT "The "
                                  EFFECT_SCROLL_DOWN "Lazy  "
                                  EFFECT_SCROLL_LEFT "Dog " };
*/

// Wave variables
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
CRGB w(85, 85, 85), W(CRGB::White);
CRGBPalette16 snowColors = CRGBPalette16( W, W, W, W, w, w, w, w, w, w, w, w, w, w, w, w );
CRGB l(0xE1A024);
CRGBPalette16 incandescentColors = CRGBPalette16( l, l, l, l, l, l, l, l, l, l, l, l, l, l, l, l );
const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  OceanColors_p,
  CloudColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p,
  LavaColors_p,
  snowColors,
};
const int paletteCount = ARRAY_SIZE(palettes);
int currentPaletteIndex = 0;
CRGBPalette16 palette = palettes[0];

// Define the array of leds
CRGB leds[NUM_LEDS];
int blackoutleds[NUM_LEDS];
int blackoutledSize = 0;

// Main variables
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t chase = 0;
int16_t counter = 0;
int16_t crawlCounter = 0;
int8_t  beaconCounter = 0;
float   timer;
byte    currentBrightness = STARTBRIGHTNESS; // 0-255 will be scaled to 0-MAXBRIGHTNESS
bool    autoCycle = false;
bool    eepromOutdated;
unsigned long eepromMillis; // store time of last setting change
// Time after changing settings before settings are saved to EEPROM
#define EEPROMDELAY 1500

fract8 chanceOfGlitter = 100;

uint16_t effectDelay = 10;
long effectMillis;
byte currentEffect = 0; // index to the currently running effect
byte numEffects;
bool effectInit = false;

int chaser = 0;
static uint16_t dist;         // A random number for our noise generator.
uint16_t scale = 60;          // Wouldn't recommend changing this on the fly, or the animation will be really blocky.
uint8_t maxChanges = 48;      // Value for blending between palettes.
CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette(LavaColors_p);
CRGBPalette16 HeatColorsNoWhite_p( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::Red);

float temp = 0;

#include "wave.h"
#include "effects.h"
#include "torch.h"
#include "twinkles.h"
#include "GradientPalettes.h"
#include "colorwave.h"
#include "Fire2012WithPalette.h"
#include "Noise.h"
#include "font.h"
#include "scrolltext.h"
#include "sound.h"

typedef uint16_t (*functionList)();
functionList effectList[] = {
  //scrollTemp,
  solid,
  solidChanging,
  soundReactive,
  fire2012WithPalette,
  Fire2012,
  fireNoise,
  torch,
  rain,
  fallingCode,
  rainbowNoise,
  partyNoise,
  cloudNoise,
  oceanNoise,
  forestNoise,
  //rainbowStripeNoise,
  pride,
  //blackAndWhiteNoise,
  //blackAndBlueNoise,
  //colorWaves,
  //softtwinkles,
  //fireflies,
  incandescentTwinkles,
  twinkles,
  rainbowTwinkles,
  confetti,
  sparks,
  applause,
  wave,
  rainbow,
  bpm,
  juggle,
  sinelon,
  cylon,
  cylon2
};

void setup() {
  // Enable logging
  Serial.begin(115200);

  // SAVE POWER!
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(1);                                // give RF section time to shutdown
  //system_update_cpu_freq(FREQUENCY);

  EEPROM.begin(512);

  // Delay boot to let voltages even out, etc...
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  numEffects = (sizeof(effectList) / sizeof(effectList[0]));

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  // check to see if EEPROM has been used yet
  // if so, load the stored settings
  byte eepromWasWritten = EEPROM.read(0);
  if (eepromWasWritten == 99) {
    currentEffect =     EEPROM.read(1);
    autoCycle =         EEPROM.read(2);
    currentBrightness = EEPROM.read(3);
    blackoutPos =       EEPROM.read(4);
    blackoutWidth =     EEPROM.read(5);
    gHue =              EEPROM.read(6);
    recalculateBlackoutArea();
  }


  // set master brightness control
  FastLED.setBrightness(STARTBRIGHTNESS);

  dist = random16(12345);          // A semi-random number for our noise generator

  Serial.println("Setup Complete.");

  //getTemperature();
}

void loop() {
  EVERY_N_MILLISECONDS(20) {
    // Only update the global hue if not in the manual color choosing solid effect
    if (currentEffect != 1) {
      gHue++;
    }
  }

  // EVERY_N_MINUTES(1) {
  //   getTemperature();
  // }

  // Effects!
  // currentMillis = millis(); // save the current timer value
  // if (currentMillis - effectMillis > effectDelay) {
  //   effectMillis = currentMillis;
  //
  //   effectDelay = effectList[currentEffect]();
  //
  //   // Add entropy to random number generator; we use a lot of it.
  //   random16_add_entropy(random(1));
  // }

  //EVERY_N_MILLISECONDS(effectDelay) {
    effectDelay = effectList[currentEffect]();
    // Add entropy to random number generator; we use a lot of it.
    random16_add_entropy(random(1));
  //}

  // Ensure blackout area remains in tact
  for (int i=0; i<blackoutledSize; i++) {
    leds[blackoutleds[i]] = 0;
  }

  // Read the buttons
  buttons();

  //FastLED.show(); // The delay call below automatically handles this
  if (effectDelay > 0) {
    FastLED.delay(effectDelay);
  } else {
    FastLED.show();
  }
  checkEEPROM();

  // Read the mic input
  // int val = analogRead(MIC_PIN);
  // if (val > 800) {
  //   if (currentTime - lastTap > 1000) {
  //     numTaps = 1;
  //     Serial.printf("%d, ", val);
  //   } else if (currentTime - lastTap > 100) {
  //     numTaps++;
  //   }
  //   lastTap = currentTime;
  //   Serial.printf("%d, ", val);
  //   //Serial.printf("%d  ", numTaps);
  // }

  // int mn = 1024;     // mn only decreases
  // int mx = 0;        // mx only increases
  // // Perform 10 reads. Update mn and mx for each one.
  // for (int i = 0; i < 100; ++i) {
  //   int val = analogRead(MIC_PIN);
  //   mn = min(mn, val);
  //   mx = max(mx, val);
  // }
  //
  // // Send min, max and delta over Serial
  // Serial.print("m=");
  // Serial.print(mn);
  // Serial.print(" M=");
  // Serial.print(mx);
  // Serial.print(" D=");
  // Serial.print(mx-mn);
  // Serial.println();

  // pat the watchdog timer
  wdt_reset();
}

// void getTemperature() {
//   Serial.println(temp);
//   if (dht12.get() == 0) {
//     temp = dht12.cTemp;
//     Serial.println(temp);
//   }
// }

void buttons() {
  currentTime = (long)millis();

  button1.Update();

  if (button1.clicks != 0) {
    lastClicks = button1.clicks;
  }

  if (button1.clicks == 1) {
    // Single Click...
    Serial.println("Effect +");
    currentEffect++;
    effectInit = false;
    if (currentEffect > (numEffects - 1)) currentEffect = 0;
    Serial.println(currentEffect);

    eepromMillis = millis();
    eepromOutdated = true;
  } else if (button1.clicks == 2)   {
    // Go back an effect
    Serial.println("Effect -");
    currentEffect--;
    effectInit = false;
    if (currentEffect < 0 || currentEffect > (numEffects - 1)) currentEffect = numEffects - 1;

    Serial.println(currentEffect);

    eepromMillis = millis();
    eepromOutdated = true;
  } else if (button1.clicks  == 3)   {
    // Reset back to first effect
    Serial.println("Effect Reset");
    currentEffect = 0;
    effectInit = false;

    eepromMillis = millis();
    eepromOutdated = true;
  }

  // fade if button is held down during single-click
  if (lastClicks == -1 && button1.depressed == true)   {
    if (oldFadeUp == fadeUp) fadeUp = !fadeUp; // Switch direction

    if (currentTime - adjustFaderTime > fadeDelay) {
      adjustFaderTime = currentTime;
      if (fadeUp && currentBrightness < 255) {
        currentBrightness++;
      } else if (!fadeUp && currentBrightness > 0) {
        currentBrightness--;
      }

      eepromMillis = millis();
      eepromOutdated = true;

      Serial.println("Brightness: " + (String)currentBrightness);
    }
  } else if (lastClicks == -2 && button1.depressed == true)   {
    // move blackout if button is held down during double-click
    if (oldBlackoutMoveUp == blackoutMoveUp) blackoutMoveUp = !blackoutMoveUp; // Switch direction

    if (currentTime - adjustFaderTime > blackoutDelay) {
      adjustFaderTime = currentTime;
      if (currentEffect != 1) {
        if (blackoutMoveUp) {
          blackoutPos++;
          if (blackoutPos > MATRIX_WIDTH) blackoutPos = 0;
        } else if (!blackoutMoveUp) {
          blackoutPos--;
          if (blackoutPos < 0 || blackoutPos > MATRIX_WIDTH) blackoutPos = MATRIX_WIDTH;
        }
      } else {
        if (blackoutMoveUp) {
          gHue += 2;
        } else {
          gHue -= 2;
        }
      }

      // Ensure blackout area remains in tact
      recalculateBlackoutArea();

      eepromMillis = millis();
      eepromOutdated = true;

      Serial.println("Blackout Pos: " + (String)blackoutPos);
    }
  } else if (lastClicks == -3 && button1.depressed == true)   {
    // Adjust blackout size if button is held down during triple-click
    if (oldBlackoutSizeUp == blackoutSizeUp) blackoutSizeUp = !blackoutSizeUp; // Switch direction

    if (currentTime - adjustFaderTime > blackoutDelay) {
      adjustFaderTime = currentTime;
      if (blackoutSizeUp && blackoutWidth < MATRIX_WIDTH) {
        blackoutWidth++;
      } else if (!blackoutSizeUp && blackoutWidth > 0) {
        blackoutWidth--;
      }

      // Ensure blackout area remains in tact
      recalculateBlackoutArea();

      eepromMillis = millis();
      eepromOutdated = true;

      Serial.println("Blackout Width: " + (String)blackoutWidth);
    }
  } else {
    // Save old fade direction for next time
    oldFadeUp = fadeUp;
    oldBlackoutMoveUp = blackoutMoveUp;
    oldBlackoutSizeUp = blackoutSizeUp;
    // Reset lastClicks
    lastClicks = 0;
  }

  // The eye does not respond in a linear way to light.
  // High speed PWM'd LEDs at 50% duty cycle appear far brighter then the 'half as bright' you might expect.
  // So use the dim8 function to smooth out the range to what the eye perceives.
  FastLED.setBrightness(map8(dim8_raw(currentBrightness), MINBRIGHTNESS, MAXBRIGHTNESS));
}

void recalculateBlackoutArea() {
  // Ensure blackout area remains in tact
  memset(blackoutleds, 0, ARRAY_SIZE(blackoutleds)); // Clear array first
  blackoutledSize = 0;
  if (blackoutWidth == 0) return; // No blackout, so we can bypass recalc.
  // Recalculate pixels to blackout
  for (int i=0; i<NUM_LEDS; i++) {
    // Determine if the pos is within a blackout area
    if (checkBlackout(i%MATRIX_WIDTH)) {
      blackoutleds[blackoutledSize++] = i;
    }
  }
}

// write EEPROM value if it's different from stored value
void updateEEPROM(byte location, byte value) {
  if (!autoCycle) {
    if (EEPROM.read(location) != value) {
      EEPROM.write(location, value);
    }
  }
}

// Write settings to EEPROM if necessary
void checkEEPROM() {
  if (eepromOutdated) {
    if (millis() - eepromMillis > EEPROMDELAY) {
      updateEEPROM(0, 99);
      updateEEPROM(1, currentEffect);
      updateEEPROM(2, autoCycle);
      updateEEPROM(3, currentBrightness);
      updateEEPROM(4, blackoutPos);
      updateEEPROM(5, blackoutWidth);
      updateEEPROM(6, gHue);
      EEPROM.commit();
      eepromOutdated = false;
    }
  }
}
