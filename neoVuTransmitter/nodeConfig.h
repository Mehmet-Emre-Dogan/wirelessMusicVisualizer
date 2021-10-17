#define FASTLED_ESP8266_RAW_PIN_ORDER

/******** IO pins ********/

#define MUSIC A0
#define DATA_PIN D2
#define BTN 3

/* External EEPROM */
#define DEVICE 0x50
#define SDA 5
#define SCL 4

void setIOMode(){
    pinMode(BTN, INPUT);
  }