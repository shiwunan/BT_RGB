// Bluetooth Goggles Sketch -- shows the Adafruit Bluefruit LE UART Friend
// can be used even with Trinket or Gemma!

// https://www.adafruit.com/products/2479

// Works in conjunction with Bluefruit LE Connect app on iOS or Android --
// pick colors or use '1' and '2' buttons to select pinwheel or sparkle modes.
// You can try adding more, but space is VERY tight...helps to use Arduino IDE
// 1.6.4 or later; produces slightly smaller code than the 1.0.X releases.

// BLUEFRUIT LE UART FRIEND MUST BE SWITCHED TO 'UART' MODE

#include <SoftwareSerial.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR_ATtiny85__ // Trinket, Gemma, etc.
#include <avr/power.h>
#endif

#define RX_PIN    10 // Connect this Trinket pin to BLE 'TXO' pin
#define CTS_PIN   11 // Connect this Trinket pin to BLE 'CTS' pin
#define LED_PIN   6 // Connect NeoPixels to this Trinket pin
#define NUM_LEDS 240// Two 16-LED NeoPixel rings
#define FPS      30 // Animation frames/second (ish)

SoftwareSerial    ser(RX_PIN, -1);
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRBW + NEO_KHZ800);

byte neopix_gamma[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,
  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,
  2,  3,  3,  3,  3,  3,  3,  3,  4,  4,  4,  4,  4,  5,  5,  5,
  5,  6,  6,  6,  6,  7,  7,  7,  7,  8,  8,  8,  9,  9,  9, 10,
  10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
  17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
  25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
  37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
  51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
  69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
  90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
  115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
  144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
  177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
  215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  // MUST do this on 16 MHz Trinket for serial & NeoPixels!
  clock_prescale_set(clock_div_1);
#endif
  // Stop incoming data & init software serial
  pinMode(CTS_PIN, OUTPUT); digitalWrite(CTS_PIN, HIGH);
  ser.begin(9600);
  pixels.begin(); // NeoPixel init
  pixels.setBrightness(255);
  // Flash space is tight on Trinket/Gemma, so setBrightness() is avoided --
  // it adds ~200 bytes.  Instead the color picker input is 'manually' scaled.
}

uint8_t  buf[3],              // Enough for RGB parse; expand if using sensors
         animMode = 0,        // Current animation mode
         animPos  = 0;        // Current animation position
uint32_t color    = 0xB312FF, // Current animation color (red by default)
         prevTime = 0L;       // For animation timing

void loop(void) {
  int      c;
  uint32_t t;

  // Animation happens at about 30 frames/sec.  Rendering frames takes less
  // than that, so the idle time is used to monitor incoming serial data.
  digitalWrite(CTS_PIN, LOW); // Signal to BLE, OK to send data!
  for (;;) {
    t = micros();                            // Current time
    if ((t - prevTime) >= (1000000L / FPS)) { // 1/30 sec elapsed?
      prevTime = t;
      break;                                 // Yes, go update LEDs
    }                                        // otherwise...
    if ((c = ser.read()) == '!') {           // Received UART app input?
      while ((c = ser.read()) < 0);          // Yes, wait for command byte
      switch (c) {
        case 'B':       // Button (Control Pad)
          if (readAndCheckCRC(255 - '!' - 'B', buf, 2) & (buf[1] == '1')) {
            buttonPress(buf[0]); // Handle button-press message
          }
          break;
        case 'C':       // Color Picker
          if (readAndCheckCRC(255 - '!' - 'C', buf, 3)) {
            // As mentioned earlier, setBrightness() was avoided to save space.
            // Instead, results from the color picker (in buf[]) are divided
            // by 4; essentially equivalent to setBrightness(64).  This is to
            // improve battery run time (NeoPixels are still plenty bright).
            color = pixels.Color(buf[0], buf[1], buf[2]);
          }
          break;
      }
    }
  }
  digitalWrite(CTS_PIN, HIGH); // BLE STOP!

  // Show pixels calculated on *prior* pass; this ensures more uniform timing
  pixels.show();

  // Then calculate pixels for *next* frame...
  switch (animMode) {
    case 0: // static colour mode
      pixels.clear();
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint32_t c = 0;
        c = color;
        pixels.setPixelColor(i, c);         // First eye
      }
      break;
    case 1: // rainbow mode
      uint16_t i, j;

      for (j = 0; j < 256; j++) {
        for (i = 0; i < pixels.numPixels(); i++) {
          pixels.setPixelColor(i, Wheel((i + j) & 255));
        }
        pixels.show();
        delay(10);
      }
      break;
    case 2: // set back to purple
      pixels.clear();
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint32_t c = 0;
        c = 0xB312FF;
        pixels.setPixelColor(i, c);         // First eye
      }
      break;
    case 3: // set off
      pixels.clear();
      for (uint8_t i = 0; i < NUM_LEDS; i++) {
        uint32_t c = 0;
        c = 0x000000;
        pixels.setPixelColor(i, c);         // First eye
      }
      break;
  }
}

boolean readAndCheckCRC(uint8_t sum, uint8_t *buf, uint8_t n) {
  for (int c;;) {
    while ((c = ser.read()) < 0); // Wait for next byte
    if (!n--) return (c == sum); // If CRC byte, we're done
    *buf++ = c;                  // Else store in buffer
    sum   -= c;                  // and accumulate sum
  }
}

void skipBytes(uint8_t n) {
  while (n--) {
    while (ser.read() < 0);
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 2, 0, WheelPos * 2, 0);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 2, 255 - WheelPos * 2, 0);
  }
  WheelPos -= 170;
  return pixels.Color(WheelPos * 2, 255 - WheelPos * 2, 0, 0);
}

void buttonPress(char c) {
  pixels.clear(); // Clear pixel data when switching modes (else residue)
  switch (c) {
    case '1':
      animMode = 0; // Switch to pinwheel mode
      break;
    case '2':
      animMode = 1; // Switch to sparkle mode
      break;
    case '3':
      animMode = 2;
      break;
    case '4':
      animMode = 3;
      break;
    case '5': // Up
      break;
    case '6': // Down
      break;
    case '7': // Left
      break;
    case '8': // Right
      break;
  }
}
