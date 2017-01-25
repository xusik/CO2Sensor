# CO2Sensor
MH-Z19, DHT11 sensors connecetd via ESP8266 to Blynk server

####Hardware:
  - ESP8266 NodeMCU LoLin       
  - MH-Z19         connected via software UART
  - DHT11          connected via 1wire
  - Nokia 5110 LCD connected via SPI
  
```sh
              -------
ADC0 --- A0  |       | D0   --- GP16 <- RST
GND  --- GND |       | D1   --- GP05 <- CE
VIN  --- VIN |       | D2   --- GP04 <- DC
GP10 --- SD3 |       | D3   --- GP0  <- DIN
GP9  --- SD2 |       | D4   --- GP02 <- CLK
MOSI --- SD1 |       | 3.3v --- 3V3
CS   --- CMD |       | GND  --- GND
MISO --- SD0 |       | D5   --- GP14 <- TX
SCLK --- CLK |       | D6   --- GP12 <- RX
GND  --- GND |       | D7   --- GP13 <- DHT
3.3v --- 3V3 |       | D8   --- GP15
EN   --- EN  |       | D9   --- GP03
RST  --- RST |       | D10  --- GP01
GND  --- GND |       | GND  --- GND
VIN  --- VIN |       | 3.3v --- 3V3
              -------     

```
