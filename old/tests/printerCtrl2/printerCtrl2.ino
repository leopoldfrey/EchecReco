int motorPWMA = 3;
int motorDIRA = 4;
int motorPWMB = 11;
int motorDIRB = 8;

int v = 0;
int sens = 0;

void setup() {
  Serial.begin (9600);
  Serial.println();
  
  pinMode(motorPWMA, OUTPUT);
  pinMode(motorDIRA, OUTPUT);
  pinMode(motorPWMB, OUTPUT);
  pinMode(motorDIRB, OUTPUT);
  digitalWrite(motorDIRA, HIGH);
  digitalWrite(motorDIRB, HIGH);

  Serial.println("Motor Ready");


}

void loop() {
  /*if(sens == 0)
  {
    v = v + 1;
  } else {
    v = v - 1;
  }

  if(v < 0) {
    v = 0;
    sens = 0; 
  } else if(v > 255) {
    sens = 1;
  }
  Serial.println(v);*/
  analogWrite(motorPWMA, 255);
  analogWrite(motorPWMB, 255);
  delay(100);
}
