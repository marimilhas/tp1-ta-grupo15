#include "Device.h"

// Pines
const short LED_VENT = 4;
const short LED_RIEGO = 2;
const short POTE = 13;
const short BUTTON_PIN = 33;
const short PIN_SENSOR = 14;

Device _device(128, 64, -1, PIN_SENSOR, DHT22);

// Variables globales
int humedadUmbral;
int pantallaActual = 1;
bool estadoVentilacion = false;
bool estadoRiego = false;
unsigned long ultimoTiempoRiego = 0;
bool estadoLedRiego = false;

// ✅ NUEVAS VARIABLES PARA BOTÓN DE UN SOLO CLICK
bool ultimoEstadoBoton = HIGH;
bool botonPresionado = false;
unsigned long ultimoDebounce = 0;
const unsigned long debounceDelay = 50;

void setup()
{
  Serial.begin(115200);
  pinMode(LED_VENT, OUTPUT);
  pinMode(LED_RIEGO, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  _device.begin();
  randomSeed(analogRead(0));
  humedadUmbral = random(40, 61);

  Serial.print("Umbral humedad: ");
  Serial.println(humedadUmbral);

  char buffer[64];
  sprintf(buffer, "Sistema iniciado\nUmbral: %d%%", humedadUmbral);
  _device.showDisplay(buffer);
  delay(2000);
}

void loop()
{
  // Leer sensores
  float temp = _device.readTemp();
  float hum = _device.readHum();

  if (isnan(temp) || isnan(hum))
  {
    Serial.println("Error sensor");
    return;
  }

  // Leer potenciómetro
  int potValue = analogRead(POTE);
  float tempReferencia = map(potValue, 0, 4095, 15, 35);

  // Control ventilación
  if (temp > tempReferencia)
  {
    if (!estadoVentilacion)
      Serial.println("Ventilacion ACTIVADA");
    estadoVentilacion = true;
    digitalWrite(LED_VENT, HIGH);
  }
  else
  {
    if (estadoVentilacion)
      Serial.println("Ventilacion DESACTIVADA");
    estadoVentilacion = false;
    digitalWrite(LED_VENT, LOW);
  }

  // Control riego
  if (hum < humedadUmbral)
  {
    if (!estadoRiego)
      Serial.println("Riego ACTIVADO");
    estadoRiego = true;
    if (millis() - ultimoTiempoRiego > 500)
    {
      estadoLedRiego = !estadoLedRiego;
      digitalWrite(LED_RIEGO, estadoLedRiego);
      ultimoTiempoRiego = millis();
    }
  }
  else
  {
    if (estadoRiego)
      Serial.println("Riego DESACTIVADO");
    estadoRiego = false;
    digitalWrite(LED_RIEGO, LOW);
  }

  // ✅ LÓGICA CORREGIDA PARA UN SOLO CLICK
  int lecturaActual = digitalRead(BUTTON_PIN);

  // Detectar flanco descendente (botón presionado)
  if (lecturaActual == LOW && ultimoEstadoBoton == HIGH)
  {
    // Esperar debounce para confirmar
    if ((millis() - ultimoDebounce) > debounceDelay)
    {
      botonPresionado = true;
    }
    ultimoDebounce = millis();
  }

  // Detectar flanco ascendente (botón liberado)
  if (lecturaActual == HIGH && ultimoEstadoBoton == LOW)
  {
    if ((millis() - ultimoDebounce) > debounceDelay)
    {
      // ✅ SOLO CAMBIAR CUANDO SE LIBERA EL BOTÓN
      if (botonPresionado)
      {
        pantallaActual = (pantallaActual == 1) ? 2 : 1;
        Serial.print("Cambio a Pantalla ");
        Serial.println(pantallaActual);
        botonPresionado = false; // Resetear para próximo click
      }
    }
    ultimoDebounce = millis();
  }

  ultimoEstadoBoton = lecturaActual;

  // Mostrar en OLED
  char buffer[128];
  if (pantallaActual == 1)
  {
    snprintf(buffer, sizeof(buffer),
             "== TEMPERATURA ==\nActual: %.1f C\nRef: %.1f C\nVent: %s",
             temp, tempReferencia, estadoVentilacion ? "ON" : "OFF");
  }
  else
  {
    snprintf(buffer, sizeof(buffer),
             "== HUMEDAD ==\nActual: %.1f %%\nUmbral: %d %%\nRiego: %s",
             hum, humedadUmbral, estadoRiego ? "ON" : "OFF");
  }

  _device.showDisplay(buffer);
  delay(100);
}