/*
 * SOUND
 * Based on:
 * https://learn.adafruit.com/trinket-sound-reactive-led-color-organ/code
 *
 */
 #define MIC_PIN    A0  // Microphone is attached to Trinket GPIO #2/Gemma D2 (A1)
 #define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
 #define NOISE     100  // Noise/hum/interference in mic signal
 #define SAMPLES   60  // Length of buffer for dynamic level adjustment

 byte
   peak      = 0,      // Used for falling dot
   dotCount  = 0,      // Frame counter for delaying dot-falling speed
   volCount  = 0;      // Frame counter for storing past volume data

 int
   vol[SAMPLES],       // Collection of prior volume samples
   lvl       = 10,     // Current "dampened" audio level
   minLvlAvg = 0,      // For dynamic adjustment of graph low & high
   maxLvlAvg = 512;

uint16_t soundReactive() {

  uint16_t  i;
  signed int   sample;
  unsigned int height;

  signed int peakToPeak = 0;   // peak-to-peak level
  signed int signalMax = 0;
  signed int signalMin = 1024;

  for (i=0; i<50; i++) {
    sample = analogRead(MIC_PIN);
    if (sample < 0)  {
      sample = 1023;
    }
    if (sample < 1024)  {// toss out spurious readings
       if (sample > signalMax) {
          signalMax = sample;  // save just the max levels
       } else if (sample < signalMin) {
          signalMin = sample;  // save just the min levels
       }
    }
  }
  peakToPeak = signalMax - signalMin;  // max - min = peak-peak amplitude
  //double volts = (peakToPeak * 3.3) / 1024;  // convert to volts
  //Serial.println(volts);

  if (peakToPeak < 0 || peakToPeak > 1024) {
    peakToPeak = 0;
  }

  Serial.println(peakToPeak);

  height = ((float)peakToPeak / 1024) * (float)NUM_LEDS;
  Serial.println(height);

  fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
  for (i=0; i<height; i++) {
    leds[i] = CHSV(gHue, 255, 255);
  }

  return 0;
}
