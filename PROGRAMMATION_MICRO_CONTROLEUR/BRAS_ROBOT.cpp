// ======= DECLARATION DES BIBLIOTHEQUES ========
#include <Arduino.h>            // Bibliothèque Arduino
#include <WiFi.h>                // Connexion WiFi
#include <WiFiClientSecure.h>    // Connexion sécurisée TLS
#include <PubSubClient.h>        // Client MQTT
#include <ESP32Servo.h>          // Gestion des servomoteurs

// === Définition des broches ===
#define SERVO_1 2       // Servo base du bras
#define SERVO_2 12       // Servo axe du bras
#define SERVO_3 5       // Servo pince du bras
#define LED_R 25        // LED Rouge
#define LED_G 26        // LED Verte
#define LED_B 27        // LED Bleue
#define BUZZER_PIN 13   // Buzzer
#define IR_SENSOR_PIN 34 // Capteur infrarouge

// ========= Objets Servo ============
Servo servo_pince;     // Servo pince
Servo servo_base;      // Servo base
Servo servo_axe;       // Servo axe


// =========== Paramètres WiFi et MQTT =========
const char* ssid       = "Infinix SMART 7 HD";
const char* password   = "5M2m6jiov@n!";
const char* mqtt_server = "192.168.96.197";
const int   mqtt_port   = 8883;

// =========== Certificat TLS du broker MQTT ===========
const char* root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIEITCCAwmgAwIBAgIUXRKsAYmPhzLY3ArDozySSjgzmZYwDQYJKoZIhvcNAQEL\n" \
"BQAwgZ8xCzAJBgNVBAYTAkNNMQ8wDQYDVQQIDAZEb3VhbGExETAPBgNVBAcMCExp\n" \
"dHRvcmFsMRMwEQYDVQQKDApicmFzX3JvYm90MRMwEQYDVQQLDApicmFzX3JvYm90\n" \
"MRcwFQYDVQQDDA4xOTIuMTY4Ljk2LjE5NzEpMCcGCSqGSIb3DQEJARYaamlvdmFu\n" \
"aXRjaGFuZ2FuZ0BnbWFpbC5jb20wHhcNMjYwNjA1MTgyMjQ4WhcNMzYwNjAyMTgy\n" \
"MjQ4WjCBnzELMAkGA1UEBhMCQ00xDzANBgNVBAgMBkRvdWFsYTERMA8GA1UEBwwI\n" \
"TGl0dG9yYWwxEzARBgNVBAoMCmJyYXNfcm9ib3QxEzARBgNVBAsMCmJyYXNfcm9i\n" \
"b3QxFzAVBgNVBAMMDjE5Mi4xNjguOTYuMTk3MSkwJwYJKoZIhvcNAQkBFhpqaW92\n" \
"YW5pdGNoYW5nYW5nQGdtYWlsLmNvbTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCC\n" \
"AQoCggEBAKtDqogpJf9/zet8/fVkf1vlQ++40JR8YoBEPFZOaut2aHYrPZ2PiJmE\n" \
"ICxIY0wmiq33jFT96mkbTyBlENQIH/ZUVhgHdEhqNwpB4pNx0MzEGn2VH2BsIP6p\n" \
"q4B673sfK8ZEEpxwX0o6eoQqiJ9dV/PwFWoGpq2HnOJNybpdEkNHsg+YqBEUi8zw\n" \
"klhqJVCdAeU/MD8SCjvzrivoSHu5S5MGOd8XDY9RwIP7sDrN1GtWWPZW26Y9juxC\n" \
"jAe+pKObfRrAQRIerfJO8aCBauB7FegBI6H7yeDipPhicr20dpXx/50jT+nUAVy+\n" \
"rkm5yVPd37SiyGLuDYH7mbMboK/pYNcCAwEAAaNTMFEwHQYDVR0OBBYEFGQI/+yq\n" \
"IZ+aY4WES6O53/3RhFm0MB8GA1UdIwQYMBaAFGQI/+yqIZ+aY4WES6O53/3RhFm0\n" \
"MA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQELBQADggEBAI/C/QKmnDQn86fJ\n" \
"ow3V3QSsfEv5Xap4XH5YoqvzCKbFXXPJUsCC1aM46jsZHCdzVOcQJb4CLf+93BQD\n" \
"hOVEwEfg5pl0AaPQHJ8Gr62xiITOF1P66Ix+wplY/q+0Hc7ThAnKVt7/FHlrzwwa\n" \
"3NfnTinXWcNIlDi80d38MCpaHpWI5fKZ6onf9axgQCHDkd/EeoqyIDTFEDjMkZsu\n" \
"tBAeP3bXO1n1D7nPDk6EanZebtzW0UxhF7lNP+dHcEMTuZDk6NQFU+rp4JKtAo2h\n" \
"0juThvzS3m6bf+Xo0JfL3j93bP1XoySAYbgE5/tkI1ufwklHBjjuOa3tFqKLLdzA\n" \
"bd/Bdb4=\n" \
"-----END CERTIFICATE-----\n";

// =========== Objets WiFi et MQTT ===========
WiFiClientSecure espClient;
PubSubClient client(espClient);

// ========= Variables d'état =========
bool urgenceActive = false;
bool stopActive    = false;

// ============================
//   FONCTIONS LED & BUZZER
// ============================
void controle_LED(bool r, bool g, bool b) {
  digitalWrite(LED_R, r);
  digitalWrite(LED_G, g);
  digitalWrite(LED_B, b);
}

void Start() {
  servo_base.write(90);
  servo_axe.write(65);
  servo_pince.write(45);
  controle_LED(LOW, HIGH, LOW);
  tone(BUZZER_PIN, 2000, 400);
}

void Stop() {
  controle_LED(HIGH, LOW, LOW);
  tone(BUZZER_PIN, 2000, 400);
}

void Urgence() {
  controle_LED(HIGH, LOW, LOW);
  tone(BUZZER_PIN, 3000, 1000);
}

// ============================
//     FONCTIONS SERVOS
// ============================
void descente_bras() {
  if (urgenceActive || stopActive) return;

  controle_LED(LOW, LOW, HIGH);
  for (int pos = 65; pos <= 120; pos++) {
    servo_axe.write(pos);
    delay(1);
  }
}

void monter_bras() {
  if (urgenceActive || stopActive) return;

  controle_LED(LOW, LOW, HIGH);
  for (int pos = 120; pos >= 65; pos--) {
    servo_axe.write(pos);
    delay(1);
  }
 
}

void gauche() {
  if (urgenceActive || stopActive) return;

  controle_LED(LOW, LOW, HIGH);
  for (int pos = 90; pos >= 0; pos--) {
    servo_base.write(pos);
    delay(1);
  }
  
}

void droite() {
  if (urgenceActive || stopActive) return;

  controle_LED(LOW, LOW, HIGH);
  for (int pos = 90; pos <= 180; pos++) {
    servo_base.write(pos);
    delay(1);
  }

}

void ouverture() {
  if (urgenceActive || stopActive) return;

  controle_LED(LOW, LOW, HIGH);
  for (int pos = 0; pos <= 45; pos++) {
    servo_pince.write(pos);
    delay(1);
  }

}

void fermeture() {
  if (urgenceActive || stopActive) return;

  controle_LED(LOW, LOW, HIGH);
  for (int pos = 45; pos >= 0; pos--) {
    servo_pince.write(pos);
    delay(1);
  }
  
}

// ============================
//   CALLBACK MQTT (réception)
// ============================
void callback(char* topic, byte* payload, unsigned int length) {
  String cmd = "";
  for (unsigned int i = 0; i < length; i++) {
    cmd += (char)payload[i];
  }
  Serial.println("Commande reçue : " + cmd);

  if      (cmd == "START")    { urgenceActive = false; stopActive = false; Start(); }
  else if (cmd == "STOP")     { stopActive = true;  Stop();        }
  else if (cmd == "URGENCE")  { urgenceActive = true; Urgence();   }
  else if (cmd == "GAUCHE")   gauche();
  else if (cmd == "DROITE")   droite();
  else if (cmd == "MONTER")   monter_bras();
  else if (cmd == "DESCENDRE") descente_bras();
  else if (cmd == "ATTRAPER") fermeture();
  else if (cmd == "RELACHER") ouverture();
}

// ============================
//      CONNEXION WiFi
// ============================
void connect_wifi() {
  Serial.print("Connexion au réseau WiFi : ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int tentatives = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tentatives++;
    if (tentatives > 40) {
      Serial.println("\nEchec de connexion WiFi !");
      return;
    }
  }
  Serial.println("\nWiFi connecté !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

// ============================
//          SETUP
// ============================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Démarrage du système ===");

  // Initialisation servos
  servo_base.attach(SERVO_1);
  servo_axe.attach(SERVO_2);
  servo_pince.attach(SERVO_3);

  // Initialisation broches
  pinMode(LED_R,        OUTPUT);
  pinMode(LED_G,        OUTPUT);
  pinMode(LED_B,        OUTPUT);
  pinMode(BUZZER_PIN,   OUTPUT);
  pinMode(IR_SENSOR_PIN, INPUT);

  // Connexion WiFi et MQTT
  connect_wifi();

  espClient.setCACert(root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // Première connexion MQTT
  Serial.println("Connexion au broker MQTT...");
  if (client.connect("ESP32Client" )) {
    Serial.println("Broker MQTT connecté !");
    client.subscribe("esp32/robot/cmd");
  } else {
    Serial.print("Echec MQTT, code : ");
    Serial.println(client.state());
  }
}

// ============================
//      BOUCLE PRINCIPALE
// ============================
void loop() {

  // --- Reconnexion MQTT  ---
  if (!client.connected()) {
    static unsigned long lastAttempt = 0;
    if (millis() - lastAttempt > 3000) {
      lastAttempt = millis();
      Serial.println("Tentative reconnexion MQTT...");
      if (client.connect("ESP32Client")) {
        Serial.println("MQTT reconnecté !");
        client.subscribe("esp32/robot/cmd");
      } else {
        Serial.print("Echec MQTT, code : ");
        Serial.println(client.state());
      }
    }
    return;
  }

  client.loop();

  // --- Capteur IR : détection obstacle ---
  int Valeur_ir = analogRead(IR_SENSOR_PIN);
  if (Valeur_ir < 2000) {
    client.publish("esp32/robot/state", "Obstacle détecté - arrêt");
    controle_LED(true, false, false);
    tone(BUZZER_PIN, 3000, 500);
  }
}