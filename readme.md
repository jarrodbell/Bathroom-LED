# Bathroom LED Controller
2 x LED strips controlled by a Wemos D1 Mini Pro board, with PIR motion sensor to trigger the LED to automatically turn on.

Also has a button to change the LED strip animation sequence. Animations:

1. Soft glow (default used by motion sensor)
2. Subtle fade between red and orange
3. Rainbow sequence
4. Fire - simulates a flickering fire

Double click the button to turn off the LEDs.

Motion triggered timer will leave lights on for 15 minutes (plenty of time for a shower if the motion detector fails to detect within shower area).

After button press the timer will stay on for 30 minutes to allow for longer baths without motion trigger.

A simple wave of your hand will be enough usually to trigger the motion sensor if you have been standing/lying still for too long for it to automatically detect you, but this can be adjusted by a trimpot on the PIR sensor board to change the sensitivity.

# Hardware Used

1. Wemos D1 Mini Pro
2. 5V DC 3A PSU (Mean Well)
3. HC-SR501 PIR Sensor module
4. 5mm LEDs (blue & red)
5. Momentary contact button
6. WS2812b 60/m LED strips
7. 330ohm resistor on each LED strip data line (as close to LED strip as possible)
8. 1000uF capacitor on PSU end of LED strip