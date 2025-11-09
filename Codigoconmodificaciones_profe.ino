#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>

// --- Definiciones de pines y configuraciones ---
#define DHTPIN 2           // Pin del sensor DHT11
#define DHTTYPE DHT11      // Tipo de sensor
#define BUTTON_PIN 7       // Botón (no está implementado en el código aún)
#define LDR_PIN A0         // Sensor de luz (fotoresistencia)
#define ANEMO_PIN A2       // Entrada analógica del anemómetro
#define SD_CS 10           // Pin de selección de chip para SD

// --- Creación de objetos para cada módulo ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS3231 rtc;
File dataFile;

// --- Variables globales para anemómetro y velocidad ---
float voltaje = 0;
float velocidadViento = 0;
const float FACTOR_CALIBRADO = 350.0; // Factor para convertir voltaje a km/h (ajustable)

// --- Variables para cálculo del promedio de viento cada 2 minutos ---
unsigned long tiempoInicio = 0;
const unsigned long intervaloPromedio = 120000; // 2 minutos
float sumaVelocidades = 0;
int cantidadLecturas = 0;
float velocidadPromedio = 0;

// --- Control de guardado en SD cada 30 minutos ---
int ultimoMinutoGuardado = -1;

// --- Control dinámico del estado de la tarjeta SD ---
bool sdDisponible = false;
unsigned long ultimaActualizacionLCD = 0;
const unsigned long intervaloLCD = 1000; // Actualiza LCD cada 1 segundo

//=================================== SETUP ===================================
void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- Inicialización de tarjeta SD ---
  sdDisponible = SD.begin(SD_CS);
  lcd.setCursor(0, 0);
  if (sdDisponible) {
    lcd.print("SD OK");
    // Si el archivo no existe, lo crea y escribe encabezado
    if (!SD.exists("datos.csv")) {
      dataFile = SD.open("datos.csv", FILE_WRITE);
      if (dataFile) {
        dataFile.println("Fecha,Hora,Temp,Hum,Luz,VientoProm");
        dataFile.close();
      }
    }
  } else {
    lcd.print("SD no detectada");
  }

  // --- Inicialización del RTC (módulo de tiempo real) ---
  if (!rtc.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("RTC no encontrado");
    while (1); // Se detiene si no hay RTC
  }

  // Si el RTC perdió alimentación, se ajusta con la hora de compilación
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  tiempoInicio = millis();
}

//=================================== LOOP ====================================
void loop() {

  // --- Detecta si la SD fue conectada o retirada en caliente ---
  bool sdEstadoActual = SD.begin(SD_CS);
  if (sdEstadoActual != sdDisponible) {
    sdDisponible = sdEstadoActual;
    lcd.setCursor(0, 0);
    lcd.print("                    ");
    lcd.setCursor(0, 0);
    if (sdDisponible) {
      lcd.print("SD insertada");
      Serial.println("Tarjeta SD insertada");
    } else {
      lcd.print("SD removida");
      Serial.println("Tarjeta SD removida");
    }
    delay(2000);
  }

  // --- Lectura de sensores ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int luz = analogRead(LDR_PIN);

  // --- Lectura y cálculo de velocidad del viento ---
  int lectura = analogRead(ANEMO_PIN);
  voltaje = lectura * (5.0 / 1023.0);
  velocidadViento = voltaje * FACTOR_CALIBRADO;

  // Se suma para promediar
  sumaVelocidades += velocidadViento;
  cantidadLecturas++;

  // --- Cada 2 minutos calcula el promedio del viento ---
  if (millis() - tiempoInicio >= intervaloPromedio) {
    velocidadPromedio = sumaVelocidades / cantidadLecturas;
    tiempoInicio = millis();
    sumaVelocidades = 0;
    cantidadLecturas = 0;
    Serial.println("Promedio de viento actualizado");
  }

  // --- Mostrar datos en LCD sin parpadeo, cada 1 segundo ---
  if (millis() - ultimaActualizacionLCD >= intervaloLCD) {
    ultimaActualizacionLCD = millis();

    DateTime now = rtc.now();
    char fecha[11];
    char hora[9];
    sprintf(fecha, "%02d/%02d/%04d", now.day(), now.month(), now.year());
    sprintf(hora, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(temp, 1); lcd.print("C H:"); lcd.print(hum, 0); lcd.print("%   ");

    lcd.setCursor(0, 1);
    lcd.print("Luz:"); lcd.print(luz); lcd.print("     ");

    lcd.setCursor(0, 2);
    lcd.print("VProm: "); lcd.print(velocidadPromedio, 1); lcd.print(" km/h   ");

    lcd.setCursor(0, 3);
    lcd.print(fecha); lcd.print(" "); lcd.print(hora);
  }

  // --- Guardado de datos en la SD cada 30 minutos ---
  DateTime now = rtc.now();
  if (sdDisponible && now.minute() % 30 == 0 && now.minute() != ultimoMinutoGuardado) {
    dataFile = SD.open("datos.csv", FILE_WRITE);
    if (dataFile) {

      dataFile.print(now.day()); dataFile.print("/");
      dataFile.print(now.month()); dataFile.print("/");
      dataFile.print(now.year()); dataFile.print(",");

      dataFile.print(now.hour()); dataFile.print(":");
      dataFile.print(now.minute()); dataFile.print(":");
      dataFile.print(now.second()); dataFile.print(",");

      dataFile.print(temp, 1); dataFile.print(",");
      dataFile.print(hum, 1); dataFile.print(",");
      dataFile.print(luz); dataFile.print(",");
      dataFile.println(velocidadPromedio, 1);
      dataFile.close();

      ultimoMinutoGuardado = now.minute();
      Serial.println("Datos guardados en SD.");
    } else {
      Serial.println("Error al abrir datos.csv");
    }
  }

  delay(200); // Estabilidad del loop
}
