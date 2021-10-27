# wirelessMusicVisualizer
Using two node MCU ESP8266 modules and addressable RGB LED strip, visualize the music, display animated flags, brighten the darkness (with the help of torch and emergency torch modes), and even Adalight compatible. Therefore, you can use it with any kind of Adalight compatible software, such as [Prismatik Unofficial](https://github.com/psieg/Lightpack)

## Videos & Gifs of the project
<div align="center">
<img src=https://user-images.githubusercontent.com/87245315/138785878-a3807bdd-ef61-46ae-9055-652e1cd40250.gif alt= demoGif></img>
<br>
</div>
<br>
The real project looks way smoother than the gif. You may watch the YouTube videos to see that. In fact, the real refresh rate of the project is nearly 100 Hz; however, YouTube supports only up to 60 fps, and the gif is unfortunately only 30 fps.
<br>

- Please watch the [complete demonstration video](https://www.youtube.com/watch?v=xi880eUqIho) on YouTube.
- You may also want to watch my [playlist](https://www.youtube.com/playlist?list=PLV24y8ZhNEglrx4vv8YyNaVGACWpZlcM2) consisting of uncopyrighted music visualized by this project.

## Features
### Music visualization (Transmitter needed)
- Smooth bar music visualizer in different color modes (steady gradient, flowing gradient, color shifting)
- Fast bar visualizer
- Brightness changing visualizer
- Optional music indicator and seconds indicator for all modes
### Torch (Transmitter needed)
- Warm white light, 2700K color temperature.
- Optional seconds indicator
### Animated flags (Transmitter needed)
- Display your county's flag colors on the RGB stick
- Not music reactive
### Emergency torch mode (nothing needed) 
- Various colored lights in the color range 2700K to 6500K
- Adjustable brightness
- Optional seconds indicator
### Adalight mode (USB micro type-B to type A cable needed)
- Adalight client for Adalight compatible software


## Usage
### Connection with the transmitter station
Unpress the toggle switch first, then power up. If the transmitter station is near the receiver and turned on, the receiver will connect to the transmitter automatically and run according to the transmitter's configuration. 

### Enabling Adalight client mode
Press the toggle switch, adjust the potentiometer knob to the maximum, then power up the receiver. If you see the red, green, blue colors, respectively, the Adalight client mode has been initialized successfully.


### Enabling emergency torch mode
Press the toggle switch, and adjust the potentiometer below the maximum value, then power up. Note that the potentiometer is software inverted in the emergency torch mode so that the brightness increases when the potentiometer is rotated clockwise. (I did this because I was lazy to open the box again and swap the potentiometer positive and ground connections by desoldering and resoldering. I need to do the soldering job again because the purple cable has a jumper pin, but the blue one does not. The cables mentioned above can be seen on the image titled [Receiver station lid](#receiver-station-lid) ) 

### Browsing through the menu & changing the parameters
When the menu item title is highlighted, rotating the rotary encoder will change which setting you are bout to change.
When a variable is highlighted, rotating the encoder will change the value of the variable.
Pressing the rotary encoder button will swap the selected item. For example, when the 'Brightness' is selected, if you press the encoder button, the value of the brightness will be selected so that you can adjust the brightness of the LEDs.

### Saving the configuration
When you are done with adjusting the settings, move to 'saveConfig.' on the menu and press the rotary encoder button to write your current configuration to the external EEPROM.

## Necessary Libraries
Please install the libraries below if you have not done yet.
- [Fastled library](https://github.com/FastLED/FastLED)
- [Adafruit GFX library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit SSD1306 library](https://github.com/adafruit/Adafruit_SSD1306)
- [SoftwareI2C library](https://github.com/Seeed-Studio/Arduino_Software_I2C)

## Schematics
### Transmitter
![Transmitter Schematic](./Transmitter.png)

### Receiver
![Receiver Schematic](./Receiver.png)


## Pictures

### Transmitter
#### Inside of the transmitter station
![insideTransmitter.jpg](./pictures/insideTransmitter.jpg)
#### Transmitter station lid
![transmitterControls.jpg](./pictures/transmitterControls.jpg)

### Receiver
#### Inside of the receiver station
![insideReceiver.jpg](./pictures/insideReceiver.jpg)
#### Receiver station lid
![receiverControls.jpg](./pictures/receiverControls.jpg)
#### Adjustable XL4015 step down converter module
![xl4015.jpg](./pictures/xl4015.jpg)
#### Adjustable LM2596 step down converter module
![lm2596.jpg](./pictures/lm2596.jpg)

## Additional modules
They are not directly related to the project. However, they are worth mentioning.

### Powering the device mobile or in mains power loss & failure
When necessary, the external boost converter module can be used to power the device via a power bank or any device that is powerful enough and has a USB out. Warning: You may need to decrease the brightness of the LEDs if your power gadget is not capable enough to run the device flawlessly. Most of the time, if you try to draw more current than your power gadget can handle, your power gadget will disconnect its battery from the output, and you will need to restart your power device. Sometimes, your power device can automatically do this restart process and cause the LEDs to flicker/blink. Therefore, if you face any abnormal behaviors like above mentioned, my advice is to decrease the brightness first (from the transmitter module or the potentiometer, in emergency torch mode) and then turn off and on the receiver station using the switch on the external boost converter module or directly plug out the USB cable and plug it in back.

#### External XL6009 step up converter module
![stepUpPowerbankStation.jpg](pictures/stepUpPowerbankStation.jpg)

#### External XL6009 step up converter module

### Recording setup
The setup to record line output of the PC with any mobile phone.
There is a 10 kOhm resistor between any signal output channel, left or right, to mobile phone microphone input.

#### Y cable to split recording setup and my project (transmitter station + headphones, which also uses another Y cable)
![yCable.jpg](pictures/yCable.jpg)

#### Recording setup
![soundRecordingCable.jpg](pictures/soundRecordingCable.jpg)

#### 10 kOhm resistor close up
![resistor.jpg](pictures/resistor.jpg)
