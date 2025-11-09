#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // LCD I2C 16x2

const int sensorPin = A2;
float voltaje = 0;
float velocidadViento = 0;

const float FACTOR_CALIBRADO = 350.0; // Ajustá según tu sensor

// Variables para promedio
unsigned long tiempoInicio = 0;
unsigned long intervaloPromedio = 300000; // 5 minutos en milisegundos
float sumaVelocidades = 0;
int cantidadLecturas = 0;
float velocidadPromedio = 0;

void setup() {
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  tiempoInicio = millis(); // Marca el inicio del conteo
}

void loop() {
  int lectura = analogRead(sensorPin);
  voltaje = lectura * (5.0 / 1023.0);
  velocidadViento = voltaje * FACTOR_CALIBRADO;

  // Acumular para promedio
  sumaVelocidades += velocidadViento;
  cantidadLecturas++;

  // Mostrar velocidad actual
  lcd.setCursor(0, 0);
  lcd.print("Viento: ");
  lcd.print(velocidadViento, 1);
  lcd.print(" km/h ");

  // Verificar si pasaron 5 minutos
  if (millis() - tiempoInicio >= intervaloPromedio) {
    velocidadPromedio = sumaVelocidades / cantidadLecturas;

    // Mostrar promedio
    lcd.setCursor(0, 1);
    lcd.print("Prom: ");
    lcd.print(velocidadPromedio, 1);
    lcd.print(" km/h ");

    // Reiniciar variables
    tiempoInicio = millis();
    sumaVelocidades = 0;
    cantidadLecturas = 0;
  }

  delay(500);
}
