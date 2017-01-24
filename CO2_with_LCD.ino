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
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <SimpleDHT.h>
#include "Nokia_5110.h"
#include <BlynkSimpleEsp8266.h>


#define RST 16
#define CE 5
#define DC 4
#define DIN 0
#define CLK 2

int pinDHT11 = 13;


byte temperature_web = 0;
byte humidity_web = 0;

bool tickOccured;
bool printToDisplay;

int k = 0, t;
int oneHour = 0;
int timezoneCorrection;
int timeIndex = 0;
unsigned int ppm;
int retry = 0;
int DHT_Problem;

String co2[24] = "";
String GMTTime = "no time";
String printCO2Measurements;
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

  if ( oneHour == 60) {
    tickOccured = true;
    oneHour == 0;
  } else {
    oneHour++;
  }

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
  Blynk.virtualWrite(V0,ppm);
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
      temperature_web = temperature;
      humidity_web = humidity;
      lcd.setCursor(0, 2);
      lcd.print("TEMP Level: ");
      lcd.print(int(temperature));
      Blynk.virtualWrite(V1,int(temperature));
      lcd.setCursor(0, 3);
      lcd.print("HUM  Level: ");
      lcd.print(int(humidity));
      Blynk.virtualWrite(V2,int(humidity));
    }
  }


}

void handleRoot() {

  printCO2LCD();
  ppm = checkCO2_level();

  String message =  "<html>\n";
  message += "  <head>\n";
  message += "  <meta http-equiv=\"refresh\" content=\"60\">\n";
  message += "    <!--Load the AJAX API-->\n";
  message += "    <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n";
  message += "    <script type=\"text/javascript\">\n";
  message += "      google.charts.load('current', {'packages':['gauge']});\n";
  message += "      google.charts.setOnLoadCallback(drawChart);\n";
  message += "      function drawChart() {\n";
  message += "\n";
  message += "        var data = google.visualization.arrayToDataTable([\n";
  message += "          ['Label', 'Value'],\n";
  message += "          ['PPM', ";
  message += String(ppm);
  message += "],\n";
  message += "        ]);\n";
  message += "\n";
  message += "        var options = {\n";
  message += "          width: 300, height: 300,\n";
  message += "          redFrom: 1500, redTo: 2000,\n";
  message += "          yellowFrom:1000, yellowTo: 1500,\n";
  message += "          minorTicks: 5,\n";
  message += "          max: 2000,\n";
  message += "          min: 400\n";
  message += "        };\n";
  message += "\n";
  message += "        var chart = new google.visualization.Gauge(document.getElementById('chart_div'));\n";
  message += "\n";
  message += "        chart.draw(data, options);\n";
  message += "      } \n";
  message += "    </script>\n";
  message += "  </head>\n";
  message += "\n";
  message += "  <body>\n";
  message += "<style type=\"text/css\">\n";
  message += ".tg  {border-collapse:collapse;border-spacing:0;border-color:#aaa;}\n";
  message += ".tg td{font-family:Arial, sans-serif;font-size:14px;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:#aaa;color:#333;background-color:#fff;}\n";
  message += ".tg th{font-family:Arial, sans-serif;font-size:14px;font-weight:normal;padding:10px 5px;border-style:solid;border-width:1px;overflow:hidden;word-break:normal;border-color:#aaa;color:#fff;background-color:#f38630;}\n";
  message += ".tg .tg-bzci{font-size:20px;text-align:center;vertical-align:top}\n";
  message += "</style>\n";
  message += "\n";
  message += " <table class=\"column\">\n";
  message += "    <tr>\n";
  message += "    <td><div id=\"chart_div\"></div></td>\n";
  message += "\t</tr>\n";
  message += " </table>\n";
  message += "\n";
  message += " <table class=\"tg\">\n";
  message += "  <tr>\n";
  message += "    <th class=\"tg-bzci\">CO<sub>2</sub> Level<br>ppm</th>\n";
  message += "    <th class=\"tg-bzci\">Temperature<br>&deg;C</th>\n";
  message += "    <th class=\"tg-bzci\">Humidity<br>%</th>\n";
  message += "  </tr>\n";
  message += "  <tr>\n";
  message += "    <td class=\"tg-bzci\">";
  message += String(ppm);
  message += "</td>\n";
  message += "    <td class=\"tg-bzci\">";
  message += String(temperature_web);
  message += "</td>\n";
  message += "    <td class=\"tg-bzci\">";
  message += String(humidity_web);
  message += "</td>\n";
  message += "  </tr>\n";
  message += " </table>\n";
  message += " \n";

  if (String(WiFi.localIP()) == "0") {
    message += "<p/>";
    message += "<form action=\"wifi_connect\" method=\"get\">\n";
    message += "SSID:<input type=\"text\" name=\"ssid\">\n";
    message += "<p/>Password:<input type=\"text\" name=\"pass\">\n";
    message += "<br><br><input type=\"submit\" value=\"Connect\">\n";
    message += "</form>\n";
  }

  message += "  </body>\n";
  message += " </html>";


  server.send ( 200, "text/html", message );
}



void handle_charts() {

  k = 0;
  t = timeIndex;
  printCO2Measurements = "";
  while (t < 23) {
    t++;
    if (co2[t] != "") {
      printCO2Measurements += "[" + String(k) + "," + co2[t] + "]";
      printCO2Measurements += ",";
      k++;
    }
  }
  t = 0;
  while (t <= timeIndex) {
    if (co2[t] != "") {
      printCO2Measurements += "[" + String(k) + "," + co2[t] + "]";
      printCO2Measurements += ",";
      k++;
    }
    t++;
  }
  // index = i
  String message = "<html>\n";
  message += "  <head>\n";
  message += "    <!--Load the AJAX API-->\n";
  message += "    <script type=\"text/javascript\" src=\"https://www.gstatic.com/charts/loader.js\"></script>\n";
  message += "    <script type=\"text/javascript\">\n";
  message += "\n";
  message += "      google.charts.load('current', {packages: ['corechart', 'line']});\n";
  message += "      google.charts.setOnLoadCallback(drawChartTemp);\n";
  message += "\n";
  message += "    function drawChartTemp() {\n";
  message += "\n";
  message += "      var data = new google.visualization.DataTable();\n";
  message += "      data.addColumn('number', 'Time');\n";
  message += "      data.addColumn('number', 'CO2 in ppm');\n";
  message += "\n";
  message += "      data.addRows([\n";
  message += "        " + printCO2Measurements + "\n";
  message += "\n";
  message += "      ]);\n";
  message += "\n";
  message += "      var options = {\n";
  message += "        chart: {\n";
  message += "          title: 'Temperature',\n";
  message += "          subtitle: 'in C'\n";
  message += "        },\n";
  message += "        width: 900,\n";
  message += "        pointSize : 5,\n";
  message += "        height: 500\n";
  message += "      };\n";
  message += "\n";
  message += "      var chart = new google.visualization.LineChart(document.getElementById('linechart_material'));\n";
  message += "\n";
  message += "      chart.draw(data, options);\n";
  message += "    }\n";
  message += "    </script>\n";
  message += "  </head>\n";
  message += "\n";
  message += "  <body>\n";
  message += "\n";
  message += "    <div id=\"linechart_material\"></div>\n";
  message += "  </body>\n";
  message += " </html>\n";

  server.send(200, "text/html", message);

}



void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}


void wifi_connect() {
  int arg1len = server.arg(0).length() + 1;
  int arg2len = server.arg(1).length() + 1;

  char char_array[arg1len];
  char char_array1[arg2len];

  server.arg(0).toCharArray(char_array, arg1len);
  server.arg(1).toCharArray(char_array1, arg2len);
  if (String(WiFi.localIP()) == "0") {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Conn to WiFi");
    yield();
    lcd.setCursor(0, 1);
    lcd.print("ssid:");
    yield();
    lcd.setCursor(0, 2);
    lcd.print(server.arg(0));
    yield();
    lcd.setCursor(0, 3);
    lcd.print("pass:");
    yield();
    lcd.setCursor(0, 4);
    lcd.print(server.arg(1));
    yield();

    WiFi.begin ( char_array, char_array1 );
  }

  String message = WiFi.localIP().toString();
  lcd.setCursor(0, 5);
  delay(20000);
  lcd.print(WiFi.localIP().toString());
  server.send ( 200, "text/plain", message );
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
//  WiFi.begin ( ssid2, password2 );
//  while ( WiFi.status() != WL_CONNECTED ) {
//    delay ( 500 );
//    lcd.print( "." );
//    if (retry == 40) {
//      break;
//    }
//    retry ++;
//  }
  Blynk.begin(auth, ssid2, password2);
  yield();
  checkCO2_level();

  if (WiFi.status() != WL_CONNECTED) {
    retry = 0;
    lcd.clear();
    yield();
    lcd.print("Conn to WiFi:");
    yield();
    lcd.setCursor(0, 1);
    lcd.print("SSID: " + String(ssid));
    lcd.setCursor(0, 2);
    yield();
//    WiFi.begin ( ssid, password );
//    while ( WiFi.status() != WL_CONNECTED ) {
//      delay ( 500 );
//      lcd.print( "." );
//      if (retry == 40) {
//        break;
//      }
//      retry ++;
//    }
    Blynk.begin(auth, ssid, password);
  }

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.softAP(ssid1, password);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("NO WiFi");
    yield();
    lcd.setCursor(0, 1);
    lcd.print(WiFi.softAPIP().toString());
    yield();
    lcd.setCursor(0, 2);
    lcd.print("ssid:");
    yield();
    lcd.setCursor(0, 3);
    lcd.print("ESP_CO2");
    yield();
    lcd.setCursor(0, 4);
    lcd.print("pass:");
    yield();
    lcd.setCursor(0, 5);
    lcd.print("lobster1234");


    delay (30000);

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

  server.on("/wifi_connect", wifi_connect);
  server.on ( "/", handleRoot );
  server.onNotFound ( handleNotFound );
  server.on("/CO2", handle_charts);
  yield();
  server.begin();

}


void setup ( void ) {
  lcd.clear();
  Serial.begin(9600);
  mySerial.begin(9600);
  delay(100);

  startWIFI();

  tickOccured = true;
  printToDisplay = true;
  user_init();

}

void loop ( void ) {
  if (WiFi.status() != WL_CONNECTED) {
    startWIFI();
  }
  server.handleClient();
  Blynk.run();


  if (tickOccured == true)
  {

    GMTTime = getTime();
    yield();
    if (GMTTime != "No internet connection.") {
      if ( GMTTime.substring(17, 19).toInt() > 21 ) {
        timeIndex = GMTTime.substring(17, 19).toInt() - 22;
      } else {
        timeIndex = GMTTime.substring(17, 19).toInt() + 2;
      }
    } else {

      if (timeIndex < 23) {
        timeIndex++;
      }
      else {
        timeIndex = 0;
      }
    }

    ppm = checkCO2_level();
    yield();
    co2[timeIndex] = ppm;
    tickOccured = false;
  }

  if (printToDisplay == true) {

    printCO2LCD();
    printToDisplay = false;
  }


  yield();
}

