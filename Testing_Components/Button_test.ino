const int buttonPin = 26;
int buttonState = 0;
void setup(){
  Serial.begin(115200);
  pinMode(buttonPin,INPUT_PULLUP);
}
void loop()
{
  buttonState = digitalRead(buttonPin);
  Serial.println(buttonState);
  delay(500);
}
