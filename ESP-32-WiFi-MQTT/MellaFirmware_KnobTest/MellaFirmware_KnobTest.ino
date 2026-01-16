#define KNOB_PIN 33

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // Full 0–3.3V range
}

void loop() {
  int raw = analogRead(KNOB_PIN);
  Serial.print("Raw ADC: ");
  Serial.println(raw);
  delay(500);
}
