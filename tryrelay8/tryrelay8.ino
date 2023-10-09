#include <WiFi.h>
#include <OSCBundle.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include "SparkFun_Qwiic_Relay.h"

// Relay Variables
#define RELAY_ADDR 0x6C   // R1 > R4
#define RELAY_ADDR2 0x6D  // R5 > R8

Qwiic_Relay quadRelay1(RELAY_ADDR);
Qwiic_Relay quadRelay2(RELAY_ADDR2);

// Pix Variables

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
const unsigned int outPort = 13997;   // remote port of the target device where the NodeMCU sends OSC to
IPAddress broadcast(0, 0, 0, 0);

/*******************
    SETUP
 *******************/

void setup() {

  Wire.begin();

  Serial.begin(115200);
  Serial.println(" ");
  Serial.println("*** Try Relay Init ***");

  Serial.println("__ Init Relays");

  // Let's make sure the hardware is set up correctly.
  if (!quadRelay1.begin())
    Serial.println("R1>R4 : Check connections to Qwiic Relay 1.");
  else
    Serial.println("R1>R4 : Ready to flip some switches.");
  // Let's make sure the hardware is set up correctly.
  if (!quadRelay2.begin())
    Serial.println("R5>R8 : Check connections to Qwiic Relay 2.");
  else
    Serial.println("R5>R8 : Ready to flip some switches.");

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

  WiFi.setHostname("tryrelay8");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();



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

  receiveOsc();
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

  WiFi.setHostname("tryrelay8");
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
  Serial.println("*** Tryrelay Ready ***");
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
  OSCMessage msg("/tryrelay8/ip");
  msg.add(WiFi.localIP()[0]);
  msg.add(WiFi.localIP()[1]);
  msg.add(WiFi.localIP()[2]);
  msg.add(WiFi.localIP()[3]);
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

void oscRelay1(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay1.turnRelayOn(1);
  } else {
    quadRelay1.turnRelayOff(1);
  }
}

void oscRelay2(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay1.turnRelayOn(2);
  } else {
    quadRelay1.turnRelayOff(2);
  }
}

void oscRelay3(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay1.turnRelayOn(3);
  } else {
    quadRelay1.turnRelayOff(3);
  }
}

void oscRelay4(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay1.turnRelayOn(4);
  } else {
    quadRelay1.turnRelayOff(4);
  }
}

void oscRelay5(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay2.turnRelayOn(1);
  } else {
    quadRelay2.turnRelayOff(1);
  }
}

void oscRelay6(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay2.turnRelayOn(2);
  } else {
    quadRelay2.turnRelayOff(2);
  }
}

void oscRelay7(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay2.turnRelayOn(3);
  } else {
    quadRelay2.turnRelayOff(3);
  }
}

void oscRelay8(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    quadRelay2.turnRelayOn(4);
  } else {
    quadRelay2.turnRelayOff(4);
  }
}

void oscTest(OSCMessage &msg) {
  OSCMessage msg2("/tryrelay8/ready");
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

void receiveOsc() {
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }

    if (!msg.hasError()) {
      msg.dispatch("/tryrelay8/led", oscLed);
      msg.dispatch("/tryrelay8/test", oscTest);
      msg.dispatch("/tryrelay8/bright", oscBright);
      msg.dispatch("/tryrelay8/feed", oscFeed);
      msg.dispatch("/tryrelay8/relay1", oscRelay1);
      msg.dispatch("/tryrelay8/relay2", oscRelay2);
      msg.dispatch("/tryrelay8/relay3", oscRelay3);
      msg.dispatch("/tryrelay8/relay4", oscRelay4);
      msg.dispatch("/tryrelay8/relay5", oscRelay5);
      msg.dispatch("/tryrelay8/relay6", oscRelay6);
      msg.dispatch("/tryrelay8/relay7", oscRelay7);
      msg.dispatch("/tryrelay8/relay8", oscRelay8);

    } else {
      OSCErrorCode error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}



/*
void setup() {
  

  Serial.println("Let's turn each relay on, one at a time.");
  // To turn on a relay give the function the number you want to turn on (or
  // off).
  quadRelay.turnRelayOn(1);
  delay(200);
  quadRelay.turnRelayOn(2);
  delay(200);
  quadRelay.turnRelayOn(3);
  delay(200);
  quadRelay.turnRelayOn(4);
  delay(1000);

  quadRelay2.turnRelayOn(1);
  delay(200);
  quadRelay2.turnRelayOn(2);
  delay(200);
  quadRelay2.turnRelayOn(3);
  delay(200);
  quadRelay2.turnRelayOn(4);
  delay(1000);


  // You can turn off all relays at once with the following function...
  Serial.println("Now let's turn them all off.");
  quadRelay.turnAllRelaysOff();
  quadRelay2.turnAllRelaysOff();
  delay(1000);

  // Toggle is a bit different then using the on and off functions. It will
  // check the current state of the relay (on or off) and turn it to it's
  // opposite state: on ---> off or off -----> on.
  Serial.println("Toggle relay one and two.");
  quadRelay.toggleRelay(1);
  delay(200);
  quadRelay.toggleRelay(2);
  delay(1000);

  // Let's turn off relay one and two....
  Serial.println("Turn off relay one and two.");
  quadRelay.turnRelayOff(1);
  delay(200);
  quadRelay.turnRelayOff(2);
  delay(1000);

  // You can turn all the relays on at once with this function.
  Serial.println("Turn them all on once again.");
  quadRelay.turnAllRelaysOn();
  delay(1000);

  // And finally you can toggle each relay at once. It's helpful when you have
  // two relays on but need them off, and also need the other two relays on.
  Serial.println("....and turn them off again.");
  quadRelay.toggleAllRelays();

  // Finally we can see if the relays are on or off without physically looking
  // at them. Once again, give the function the number of the relay you want to
  // check.
  Serial.print("Relay One is now: ");
  // Is the relay on or off?
  int state = quadRelay.getState(1);
  if (state == 1)
    Serial.println("On!");
  else if (state == 0)
    Serial.println("Off!");
  delay(1000);

  // Turn on relay two just to get a sense of how this works.
  Serial.print("Relay two is now: ");
  quadRelay.turnRelayOn(2);
  state = quadRelay.getState(2);
  if (state == 1)
    Serial.println("On!");
  else if (state == 0)
    Serial.println("Off!");

  Serial.println("Now they're all off...");
  // Show's over people...
  quadRelay.turnAllRelaysOff();
}*/
