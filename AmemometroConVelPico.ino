#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C 16x2

const int sensorPin = A2;
float voltaje = 0;
float velocidadViento = 0;
float velocidadPico = 0; // Valor máximo registrado

const float FACTOR_CALIBRADO = 350.0; // Ajustá según tu sensor

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
}

void loop() {
  int lectura = analogRead(sensorPin);
  voltaje = lectura * (5.0 / 1023.0);
  velocidadViento = voltaje * FACTOR_CALIBRADO;

  // Actualizar el valor pico solo si se supera
  if (velocidadViento > velocidadPico) {
    velocidadPico = velocidadViento;
  }

  // Mostrar velocidad actual
  lcd.setCursor(0, 0);
  lcd.print("Viento: ");
  lcd.print(velocidadViento, 1);
  lcd.print(" km/h ");

  // Mostrar valor pico
  lcd.setCursor(0, 1);
  lcd.print("Pico: ");
  lcd.print(velocidadPico, 1);
  lcd.print(" km/h ");

  delay(500);
}
