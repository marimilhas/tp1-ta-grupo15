#include "Device.h"

// Pines
const short LED_VENT = 4;      // D4 - LED ventilación (verde)
const short LED_RIEGO = 2;     // D2 - LED riego (azul)
const short POTE = 13;         // D13 - Potenciómetro
const short BUTTON_PIN = 33;   // D33 - Botón
const short PIN_SENSOR = 14;   // D14 - DHT22

// Crear dispositivo
Device _device(128, 64, -1, PIN_SENSOR, DHT22);

// Variables para el sistema
int humedadUmbral;
int pantallaActual = 1;
bool estadoVentilacion = false;
bool estadoRiego = false;
unsigned long ultimoTiempoRiego = 0;
bool estadoLedRiego = false;
bool ultimoEstadoBoton = HIGH;
unsigned long ultimoDebounce = 0;

void setup() {
  Serial.begin(115200);

  // Configurar pines
  pinMode(LED_VENT, OUTPUT);
  pinMode(LED_RIEGO, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // Iniciar dispositivo
  _device.begin();
  
  // Generar umbral aleatorio de humedad (40-60%)
  randomSeed(analogRead(0));
  humedadUmbral = random(40, 61);
  
  // Mensaje inicial
  Serial.print("Sistema iniciado. Umbral humedad: ");
  Serial.print(humedadUmbral);
  Serial.println("%");

  char buffer[64];
  sprintf(buffer, "Sistema iniciado\nUmbral: %d%%", humedadUmbral);
  _device.showDisplay(buffer);
  delay(2000);
}

void loop() {
  // Leer sensores
  float temp = _device.readTemp();
  float hum = _device.readHum();
  
  if (isnan(temp) || isnan(hum)) {
    Serial.println("Error leyendo sensor");
    return;
  }
  
  // Leer potenciómetro para temperatura referencia
  int potValue = analogRead(POTE);
  float tempReferencia = map(potValue, 0, 4095, 15, 35);
  
  // Control de ventilación
  if (temp > tempReferencia) {
    if (!estadoVentilacion) {
      Serial.println("Ventilacion ACTIVADA");
      estadoVentilacion = true;
    }
    digitalWrite(LED_VENT, HIGH);
  } else {
    if (estadoVentilacion) {
      Serial.println("Ventilacion DESACTIVADA");
      estadoVentilacion = false;
    }
    digitalWrite(LED_VENT, LOW);
  }
  
  // Control de riego intermitente
  if (hum < humedadUmbral) {
    if (!estadoRiego) {
      Serial.println("Riego ACTIVADO");
      estadoRiego = true;
    }
    if (millis() - ultimoTiempoRiego > 500) {
      estadoLedRiego = !estadoLedRiego;
      digitalWrite(LED_RIEGO, estadoLedRiego);
      ultimoTiempoRiego = millis();
    }
  } else {
    if (estadoRiego) {
      Serial.println("Riego DESACTIVADO");
      estadoRiego = false;
    }
    digitalWrite(LED_RIEGO, LOW);
  }
  
  // Leer botón para cambiar pantallas
  bool estadoBoton = digitalRead(BUTTON_PIN);
  if (estadoBoton != ultimoEstadoBoton) {
    ultimoDebounce = millis();
  }
  
  if ((millis() - ultimoDebounce) > 50) {
    if (estadoBoton == LOW) {
      pantallaActual = (pantallaActual == 1) ? 2 : 1;
      Serial.print("Cambio a Pantalla ");
      Serial.println(pantallaActual);
      delay(300);
    }
  }
  ultimoEstadoBoton = estadoBoton;
  
  // Mostrar en OLED
  char buffer[128];
  if (pantallaActual == 1) {
    snprintf(buffer, sizeof(buffer), 
             "== TEMPERATURA ==\nActual: %.1f C\nRef: %.1f C\nVent: %s",
             temp, tempReferencia, estadoVentilacion ? "ON" : "OFF");
  } else {
    snprintf(buffer, sizeof(buffer),
             "== HUMEDAD ==\nActual: %.1f %%\nUmbral: %d %%\nRiego: %s",
             hum, humedadUmbral, estadoRiego ? "ON" : "OFF");
  }
  
  _device.showDisplay(buffer);
  delay(100);
}