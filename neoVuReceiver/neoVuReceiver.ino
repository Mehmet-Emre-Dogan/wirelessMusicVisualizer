/******** References *********/
/* 
 *  ============= Disabling wifi =============
 *  https://www.esp8266.com/viewtopic.php?p=72207
 *  
 *  ============= Animated Flag ============= 
 * https://www.youtube.com/watch?v=2owTxbrmY-s&list=PLgXkGn3BBAGi5dTOCuEwrLuFtfz0kGFTC&index=4
 * https://github.com/s-marley/FastLED-basics/blob/main/4.%20Waves%20and%20blur/phaseBeat/phaseBeat.ino
 * 
 * ============= Adalight =============
 * https://github.com/Wifsimster/adalight_ws2812/blob/master/Adalight_WS2812.ino
 * 
 */
#define FASTLED_ESP8266_RAW_PIN_ORDER
/* Libraries */
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUDP.h>

/* Misc */
#define DEBUG_ENABLED false // Disable debug to be able to use the Adalight mode
#include "secrets.h"

/******** Constants *********/
#define EMERGENCY_TORCH_NUM 4
#define BUTTON_CONSIDERATION_LIMIT 50 // if the consequent button presses are less than it; ignore last press
#define BUTTON_DOUBLE_PRESS_TIMEOUT 120
#define BUTTON_MAX_PRESS_COU 2
#define CONNECTION_LOST_TIMEOUT 10000
#define ARRAY_SHIFT_INTERVAL 9
#define COLOR_SHIFT_INTERVAL 1

/* Display */
#define INDICATOR_OFFSET 5
#define DECAY_INTERVAL 9
#define INDICATOR_ITER_LIM 20
#define CRITICAL_LEVEL 5

/* Leds */
#define DATA_PIN D2
#define NUM_LEDS 58
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

/* Drop width modulation mode */
#define DIVIDER 6
#define MAX_WIDTH 10 //NUM_LEDS/DIVIDER
#define EMPTY_WIDTH 30
//#define DWM_MEAS_LIM 5 // Drop width modulation measurement count
#define DROP_INTERVAL 4

/* Serial communication */
#define ADA_RATE 1000000 // Baud rate for Adalight mode 
#define DEBUG_RATE 9600 // Baud rate to print debug info to computer

/* Physical ui elemets */
#define BTN D5
#define SWITCH D6
#define POT A0

/******** Function templates ********/
void ICACHE_RAM_ATTR handleBtn();


/* Global variables */
byte physicalMode = 0; // Using the switch, button, and tyhe potentiometer on the module, decide which mode to execute
unsigned long lastPressTime = 0;
byte pressCou = 0;

const char delim[2] = ":";

int measurementRes = 0; // stores the raw measurement value
byte brightness = 127;
byte currMode = 0; // current mode
byte prevMode = 0; // previous mode
byte measurementCoeff = 4; // Scales the measurement // i.e. sensivity
bool musicIndicatorEnabled = true; 
bool indicateSeconds = true; // Whether the seconds being indicated

//byte threshold = 0; // ignore values below
//int prevMeasurementRes = 0; // previous measurement value

bool isThisMaxFirst = false; // whether measurement max value was reached previously

int ledLevel = 0; // current height of the led bar
int maxVal = 0; // reached maximum led level
int prewIndicatorLevel = 0;

byte numberOfItersInd = 0; // Number of iterations of indicator drawer

unsigned long prewDecayTime = 0; 
unsigned long prewIndicatorDecayTime = 0;

// 4294967295; // 2^32 - 1
unsigned long lastConnectionTime = 0;

//Array shifter
unsigned long lastArrayShift = 0;

//Color shifter
uint8_t minG = 0;
uint8_t minB = 0;
byte gbShiftMode = 0;
uint8_t G = 255;
uint8_t B = 255;
unsigned long lastColorShift = 0;

//Drop Width Modulation
unsigned long prevDropTime = 0;
byte dwmMode = 0; // Necessary to provide correct run order of the codes
int dwmIterator = 0; // Iterator to imitate outer for loop
int outputDwm = 0; // Will be our final result
int dwmMeasCou = 0; // How many measurements taken so far

CRGB secsIndicatorCol = CRGB(154, 205, 16); // seconds indicator color
CRGB IndicatorCol = CRGB::White; // music indicator color

CRGB leds[NUM_LEDS];
CRGB filled[NUM_LEDS];
CRGB buff[MAX_WIDTH];
CRGB buffFilled[MAX_WIDTH];

WiFiUDP UDP;
char packetBuffer[255]; //buffer to hold incoming packet

/* Color palettes */

DEFINE_GRADIENT_PALETTE( pal1 ) {
    0, 0,  255,  255, 
  255, 0, 255, 0};

DEFINE_GRADIENT_PALETTE( pal2 ) {
    0, 0,  255,  0, 
  255, 255, 0, 0};
  
DEFINE_GRADIENT_PALETTE( pal3 ) {
  0, 0, 255, 0,
  64, 0, 255, 255,
  127, 0, 255,  0,
  192, 0, 255, 255, 
  255, 0, 255, 0};
  
DEFINE_GRADIENT_PALETTE( pal4 ) {
  0, 0, 255, 0,
  255, 0, 255, 0};

DEFINE_GRADIENT_PALETTE( pal5 ) {
  0, 50, 255, 50,
  255, 50, 255, 50};

DEFINE_GRADIENT_PALETTE( pal6 ) {
  0, 255, 255, 255,
  255, 255, 255, 255};

DEFINE_GRADIENT_PALETTE( pal7 ) {
  0, 255, 255, 0,
  255, 255, 255, 0};

CRGBPalette16 palArr[] = {pal1, pal1, pal2, pal2, pal5, pal7, pal3, pal3, pal1, pal2, pal1, pal2, pal4, pal5, pal6, pal6};
CRGB IndicatorColors[] = {CRGB(255,128,0), CRGB::White, CRGB(0,0,128), CRGB::White, CRGB::Green, CRGB::Red, CRGB::White, CRGB::White, CRGB::White, CRGB::White,  CRGB::White, CRGB::White, CRGB::White, CRGB::Green, CRGB::White, CRGB::Red};
CRGB secsIndicatorColors[] = {CRGB::Yellow, CRGB::Yellow, CRGB::Cyan, CRGB::Cyan, CRGB::White, CRGB::Green, CRGB(255,128,0), CRGB(255,128,0), CRGB::White, CRGB::White, CRGB::White, CRGB::White, CRGB(255,128,0), CRGB::White, CRGB::Red, CRGB(100,20,100)};

void setupWifi(){
  WiFi.disconnect(); //Disconnect from old connections
  WiFi.begin(MY_SSID, PASSWORD);
  WiFi.mode(WIFI_STA); // Only connect the router
  while (WiFi.status() != WL_CONNECTED) { //Loop here until connected
    digitalWrite(LED_BUILTIN, LOW);
    delay(400);
    digitalWrite(LED_BUILTIN, HIGH);                
    delay(100);
    // For delay, 500 ms in total is good. Splitting it to 400 + 100 enables us to see blinking LED                          
  }
  WiFi.config(receiverIP, subnet, gateway, dns); // Configure Wifi settings
  
  UDP.begin(UDP_RESPONSE_PORT);
  UDP.beginPacket("192.168.4.1", UDP_REQUEST_PORT); //send ip to server
  UDP.write("i"); // "i" for init, i.e. request all variables available
  UDP.endPacket();
}

void disableWifi(){
  WiFi.disconnect(); //Disconnect from old connections
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
}

void painter(){
  if (currMode >= 16){
    prevMode = currMode;
    return;
  }
     
  fill_palette(filled, NUM_LEDS, 0, 255 / NUM_LEDS, palArr[currMode], 255, LINEARBLEND);
  secsIndicatorCol = secsIndicatorColors[currMode];
  IndicatorCol = IndicatorColors[currMode];
  
  byte gbShiftMode = 0;
  uint8_t G = 255;
  uint8_t B = 255;
  unsigned long lastColorShift = 0;

  fill_palette(buffFilled, MAX_WIDTH, 0, 255 / MAX_WIDTH, palArr[currMode], 255, LINEARBLEND);
  dwmMode = 0;
  
  prevMode = currMode;
}

void setIOMode(){
  pinMode(POT,INPUT);
  pinMode(SWITCH,INPUT_PULLUP);
  pinMode(BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BTN), handleBtn, FALLING); 
}

void decidePhysMode(){
  if(digitalRead(SWITCH))
    physicalMode = 0;
  else if (analogRead(POT) < 1022)
    physicalMode = 1;
  else
    physicalMode = 2;
}

void setup() {
  if (DEBUG_ENABLED){
    Serial.begin(DEBUG_RATE);
  }
  
  setIOMode();
  FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  //FastLED.setTemperature(Tungsten40W);

  decidePhysMode();
  if (!physicalMode){
    setupWifi();
    painter();
  }
  else{
    disableWifi();
  }

  if (physicalMode == 2){// Start Adalight
    // Initial RGB flash
    LEDS.showColor(CRGB(255, 0, 0));
    delay(500);
    LEDS.showColor(CRGB(0, 255, 0));
    delay(500);
    LEDS.showColor(CRGB(0, 0, 255));
    delay(500);
    LEDS.showColor(CRGB(0, 0, 0));
    
    Serial.begin(ADA_RATE);
    Serial.print("Ada\n");
  }

}

void handleRemoteMode(){
  receiveData();
  if (prevMode != currMode)
    painter();
  FastLED.setBrightness(brightness);
  
  if (0 <= currMode && currMode <= 5){
    drawVuSmooth(musicIndicatorEnabled, IndicatorCol); // drawVuSmooth(bool indicatorEnabled, CRGB indicatorColor)
  }
  else if (currMode == 6){
    arrayShifter(1);
    drawVuSmooth(musicIndicatorEnabled, IndicatorCol); // drawVuSmooth(bool indicatorEnabled, CRGB indicatorColor)
  }
  else if (currMode == 7){
    arrayShifter(0);
    drawVuSmooth(musicIndicatorEnabled, IndicatorCol); // drawVuSmooth(bool indicatorEnabled, CRGB indicatorColor)    
  }
  else if (8 <= currMode && currMode <= 9){
    dropWidthModulation();
  }
  else if (10 <= currMode && currMode <= 11){
    drawVuFast();
  }
  else if (currMode == 12){
    colorShifter();
    drawVuSmooth(musicIndicatorEnabled, IndicatorCol); // drawVuSmooth(bool indicatorEnabled, CRGB indicatorColor)
  }
  else if (13 <= currMode && currMode <= 14){
    pwmVu();
  }
  else if (currMode == 15){
    torch(CRGB(255, 150, 40));
  }
  else if (16 <= currMode && currMode <= 17){
    animatedFlag(currMode);
  }
}

void handleBtn(){
  if (DEBUG_ENABLED)
    Serial.println("Button pressed");
  
  delay(10);
  while(!digitalRead(BTN)) // wait until the btn released
    delay(2);

  if ( (millis() - lastPressTime) >= BUTTON_CONSIDERATION_LIMIT ){// i.e. button intentionally pressed
    pressCou++;
    lastPressTime = millis();
  }
}

void emergencyTorch(){
  FastLED.setBrightness((1023 - analogRead(POT))/4);

  if (pressCou){
    while( (millis() - lastPressTime) < BUTTON_DOUBLE_PRESS_TIMEOUT) //wait for second or more presses
      ;
  }

  if (pressCou >= BUTTON_MAX_PRESS_COU)
    pressCou = BUTTON_MAX_PRESS_COU;

  switch(pressCou){
  case 1:
    currMode++;
    currMode %= EMERGENCY_TORCH_NUM;
    pressCou = 0;
    break;
  case 2:
    indicateSeconds = !indicateSeconds; // toggle
    pressCou = 0;
    break;
  default:
    ;
}




  switch(currMode){ 
    case 3:
      torch(CRGB(255, 255, 255));
      break;
    case 2:
      torch(CRGB(255, 255, 220));
      break;
    case 1:
      torch(CRGB(255, 220, 127));
      break;
    case 0:
      torch(CRGB(255, 150, 40));
      break;
    default:
      ;
    }

  /*if (DEBUG_ENABLED){
    Serial.print("Emergency Torch Mode: ");
    Serial.println(currMode);
  }*/

}


void handleAdalight(){
  uint8_t prefix[] = {'A', 'd', 'a'}, hi, lo, chk, i;
  
  // Wait for first byte of Magic Word
  for(i = 0; i < sizeof prefix; ++i) {
    waitLoop: while (!Serial.available()) ;;
    // Check next byte in Magic Word
    if(prefix[i] == Serial.read()) continue;
    // otherwise, start over
    i = 0;
    goto waitLoop;
  }
  
  // Hi, Lo, Checksum  
  while (!Serial.available()) ;;
  hi = Serial.read();
  while (!Serial.available()) ;;
  lo = Serial.read();
  while (!Serial.available()) ;;
  chk = Serial.read();
  
  // If checksum does not match go back to wait
  if (chk != (hi ^ lo ^ 0x55)) {
    i = 0;
    goto waitLoop;
  }
  
  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
  // Read the transmission data and set LED values
  for (uint8_t i = 0; i < NUM_LEDS; i++) {
    byte r, g, b;    
    while(!Serial.available());
    r = Serial.read();
    while(!Serial.available());
    g = Serial.read();
    while(!Serial.available());
    b = Serial.read();
    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;
  }
  
  // Shows new values
  FastLED.show();
}

void loop(){
  switch(physicalMode){
    case 0:{
      handleRemoteMode();
      break;
    }
    case 1:{
      emergencyTorch();
      break;
    }
    case 2:{
      handleAdalight();
      break;
    }
    default:
      ;   
  }
}

void receiveData(){
  UDP.parsePacket();
  UDP.beginPacket("192.168.4.1", UDP_REQUEST_PORT); //send ip to server
  UDP.write("1");
  UDP.endPacket();
  int len = UDP.read(packetBuffer, 255);
  char *subs = "empty";
  if (len > 0) {
      if (DEBUG_ENABLED){
    Serial.println(packetBuffer);
   }
    packetBuffer[len] = 0;   
    subs = strtok(packetBuffer, delim);
    measurementRes = rescale(atoi(subs) * measurementCoeff);
    subs = strtok(NULL, delim);
    brightness = (byte) atoi(subs);
    subs = strtok(NULL, delim);
    currMode = (byte) atoi(subs);
    subs = strtok(NULL, delim);
    measurementCoeff = (byte) atoi(subs);
    subs = strtok(NULL, delim);
    musicIndicatorEnabled = (bool) atoi(subs);
    subs = strtok(NULL, delim);
    indicateSeconds = (bool) atoi(subs);

    lastConnectionTime = millis();
  }
  else{
    // If connection is lost for a while, then asign measurementRes = 0 to let the LED decay
    if( millis() - lastConnectionTime > CONNECTION_LOST_TIMEOUT){
      measurementRes = 0;
      lastConnectionTime = millis();
    }
    String myStr = "EMPTY";
    myStr.toCharArray(packetBuffer, 35);
  }

}

int rescale(int res){
  res = map(res, 0, 512, 0, (NUM_LEDS+1));
  if (res >= NUM_LEDS-1)
    res = NUM_LEDS-1;
  return res;
}

void drawVuSmooth(bool indicatorEnabled, CRGB indicatorColor){
  if (measurementRes >= ledLevel){
    ledLevel = measurementRes;
  }

  else{
    if ( ( (millis() - prewDecayTime) >= DECAY_INTERVAL ) && ledLevel>(measurementRes)){
      ledLevel--;
      prewDecayTime = millis();
    }
  }
  FastLED.clear();
  for(int i=0; i < ledLevel; i++){
    leds[i] = filled[i];
  }
  if (indicatorEnabled)
    drawIndicator(ledLevel, indicatorColor);
  if(indicateSeconds)
    leds[(millis()/1000)%NUM_LEDS] = secsIndicatorCol;
 
  FastLED.show();
  
  //prevMeasurementRes = measurementRes;
}

void drawIndicator(int value, CRGB color){
  if (value+1 >= maxVal && value > CRITICAL_LEVEL && value > prewIndicatorLevel){
    maxVal = value;
    isThisMaxFirst = true;
    numberOfItersInd = 0;
  }

  if (isThisMaxFirst && ( (millis() - prewIndicatorDecayTime) >= DECAY_INTERVAL )){
      maxVal += 1;
      prewIndicatorDecayTime = millis();
      numberOfItersInd++;      
  }
  else if ( ( (millis() - prewIndicatorDecayTime) >= DECAY_INTERVAL*1.3 && !isThisMaxFirst && value+1 <= maxVal)){
      maxVal -= 1;
      prewIndicatorDecayTime = millis();
    }
    
  if (numberOfItersInd >= INDICATOR_ITER_LIM){
    isThisMaxFirst = false; 
  }
  prewIndicatorLevel = maxVal;
  
  if (maxVal > CRITICAL_LEVEL){
    if (maxVal < (NUM_LEDS))
      leds[maxVal] = color;
    if (maxVal < (NUM_LEDS-1))
      leds[maxVal+1] = color;
    if (maxVal < (NUM_LEDS-2))
      leds[maxVal+2] = color;
  }
  else
    return;
}

void arrayShifter (bool shiftUp){
  if(millis() - lastArrayShift >= ARRAY_SHIFT_INTERVAL){
    if(shiftUp){
      CRGB holder = filled[NUM_LEDS-1];
      for(int i = NUM_LEDS-1; i > 0; i--){
        filled[i] = filled[i-1];
      }
      filled[0] = holder;
    }
    else{
      CRGB holder = filled[0];
      for( int i = 0; i < (NUM_LEDS-1); i++){
        filled[i] = filled[i+1];
      }
      filled[NUM_LEDS-1] = holder;
    } 
  lastArrayShift = millis();
  }
}

void colorShifter(){
  if(millis() - lastColorShift >= COLOR_SHIFT_INTERVAL){
    switch (gbShiftMode){
      case 0:
        G -= 1;
        if (G == minG)
        gbShiftMode = 1;
        break;
      case 1:
        G += 1;
        if (G == 255)
          gbShiftMode = 2;
        break;
      case 2:
        B -= 1;
        if (B == minB)
        gbShiftMode = 3;
        break;
      case 3:
        B += 1;
        if (B == 255)
        gbShiftMode = 0;
        break;
      default:
        ;
    }
  lastColorShift = millis();
  fill_solid(filled, NUM_LEDS, CRGB(0, G, B));
  }
}

void accumulateMeasurementsForDwm(){
  outputDwm += measurementRes;
  dwmMeasCou++;
}

void dropWidthModulation(){
  if( (millis() - prevDropTime) >= DROP_INTERVAL ){
    switch (dwmMode){
      case 0:{
        int output = 0;
        if (dwmMeasCou) // if dwmMeasCou is not 0
          output = outputDwm/dwmMeasCou/DIVIDER;
          
        // Clear buffer
        for(int i = 0; i < MAX_WIDTH; i++){ 
          buff[i] = CRGB::Black;
        }
        // Fill buffer until output value reached
        for(int i = 0; i < (output); i++){
          buff[i] = buffFilled[i];
        }
        
        // clear variables
        outputDwm = 0;
        dwmMeasCou = 0;
        dwmMode = 1;
        dwmIterator = 0;
        break;
      }
    
      // Write buffer to leds while shifting the old outputs
      case 1:{
        for(int i = 0; i < (NUM_LEDS - 1); i++){
          leds[i] = leds[i+1];
        }
        leds[NUM_LEDS - 1] = buff[dwmIterator];
        accumulateMeasurementsForDwm();
        FastLED.show();
        
        dwmIterator++;
        if (dwmIterator == MAX_WIDTH){
          dwmMode = 2;
          dwmIterator = 0;
        }
        break;
      } 
  
      // Write buffer black leds in order to create some gap between outputs
      case 2:{
        for(int i = 0; i < (NUM_LEDS - 1); i++){
           leds[i] = leds[i+1];
        }
        leds[NUM_LEDS - 1] = CRGB::Black;
        FastLED.show();
        accumulateMeasurementsForDwm();
        
        dwmIterator++;
        if (dwmIterator == EMPTY_WIDTH){
          dwmMode = 0;
          dwmIterator = 0;
        }  
        break;
      }
      default:
        ;
      
    }
  prevDropTime = millis();
  }
}

void pwmVu(){
  if (measurementRes >= ledLevel){
    ledLevel = measurementRes;
  }

  else{
    if ( ( (millis() - prewDecayTime) >= DECAY_INTERVAL ) && ledLevel>(measurementRes)){
      ledLevel--;
      prewDecayTime = millis();
    }
  }

  for(int i = 0; i < NUM_LEDS; i++){
    leds[i].r = filled[i].r*ledLevel/NUM_LEDS;
    leds[i].g = filled[i].g*ledLevel/NUM_LEDS;
    leds[i].b = filled[i].b*ledLevel/NUM_LEDS;
  }
  
  if(indicateSeconds)
    leds[(millis()/1000)%NUM_LEDS] = secsIndicatorCol;
    
  FastLED.show();
}

void drawVuFast(){
  FastLED.clear();
  for(int i=0; i < measurementRes; i++){
    leds[i] = filled[i];
  }
  if(indicateSeconds)
    leds[(millis()/1000)%NUM_LEDS] = secsIndicatorCol;
  FastLED.show();
}

void animatedFlag(int which){
  uint16_t sinBeat = beatsin16(30, 0, NUM_LEDS - 1, 0, 0);
  uint16_t sinBeat2 = beatsin16(30, 0, NUM_LEDS - 1, 0, 32767);
  uint16_t sinBeat3 = beatsin16(30, 0, NUM_LEDS - 1, 0, 0);
  uint16_t sinBeat4 = beatsin16(30, 0, NUM_LEDS - 1, 0, 21845);
  uint16_t sinBeat5 = beatsin16(30, 0, NUM_LEDS - 1, 0, 43690);
  switch(which){
    case 16:
      leds[sinBeat] = CRGB::White;
      leds[sinBeat2] = CRGB::Red;
      fadeToBlackBy(leds, NUM_LEDS, 10);
      FastLED.show();
      break;
    case 17:
      leds[sinBeat3] = CRGB::Green;
      leds[sinBeat4] = CRGB::Red;
      leds[sinBeat5] = CRGB::Cyan;
      fadeToBlackBy(leds, NUM_LEDS, 10);
      FastLED.show();
      break;
  default:
  ;
  }
}

void torch(CRGB color){
  fill_solid(leds, NUM_LEDS, color);
  if(indicateSeconds)
    leds[(millis()/1000)%NUM_LEDS] = secsIndicatorCol;
  FastLED.show();
}
