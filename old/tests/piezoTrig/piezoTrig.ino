int m = 0;
int v1 = 0;
int v2 = 0;
int v3 = 0;
int v4 = 0;
int v5 = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  //Serial.print(analogRead(A0));
  //Serial.print(" ");
  v1 = 10*analogRead(A1);
  v2 = 10*analogRead(A2);
  v3 = 10*analogRead(A3);
  v4 = 10*analogRead(A4);
  v5 = 10*analogRead(A5);
  Serial.print(v1);
  Serial.print(" ");
  Serial.print(v2);
  Serial.print(" ");
  Serial.print(v3);
  Serial.print(" ");
  Serial.print(v4);
  Serial.print(" ");
  Serial.print(v5);

  m = max(v1, v2);
  m = max(m, v3);
  m = max(m, v4);
  m = max(m, v5);

  Serial.print(" max ");
  Serial.println(m);

  /*if (m == 1023)
    Serial.println("Knock !!!");*/

  delay(10);

}
