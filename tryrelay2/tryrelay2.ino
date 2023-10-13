#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>

WiFiEventHandler stationConnectedHandler;
WiFiEventHandler stationDisconnectedHandler;
WiFiEventHandler stationGotIpHandler;

// Relay Variables

#define RELAY1 4
#define RELAY2 14

#define LED_PIN 5

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
const unsigned int outPort = 13996;   // remote port of the target device where the NodeMCU sends OSC to
IPAddress broadcast(0, 0, 0, 0);

/*******************
    SETUP
 *******************/

void setup() {

  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println(" ");
  Serial.println("*** Try Relay Init ***");

  Serial.println("__ Init Wifi");
  stationConnectedHandler = WiFi.onStationModeConnected(&WiFiConnect);
  stationDisconnectedHandler = WiFi.onStationModeDisconnected(&WiFiDisconnect);
  stationGotIpHandler = WiFi.onStationModeGotIP(&WiFiGotIP);
  
  WiFi.setHostname("tryrelay2");
  WiFi.mode(WIFI_STA);
  //WiFi.disconnect();

  if (!connected && !scanning) {
    scan();
  }

  delay(1000);

  Serial.println("__ Init Relays");
  pinMode(RELAY1, OUTPUT);
  pinMode(RELAY2, OUTPUT);

  digitalWrite(RELAY1, LOW);
  digitalWrite(RELAY2, LOW);

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

void WiFiConnect(const WiFiEventStationModeConnected& evt) {
  Serial.println("WiFi connected");
}

void WiFiGotIP(const WiFiEventStationModeGotIP &evt) {
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

void WiFiDisconnect(const WiFiEventStationModeDisconnected& evt) {
  Serial.println("WiFi disconnected");
  connected = false;
  //WiFi.disconnect();
}

void connect() {

  WiFi.setHostname("tryrelay2");
  WiFi.begin(cur_ssid, cur_pass);

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
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
  sendIP();
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("*** Tryrelay Ready ***");
}

int scan() {
  scanning = true;
  Serial.println("Wifi scan started");

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("Wifi scan ended");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
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
  OSCMessage msg("/tryrelay2/ip");
  msg.add(WiFi.localIP()[0]);
  msg.add(WiFi.localIP()[1]);
  msg.add(WiFi.localIP()[2]);
  msg.add(WiFi.localIP()[3]);
  Udp.beginPacket(broadcast, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
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
}

void oscRelay1(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    digitalWrite(RELAY1, HIGH);
  } else {
    digitalWrite(RELAY1, LOW);
  }
}

void oscRelay2(OSCMessage &msg) {
  int v = msg.getInt(0);
  if (v > 0) {
    digitalWrite(RELAY2, HIGH);
  } else {
    digitalWrite(RELAY2, LOW);
  }
}

void oscTest(OSCMessage &msg) {
  OSCMessage msg2("/tryrelay2/ready");
  msg2.add(1);
  Udp.beginPacket(broadcast, outPort);
  msg2.send(Udp);
  Udp.endPacket();
  msg2.empty();
}

void receiveOsc() {
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }

    if (!msg.hasError()) {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println(msg.getAddress());
      msg.dispatch("/tryrelay2/test", oscTest);
      msg.dispatch("/tryrelay2/feed", oscFeed);
      msg.dispatch("/tryrelay2/relay1", oscRelay1);
      msg.dispatch("/tryrelay2/relay2", oscRelay2);
      digitalWrite(LED_BUILTIN, HIGH);

    } else {
      OSCErrorCode error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}
