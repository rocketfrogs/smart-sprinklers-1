/////////////////////////////////////////////////////////////////////////
//////////////////////// functions declarations /////////////////////////
/////////////////////////////////////////////////////////////////////////

/************************* WiFi Access Point *********************************/

#include <WiFi.h>        // Include the Wi-Fi library
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "credentials.h"        // Include Credentials (you need to create that file in the same folder if you cloned it from git)

#define DEVICE_NAME "smart-sprinkler-1"

#define PIN_SOURCE   25
#define PIN_CHANNEL1 26
#define PIN_CHANNEL2 27
#define PIN_CHANNEL3 14
#define PIN_CHANNEL4 13
#define PIN_CHANNEL5 21
#define PIN_CHANNEL6 22
#define PIN_CHANNEL7 23

#define SPRINKLER_ON_LEVEL HIGH
#define SPRINKLER_OFF_LEVEL LOW

#define SOURCE_ESP_LEVEL LOW
#define SOURCE_RAINBIRD_LEVEL HIGH

/*
  Content of "credentials.h" that matters for this section

  // WIFI Credentials

  #define WIFI_SSID        "[REPLACE BY YOUR WIFI SSID (2G)]"     // The SSID (name) of the Wi-Fi network you want to connect to
  #define WIFI_PASSWORD    "[REPLACE BY YOUR WIFI PASSWORD]"      // The password of the Wi-Fi

  // MQTT Credentials

  Content of "credentials.h" that matters for this section

  #define AIO_SERVER      "[REPLACE BY YOUR MQTT SERVER IP ADDRESS OR ITS FQDN]"
  #define AIO_SERVERPORT  [REPLACE BY THE PORT NUMBER USED FOR THE MQTT SERVICE ON YOUR MQTT SERVEUR (DEFAULT IS 1883)]       // use 8883 for SSL"
  #define AIO_USERNAME    ""  // USE THIS IF YOU HAVE USERNAME AND PASSWORD ENABLED ON YOUR MQTT SERVER
  #define AIO_KEY         ""  // USE THIS IF YOU HAVE USERNAME AND PASSWORD ENABLED ON YOUR MQTT SERVER
*/

const char* ssid     = WIFI_SSID;         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = WIFI_PASSWORD;     // The password of the Wi-Fi

/************************* MQTT Setup *********************************/

// Create a WiFiClient class to connect to the MQTT server.
WiFiClient client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

// Setup feeds for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish stat_source = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/source");
Adafruit_MQTT_Publish stat_channel1 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel1");
Adafruit_MQTT_Publish stat_channel2 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel2");
Adafruit_MQTT_Publish stat_channel3 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel3");
Adafruit_MQTT_Publish stat_channel4 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel4");
Adafruit_MQTT_Publish stat_channel5 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel5");
Adafruit_MQTT_Publish stat_channel6 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel6");
Adafruit_MQTT_Publish stat_channel7 = Adafruit_MQTT_Publish(&mqtt, "/" DEVICE_NAME "/stat/channel7");

// Setup a feeds for subscribing to changes.
Adafruit_MQTT_Subscribe cmnd_source = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/source");
Adafruit_MQTT_Subscribe cmnd_channel1 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel1");
Adafruit_MQTT_Subscribe cmnd_channel2 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel2");
Adafruit_MQTT_Subscribe cmnd_channel3 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel3");
Adafruit_MQTT_Subscribe cmnd_channel4 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel4");
Adafruit_MQTT_Subscribe cmnd_channel5 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel5");
Adafruit_MQTT_Subscribe cmnd_channel6 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel6");
Adafruit_MQTT_Subscribe cmnd_channel7 = Adafruit_MQTT_Subscribe(&mqtt, "/" DEVICE_NAME "/cmnd/channel7");

enum {
  SOURCE_RAINBIRD,
  SOURCE_ESP
} source;

#define CHANNEL_COUNT 7
struct {
  bool enabled; // is the channel currently enabled?
  Adafruit_MQTT_Publish *stat; // stats
  Adafruit_MQTT_Subscribe *cmnd; // command
  int gpio; // which gpio the enable signal is connected to
} channels[CHANNEL_COUNT] = {
  {false, &stat_channel1, &cmnd_channel1, PIN_CHANNEL1},
  {false, &stat_channel2, &cmnd_channel2, PIN_CHANNEL2},
  {false, &stat_channel3, &cmnd_channel3, PIN_CHANNEL3},
  {false, &stat_channel4, &cmnd_channel4, PIN_CHANNEL4},
  {false, &stat_channel5, &cmnd_channel5, PIN_CHANNEL5},
  {false, &stat_channel6, &cmnd_channel6, PIN_CHANNEL6},
  {false, &stat_channel7, &cmnd_channel7, PIN_CHANNEL7}
};

///////////////////// initialize variables  ///////////////////////////

///////////////////////////////////////////////////////
//////  ///////////////////////////////////////////////
//////  ///////////////////////////////////////////////
//////  ///////////                         ///////////
//////  ///////////                         ///////////
//\        ////////          SETUP          ///////////
///\      /////////                         ///////////
////\    //////////                         ///////////
/////\  ///////////////////////////////////////////////
///////////////////////////////////////////////////////
void setup() {

  ///////////////////////////////////////////////////////
  ///////////////////// Start Serial ////////////////////
  ///////////////////////////////////////////////////////
  Serial.begin(115200);
  Serial.println("Booting");

  ///////////////////////////////////////////////////////
  //////////////////// Start Wifi ///////////////////////
  ///////////////////////////////////////////////////////

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  ////////////////// Initialize OTA /////////////////////
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(DEVICE_NAME);

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  ///////////////////////////////////////////////////////
  ////////////// Suscribing to MQTT topics //////////////
  ///////////////////////////////////////////////////////
  
  // Setup MQTT subscription feeds.
  mqtt.subscribe(&cmnd_source);
  mqtt.subscribe(&cmnd_channel1);
  mqtt.subscribe(&cmnd_channel2);
  mqtt.subscribe(&cmnd_channel3);
  mqtt.subscribe(&cmnd_channel4);
  mqtt.subscribe(&cmnd_channel5);
  mqtt.subscribe(&cmnd_channel6);
  mqtt.subscribe(&cmnd_channel7);

  ///////////////////////////////////////////////////////
  ////////////////// Initialize PINs ////////////////////
  ///////////////////////////////////////////////////////

  // Open drain outputs (either input or low output)
  source = SOURCE_RAINBIRD;
  openDrainWrite(PIN_SOURCE, SOURCE_RAINBIRD_LEVEL);
  stat_source.publish(SOURCE_RAINBIRD);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    openDrainWrite(channels[i].gpio, SPRINKLER_OFF_LEVEL);
    channels[i].stat->publish(0);
  }

}

///////////////////////////////////////////////////////
///  //////////////////////////////////////////////////
///  //////////////////////////////////////////////////
///  /// ///////////                        ///////////
///  ///   /////////                        ///////////
///          ///////      END OF SETUP      ///////////
////////   /////////                        ///////////
//////// ///////////                        ///////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////



///////////////////////////////////////////////////////
//////  ///////////////////////////////////////////////
//////  ///////////////////////////////////////////////
//////  ///////////                         ///////////
//////  ///////////                         ///////////
//\        ////////           LOOP          ///////////
///\      /////////                         ///////////
////\    //////////                         ///////////
/////\  ///////////////////////////////////////////////
///////////////////////////////////////////////////////

void loop() {
  ArduinoOTA.handle();

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(250))) {
    int value = atoi((char*)subscription->lastread);
    if (subscription == &cmnd_source) {
      if (value == SOURCE_RAINBIRD) {
        source = SOURCE_RAINBIRD;
        openDrainWrite(PIN_SOURCE, SOURCE_RAINBIRD_LEVEL);
      } else if (value == SOURCE_ESP) {
        source = SOURCE_ESP;
        openDrainWrite(PIN_SOURCE, SOURCE_ESP_LEVEL);
      }
      stat_source.publish(source);
    } else {
      for (int i = 0; i < CHANNEL_COUNT; i++) {
        if (subscription == channels[i].cmnd) {
          bool enable = (value != 0);
          openDrainWrite(channels[i].gpio, enable ? SPRINKLER_ON_LEVEL : SPRINKLER_OFF_LEVEL);
          channels[i].enabled = enable;
          channels[i].stat->publish(enable);
          break;
        }
      }
    }
  }

  print_stats();
}

///////////////////////////////////////////////////////
///  //////////////////////////////////////////////////
///  //////////////////////////////////////////////////
///  /// ///////////                        ///////////
///  ///   /////////                        ///////////
///          ///////       END OF LOOP      ///////////
////////   /////////                        ///////////
//////// ///////////                        ///////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////



///////////////////////////////////////////////////////
//////  ///////////////////////////////////////////////
//////  ///////////////////////////////////////////////
//////  ///////////                         ///////////
//////  ///////////                         ///////////
//\        ////////        FUNCTIONS        ///////////
///\      /////////                         ///////////
////\    //////////                         ///////////
/////\  ///////////////////////////////////////////////
///////////////////////////////////////////////////////

void openDrainWrite(int pin, int level) {
  if (level) {
    pinMode(pin, INPUT);
  } else {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

void print_stats() {
  unsigned long now = millis();
  static unsigned long prev = 0;
  if (now - prev > 1000) {
    Serial.println("älive!");
    prev = now;
  }
}

///////////////////////////////////////////////////////
//////////////// MQTT_connect Function ////////////////
///////////////////////////////////////////////////////

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(250);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");

  stat_source.publish(source);
  for (int i = 0; i < CHANNEL_COUNT; i++) {
    channels[i].stat->publish(channels[i].enabled);
  }

}

///////////////////////////////////////////////////////
///  //////////////////////////////////////////////////
///  //////////////////////////////////////////////////
///  /// ///////////                        ///////////
///  ///   /////////                        ///////////
///          ///////    END OF FUNCTIONS    ///////////
////////   /////////                        ///////////
//////// ///////////                        ///////////
///////////////////////////////////////////////////////
///////////////////////////////////////////////////////
