#include <HCSR04.h> 
#include <TM1637.h> 
#include <WiFi.h> 
#include <HTTPClient.h> 

#define PIN_LED 5

#define STATE_TOOFAR 1
#define STATE_TOOCLOSE 2
#define STATE_PLAYING 3

#define CLK 32
#define DIO 33

#define PIN_SWITCH 27

//// Config
#define MAX_DISTANCE 50
#define MIN_DISTANCE 5
#define USE_HTTP 0
#define USE_WIFI 0
///////////

const byte triggerPin = 22;
const byte echoPin = 19;

UltraSonicDistanceSensor distanceSensor(triggerPin, echoPin);
TM1637 display(CLK, DIO);

int state = STATE_TOOCLOSE; 

void setup() {
    Serial.begin(115200);
    pinMode(PIN_LED, OUTPUT);
    
    display.init();
    display.set(BRIGHT_TYPICAL);

    pinMode(PIN_SWITCH, INPUT); 

    if (USE_WIFI == 1) {
      setupWifi();
    }
}

void loop() {
    // Schalter-Abfrage
    if (digitalRead(PIN_SWITCH) != 1) { 
      display.displayNum(0);
      digitalWrite(PIN_LED, LOW);
      delay(100);
      return;
    }
    
    float distance = distanceSensor.measureDistanceCm();
    int currentdistance = (int)distance;

    // Zustandsmaschine für die LED
    if (distance < MIN_DISTANCE) {
        state = STATE_TOOCLOSE;
    }
    else if (distance > MAX_DISTANCE || distance < 0) { 
        state = STATE_TOOFAR;
    }
    else {
        state = STATE_PLAYING;
    }

    // Display-Anzeige & HTTP-Übertragung (Strikt alle 250ms)
    if (state == STATE_PLAYING) {
        display.displayNum(currentdistance);
        if (USE_HTTP == 1) {
          sendCurrentToneHTTP(currentdistance); // Sendet den aktuellen Wert
        }
    } 
    else {
        display.displayNum(0);
        if (USE_HTTP == 1) {
          sendCurrentToneHTTP(0); // Sendet 0, wenn außerhalb des Bereichs        
        }
    }
    
    blink();
    
    // Exakt 250ms Delay = 4 Übertragungen pro Sekunde
    delay(250); 
}

void blink() {
    if (state == STATE_PLAYING) {
        digitalWrite(PIN_LED, HIGH);
    } else {
        digitalWrite(PIN_LED, LOW); 
    }
}

void setupWifi() {
  const char *ssid = "testingiot";        
  const char *password = "testingiot";   
  
  WiFi.setAutoReconnect(true);  
  WiFi.setSleep(false);         

  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password); 
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print(".");
  } 
  
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());
}

void sendCurrentToneHTTP(int toneDistance) {
    if (WiFi.status() == WL_CONNECTED) { 
        HTTPClient http;

        // WICHTIG: Kurzes Timeout (150ms), damit der ESP32 nicht blockiert,
        // falls der Server mal ein paar Millisekunden länger braucht!
        http.setTimeout(150); 

        String url = "http://192.168.0.2/echo_get.php?string=" + String(toneDistance);
        http.begin(url); 

        int httpResponseCode = http.GET();
        
        if (httpResponseCode > 0) {
            Serial.printf("HTTP Code: %d | Stream: %d\n", httpResponseCode, toneDistance);
        } else {
            Serial.printf("Timeout/Fehler: %d bei Wert: %d\n", httpResponseCode, toneDistance);
        }
        
        http.end(); 
    } else {
        Serial.println("WLAN-Verbindung verloren!");
    }
}