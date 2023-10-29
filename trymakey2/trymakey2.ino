#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCBundle.h>
#include <movingAvg.h>

int ledPin = 5;
int sensorPin = A0;
movingAvg filter = movingAvg(20);

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
const unsigned int outPort = 13991;   // remote port of the target device where the NodeMCU sends OSC to
IPAddress broadcast(0, 0, 0, 0);
bool IPreceived = false;

/*******************
    SETUP
 *******************/

void setup() {
  Serial.begin(115200);
  Serial.println(" ");
  Serial.println("*** TryMakey2 Init ***");

  //analogReadResolution(10);
  pinMode(ledPin, OUTPUT);
  pinMode(sensorPin, INPUT);
  filter.begin();

  Serial.println("__ Init Wifi");
  WiFi.setHostname("trymakey2");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (!connected && !scanning) {
    scan();
  }
  /*cur_ssid = ssid[1];
  cur_pass = pass[1];
  connect();*/

  delay(200);
}

/*******************
    LOOP
 *******************/

void loop() {

  float currentAverage = filter.reading(analogRead(sensorPin)*4);
  //Serial.print("press: ");
  //Serial.println(currentAverage);
  sendPress(currentAverage);
  //Serial.println(currentAverage);
  if (!IPreceived)
    receiveOsc();

  delay(2);
}


/*******************
    WIFI Functions
 *******************/

void WiFiConnect(const WiFiEventStationModeConnected& evt) {
  Serial.println("WiFi connected");
}


void connect() {

  WiFi.setHostname("trymakey2");
  WiFi.mode(WIFI_STA);
  WiFi.begin(cur_ssid, cur_pass);
  digitalWrite(ledPin, HIGH);

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, LOW);
    delay(50);
    digitalWrite(ledPin, LOW);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected to ");
  Serial.println(cur_ssid);

  Udp.begin(localPort);

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

  connected = true;
  digitalWrite(ledPin, HIGH);
  sendIP();
  Serial.println("*** TryMakey2 Ready ***");
}

int scan() {
  scanning = true;
  Serial.println("Wifi scan started");
  digitalWrite(ledPin, LOW);

  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("Wifi scan ended");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    digitalWrite(ledPin, HIGH);
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
  delay(200);
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

void sendPress(float v) {
  OSCMessage msg("/trymakey2/press");
  msg.add(v);
  Udp.beginPacket(broadcast, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
}

void sendIP() {
  OSCMessage msg("/trymakey2/ip");
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
  IPreceived = true;
}

void oscTest(OSCMessage &msg) {
  OSCMessage msg2("/trymakey2/ready");
  msg2.add(1);
  Udp.beginPacket(broadcast, outPort);
  msg2.send(Udp);
  Udp.endPacket();
  msg2.empty();


  digitalWrite(ledPin, LOW);
  delay(50);
  digitalWrite(ledPin, HIGH);
}

void receiveOsc() {
  OSCMessage msg;
  int size = Udp.parsePacket();
  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }

    if (!msg.hasError()) {
      msg.dispatch("/trymakey2/test", oscTest);
      msg.dispatch("/trymakey2/feed", oscFeed);

    } else {
      OSCErrorCode error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}
