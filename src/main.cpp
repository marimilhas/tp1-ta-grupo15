#include "Device.h"

// Pines
const short LED_VENT = 4;
const short LED_RIEGO = 2;
const short POTE = 32;
const short BUTTON_PIN = 34;
const short PIN_SENSOR = 33;

Device _device(128, 64, -1, PIN_SENSOR, DHT22);

// VARIABLES GLOBALES
int humedadUmbral;
float tempReferencia; 
int pantallaActual = 1;
bool estadoVentilacion = false;
bool estadoRiego = false;
unsigned long ultimoTiempoRiego = 0;
bool estadoLedRiego = false;

// --- Variables para debounce (botón)
unsigned long ultimoCambioBoton = 0;
int estadoAnteriorBoton = HIGH;
const unsigned long debounceDelayBoton = 200;

// --- Flags de control manual
bool controlManualVent = false;
bool controlManualRiego = false;
bool usarTempManual = false;
bool usarHumManual = false;

void setup()
{
  Serial.begin(115200);
  pinMode(LED_VENT, OUTPUT);
  pinMode(LED_RIEGO, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  _device.begin();
  humedadUmbral = random(40, 61);

  Serial.println("\nSISTEMA INICIADO");
  Serial.print("Umbral humedad inicial: %");
  Serial.println(humedadUmbral);
  Serial.println("───────────────\n");

  char buffer[64];
  sprintf(buffer, "Sistema iniciado\nUmbral: %d%%", humedadUmbral);
  _device.showDisplay(buffer);
  delay(2000);
}

void loop()
{
  // LECTURA SENSOR
  float temp = _device.readTemp();
  float hum = _device.readHum();

  if (isnan(temp) || isnan(hum))
  {
    Serial.println("Error sensor");
    return;
  }

  // LECTURA POTENCIÓMETRO
  if (!usarTempManual)
  {
    int potValue = analogRead(POTE);
    tempReferencia = map(potValue, 0, 4095, 15, 35);
  }

  // CONTROL VENTILACIÓN
  if (!controlManualVent)
  {
    if (temp > tempReferencia)
    {
      if (!estadoVentilacion)
      {
        Serial.println("Ventilacion ACTIVADA");
        estadoVentilacion = true;
      }
      digitalWrite(LED_VENT, HIGH);
    }
    else
    {
      if (estadoVentilacion)
      {
        Serial.println("Ventilacion DESACTIVADA");
        estadoVentilacion = false;
      }
      digitalWrite(LED_VENT, LOW);
    }
  }

  // CONTROL RIEGO
  if (!controlManualRiego)
  {
    if (hum < humedadUmbral)
    {
      if (!estadoRiego)
      {
        Serial.println("Riego ACTIVADO");
        estadoRiego = true;
        estadoLedRiego = true;
        digitalWrite(LED_RIEGO, HIGH);
        ultimoTiempoRiego = millis();
      }

      // Parpadeo cada 500 ms
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
      {
        Serial.println("Riego DESACTIVADO");
        estadoRiego = false;
      }
      digitalWrite(LED_RIEGO, LOW);
    }
  }

  // LECTURA DEL BOTÓN
  int lecturaActual = digitalRead(BUTTON_PIN);

  if (lecturaActual == LOW && estadoAnteriorBoton == HIGH)
  {
    if (millis() - ultimoCambioBoton > debounceDelayBoton)
    {
      pantallaActual++;
      if (pantallaActual > 4)
        pantallaActual = 1; 
      ultimoCambioBoton = millis();
    }
  }

  estadoAnteriorBoton = lecturaActual;

  // LECTURA DEL PUERTO SERIE
  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("T="))
    {
      tempReferencia = input.substring(2).toFloat();
      usarTempManual = true;
      Serial.print("Temperatura de referencia fijada a ");
      Serial.println(tempReferencia);
    }
    else if (input.startsWith("H="))
    {
      humedadUmbral = input.substring(2).toInt();
      usarHumManual = true;
      Serial.print("Humedad de referencia fijada a ");
      Serial.println(humedadUmbral);
    }
    else if (input.equalsIgnoreCase("VENT ON"))
    {
      controlManualVent = true;
      digitalWrite(LED_VENT, HIGH);
      Serial.println("Ventilación forzada ON");
    }
    else if (input.equalsIgnoreCase("VENT OFF"))
    {
      controlManualVent = true;
      digitalWrite(LED_VENT, LOW);
      Serial.println("Ventilación forzada OFF");
    }
    else if (input.equalsIgnoreCase("RIEGO ON"))
    {
      controlManualRiego = true;
      digitalWrite(LED_RIEGO, HIGH);
      Serial.println("Riego forzado ON");
    }
    else if (input.equalsIgnoreCase("RIEGO OFF"))
    {
      controlManualRiego = true;
      digitalWrite(LED_RIEGO, LOW);
      Serial.println("Riego forzado OFF");
    }
    else if (input.equalsIgnoreCase("AUTO"))
    {
      controlManualVent = false;
      controlManualRiego = false;
      usarTempManual = false;
      usarHumManual = false;
      Serial.println("Modo automático restaurado");
    }
  }

  // PANTALLAS DEL OLED
  char buffer[128];
  if (pantallaActual == 1)
  {
    snprintf(buffer, sizeof(buffer),
             "== TEMPERATURA ==\nActual: %.1f C\nRef: %.1f C\nVent: %s",
             temp, tempReferencia, estadoVentilacion ? "ON" : "OFF");
  }
  else if (pantallaActual == 2)
  {
    snprintf(buffer, sizeof(buffer),
             "== HUMEDAD ==\nActual: %.1f %%\nUmbral: %d %%\nRiego: %s",
             hum, humedadUmbral, estadoRiego ? "ON" : "OFF");
  }
  else if (pantallaActual == 3)
  {
    snprintf(buffer, sizeof(buffer),
             "== ESTADO COMPLETO ==\nT: %.1f / %.1f\nH: %.1f / %d\nVent:%s Riego:%s",
             temp, tempReferencia, hum, humedadUmbral,
             estadoVentilacion ? "ON" : "OFF",
             estadoRiego ? "ON" : "OFF");
  }
  else if (pantallaActual == 4)
  {
    snprintf(buffer, sizeof(buffer),
             "== MENU MANUAL ==\nComandos Serie:\nT=n / H=n\nVENT ON/OFF\nRIEGO ON/OFF\nAUTO");
  }

  _device.showDisplay(buffer);
}
