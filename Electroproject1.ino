/*
 * Project: Electromagnetic Field & Induction Visualizer
 * Hardware: Arduino UNO R3, KY-035 Linear Hall Sensor (49E),
 *           ~260-Turn Copper Induction Coil (2.5V Voltage Divider),
 *           8-LED Active-Low Bar Graph (220-ohm current-limiting resistors)
 */

const int HALL_PIN = A0;                       // sensor output
const int COIL_PIN = A1;                       // induction coil (2.5V biased)
const int ledPins[] = {2, 3, 4, 5, 6, 7, 8, 9}; // 8-LED bar graph 

const float VCC = 5.0;                         // Arduino operating voltage 
const float ADC_MAX = 1023.0;                  // 10-bit ADC resolution 
const float HALL_SENSITIVITY = 0.014;          // 49E Hall sensitivity (~14 mV/mT)

int hallRest = 535;                            // Auto-calibrated zero-field Hall baseline
int coilRest = 512;                            // Auto-calibrated 2.5V coil midpoint
int mode = 1;                                  // 1 is for the Hall B-Field (mT) and 2 for Faraday EMF (V)

void setup() {
  Serial.begin(115200);

  //starting with all LEDs OFF (Active-Low: HIGH = OFF, LOW = ON)
  for (int i = 0; i < 8; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], HIGH);
  }

  // Auto-calibrate, taking the first 50 samples
  long hallSum = 0, coilSum = 0;
  for (int i = 0; i < 50; i++) {
    hallSum += analogRead(HALL_PIN);
    coilSum += analogRead(COIL_PIN);
    delay(5);
  }
  hallRest = hallSum / 50;
  coilRest = coilSum / 50;

  Serial.println("=== Electromagnetic Field & Induction Visualizer ===");
  Serial.println("Type '1' for Hall Magnetic Field (mT) | Type '2' for Faraday Induction (V)");
}

void loop() {
  // Checking Serial Monitor for user mode switching 
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    if (cmd == '1') {
      mode = 1;
      Serial.println("\n[MODE 1] Hall Effect Magnetic Field (B)");
    } else if (cmd == '2') {
      mode = 2;
      Serial.println("\n[MODE 2] Faraday Induction Coil (E_ep)");
    }
  }

  int leds = 0;

  if (mode == 1) {
    // --- MODE 1: Static & Dynamic Magnetic Flux Density (Hall Effect) ---
    int raw = analogRead(HALL_PIN);
    int change = abs(raw - hallRest);

    // Map magnetic field change (15 to 310 ADC steps) LEDs
    leds = map(change, 15, 310, 0, 8);

    // Convert ADC delta to voltage and magnetic flux density in mT
    float bField_mT = ((raw - hallRest) / ADC_MAX * VCC) / HALL_SENSITIVITY;
    Serial.print("B-Field: ");
    Serial.print(bField_mT, 1);
    Serial.println(" mT");
    delay(20);

  } else {
    // --- MODE 2: Electromagnetic Induction (Faraday's & Lenz's Laws) ---
    // Sample continuously for 60 ms to capture the full magnet stroke peak
    int maxSpike = 0;
    float peakEMF = 0.0;
    unsigned long start = millis();

    while (millis() - start < 60) {
      int raw = analogRead(COIL_PIN);
      int spike = abs(raw - coilRest);
      if (spike > maxSpike) {
        maxSpike = spike;
        peakEMF = ((raw - coilRest) / ADC_MAX) * VCC;
      }
    }

    // Ignore room hum and scale 0.034V–0.11V across LEDs
    leds = (maxSpike >= 7) ? map(maxSpike, 7, 22, 1, 8) : 0;

    if (maxSpike >= 7) {
      Serial.print("Peak Induced EMF (E_ep): ");
      Serial.print(peakEMF, 3);
      Serial.println(" V");
    }
  }

  // Updated the 8-LED Active-Low bar graph
  leds = constrain(leds, 0, 8);
  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPins[i], i < leds ? LOW : HIGH);
  }

  // Hold LED flash for 150 ms in Mode 2 so that the fast induction spikes remain visible
  if (mode == 2 && leds > 0) {
    delay(150);
  }
}
