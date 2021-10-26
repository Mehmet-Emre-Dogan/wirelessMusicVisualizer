# wirelessMusicVisualizer
Using two node MCU ESP8266 modules and addressable RGB LED strip, visualize the music, display animated flags, brighten the darkness (with the help of torch and emergency torch modes), and even Adalight compatible. Therefore, you can use it with any kind of Adalight compatible software, such as [Prismatik Unofficial](https://github.com/psieg/Lightpack)

## Videos & Gifs of the project
<div align="center">
<img src=https://user-images.githubusercontent.com/87245315/138785878-a3807bdd-ef61-46ae-9055-652e1cd40250.gif alt= demoGif></img>
<br>
</div>
<br>
The real project looks way more smoother than the gif. You may watch the YouTube videos to see that. In fact, the real refresh rate of the project is nearly 100 Hz; however, YouTube supports only up to 60 fps and the gif is unfortunately only 30 fps.

<br>

- Please watch the [complete demonstration video](https://www.youtube.com/watch?v=xi880eUqIho) on YouTube.
- You may also want to watch my [playlist](https://www.youtube.com/playlist?list=PLV24y8ZhNEglrx4vv8YyNaVGACWpZlcM2) consisting of uncopyrighted music visualized by this project.

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

### Recording setup
The setup to record line output of the PC with any mobile phone.
There is a 10 kOhm resistor between any signal output channel, left or right, to mobile phone microphone input.

#### Y cable to split recording setup and my project (transmitter station + headphones, which also uses another Y cable)
![yCable.jpg](pictures/yCable.jpg)

#### Recording setup
![soundRecordingCable.jpg](pictures/soundRecordingCable.jpg)

#### 10 kOhm resistor close up
![resistor.jpg](pictures/resistor.jpg)
