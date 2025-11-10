#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SD.h>
#include <SPI.h>
#include <RTClib.h>

// ----- DEFINICIÓN DE PINES Y MODELOS -----
#define DHTPIN 2            // Pin del sensor DHT22
#define DHTTYPE DHT22       // Tipo de sensor (DHT22)
#define BUTTON_PIN 7        
#define LDR_PIN A0          // Pin analógico del sensor de luz (LDR)
#define ANEMO_PIN A2        // Pin analógico del anemómetro
#define SD_CS 10            // Pin Chip Select para el módulo SD

// ----- CREACIÓN DE OBJETOS -----
DHT dht(DHTPIN, DHTTYPE);                     // Inicializa sensor DHT
LiquidCrystal_I2C lcd(0x27, 20, 4);           // Configura LCD 20x4 con dirección I2C 0x27
RTC_DS3231 rtc;                               // Crea objeto para el RTC
File dataFile;                                // Objeto para manejo de archivos en SD

// ----- VARIABLES GENERALES -----
float voltaje = 0;                 // Guardará el voltaje leído del anemómetro
float velocidadViento = 0;         // Velocidad instantánea calculada
const float FACTOR_CALIBRADO = 350.0; // Factor de calibración del sensor de viento

// ----- VARIABLES PARA PROMEDIO DE VIENTO -----
unsigned long tiempoInicio = 0;
const unsigned long intervaloPromedio = 120000; // 2 minutos
float sumaVelocidades = 0;
int cantidadLecturas = 0;
float velocidadPromedio = 0;

// ----- VARIABLES PARA GUARDADO EN SD -----
int ultimoMinutoGuardado = -1; // Control para no repetir guardado en el mismo minuto

// ----- CONTROL DE DETECCIÓN DE SD -----
bool sdDisponible = false;

// ----- CONTROL DE ACTUALIZACIÓN DE LCD -----
unsigned long ultimaActualizacionLCD = 0;
const unsigned long intervaloLCD = 1000; // 1 segundo

// ----- CONTROL DE ON/OFF DEL BACKLIGHT DEL LCD -----
unsigned long tiempoCambioLCD = 0;
const unsigned long intervaloLCDToggle = 180000; // 3 minutos
bool lcdEncendido = true;

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ----- INICIALIZACIÓN DE LA TARJETA SD -----
  sdDisponible = false;
  lcd.setCursor(0, 0);
  lcd.print("Iniciando SD...");
  
  // Intenta inicializar la SD hasta 5 veces
  for (int i = 0; i < 5; i++) {
    if (SD.begin(SD_CS)) {
      sdDisponible = true;
      break;
    }
    delay(200);
  }

  lcd.clear();
  if (sdDisponible) {
    lcd.print("SD OK");
    Serial.println("Tarjeta SD detectada.");
    
    // Si no existe el archivo CSV lo crea con los encabezados
    if (!SD.exists("datos.csv")) {
      dataFile = SD.open("datos.csv", FILE_WRITE);
      if (dataFile) {
        dataFile.println("Fecha;Hora;Temp;Hum;Luz;VientoProm");
        dataFile.close();
      }
    }
  } else {
    lcd.print("SD no detectada");
    Serial.println("No se pudo inicializar la SD después de varios intentos.");
  }

  // ----- INICIALIZACIÓN DEL RTC -----
  if (!rtc.begin()) { // Si no encuentra el RTC
    lcd.setCursor(0, 1);
    lcd.print("RTC no encontrado");
    while (1); // Se detiene el programa
  }

  if (rtc.lostPower()) { 
    // Si el RTC perdió energía, ajusta la hora a la hora de compilación
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Inicializa contadores de tiempo
  tiempoInicio = millis();
  tiempoCambioLCD = millis();
  delay(2000);
  lcd.clear();
}

void loop() {
  // ----- DETECCIÓN SI LA SD SIGUE INSERTADA -----
  bool sdEstadoActual = SD.begin(SD_CS);
  if (sdEstadoActual != sdDisponible) {
    sdDisponible = sdEstadoActual;
    lcd.setCursor(0, 0);
    lcd.print("                    "); // Borra la línea
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

  // ----- LECTURA DE SENSORES -----
  float temp = dht.readTemperature(); // Temperatura
  float hum = dht.readHumidity();     // Humedad
  int luz = analogRead(LDR_PIN);      // Luz ambiente (LDR)

  // ----- LECTURA Y CÁLCULO DEL ANEMÓMETRO -----
  int lectura = analogRead(ANEMO_PIN);
  voltaje = lectura * (5.0 / 1023.0);
  velocidadViento = voltaje * FACTOR_CALIBRADO;

  // Acumula valores para promedio
  sumaVelocidades += velocidadViento;
  cantidadLecturas++;

  // ----- CÁLCULO DE PROMEDIO CADA 2 MINUTOS -----
  if (millis() - tiempoInicio >= intervaloPromedio) {
    velocidadPromedio = sumaVelocidades / cantidadLecturas;
    tiempoInicio = millis();
    sumaVelocidades = 0;
    cantidadLecturas = 0;
    Serial.println("Promedio de viento actualizado");
  }

  // ----- OBTENER FECHA Y HORA DESDE RTC -----
  DateTime now = rtc.now();
  char fecha[11];
  char hora[9];
  sprintf(fecha, "%02d/%02d/%04d", now.day(), now.month(), now.year());
  sprintf(hora, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());

  // ----- CONTROL DE BACKLIGHT SEGÚN HORARIO -----
  bool dentroHorario = (now.hour() >= 7 && now.hour() < 22); // Entre 07:00 y 22:00
  if (dentroHorario) {
    if (millis() - tiempoCambioLCD >= intervaloLCDToggle) {
      lcdEncendido = !lcdEncendido;
      tiempoCambioLCD = millis();
      if (lcdEncendido) {
        lcd.backlight();
        Serial.println("LCD encendido");
      } else {
        lcd.noBacklight();
        Serial.println("LCD apagado");
      }
    }
  } else {
    lcdEncendido = false;
    lcd.noBacklight();
  }

  // ----- MOSTRAR DATOS EN LCD CADA 1 SEGUNDO -----
  if (millis() - ultimaActualizacionLCD >= intervaloLCD) {
    ultimaActualizacionLCD = millis();

    lcd.setCursor(0, 0);
    lcd.print("T:"); lcd.print(temp, 1); lcd.print("C H:"); lcd.print(hum, 0); lcd.print("%   ");

    lcd.setCursor(0, 1);
    lcd.print("Luz:"); lcd.print(luz); lcd.print("     ");

    lcd.setCursor(0, 2);
    lcd.print("VProm: "); lcd.print(velocidadPromedio, 1); lcd.print(" km/h   ");

    lcd.setCursor(0, 3);
    lcd.print(fecha); lcd.print(" "); lcd.print(hora);
  }

  // ----- GUARDADO DE DATOS EN SD CADA 30 MINUTOS -----
  if (sdDisponible && now.minute() % 30 == 0 && now.minute() != ultimoMinutoGuardado) {
    dataFile = SD.open("datos.csv", FILE_WRITE);
    if (dataFile) {
      dataFile.print(fecha); dataFile.print(";");
      dataFile.print(hora); dataFile.print(";");
      dataFile.print(temp, 1); dataFile.print(";");
      dataFile.print(hum, 1); dataFile.print(";");
      dataFile.print(luz); dataFile.print(";");
      dataFile.println(velocidadPromedio, 1);
      dataFile.close();

      ultimoMinutoGuardado = now.minute();
      Serial.println("Datos guardados en SD.");
    } else {
      Serial.println("Error al abrir datos.csv");
    }
  }

  delay(200); // Pequeña pausa para estabilidad
}
