/*
 * SOUND
 * Based on:
 * https://learn.adafruit.com/trinket-sound-reactive-led-color-organ/code
 *
 */
 #define MIC_PIN    A0  // Microphone is attached to Trinket GPIO #2/Gemma D2 (A1)
 #define DC_OFFSET  0  // DC offset in mic signal - if unusure, leave 0
 #define NOISE     10  // Noise/hum/interference in mic signal
 #define SAMPLES   60  // Length of buffer for dynamic level adjustment
 #define MAX      600 // 1024 is full range, use lower number to increase sensitivity

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

  //Serial.println(peakToPeak);

  height = ((float)peakToPeak / MAX) * (float)NUM_LEDS;
  if (height > NUM_LEDS) height = NUM_LEDS - 1;

  //Serial.println(height);

  fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
  for (i=0; i<height; i++) {
    leds[i] = CHSV(gHue, 255, 255);
  }

  return 0;
}

uint16_t soundReactiveRainbow() {

  static uint8_t hue = 1;
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

  //Serial.println(peakToPeak);

  height = ((float)peakToPeak / MAX) * (float)(NUM_LEDS+1);
  if (height > NUM_LEDS) height = NUM_LEDS - 1;
  //Serial.println(height);

  //fill_solid(leds, NUM_LEDS, CRGB(0,0,0));
  fadeToBlackBy(leds, NUM_LEDS, 20);
  for (i=0; i<height; i++) {
    leds[i] = ColorFromPalette(palette, i + hue);
  }

  EVERY_N_MILLISECONDS(10) {
    hue++;
  }

  return 0;
}

void displayBand(int band, int dsize) {
  int dmax = 50;
  if (dsize > dmax) dsize = dmax;
  //Serial.println("f: " + (String) band + ", l: " + (String) dsize);
  // for (int s = 0; s <= dsize; s=s+2) {
  //
  // }
  //Serial.println("-");
  if (dsize > peak[band]) {
    peak[band] = dsize;
  }
}

uint16_t soundFreq() {
  if (effectInit) {
    //Serial.println("test1");
  }
  for (int i = 0; i < SAMPLES; i++) {
    newTime = micros()-oldTime;
    oldTime = newTime;
    vReal[i] = analogRead(MIC_PIN); // A conversion takes about 1mS on an ESP8266
    vImag[i] = 0;
    while (micros() < (newTime + sampling_period_us)) { /* do nothing to wait */ }
  }

  //Serial.println("test2");

  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  //Serial.println("test3");
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  //Serial.println("test4");
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  //Serial.println("test5");
  for (int i = 2; i < (SAMPLES/2); i++) { // Don't use sample 0 and only first SAMPLES/2 are usable. Each array eleement represents a frequency and its value the amplitude.
    if (vReal[i] > 200) { // Add a crude noise filter, 4 x amplitude or more
      if (i<=5 )             displayBand(0,(int)vReal[i]/amplitude); // 125Hz
      if (i >5   && i<=12 )  displayBand(1,(int)vReal[i]/amplitude); // 250Hz
      if (i >12  && i<=32 )  displayBand(2,(int)vReal[i]/amplitude); // 500Hz
      if (i >32  && i<=62 )  displayBand(3,(int)vReal[i]/amplitude); // 1000Hz
      if (i >62  && i<=105 ) displayBand(4,(int)vReal[i]/amplitude); // 2000Hz
      if (i >105 && i<=120 ) displayBand(5,(int)vReal[i]/amplitude); // 4000Hz
      if (i >120 && i<=146 ) displayBand(6,(int)vReal[i]/amplitude); // 8000Hz
      //Serial.println(i);
    }
    for (byte band = 0; band <= 6; band++) {
      //display.drawHorizontalLine(18*band,64-peak[band],14);
    }
  }
  //if (millis()%4 == 0) {for (byte band = 0; band <= 6; band++) {if (peak[band] > 0) peak[band] -= 1;}} // Decay the peak

  return 0;
}
