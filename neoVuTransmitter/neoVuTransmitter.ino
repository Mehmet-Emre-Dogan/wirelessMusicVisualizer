      /******** References *********/
/* 
 *  ============= AP ============= 
 *  https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/soft-access-point-class.rst
 *  
 *  ============= UDP ============= 
 * https://siytek.com/communication-between-two-esp8266/
 * https://stackoverflow.com/questions/39064193/esp8266-send-udp-string-to-ap
 * https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
 * 
 * ============= OLED Display ============= 
 * https://www.instructables.com/OLED-I2C-Display-ArduinoNodeMCU-Tutorial/
 * https://www.circuitgeeks.com/oled-display-arduino/
 * https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
 * https://learn.adafruit.com/adafruit-gfx-graphics-library/graphics-primitives
 * https://oleddisplay.squix.ch/#/home // Font generator
 * 
 * ============= Rotary Encoder =============
 * See the link below for extra info
 * https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
 * https://dronebotworkshop.com/rotary-encoders-arduino/
 * 
 * ============= External i2c EEPROM =============
 * External EEPROM reading-writing functions are adopted from the link below:
 * https://dronebotworkshop.com/eeprom-arduino/
 * https://arduino-projects4u.com/arduino-24c16/
 * 
*/
#define DEBUG_ENABLED false

/* Libraries */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts\FreeSans18pt7b.h>
#include <Fonts\FreeSans12pt7b.h>

#include <SoftwareI2C.h>

/* My files */
#include "secrets.h"
#include "dialog13.h"
#include "dialog11.h"
#include "dialog9.h"

/******** Constants *********/
#define PACKET_BUFFER_SIZE 255
#define MODE_NUM 18
#define MENU_COUNT 9

/* OLED Display */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

/* External i2c EEPROM */
#define EEPROM_ADDR 0x50
#define EEPROM_SCL D4
#define EEPROM_SDA D5
#define EEPROM_TYPE "24C08" // The other model I have tested is "24C32"

/* Rotary Encoder */
#define ROTARY_CLK D6
#define ROTARY_DT D7
#define ROTARY_BTN D3

/* Measurements */
#define MUSIC A0
#define NUM_MEASUREMENTS 15

/* Serial communication */
#define DEBUG_RATE 9600 // Baud rate to print debug info to computer

/******** Function templates ********/
void ICACHE_RAM_ATTR checkRotary();
void ICACHE_RAM_ATTR handleBtn();

/******** Global variables & objects ********/
/* UDP */
WiFiUDP UDP; // creating UDP object
char packetBuffer[PACKET_BUFFER_SIZE]; // UDP Buffer

/* OLED Display */
String titles[MENU_COUNT] = {"Brightness", "Mode", "Sensivity", "Threshold", "Music Indicator", "Second Indicator", "Wifi Channel*", "Save Config.", "Restart"};
bool titleOrValue = 0; // 0: title selected, 1: value selected
int counter = 0; // titleIndex_x2
int titleIndex = 0;
int varArr_x2[MENU_COUNT - 2] = {511, 0, 8, 20, 2, 2, 2}; // int brightness_x2 = 511, vuMode_x2 = 0, sensivity_x2 = 8, threshold_x2 = 20, bool musicInd_x2 = 2; secsInd_x2 = 2;
uint8_t varArr[MENU_COUNT - 2] = {255, 0, 4, 10, 1, 1, 1}; // uint8_t brightness = 255, vuMode = 0, sensivity = 4, threshold = 10,  bool musicInd = 1, bool secsInd = 1;
String varRangeLimMin[MENU_COUNT] = {"0", "0", "0", "0", "0", "0", "1", "0", "0"};
String varRangeLimMax[MENU_COUNT] = {"255", String(MODE_NUM-1), "255", "255", "1", "1", "13", "0", "0"};
String boolArr[] = {"disabled", "enabled"};
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
byte displayUpdateIterCou = 0;

/* Rotary Encoder */
int currentStateCLK;
int previousStateCLK; 

/* External i2c EEPROM */
bool configWillBeSaved = false;
SoftwareI2C softwarei2c;

/* General */
bool goingToRestart = false;

/******** Functions ********/

void(* resetFunc) (void) = 0; // when you call it, it resets the board

void restart_(){
  display.clearDisplay();
  display.fillRect(0, 0, 128, 16, WHITE);
  display.setFont(&Dialog_plain_13);
  display.setCursor(1, 12);
  display.setTextColor(BLACK); // 'inverted' text
  display.println("Please wait...");
  display.setCursor(1, 25);
  display.setFont(&Dialog_plain_11);
  display.setTextColor(WHITE);
  display.println("Device will restart in 2 seconds");
  display.display();
  
  delay(2000);

  resetFunc();
}

void normalizeVars(){
  for(int i = 0; i<(MENU_COUNT - 2); i++){
    varArr[i] = (uint8_t) (varArr_x2[i]/2);
  }

  // Normalizing "Mode" variable
  //varArr[1] %= MODE_NUM;
  if (varArr[1] == 255){ // i.e. varArr[1] < 0 if it were not uint8_t
    varArr[1] = MODE_NUM - 1;
    varArr_x2[1] = 2*(MODE_NUM - 1);
  }
  else if (varArr[1] > (MODE_NUM - 1)){
    varArr[1] = 0;
    varArr_x2[1] = 0;
  }
  
  varArr[4] = (varArr[4] % 2); // In fact, music indicator is a bool so scale it to be bool
  varArr[5] = (varArr[5] % 2); // In fact, indicate seconds is a bool so scale it to be bool

  // Channel can be only in between [1-13]
  if (varArr[6] < 1){
    varArr[6] = 13;
    varArr_x2[6] = 26;
  }
  else if (varArr[6] > 13){
    varArr[6] = 1;
    varArr_x2[6] = 2;
  }
  
}

void enableInterrupts(){
  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK), checkRotary, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_DT), checkRotary, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_BTN), handleBtn, FALLING);
}

void disableInterrupts(){
  detachInterrupt(digitalPinToInterrupt(ROTARY_CLK) );
  detachInterrupt(digitalPinToInterrupt(ROTARY_DT) );
  detachInterrupt(digitalPinToInterrupt(ROTARY_BTN) );
}

void initRotary() { 
  pinMode(ROTARY_CLK,INPUT);
  pinMode(ROTARY_DT,INPUT);
  pinMode(ROTARY_BTN, INPUT_PULLUP);
  previousStateCLK = digitalRead(ROTARY_CLK);  
}

void checkRotary(){
  if (titleOrValue){
    // Read the current state of inputCLK
    currentStateCLK = digitalRead(ROTARY_CLK);
    // If the previous and the current state of the ROTARY_CLK are different then a pulse has occured
    if (currentStateCLK != previousStateCLK){  
      if (digitalRead(ROTARY_DT) != currentStateCLK){   // If the inputDT state is different than the ROTARY_CLK state then the encoder is rotating counterclockwise
        varArr_x2[titleIndex] --; 
      } 
      else{  // Encoder is rotating clockwise
        varArr_x2[titleIndex] ++; 
      }
    } 
    // Update previousStateCLK with the current state
    previousStateCLK = currentStateCLK;
  
    normalizeVars();
  }

  else{
    // Read the current state of inputCLK
    currentStateCLK = digitalRead(ROTARY_CLK);
    // If the previous and the current state of the ROTARY_CLK are different then a pulse has occured
    if (currentStateCLK != previousStateCLK){  
      if (digitalRead(ROTARY_DT) != currentStateCLK){   // If the inputDT state is different than the ROTARY_CLK state then the encoder is rotating counterclockwise
        counter --; 
      } 
      else{  // Encoder is rotating clockwise
        counter ++; 
      }
    } 
    // Update previousStateCLK with the current state
    previousStateCLK = currentStateCLK;

    titleIndex = (counter/2);
    titleIndex %= MENU_COUNT;
    if (titleIndex < 0){
      titleIndex = MENU_COUNT - 1;
      counter = (MENU_COUNT - 1)*2;
    }  
  }
  displayUpdateIterCou = 0;
}

void handleBtn(){
  disableInterrupts();
  delay(10);
  
  while(!digitalRead(ROTARY_BTN)) // wait until the btn released
    delay(2);
  
  if(titleIndex == (MENU_COUNT - 1) ){ // If the menu on "restart"
    goingToRestart = true;
  }
  else if(titleIndex == (MENU_COUNT - 2) ){ // If the menu on "save config"
    configWillBeSaved = true;
  }
  else{
    titleOrValue = !titleOrValue; // toggle
  }

  displayUpdateIterCou = 0; // trigger display refresh
  enableInterrupts();
}

void saveConfig(){
  if(DEBUG_ENABLED){
    Serial.println("Started to saving configuration");
  }

  display.clearDisplay();
  display.fillRect(0, 0, 128, 16, WHITE);
  display.setFont(&Dialog_plain_13);
  display.setCursor(1, 12);
  display.setTextColor(BLACK); // 'inverted' text
  display.println("Please wait");
  display.setCursor(1, 25);
  display.setFont(&Dialog_plain_11);
  display.setTextColor(WHITE);
  display.println("Dumping configur-ation to external\nEEPROM...");
  display.display();

  dumpEEPROM();
  delay(100);

  display.clearDisplay();
  display.fillRect(0, 0, 128, 16, WHITE);
  display.setFont(&Dialog_plain_13);
  display.setCursor(1, 12);
  display.setTextColor(BLACK); // 'inverted' text
  display.println("Success");
  display.setCursor(1, 25);
  display.setFont(&Dialog_plain_9);
  display.setTextColor(WHITE);
  display.println("EEPROM dumping pro-cess finished.\nThis message will dis-appear in 3 seconds.");
  display.display();
  delay(3000);

  configWillBeSaved = false;
  if(DEBUG_ENABLED){
    Serial.println("Configuration saved");
  }
}

void initDisplay(){
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.setFont();
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.display();
}

void updateDisplay(){
  display.clearDisplay();
    
  // set text
  switch(titleIndex){
    case 4:
      // 0: title selected, 1: value selected
      // BLACK: 0, WHITE: 1
      
      display.fillRect(0, 0, 128, 16, !titleOrValue);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(titleOrValue); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Enabled/Disabled");
    
      display.fillRect(0, 36, 128, 27, titleOrValue);
      display.setCursor(1, 61);
      display.setFont(&FreeSans18pt7b);
      display.setTextColor(!titleOrValue); // 'inverted' text
      display.println( boolArr[varArr[titleIndex]] );
    break;

    case 5:  
      // 0: title selected, 1: value selected
      // BLACK: 0, WHITE: 1
      
      display.fillRect(0, 0, 128, 16, !titleOrValue);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(titleOrValue); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Enabled/Disabled");
    
      display.fillRect(0, 36, 128, 27, titleOrValue);
      display.setCursor(1, 61);
      display.setFont(&FreeSans18pt7b);
      display.setTextColor(!titleOrValue); // 'inverted' text
      display.println( boolArr[varArr[titleIndex]] );
    break;

    case (MENU_COUNT - 2):  
      display.fillRect(0, 0, 128, 16, WHITE);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(BLACK); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Click to save");
    break;

    case (MENU_COUNT - 1):  
      display.fillRect(0, 0, 128, 16, WHITE);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(BLACK); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Click to restart");
    break;
    
    default:
      // 0: title selected, 1: value selected
      // BLACK: 0, WHITE: 1
      
      display.fillRect(0, 0, 128, 16, !titleOrValue);
      display.setFont(&Dialog_plain_13);
      display.setCursor(1, 12);
      display.setTextColor(titleOrValue); // 'inverted' text
      display.println(titles[titleIndex]);
    
      display.setCursor(1, 30);
      display.setFont(&Dialog_plain_13);
      display.setTextColor(WHITE); // 'inverted' text
      display.println("Range: [" + varRangeLimMin[titleIndex] + "-" + varRangeLimMax[titleIndex] + "]");
    
      display.fillRect(0, 36, 128, 27, titleOrValue);
      display.setCursor(1, 61);
      display.setFont(&FreeSans18pt7b);
      display.setTextColor(!titleOrValue); // 'inverted' text
      display.println((uint8_t) (varArr[titleIndex]));
  }
  
  display.display();
}
 
void setup() {
  if (DEBUG_ENABLED){
    Serial.begin(DEBUG_RATE);
  }
  pinMode(LED_BUILTIN, OUTPUT);

  softwarei2c.begin(EEPROM_SDA, EEPROM_SCL);
  loadEEPROM();
  initRotary();
  initDisplay();
  updateDisplay();
  
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(MY_SSID, PASSWORD, varArr[6], false); //  WiFi.softAP(ssid, psk, channel, hidden, max_connection)
  // Begin listening to UDP port
  UDP.begin(UDP_REQUEST_PORT);

  enableInterrupts();
}// END OF SETUP
 
void loop() {
  // HANDLE UDP COMMUNICATION
  // if there's data available, read a packet
  int packetSize = UDP.parsePacket();
  if (packetSize) {
    // read the packet into packetBufffer
    int len = UDP.read(packetBuffer, PACKET_BUFFER_SIZE);
    if (len > 0) {
      packetBuffer[len] = 0;
    }
     // send the measurement result to the receiver
    UDP.beginPacket(UDP.remoteIP(), UDP_RESPONSE_PORT);
    char* strBuf = "err";
    String mystr = (String(measureAverage(NUM_MEASUREMENTS)) + ":" );
    for( int i = 0; i < (MENU_COUNT-3); i++){
      if (i == 3)
        continue;
      mystr += (String(varArr[i]) + ":");
    }
    mystr.toCharArray(strBuf, 35);
    UDP.write(strBuf);
    UDP.endPacket();
    

  }

  // HANDLE OLED DISPLAY SCREEN  i.e. update the creeen if necessary
  if (displayUpdateIterCou < 2){
    updateDisplay();
    displayUpdateIterCou++;
  }

  // HANDLE EXTERNAL EEPROM DUMPING  i.e. dump the config to EEPROM when necessary
  if(configWillBeSaved){
    disableInterrupts();
    saveConfig();
    configWillBeSaved = false;
    enableInterrupts();
  }

  // HANDLE RESTARTING
  if(goingToRestart){
    restart_();
  }
  
}// END OF LOOP

int measureAverage(int sampleCount){
  int averageOutput = 0;
  for(int i = 0; i < sampleCount; i++){
    averageOutput += measureRaw();
  }
  averageOutput = averageOutput/sampleCount;
  if(DEBUG_ENABLED)
    Serial.println(averageOutput);
  return averageOutput * varArr[2]; // Sensivity, i.e. multiplier
}

int measureRaw(){
  int output = 0;
  output = (512 - analogRead(MUSIC));
  if (output < 0)
    output *= -1;
  output -= varArr[3]; // Threshold
  if(output < 0)
    output = 0;
  if(output > 512)
    output = 512;
  return output;
}


void loadEEPROM(){
  for(int i = 0; i < (MENU_COUNT - 2); i++){
    varArr[i] = readEEPROM(i);
    varArr_x2[i] = 2*readEEPROM(i);
  }  
}

void dumpEEPROM(){
  for(int i = 0; i < (MENU_COUNT - 2); i++){
    updateEEPROM(i, (byte) varArr[i]);
  } 
}

void updateEEPROM(int address, byte val){
    if (val != readEEPROM(address))
      writeEEPROM(address, val); 
}


// Function to write to EEPROOM
void writeEEPROM(int address, byte val)
{
  // Begin transmission to I2C EEPROM
  softwarei2c.beginTransmission(EEPROM_ADDR);

  if (EEPROM_TYPE == "24C32"){
    // Send memory address as two 8-bit bytes
    softwarei2c.write((int)(address >> 8));   // MSB
    softwarei2c.write((int)(address & 0xFF)); // LSB
  }
  else{
    softwarei2c.write((int)(address));   // Memory address is only one byte
  }
 
  // Send data to be stored
  softwarei2c.write(val);
 
  // End the transmission
  softwarei2c.endTransmission();
 
  // Add 5ms delay for EEPROM
  delay(5);
  if(DEBUG_ENABLED){
    Serial.print("Data written to address: ");
    Serial.println(address);
  }
}
 
// Function to read from EEPROM
byte readEEPROM(int address)
{
  // Define byte for received data
  byte rcvData = 0xFF;
 
  // Begin transmission to I2C EEPROM
  softwarei2c.beginTransmission(EEPROM_ADDR);
 
  
  if (EEPROM_TYPE == "24C32"){
    // Send memory address as two 8-bit bytes
    softwarei2c.write((int)(address >> 8));   // MSB
    softwarei2c.write((int)(address & 0xFF)); // LSB
  }

  else{
    softwarei2c.write((int)(address));   // Memory address is only one byte
  }
 
  // End the transmission
  softwarei2c.endTransmission();
 
  // Request one byte of data at current memory address
  softwarei2c.requestFrom(EEPROM_ADDR, 1);
 
  // Read the data and assign to variable
  rcvData =  softwarei2c.read();
 
  // Return the data as function output
  return rcvData;
}
