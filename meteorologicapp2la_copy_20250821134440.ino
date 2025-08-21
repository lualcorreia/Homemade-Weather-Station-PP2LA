#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define ONE_WIRE_BUS 8
#define SENSOR_CHAO 11

#define BUZZER_PIN 9

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

Adafruit_BMP085_Unified bmp;

float umidade;
float temperaturaBMP;
float pressao;
float temperaturaExterna;
float pontoOrvalho;
float cape;
float tendenciaPressao;

float temperaturaMax = -1000;
float temperaturaMin = 1000;
float umidadeMax = 0;
float umidadeMin = 100;
float pressaoAnterior = 0.0;
unsigned long lastUpdate = 0;
unsigned long lastPressaoTime = 0;

int screenState = 0;

const float PRESSAO_REAL = 1016.0;
const float ERRO_PRESSAO = 135.0;

const float a = 17.27;
const float b = 237.7;

void setup() {
  Wire.begin();
  dht.begin();
  lcd.init();
  lcd.backlight();
  sensors.begin();

  pinMode(SENSOR_CHAO, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  // Inicializa Serial para enviar dados à Raspberry
  Serial.begin(9600);

  // Piscar "PP2LA" na inicialização
  for (int i = 0; i < 5; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PP2LA V1.0.2");
    delay(500);
    lcd.clear();
    delay(500);
  }

  // Código Morse "PP2LA"
  String morseMessage = ".--. .--. ..--- .-.. .-";
  playMorse(morseMessage);

  // Checagem do DHT11
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Verificando DHT11");
  delay(500);
  float checkHum = dht.readHumidity();
  if (isnan(checkHum)) {
    lcd.setCursor(0, 1);
    lcd.print("Erro DHT11");
    while (1);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("DHT11 OK");
  }
  delay(1000);

  // Checagem do BMP180
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Verificando BMP180");
  if (!bmp.begin()) {
    lcd.setCursor(0, 1);
    lcd.print("Erro BMP180");
    while (1);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("BMP180 OK");
  }
  delay(1000);

  // Checagem DS18B20
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Verificando DS18B20");
  sensors.requestTemperatures();
  float t = sensors.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) {
    lcd.setCursor(0, 1);
    lcd.print("Erro DS18B20");
    while (1);
  } else {
    lcd.setCursor(0, 1);
    lcd.print("DS18B20 OK");
  }
  delay(1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("INICIANDO...");
  delay(2000);
  lcd.clear();
}

void loop() {
  umidade = dht.readHumidity();
  umidade += 38;

  if (isnan(umidade)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Erro DHT11");
    delay(2000);
    return;
  }

  sensors_event_t event;
  bmp.getEvent(&event);
  if (event.pressure) {
    pressao = event.pressure;
  }

  pressao += ERRO_PRESSAO;
  bmp.getTemperature(&temperaturaBMP);

  sensors.requestTemperatures();
  temperaturaExterna = sensors.getTempCByIndex(0);

  float alpha = (a * temperaturaExterna) / (b + temperaturaExterna) + log(umidade / 100.0);
  pontoOrvalho = (b * alpha) / (a - alpha);

  cape = calcularCAPE(temperaturaExterna, pontoOrvalho);

  unsigned long currentMillis = millis();
  if (currentMillis - lastPressaoTime >= 5000) {
    tendenciaPressao = (pressao - pressaoAnterior) / 5.0;
    pressaoAnterior = pressao;
    lastPressaoTime = currentMillis;
  }

  if (temperaturaExterna > temperaturaMax) temperaturaMax = temperaturaExterna;
  if (temperaturaExterna < temperaturaMin) temperaturaMin = temperaturaExterna;

  if (umidade > umidadeMax) umidadeMax = umidade;
  if (umidade < umidadeMin) umidadeMin = umidade;

  if (pressao < 1000.0) {
    playMorse("... --- ...");
  }

  if (abs(temperaturaExterna - pontoOrvalho) < 1.0) {
    buzinarTresVezes();
  }

  if (currentMillis - lastUpdate >= 1000) {  
    lastUpdate = currentMillis;
    screenState = (screenState + 1) % 13;
  }

  lcd.clear();
  switch (screenState) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Umidade:");
      lcd.setCursor(0, 1);
      lcd.print(umidade, 1);
      lcd.print(" %");
      break;

    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Temp Interna:");
      lcd.setCursor(0, 1);
      lcd.print(temperaturaBMP, 1);
      lcd.print(" C");
      break;

    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Pressao:");
      lcd.setCursor(0, 1);
      lcd.print(pressao, 1);
      lcd.print(" hPa");
      break;

    case 3:
      lcd.setCursor(0, 0);
      lcd.print("P. Orvalho:");
      lcd.setCursor(0, 1);
      lcd.print(pontoOrvalho, 1);
      lcd.print(" C");
      break;

    case 4:
      lcd.setCursor(0, 0);
      lcd.print("Temp Externa:");
      lcd.setCursor(0, 1);
      lcd.print(temperaturaExterna, 1);
      lcd.print(" C");
      break;

    case 5:
      lcd.setCursor(0, 0);
      lcd.print("Sens Termica:");
      lcd.setCursor(0, 1);
      lcd.print(calcularSensacao(temperaturaExterna, umidade), 1);
      lcd.print(" C");
      break;

    case 6:
      lcd.setCursor(0, 0);
      lcd.print("Umid Especifica:");
      lcd.setCursor(0, 1);
      lcd.print(calcularUmidadeEspecifica(temperaturaExterna, umidade), 1);
      lcd.print(" g/m3");
      break;

    case 7:
      lcd.setCursor(0, 0);
      lcd.print("Temp Maxima:");
      lcd.setCursor(0, 1);
      lcd.print(temperaturaMax, 1);
      lcd.print(" C");
      break;

    case 8:
      lcd.setCursor(0, 0);
      lcd.print("Temp Minima:");
      lcd.setCursor(0, 1);
      lcd.print(temperaturaMin, 1);
      lcd.print(" C");
      break;

    case 9:
      lcd.setCursor(0, 0);
      lcd.print("Umidade Max:");
      lcd.setCursor(0, 1);
      lcd.print(umidadeMax, 1);
      lcd.print(" %");
      break;

    case 10:
      lcd.setCursor(0, 0);
      lcd.print("Umidade Min:");
      lcd.setCursor(0, 1);
      lcd.print(umidadeMin, 1);
      lcd.print(" %");
      break;

    case 11:
      lcd.setCursor(0, 0);
      lcd.print("CAPE:");
      lcd.setCursor(0, 1);
      lcd.print(cape, 1);
      lcd.print(" J/kg");
      break;

    case 12:
      lcd.setCursor(0, 0);
      lcd.print("Tend Pressao:");
      lcd.setCursor(0, 1);
      lcd.print(tendenciaPressao, 2);
      lcd.print(" hPa/s");
      break;
  }

  // ---- ENVIO PARA A RASPBERRY VIA SERIAL ----
  Serial.print(temperaturaBMP);      Serial.print(",");  // Temp interna
  Serial.print(temperaturaExterna);  Serial.print(",");  // Temp externa
  Serial.print(pressao);             Serial.print(",");  // Pressão
  Serial.print(umidade);             Serial.print(",");  // Umidade
  Serial.print(pontoOrvalho);        Serial.print(",");  // Ponto de orvalho
  Serial.print(cape);                Serial.print(",");  // CAPE
  Serial.println(tendenciaPressao);  // Tendência de pressão

  delay(100);
}

void playMorse(String message) {
  for (int i = 0; i < message.length(); i++) {
    char c = message.charAt(i);

    if (c == '.') {
      tone(BUZZER_PIN, 1000);
      delay(50);
      noTone(BUZZER_PIN);
      delay(50);
    } 
    else if (c == '-') {
      tone(BUZZER_PIN, 1000);
      delay(150);
      noTone(BUZZER_PIN);
      delay(50);
    } 
    else if (c == ' ') {
      delay(150);
    }
  }
}

void buzinarTresVezes() {
  for (int i = 0; i < 3; i++) {
    tone(BUZZER_PIN, 1000);
    delay(100);
    noTone(BUZZER_PIN);
    delay(100);
  }
}

float calcularSensacao(float temp, float umid) {
  if (temp >= 27.0 && umid >= 40.0) {
    return temp + 0.33 * (umid / 100.0) * 6.105 * exp(17.27 * temp / (237.7 + temp)) - 4.0;
  } else {
    return temp;
  }
}

float calcularUmidadeEspecifica(float temp, float umid) {
  float es = 6.112 * exp((17.67 * temp) / (temp + 243.5));
  float e = (umid / 100.0) * es;
  float q = 622 * (e / (1013.25 - e)); // g/kg
  return q * 1.225; // g/m³
}

float calcularCAPE(float temp, float pontoOrvalho) {
  float g = 9.8;
  return 2.0 * g * (temp - pontoOrvalho);
}
