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
#include <ClickButton.h>

// Declare local functions
void processButton();
void processPIR();
int mapRange(int, int);

#define LED_PIN D5  // Data pin where the LED strip is connected to.
#define LED_PIN_2 D6  // Data pin where the LED strip is connected to.
#define BUTTON_PIN D3
#define SENSOR_PIN D1
//#define LED_BUILTIN D4 // No need to define here, defined by default. Used to show power state
#define LED_SENSOR D2 // Used to show sensor state

// How many leds in your strip?
#define NUM_LEDS 44
#define NUM_LEDS_2 42

// Define the array of leds
CRGB leds[NUM_LEDS];
CRGB leds2[NUM_LEDS_2];

ClickButton button1(BUTTON_PIN, LOW, CLICKBTN_PULLUP);

#define BRIGHTNESS  255
#define BRIGHTNESS2  128
#define MINBRIGHTNESS 3
#define MAXBRIGHTNESS 200

#define TIMEOUT_MANUAL 30 * 60000 // Timeout for manual trigger via button
#define TIMEOUT_AUTO 15 * 60000 // Timeout for auto trigger via PIR
#define TIMEOUT_MANUAL_OFF 30 * 60000 // Timeout for manual off mode triggered via button

//#define TIMEOUT_MANUAL 5000 // Timeout for manual trigger via button
//#define TIMEOUT_AUTO 10000  // Timeout for auto trigger via PIR

#define OFF_MODE 5 // Which effect to call on when OFF is triggered (position in array, 0-based)

// FastLED provides these pre-conigured incandescent color profiles:
//     Candle, Tungsten40W, Tungsten100W, Halogen, CarbonArc,
//     HighNoonSun, DirectSunlight, OvercastSky, ClearBlueSky,
// FastLED provides these pre-configured gaseous-light color profiles:
//     WarmFluorescent, StandardFluorescent, CoolWhiteFluorescent,
//     FullSpectrumFluorescent, GrowLightFluorescent, BlackLightFluorescent,
//     MercuryVapor, SodiumVapor, MetalHalide, HighPressureSodium,
// FastLED also provides an "Uncorrected temperature" profile
//    UncorrectedTemperature;

//#define TEMPERATURE_1 Candle
//#define TEMPERATURE_2 Candle

uint8_t gHue = 0; // rotating "base color" used by many of the patterns
byte    currentBrightness = BRIGHTNESS; // 0-255 will be scaled to 0-MAXBRIGHTNESS
uint16_t effectDelay = 10;
long effectMillis;
byte currentEffect = 0; // index to the currently running effect
byte numEffects;
int8_t lastEffectBeforeOff = -1;
bool effectInit = false;

unsigned long sensorLastTriggerTime = 0;  // the last time the sensor was triggered

int buttonState; // Store the state of the button to compare if it has changed
int buttonLastState = LOW; // Last known state of the button for comparison
unsigned long buttonLastPressTime = 0;  // the last time the blower button change registered
unsigned long buttonLastDebounceTime = 0;  // Time the initial input change was triggered
bool buttonEnable = false;

unsigned long buttonDelay = 250;
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

CRGB color;
CRGBPalette16 gPal;

int AVERAGE_DELAY = 100;

int heat[NUM_LEDS];			// value of led
int flame[NUM_LEDS];		// modifier: positive for rise, negative for fall
// Second strip
int heat2[NUM_LEDS_2];			// value of led
int flame2[NUM_LEDS_2];		// modifier: positive for rise, negative for fall
int i;
int FLAME = 40;			// 10 - 30 is ideal, min 0 = no flame, 50 = white flame

bool isManualMode = false;
bool isAutoMode = false;
bool manuallyTurnedOff = false;

// FIRE 2012
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100 
#define COOLING  120
// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 100
bool gReverseDirection = false;

uint16_t glow() {
  FastLED.setTemperature( Candle );
  fill_solid(leds, NUM_LEDS, Candle);
  fill_solid(leds2, NUM_LEDS_2, Candle);
  return 10;
}

uint16_t solidColors() {
  FastLED.setTemperature( UncorrectedTemperature );
  fill_solid(leds, NUM_LEDS, CHSV(gHue, 255, 255));
  fill_solid(leds2, NUM_LEDS_2, CHSV(gHue, 255, 255));
  return 10;
}

uint16_t FillLEDsFromPaletteColors()
{
    FastLED.setTemperature( Candle );

    static uint8_t startIndex;
    static CRGBPalette16 currentPalette;
    static TBlendType    currentBlending;
    CRGBPalette16 HeatColorsNoWhite_p( CRGB::Black, CRGB::Red, CRGB::Yellow, CRGB::Red);
    CRGBPalette16 HeatColorsNoWhiteNoBlack_p( CRGB::Orange, CRGB::Red, CRGB::Gold, CRGB::Red);

    //currentPalette = LavaColors_p;
    currentPalette = HeatColorsNoWhiteNoBlack_p;
    currentBlending = LINEARBLEND;

    EVERY_N_MILLISECONDS(50) {
      startIndex++;
    }

    uint8_t brightness = 255;
    uint8_t colorIndex = startIndex;

    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }

    for( int i = 0; i < NUM_LEDS_2; i++) {
        leds2[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }

    return 10;
}

uint16_t fillFire() {			// flame
	static int boom;
  static int boom2;

	// spark random led
	i = random(0,NUM_LEDS);
  //	heat[i] = qsub8( heat[i], random8(1, 10) );
	flame[i] = random(-FLAME,FLAME);
	if ( flame[i] == 0 ) {
		flame[i] = random(1,FLAME);
	}

	// average values should produce slim results
	for ( i=0 ; i<NUM_LEDS ; i++ ) {
		// averaging heat makes a smoother colors
		heat[i] = (heat[mapRange(i-1,NUM_LEDS)] + heat[i] + heat[mapRange(i+1,NUM_LEDS)] ) / 3; // average heat
	}

	// run:
	for ( i=0 ; i<NUM_LEDS ; i++ ) {
		// change color
		heat[i] = min(255, max(0, heat[i] + flame[i]));

		leds[i] = ColorFromPalette( gPal, scale8( heat[i], 240), scale8( heat[i], 250), LINEARBLEND );
		if (heat[i]>250) {
			boom = i;
		}

		// boom is peak
		flame[i] = (heat[i] + flame[i] > 255 ? random(-FLAME,-1) : heat[i] < 0 ? random(1,FLAME) : flame[i]);	// return flame into range 10-250

		if (heat[i]<250 && boom==i) {
			boom = -1;
		}
	}

  // spark random led
	i = random(0,NUM_LEDS_2);
  //	heat[i] = qsub8( heat[i], random8(1, 10) );
	flame2[i] = random(-FLAME,FLAME);
	if ( flame2[i] == 0 ) {
		flame2[i] = random(1,FLAME);
	}

  // average values should produce slim results
	for ( i=0 ; i<NUM_LEDS_2 ; i++ ) {
		// averaging heat makes a bit smooth colors
		heat2[i] = (heat2[mapRange(i-1,NUM_LEDS_2)] + heat2[i] + heat2[mapRange(i+1,NUM_LEDS_2)] ) / 3; // average heat
	}

  for ( i=0 ; i<NUM_LEDS_2 ; i++ ) {
		// change color
		heat2[i] = min(255, max(0, heat2[i] + flame2[i]));

    leds2[i] = ColorFromPalette( gPal, scale8( heat2[i], 240), scale8( heat2[i], 250), LINEARBLEND );
		if (heat2[i]>250) {
			boom2 = i;
		}

		// boom is peak
		flame2[i] = (heat2[i] + flame2[i] > 255 ? random(-FLAME,-1) : heat2[i] < 0 ? random(1,FLAME) : flame2[i]);	// return flame into range 10-250

		if (heat2[i]<250 && boom2==i) {
			boom2 = -1;
		}
	}
  return 10;
}	// flame

uint16_t effectOff() {
  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
  fill_solid(leds2, NUM_LEDS, CRGB(0, 0, 0));
  return 10;
}

uint16_t Fire2012() {
// Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS/2];

  // Step 1.  Cool down every cell a little
  for( int i = 0; i < NUM_LEDS/2; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS/2) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for( int k= NUM_LEDS/2 - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }
  
  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160,255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for( int j = 0; j < NUM_LEDS/2; j++) {
    CRGB color = HeatColor( constrain(heat[j], 0, 120)); // Constrain to prevent bright whites
    int pixelnumber;
    if( gReverseDirection ) {
      pixelnumber = (NUM_LEDS/2-1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }

    // copy in reverse order first half of strip to second half
  for (uint8_t i = 0; i < NUM_LEDS/2; i++) {
    leds[NUM_LEDS-1-i] = leds[i];  
  }

  return 30;
}

typedef uint16_t (*functionList)();
functionList effectList[] = {
  glow,
  Fire2012,
  FillLEDsFromPaletteColors,
  solidColors,
  fillFire,
  effectOff
};
int solidEffectColor = 3;

void setup() {

  // Enable logging
  Serial.begin(115200);

  // SAVE POWER!
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(1);                                // give RF section time to shutdown
  //system_update_cpu_freq(FREQUENCY);

  // Delay boot to let voltages even out, etc...
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\r\n", t);
    Serial.flush();
    delay(1000);
  }

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_SENSOR, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SENSOR_PIN, INPUT);

  digitalWrite(LED_BUILTIN, HIGH); // LED always on to show power state

  numEffects = (sizeof(effectList) / sizeof(effectList[0]));

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);//.setCorrection( TypicalSMD5050 );
  FastLED.addLeds<WS2812B, LED_PIN_2, GRB>(leds2, NUM_LEDS_2);//.setCorrection( TypicalSMD5050 );

  FastLED.setMaxPowerInVoltsAndMilliamps(5,3000);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);

  // Set start fire
  gPal = CRGBPalette16( CRGB::Black, CRGB(255,0,0), CRGB::Orange, CRGB::Yellow);
  AVERAGE_DELAY = 80;
  //currentBrightness = 180;
  FLAME = 20;

	for ( i = 0 ; i < NUM_LEDS ; i ++ ) {
		heat[i] = random(1,180);
		flame[i] = random(-FLAME,FLAME);
		if ( flame[i] == 0 ) {
			flame[i] = random(1,FLAME);
		}
	}

  Serial.println("Setup Complete.");
}

void loop() {

  // FastLED.setTemperature( TEMPERATURE_1 );
  // fill_solid(leds, NUM_LEDS, TEMPERATURE_1);
  // FastLED.show();
  // FastLED.delay(8);

  EVERY_N_MILLISECONDS(50) {
    // Only update the global hue if not in the manual color choosing solid effect
    if (currentEffect == solidEffectColor) {
      gHue++;
    }
  }

  if (currentEffect > numEffects) {
    currentEffect = 0;
  }
  effectDelay = effectList[currentEffect]();

  // Add entropy to random number generator; we use a lot of it.
  random16_add_entropy(random(1));

  // The eye does not respond in a linear way to light.
  // High speed PWM'd LEDs at 50% duty cycle appear far brighter then the 'half as bright' you might expect.
  // So use the dim8 function to smooth out the range to what the eye perceives.
  //FastLED.setBrightness(map8(dim8_raw(currentBrightness), MINBRIGHTNESS, MAXBRIGHTNESS));

  processButton();
  processPIR();

  EVERY_N_SECONDS(1) {
    // Check if any timeouts have been reached
    unsigned long currentTime = millis(); // Store current time for comparisons
    if (isManualMode && (currentTime - buttonLastPressTime) > TIMEOUT_MANUAL) {
      currentEffect = OFF_MODE;
      isManualMode = false;
      Serial.println("DISABLE MANUAL MODE");
    } else if (isManualMode && manuallyTurnedOff && (currentTime - buttonLastPressTime) > TIMEOUT_MANUAL_OFF) {
      // Was manually turned off, so use a different timeout to re-enable the PIR sensor sooner
      isManualMode = false;
      Serial.println("DISABLE MANUAL MODE");
    }

    if (isAutoMode && ((currentTime - sensorLastTriggerTime) > TIMEOUT_AUTO)) {
      currentEffect = OFF_MODE;
      isManualMode = false;
      isAutoMode = false;
      Serial.println("DISABLE AUTO MODE");
    }
  }

  //FastLED.show(); // The delay call below automatically handles this
  if (effectDelay > 0) {
    FastLED.delay(effectDelay);
  } else {
    FastLED.show();
  }

  // pat the watchdog timer
  wdt_reset();
}

// PROCESS PIR SENSOR
void processPIR() {
  int state = digitalRead(SENSOR_PIN); // Read the input pin

  unsigned long currentTime = millis(); // Store current time for comparisons

  if (state) {
    sensorLastTriggerTime = currentTime;
    if (!isManualMode) {
      isAutoMode = true;
      currentEffect = 0; // Turn on the first effect via sensor
    }
  }

  //Serial.printf("SENSOR: %d...\r\n", state);
  digitalWrite(LED_SENSOR, !state); // set onboard LED state to match PIR detection state
}

// PROCESS BUTTON PRESSES
void processButton() {

  button1.Update();

  unsigned long currentTime = millis(); // Store current time for comparisons

  if (button1.clicks == 1) {
    // Single Click...
    // Store the last time the button was pressed and processed
    buttonLastPressTime = currentTime;

    isManualMode = true;
    isAutoMode = false;

    Serial.println("Effect +");
    currentEffect++;
    effectInit = false;
    if (currentEffect > (numEffects - 1)) currentEffect = 0;
    Serial.printf("EFFECT: %d...\r\n", currentEffect);

    // If manually turned off, then we use a different timeout period
    manuallyTurnedOff = false;
  } else if (button1.clicks > 1)   {
    // Double or more clicks - turn off!
    // Store the last time the button was pressed and processed
    buttonLastPressTime = currentTime;

    isManualMode = true;
    isAutoMode = false;

    Serial.println("OFF");
    currentEffect = OFF_MODE;

    // If manually turned off, then we use a different timeout period
    manuallyTurnedOff = true;
  }
}

int mapRange(int num, int max) {				// scale number to fit in a range
	while (num >= max) {
		num -= max;
	}
	while (num < 0) {
		num += max;
	}
	return num;
}			// scale number to fit in a range