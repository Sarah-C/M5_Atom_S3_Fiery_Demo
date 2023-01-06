
#include <M5AtomS3.h>

#define MAXPAL 4

uint16_t matrix[16384 + 128];
uint16_t backBuffer565[16384];
uint16_t color[200 * (MAXPAL + 1)]; // 4 palettes and current pallet space.
uint8_t pallet = 1;
uint8_t maxPal = 0;
uint32_t XORRand = 0;

// A standard XOR Shift PRNG but with a floating point twist.
// https://www.doornik.com/research/randomdouble.pdf
float random2(){
  XORRand ^= XORRand << 13;
  XORRand ^= XORRand >> 17;
  XORRand ^= XORRand << 5;
  return (float)((float)XORRand * 2.32830643653869628906e-010f);
}

void makePallets(){
  // 0b00011111 00000000 : blue
  // 0b00000000 11111000 : red
  // 0blll00000 00000hhh : green
  // Flame effect pallet
  for (int i = 0; i < 64; i++){
    uint8_t r = i * 4;
    uint8_t g = 0;
    uint8_t b = 0;
    color[200 + i] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
    r = 255;
    g = i * 4;
    b = 0;
    color[200 + i + 64] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
    r = 255;
    g = 255;
    b = i * 2;
    color[200 + i + 128] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
  }
  uint8_t r = 255;
  uint8_t g = 255;
  uint8_t b = 64 * 2;
  for (int i = 192; i < 200; i++){
    color[200 + i] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
  }   
  // Cold flame effect pallet
  for (int i = 0; i < 200; i++){
    uint8_t r = (i > 100) ? (float)(i-100) * 1.775f: i / 3.0f;
    uint8_t g = (i > 100) ? (float)(i-100) * 1.775f: i / 3.0f;
    uint8_t b = (float)i * 1.275f;
    color[400 + i] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
  }
  // Black and white pallet
  for (int i = 0; i < 200; i++){
    uint8_t r = (float)i * 1.275f;
    uint8_t g = (float)i * 1.275f;
    uint8_t b = (float)i * 1.275f;
    color[600 + i] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
  }
  // Green flame effect pallet
  for (int i = 0; i < 200; i++){
    uint8_t r = (i > 100) ? (float)(i-100) * 1.175f: i / 5.0f;
    uint8_t g = (float)i * 1.275f;
    uint8_t b = (i > 100) ? (float)(i-100) * 1.775f: i / 3.0f;
    color[800 + i] = ((b & 0b11111000) << 8) | ((g & 0b11111100) << 3) | (r >> 3);
  }
}

void usePalette(uint8_t pal){
  uint16_t palOffset = pal * 200;
  for(uint16_t i = 0; i < 200; i++){
    color[i] = color[palOffset + i];
  }
}

void prevPal(){
  if(--pallet == 0) pallet = MAXPAL;
  usePalette(pallet);
}

void nextPal(){
  if(++pallet == MAXPAL + 1) pallet = 1;
  usePalette(pallet);
}

void setup(){
  M5.begin(true, true, false, false);// bool LCDEnable, bool USBSerialEnable, bool I2CEnable, bool LEDEnable
  USBSerial.begin(115200);
  USBSerial.println("M5AtomS3 booting...");
  XORRand = esp_random();
  makePallets();
  usePalette(1);
  // Draw the palette for a bit.
  //for (int i = 0; i < 16384; i++){
  //  backBuffer565[i] = color[(i << 1) % 200];
  //}
  //lcd_PushColors(0, 0, 128, 128, (uint16_t *)backBuffer565, 128 * 128);
  //delay(2000);
}

void loop(){
  M5.update();
  if (M5.Btn.wasReleased()) {
    nextPal();
  }
  // Heat up the bottom of the fire.
  for (uint16_t i = 16384; i < 16384 + 127; i++) {
    matrix[i] = 300.0f * random2();
  }
  // Nasty floating point maths to produce the billowing and nice blending.
  // Floats are accelerated on the ESP32 S3.
  for (uint16_t i = 0; i < 16384; i++) {
    uint16_t pixel = (float)i + 128.0f - random2() + 0.8f;
    float sum = matrix[pixel] + matrix[pixel + 1] + matrix[pixel - 128] + matrix[pixel - 128 + 1];
    uint16_t value = sum * 0.49f * random2() + 0.5f;
    matrix[i] = value;
    if(value > 199) value = 199;
    backBuffer565[i] = color[value];
  }
  M5.Lcd.drawBitmap(0, 0, 128, 128, backBuffer565);
}