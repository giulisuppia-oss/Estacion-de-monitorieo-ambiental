#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16 ,2);

const int potPin= A0;

int analogValue = 0;
float windSpeed = 0.0;

void setup()
{
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  lcd.setCursor (0, 0);
  lcd.print ("Anemometro");
  lcd.setCursor (0, 1);
  lcd.print("Simulador Wokwi");
  delay(2000);
  lcd.clear();

}

void loop ()
{
  analogValue = analogRead(potPin);

  windSpeed = map(analogValue, 0, 1023, 0, 150);

  lcd.setCursor(0, 0);
  lcd.print("Velocidad Viento");

  lcd.setCursor (0, 1);
  lcd.print(windSpeed);
  lcd.print (" km/h           ");

  delay(100);
}
