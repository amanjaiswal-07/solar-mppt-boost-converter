#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 4);

#define INPUT_VOLTAGE_SENSOR A0
#define INPUT_CURRENT_SENSOR A1
#define OUTPUT_VOLTAGE_SENSOR A2
#define OUTPUT_CURRENT_SENSOR A3

float R1 = 30000.0;
float R2 = 7500.0;

float inputCurrentZero = 512.0;
float outputCurrentZero = 512.0;

int duty = 400;
float prevPower = 0;
bool increaseDuty = true;

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  // Calibration
  float inTotal = 0.0;
  for (int i = 0; i < 200; i++) {
    inTotal += analogRead(INPUT_CURRENT_SENSOR);
    delay(3);
  }
  inputCurrentZero = inTotal / 200.0;

  float outTotal = 0.0;
  for (int i = 0; i < 200; i++) {
    outTotal += analogRead(OUTPUT_CURRENT_SENSOR);
    delay(3);
  }
  outputCurrentZero = outTotal / 200.0;

  // PWM at 20kHz on Pin 9
  pinMode(9, OUTPUT);
  TCCR1A = 0;
  TCCR1B = 0;
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13) | (1 << WGM12) | (1 << CS10);
  ICR1 = 799;
  OCR1A = duty;

  Serial.println("Vin,Iin,Power,Duty,Vout,Iout"); // CSV header for Serial Plotter
}

void loop() {
  // Input Voltage
  int vInRaw = analogRead(INPUT_VOLTAGE_SENSOR);
  float vInOUT = (vInRaw * 5.0) / 1024.0;
  float vIN = vInOUT / (R2 / (R1 + R2));

  // Output Voltage
  int vOutRaw = analogRead(OUTPUT_VOLTAGE_SENSOR);
  float vOutOUT = (vOutRaw * 5.0) / 1024.0;
  float vOUT = vOutOUT / (R2 / (R1 + R2));

  // Input Current
  float inputCurrentSum = 0.0;
  for (int i = 0; i < 50; i++) {
    float reading = analogRead(INPUT_CURRENT_SENSOR);
    float voltage = (reading - inputCurrentZero) * (5.0 / 1024.0);
    float current = voltage / 0.185;
    inputCurrentSum += current;
    delay(2);
  }
  float avgInputCurrent = inputCurrentSum / 50.0;

  // Output Current
  float outputCurrentSum = 0.0;
  for (int i = 0; i < 50; i++) {
    float reading = analogRead(OUTPUT_CURRENT_SENSOR);
    float voltage = (reading - outputCurrentZero) * (5.0 / 1024.0);
    float current = voltage / 0.185;
    outputCurrentSum += current;
    delay(2);
  }
  float avgOutputCurrent = outputCurrentSum / 50.0;

  // MPPT - Perturb & Observe
  float power = vIN * avgInputCurrent;
  if (power > prevPower) {
    if (increaseDuty) duty += 1;
    else duty -= 1;
  } else {
    increaseDuty = !increaseDuty;
    if (increaseDuty) duty += 1;
    else duty -= 1;
  }
  prevPower = power;
  duty = constrain(duty, 0, 799);
  OCR1A = duty;
  // LCD Output
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IN:");
  lcd.print(vIN, 1);
  lcd.print("V ");
  lcd.print(avgInputCurrent, 1);
  lcd.print("A");
  lcd.setCursor(0, 1);
  lcd.print("OUT:");
  lcd.print(vOUT, 1);
  lcd.print("V ");
  lcd.print(avgOutputCurrent, 1);
  lcd.print("A");

  // Serial Plot Output (CSV for graph)
  Serial.print(vIN);
  Serial.print(",");
  Serial.print(avgInputCurrent);
  Serial.print(",");
  Serial.print(power);
  Serial.print(",");
  Serial.print(duty);
  Serial.print(",");
  Serial.print(vOUT);
  Serial.print(",");
  Serial.println(avgOutputCurrent);

  // Serial Output (Readable)
  Serial.print("Duty Cycle: ");
  Serial.println(duty);
  delay(500);
}
