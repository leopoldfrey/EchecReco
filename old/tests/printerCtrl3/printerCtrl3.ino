/*
     Board pin | NodeMCU GPIO |  Arduino IDE
          A-                    1                         5 or D1
          A+                    3                         0 or D3
          B-                    2                         4 or D2
          B+                    4                         2 or D4
*/
const int pwmMotorA = 5;
const int dirMotorA = 6;
const int pwmMotorB = 9;
const int dirMotorB = 10;
int motorSpeed = 500;
void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(pwmMotorA , OUTPUT);
  pinMode(pwmMotorB, OUTPUT);
  pinMode(dirMotorA, OUTPUT);
  pinMode(dirMotorB, OUTPUT);
  Serial.println("Motor SHield 12E Initialized");
  delay(5000);
}
void loop() {
  Serial.println("Activate A");
  digitalWrite(pwmMotorA, motorSpeed);
  digitalWrite(dirMotorA, LOW);
  delay(1500);
  Serial.println("Reverse A");
  digitalWrite(dirMotorA, HIGH);
  delay(1500);
  Serial.println("Stop A");
  digitalWrite(pwmMotorA, 0);
  //digitalWrite(dirMotorA, LOW);
  delay(3000);
  Serial.println("Activate B");
  digitalWrite(pwmMotorB, motorSpeed);
  digitalWrite(dirMotorB, LOW);
  delay(1500);
  Serial.println("Reverse B");
  digitalWrite(dirMotorB, HIGH);
  delay(1500);
  Serial.println("Stop B");
  digitalWrite(pwmMotorB, 0);
  //digitalWrite(dirMotorB, LOW);
  delay(3000);
}
