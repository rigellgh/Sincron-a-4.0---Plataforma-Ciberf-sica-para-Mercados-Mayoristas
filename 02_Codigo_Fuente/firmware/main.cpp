#include <Arduino.h>
#include <HX711.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Configuración de pines
#define PIN_SENSOR_DT 4
#define PIN_SENSOR_SCK 5
#define PIN_RFID_SS 21
#define PIN_RFID_RST 22

// Objetos globales
HX711 scale;
MFRC522 rfid(PIN_RFID_SS, PIN_RFID_RST);
WiFiClient espClient;
PubSubClient client(espClient);

// Variables
float peso_actual = 0.0;
String ultimo_producto = "";

void setup() {
    Serial.begin(115200);
    
    // Inicializar sensor de peso
    scale.begin(PIN_SENSOR_DT, PIN_SENSOR_SCK);
    scale.set_scale(2280.f);  // Factor de calibración
    scale.tare();
    
    // Inicializar RFID
    SPI.begin();
    rfid.PCD_Init();
    
    // Conectar WiFi
    WiFi.begin("SSID", "password");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    client.setServer("gateway.local", 1883);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
    
    // Leer peso
    if (scale.is_ready()) {
        peso_actual = scale.get_units(5);
        
        // Si hay producto RFID
        if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
            String producto = leerRFID();
            enviarDatos(peso_actual, producto);
        }
    }
    
    delay(200);
}

void enviarDatos(float peso, String producto) {
    String payload = "{\"peso\":" + String(peso) + 
                     ",\"producto\":\"" + producto + 
                     "\",\"timestamp\":" + String(millis()) + "}";
    client.publish("estacion/pesaje", payload.c_str());
    Serial.println("Datos enviados: " + payload);
}

void reconnect() {
    while (!client.connected()) {
        if (client.connect("estacion1")) {
            client.subscribe("estacion/comandos");
        } else {
            delay(5000);
        }
    }
}

String leerRFID() {
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i], HEX);
    }
    rfid.PICC_HaltA();
    return uid;
}
