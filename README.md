# CO2Sensor
CO2, DHT11 sensors connecetd via ESP8266 to Blynk server

####Hardware:
  - ESP8266   connected via hardware SPI
  - BMP085    connected via software I2C
  - DHT11     connected via 1wire
  
```sh
                  +-\/-+
           VCC   1|    |20  GND
           P1.0  2|    |19  XIN
           P1.1  3|    |18  XOUT
           P1.2  4|    |17  TEST
           P1.3  5|    |16  RST#
           P1.4  6|    |15  P1.7 MOSI
    5  SCK P1.5  7|    |14  P1.6 MISO  7
    3   CE P2.0  8|    |13  P2.5 DHT11 2
    4  CSN P2.1  9|    |12  P2.4 SCL
    8  IRQ P2.2 10|    |11  P2.3 SDA
                  +----+            
```
