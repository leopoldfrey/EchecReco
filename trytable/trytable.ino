

#include <AccelStepper.h>
#include <WiFi.h>
#include <OSCBundle.h>
#include <Adafruit_NeoPixel.h>

/*** NEOPIXEL ***/

#define LED_PIN 0
#define BRIGHTNESS 10

Adafruit_NeoPixel pix(1, LED_PIN, NEO_GRB);

uint32_t color_red = pix.Color(255, 0, 0);
uint32_t color_orange = pix.Color(255, 46, 0);
uint32_t color_yellow = pix.Color(255, 194, 0);
uint32_t color_green = pix.Color(0, 255, 0);
uint32_t color_turquoise = pix.Color(0, 255, 255);
uint32_t color_blue = pix.Color(0, 0, 255);
uint32_t color_pink = pix.Color(255, 0, 255);
uint32_t color_white = pix.Color(255, 255, 255);

// WIFI Variables

const int RSSI_MAX = -25;   // define maximum strength of signal in dBm
const int RSSI_MIN = -100;  // define minimum strength of signal in dBm

char *ssid[30] = { "TryAgain", "artdesbruits", "wifi2", "wifi3", "wifi4", "wifi5" };
char *pass[30] = { "3nc0r3_r4t3!", "0A1B2C3D4E", "---", "---", "---", "---" };

bool connected = false;
bool scanning = false;
char *cur_ssid = "none";
char *cur_pass = "none";

WiFiUDP Udp;                          // A UDP instance to let us send and receive packets over UDP
const unsigned int localPort = 8000;  // local port to listen for UDP packets at the NodeMCU (another device must send OSC messages to this port)
const unsigned int outPort = 14000;   // remote port of the target device where the NodeMCU sends OSC to
IPAddress broadcast(0, 0, 0, 0);

/**** ACCELSTEPPER ****/

#define STEP_PIN A0  // BLANC SUR PULL+
#define DIR_PIN A1   // GRIS SUR DIR+
//GND NOIR SUR PULL- / DIR-

// Define the Pins used
AccelStepper stepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);  // (1=external driver, pinStep, pinDir)

/*******************
    SETUP
 *******************/


uint32_t Freq = 0;

void setup() {

  Serial.begin(115200);
  Serial.println("   ");
  Serial.println("   ");
  Serial.println("   ");
  Serial.println("   ");
  Serial.println("   ");
  Freq = getCpuFrequencyMhz();
  Serial.print("CPU Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  Freq = getXtalFrequencyMhz();
  Serial.print("XTAL Freq = ");
  Serial.print(Freq);
  Serial.println(" MHz");
  Freq = getApbFrequency();
  Serial.print("APB Freq = ");
  Serial.print(Freq);
  Serial.println(" Hz");

  Serial.println(" ");
  Serial.println("*** WifiDada Init ***");

  Serial.println("__ Init NeoPixel");
  pinMode(NEOPIXEL_I2C_POWER, OUTPUT);
  digitalWrite(NEOPIXEL_I2C_POWER, HIGH);

  pix.begin();
  pix.setPixelColor(0, color_red);
  pix.setBrightness(BRIGHTNESS);
  pix.show();

  Serial.println("__ Init Wifi");
  WiFi.onEvent(WiFiEvent);
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_GOT_IP);
  WiFi.onEvent(WiFiDisconnect, WiFiEvent_t::ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

  WiFi.setHostname("trytable");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.println("__ Init Stepper");
  stepper.setMinPulseWidth(10);


  if (!connected && !scanning) {
    scan();
  }

  delay(1000);
}

/*******************
    LOOP
 *******************/

void loop() {
  /*if (!connected && !scanning) {
    scan();
  }*/
  stepper.run();

  receiveOsc();

  //sendPosition();
}

/*******************
    WIFI Functions
 *******************/

void WiFiEvent(WiFiEvent_t event) {
  //Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case ARDUINO_EVENT_WIFI_READY:
      Serial.println("WiFi interface ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      Serial.println("WiFi client started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      //Serial.println("Connected to access point");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      //Serial.println("Disconnected from WiFi access point");
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      //Serial.print("Obtained IP address: ");
      //Serial.println(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      //Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("WiFi access point started");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      Serial.println("AP IPv6 is preferred");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      Serial.println("STA IPv6 is preferred");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP6:
      Serial.println("Ethernet IPv6 is preferred");
      break;
    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default: break;
  }
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  //Serial.println("WiFi connected");
  //Serial.println("IP address: ");
  //Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
  Serial.println(" ");
  Serial.print("Connected, IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(" port: ");
  Serial.println(localPort);

  Serial.print("Hostname: ");
  Serial.println(WiFi.getHostname());
  broadcast = WiFi.localIP();
  broadcast[3] = 255;
  Serial.print("Broadcast : ");
  Serial.print(broadcast);
  Serial.print(" port: ");
  Serial.println(outPort);
}

void WiFiDisconnect(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi disconnected");
  connected = false;
  WiFi.disconnect();
}

void connect() {

  WiFi.setHostname("trytable");
  WiFi.begin(cur_ssid, cur_pass);
  pix.setPixelColor(0, color_turquoise);
  pix.show();

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    pix.setPixelColor(0, color_blue);
    pix.show();
    delay(50);
    pix.setPixelColor(0, color_turquoise);
    pix.show();
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected to ");
  Serial.println(cur_ssid);

  Udp.begin(localPort);

  /*broadcast = WiFi.localIP();
    broadcast[3] = 255;
    Serial.print("Broadcast : ");
    Serial.print(broadcast);
    Serial.print(" port: ");
    Serial.println(outPort);*/

  connected = true;
  pix.setPixelColor(0, color_green);
  pix.show();
  sendIP();
  Serial.println("*** WifiDada Ready ***");
}

int scan() {
  scanning = true;
  Serial.println("Wifi scan started");
  pix.setPixelColor(0, color_orange);
  pix.show();

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("Wifi scan ended");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    pix.setPixelColor(0, color_yellow);
    pix.show();
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(") ");
      Serial.print(WiFi.SSID(i));  // SSID
      Serial.print(" ");
      Serial.print(WiFi.RSSI(i));  //Signal strength in dBm
      Serial.print("dBm (");
      Serial.print(dBmtoPercentage(WiFi.RSSI(i)));  //Signal strength in %
      Serial.println("% )");

      for (int j = 0; j < 5; j++) {
        if (WiFi.SSID(i) == ssid[j]) {
          Serial.print("Found Known SSID ");
          Serial.println(ssid[j]);
          cur_ssid = ssid[j];
          cur_pass = pass[j];
          connect();
          WiFi.scanDelete();
          scanning = false;

          return 1;
        }
      }
      delay(10);
    }
  }
  Serial.println("");

  // Wait a bit before scanning again
  delay(1000);
  WiFi.scanDelete();
  scanning = false;
  return 0;
}

int dBmtoPercentage(int dBm) {
  int quality;
  if (dBm <= RSSI_MIN) {
    quality = 0;
  } else if (dBm >= RSSI_MAX) {
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }
  return quality;
}

/*******************
    OSC Functions
 *******************/

void sendIP() {
  OSCMessage msg("/trytable/ip");
  msg.add(WiFi.localIP()[0]);
  msg.add(WiFi.localIP()[1]);
  msg.add(WiFi.localIP()[2]);
  msg.add(WiFi.localIP()[3]);
  Udp.beginPacket(broadcast, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void sendPosition() {
  //Serial.print("SEND POS : ");
  //Serial.println((int)stepper.currentPosition());
  OSCMessage msg("/trytable/pos");
  msg.add((int)stepper.currentPosition());
  Udp.beginPacket(broadcast, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void oscLed(OSCMessage &msg) {
  pix.setPixelColor(0, msg.getInt(0), msg.getInt(1), msg.getInt(2));
  pix.show();
}

void oscBright(OSCMessage &msg) {
  pix.setBrightness(msg.getInt(0));
  pix.show();
}

void oscFeed(OSCMessage &msg) {
  Serial.print("Feed IP : ");
  Serial.print(msg.getInt(0));
  Serial.print(" ");
  Serial.print(msg.getInt(1));
  Serial.print(" ");
  Serial.print(msg.getInt(2));
  Serial.print(" ");
  Serial.println(msg.getInt(3));
  broadcast[0] = msg.getInt(0);
  broadcast[1] = msg.getInt(1);
  broadcast[2] = msg.getInt(2);
  broadcast[3] = msg.getInt(3);
  /*Serial.print("Broadcast : ");
  Serial.print(broadcast[0]);
  Serial.print(" ");
  Serial.print(broadcast[1]);
  Serial.print(" ");
  Serial.print(broadcast[2]);
  Serial.print(" ");
  Serial.println(broadcast[3]);*/
}

void oscTest(OSCMessage &msg) {
  OSCMessage msg2("/trytable/ready");
  msg2.add(1);
  Udp.beginPacket(broadcast, outPort);
  msg2.send(Udp);
  Udp.endPacket();
  msg2.empty();

  pix.setPixelColor(0, color_blue);
  pix.show();
  delay(50);
  pix.setPixelColor(0, color_green);
  pix.show();
}

void oscSpeed(OSCMessage &msg) {
  stepper.setMaxSpeed(msg.getFloat(0));
}

void oscAccel(OSCMessage &msg) {
  stepper.setAcceleration(msg.getFloat(0));
}

void oscPosition(OSCMessage &msg) {
  stepper.setCurrentPosition(msg.getInt(0));
}

void oscGetPosition(OSCMessage &msg) {
  sendPosition();
}

void oscMoveTo(OSCMessage &msg) {
  stepper.moveTo(msg.getInt(0));
}

void oscStop(OSCMessage &msg) {
  stepper.stop();
}

void receiveOsc() {
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    Serial.println(msg.getAddress());

    if (!msg.hasError()) {
      msg.dispatch("/trytable/led", oscLed);
      msg.dispatch("/trytable/test", oscTest);
      msg.dispatch("/trytable/bright", oscBright);
      msg.dispatch("/trytable/feed", oscFeed);
      msg.dispatch("/trytable/speed", oscSpeed);
      msg.dispatch("/trytable/accel", oscAccel);
      msg.dispatch("/trytable/position", oscPosition);
      msg.dispatch("/trytable/getposition", oscGetPosition);
      msg.dispatch("/trytable/moveto", oscMoveTo);
      msg.dispatch("/trytable/stop", oscStop);
    } else {
      OSCErrorCode error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}
