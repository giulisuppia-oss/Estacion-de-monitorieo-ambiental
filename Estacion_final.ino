#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>

// --- Definición de pines y tipo de sensor ---
#define DHTPIN 2           // Pin donde está conectado el DHT11
#define DHTTYPE DHT11      // Tipo de sensor de temperatura y humedad
#define BUTTON_PIN 7       // Botón (de momento no se usa en el programa)
#define LDR_PIN A0         // Sensor de luz (LDR)
#define ANEMO_PIN A2       // Sensor de velocidad del viento (anemómetro)
#define SD_CS 10           // Pin CS del módulo micro SD

// --- Creación de objetos para usar las librerías ---
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS3231 rtc;
File dataFile;

// --- Variables para calculo del viento ---
float voltaje = 0;
float velocidadViento = 0;
const float FACTOR_CALIBRADO = 350.0; // Ajustar según el anemómetro usado

// --- Variables para sacar el promedio cada 5 minutos ---
unsigned long tiempoInicio = 0;
const unsigned long intervaloPromedio = 300000; // 5 minutos en milisegundos
float sumaVelocidades = 0;
int cantidadLecturas = 0;
float velocidadPromedio = 0;

// --- Control para guardar en SD cada 30 minutos ---
int ultimoMinutoGuardado = -1;

void setup() {
  Serial.begin(9600);

  // Inicialización de sensores y pantalla LCD
  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // --- Inicio de la tarjeta SD ---
  if (!SD.begin(SD_CS)) {
    lcd.setCursor(0, 0);
    lcd.print("SD fallo!");
  } else {
    lcd.setCursor(0, 0);
    lcd.print("SD OK");

    // Si el archivo no existe, se crea con encabezado
    if (!SD.exists("datos.csv")) {
      dataFile = SD.open("datos.csv", FILE_WRITE);
      if (dataFile) {
        dataFile.println("Fecha,Hora,Temp,Hum,Luz,VientoProm");
        dataFile.close();
      }
    }
  }

  // --- Inicio del RTC (reloj tiempo real) ---
  if (!rtc.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("RTC no encontrado");
    while (1); // Si falla, detiene el programa
  }

  // Si el RTC se quedó sin batería, lo ajusta con la hora de la PC
  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inicia el contador para el promedio de viento
  tiempoInicio = millis();
}

void loop() {

  // --- Lectura de sensores ---
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int luz = analogRead(LDR_PIN);

  // --- Lectura del anemómetro ---
  int lectura = analogRead(ANEMO_PIN);
  voltaje = lectura * (5.0 / 1023.0);
  velocidadViento = voltaje * FACTOR_CALIBRADO;

  // Se acumulan valores para el promedio
  sumaVelocidades += velocidadViento;
  cantidadLecturas++;

  // --- Si pasaron 5 minutos, calcula el promedio ---
  if (millis() - tiempoInicio >= intervaloPromedio) {
    velocidadPromedio = sumaVelocidades / cantidadLecturas;

    // Reinicia contadores
    tiempoInicio = millis();
    sumaVelocidades = 0;
    cantidadLecturas = 0;
  }

  // --- Obtener fecha y hora del RTC ---
  DateTime now = rtc.now();
  char fecha[11];
  char hora[9];
  sprintf(fecha, "%02d/%02d/%04d", now.day(), now.month(), now.year());
  sprintf(hora, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // --- Mostrar los datos en la pantalla LCD ---
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temp); lcd.print("C H:"); lcd.print(hum); lcd.print("%");

  lcd.setCursor(0, 1);
  lcd.print("Luz:"); lcd.print(luz);

  lcd.setCursor(0, 2);
  lcd.print("VProm: "); lcd.print(velocidadPromedio, 1); lcd.print(" km/h");

  lcd.setCursor(0, 3);
  lcd.print(fecha); lcd.print(" "); lcd.print(hora);

  // --- Guardado de datos en la SD cada 30 minutos ---
  if (now.minute() % 30 == 0 && now.minute() != ultimoMinutoGuardado) {
    dataFile = SD.open("datos.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print(fecha); dataFile.print(",");
      dataFile.print(hora); dataFile.print(",");
      dataFile.print(temp); dataFile.print(",");
      dataFile.print(hum); dataFile.print(",");
      dataFile.print(luz); dataFile.print(",");
      dataFile.println(velocidadPromedio, 1);
      dataFile.close();

      ultimoMinutoGuardado = now.minute(); // Guarda el minuto para no repetir
    }
  }

  delay(1000); // Espera 1 segundo antes del próximo ciclo
}
