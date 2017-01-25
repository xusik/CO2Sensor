//
//              -------
//ADC0 --- A0  |       | D0   --- GP16 <- RST
//GND  --- GND |       | D1   --- GP05 <- CE
//VIN  --- VIN |       | D2   --- GP04 <- DC
//GP10 --- SD3 |       | D3   --- GP0  <- DIN
//GP9  --- SD2 |       | D4   --- GP02 <- CLK
//MOSI --- SD1 |       | 3.3v --- 3V3
//CS   --- CMD |       | GND  --- GND
//MISO --- SD0 |       | D5   --- GP14 <- TX
//SCLK --- CLK |       | D6   --- GP12 <- RX
//GND  --- GND |       | D7   --- GP13 <- DHT
//3.3v --- 3V3 |       | D8   --- GP15
//EN   --- EN  |       | D9   --- GP03
//RST  --- RST |       | D10  --- GP01
//GND  --- GND |       | GND  --- GND
//VIN  --- VIN |       | 3.3v --- 3V3
//              -------
//
//
//CO2   5v
//DHT11 3.3V
//LCD   3.3V


extern "C" {
#include "user_interface.h"
}
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <SimpleDHT.h>
#include "Nokia_5110.h"
#include <BlynkSimpleEsp8266.h>

//Nokia LCD pins
#define RST 16
#define CE 5
#define DC 4
#define DIN 0
#define CLK 2

//DHT11 pin define
#define pinDHT11 13


bool printToDisplay;

int timezoneCorrection;
unsigned int ppm;
int DHT_Problem;

String GMTTime = "no time";
const char* ssid = "****";     // your network SSID (name)
const char* ssid1 = "****";     // your AP SSID (name)
const char* password = "****";  // your network password
const char* ssid2 = "****";
const char* password2 = "****";
char auth[] = "****";

os_timer_t myTimer;
SimpleDHT11 dht11;
SoftwareSerial mySerial(14, 12); // 14 - TX , 12 - RX
ESP8266WebServer server ( 80 );
Nokia_5110 lcd = Nokia_5110(RST, CE, DC, DIN, CLK);


String getTime() {
  WiFiClient client;
  while (!!!client.connect("google.com", 80)) {
    return "No internet connection.";
  }

  client.print("HEAD / HTTP/1.1\r\n\r\n");

  while (!!!client.available()) {
    yield();
  }

  while (client.available()) {
    if (client.read() == '\n') {
      if (client.read() == 'D') {
        yield();
        if (client.read() == 'a') {
          if (client.read() == 't') {
            yield();
            if (client.read() == 'e') {
              if (client.read() == ':') {
                client.read();
                yield();
                String theDate = client.readStringUntil('\r');
                client.stop();
                return theDate;
              }
            }
          }
        }
      }
    }
  }
}


// start of timerCallback
void timerCallback(void *pArg) {

  printToDisplay = true;

} // End of timerCallback

void user_init(void) {

  os_timer_setfn(&myTimer, timerCallback, NULL);

  os_timer_arm(&myTimer, 60000, true);
} // End of user_init



int checkCO2_level () {
  int i;
  const byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  unsigned char response[9];
  mySerial.write(cmd, 9);
  memset(response, 0, 9);
  yield();
  mySerial.readBytes(response, 9);
  yield();

  unsigned int ppm;

  byte crc = 0;
  for (i = 1; i < 8; i++) crc += response[i];
  crc = 255 - crc;
  crc++;

  yield();
  if ( !(response[0] == 0xFF && response[1] == 0x86 && response[8] == crc) ) {
    Serial.println("CRC error: " + String(crc) + " / " + String(response[8]));
    ESP.restart();
    return 1;

  } else {

    yield();

    unsigned int responseHigh = (unsigned int) response[2];
    unsigned int responseLow = (unsigned int) response[3];
    ppm = (256 * responseHigh) + responseLow;
    Serial.println(ppm);
  }
  return ppm;
}


int check_DHT() {
  // read without samples.
  byte temperature = 0;
  byte humidity = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
    Serial.print("Read DHT11 failed.");
    return 1;
  }

  Serial.print((int)temperature); Serial.print(" *C, ");
  Serial.print((int)humidity); Serial.println(" %");
  return 0;
}

void printCO2LCD() {
  GMTTime = getTime();
  yield();
  lcd.clear();
  lcd.setCursor(0, 0);
  yield();
  if (GMTTime != "No internet connection.") {
    if ( GMTTime.substring(17, 19).toInt() > 21 ) {
      timezoneCorrection = GMTTime.substring(17, 19).toInt() - 22;

    } else {
      timezoneCorrection = GMTTime.substring(17, 19).toInt() + 2;

    }

    lcd.println("Time: " + String(timezoneCorrection) + GMTTime.substring(19, 22));
  } else {
    lcd.println("No internet.");
  }
  yield();
  ppm = checkCO2_level();
  yield();
  lcd.setCursor(0, 1);
  lcd.print("CO2  Level: ");
  lcd.print(String(ppm));
  Blynk.virtualWrite(V0, ppm);
  byte temperature = 0;
  byte humidity = 0;

  if (DHT_Problem != 1) {
    if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
      yield();
      Serial.print("Read DHT11 failed.");
      lcd.setCursor(0, 2);
      lcd.print("TEMP Sensor: NOK");


    } else {
      yield();
      lcd.setCursor(0, 2);
      lcd.print("TEMP Level: ");
      lcd.print(int(temperature));
      Blynk.virtualWrite(V1, int(temperature));
      lcd.setCursor(0, 3);
      lcd.print("HUM  Level: ");
      lcd.print(int(humidity));
      Blynk.virtualWrite(V2, int(humidity));
    }
  }


}

void startWIFI(void) {
  lcd.clear();
  yield();
  lcd.print("Conn to WiFi:");
  lcd.setCursor(0, 1);
  yield();
  lcd.print("SSID: " + String(ssid2));
  lcd.setCursor(0, 2);
  yield();

  Blynk.begin(auth, ssid2, password2);
  yield();
  checkCO2_level();

  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    yield();
    lcd.print("Conn to WiFi:");
    yield();
    lcd.setCursor(0, 1);
    lcd.print("SSID: " + String(ssid));
    lcd.setCursor(0, 2);
    yield();
    Blynk.begin(auth, ssid, password);
  }

  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  GMTTime = getTime();
  yield();
  DHT_Problem = check_DHT();

  lcd.clear();
  lcd.print(WiFi.localIP().toString());
  lcd.setCursor(0, 1);
  if (GMTTime != "No internet connection.") {
    lcd.println("Time GMT: " + GMTTime.substring(17, 22));
  } else {
    lcd.println("No internet.");
  }
  lcd.setCursor(0, 2);
  lcd.print("CO2 sensor: OK");
  lcd.setCursor(0, 3);
  if (DHT_Problem == 1) {
    lcd.print("Tmep sensor: NOK");
  } else {
    lcd.print("Tmep sensor: OK");
  }

  delay(5000);

  if ( MDNS.begin ( "esp8266" ) ) {
    Serial.println ( "MDNS responder started" );
  }
  yield();

}


void setup ( void ) {
  lcd.clear();
  Serial.begin(9600);
  mySerial.begin(9600);
  delay(100);

  startWIFI();

  printToDisplay = true;
  user_init();

}

void loop ( void ) {
  if (WiFi.status() != WL_CONNECTED) {
    startWIFI();
  }
  Blynk.run();


  if (printToDisplay == true) {
    printCO2LCD();
    printToDisplay = false;

    if (!Blynk.connected()) {
      Blynk.connect();
    }

  }


  yield();
}

