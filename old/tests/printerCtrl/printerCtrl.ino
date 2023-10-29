int encoderCLK = D7;
int encoderDT = D6;
int encoderSW = D5;

int motorPWM = D2;
int motorDIR = D4;

volatile int lastEncoded = 0;
volatile long encoderValue = 0;

long lastencoderValue = 0;

bool running = false;

int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;


void setup() {
  Serial.begin (9600);
  Serial.println();
  Serial.println("Motor Starting...");

  pinMode(encoderCLK, INPUT);
  pinMode(encoderDT, INPUT);
  pinMode(encoderSW, INPUT);

  pinMode(motorPWM, OUTPUT);
  pinMode(motorDIR, OUTPUT);
  //digitalWrite(motorDIR, HIGH);

  digitalWrite(encoderCLK, HIGH); //turn pullup resistor on
  digitalWrite(encoderDT, HIGH); //turn pullup resistor on
  digitalWrite(encoderSW, HIGH); //turn pullup resistor on

  //call updateEncoder() when any high/low changed seen
  //on interrupt 0 (pin 2), or interrupt 1 (pin 3)
  attachInterrupt(encoderCLK, updateEncoder, CHANGE);
  attachInterrupt(encoderDT, updateEncoder, CHANGE);

  /*digitalWrite(motorPWM, 255);
  delay(1000);
  analogWrite(motorPWM, 0);*/
  Serial.println("Motor Ready");


}

void loop() {

  if (lastencoderValue != encoderValue) {
    Serial.print("Speed ");
    Serial.println(encoderValue);
    analogWrite(motorPWM, encoderValue);
    lastencoderValue = encoderValue;
  }


  int reading = digitalRead(encoderSW);
  //Serial.print("BUTTON DOWN");
  //Serial.println(reading);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        Serial.println("BUTTON DOWN");
        if (running) {
          Serial.println("Stop");
          running = false;
          analogWrite(motorPWM, 0);
        } else {
          Serial.println("Start");
          running = true;
          analogWrite(motorPWM, lastencoderValue);
        }
      }
    }
  }

  lastButtonState = reading;

  //delay(50);
}


ICACHE_RAM_ATTR void updateEncoder() {
  int MSB = digitalRead(encoderCLK); //MSB = most significant bit
  int LSB = digitalRead(encoderDT); //LSB = least significant bit

  int encoded = (MSB << 1) | LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value

  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;

  encoderValue = constrain(encoderValue, 0, 255);
  lastEncoded = encoded; //store this value for next time
}
