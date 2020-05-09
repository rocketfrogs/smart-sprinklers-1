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
Adafruit_MQTT_Publish stat_channel1 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel1");
Adafruit_MQTT_Publish stat_channel2 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel2");
Adafruit_MQTT_Publish stat_channel3 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel3");
Adafruit_MQTT_Publish stat_channel4 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel4");
Adafruit_MQTT_Publish stat_channel5 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel5");
Adafruit_MQTT_Publish stat_channel6 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel6");
Adafruit_MQTT_Publish stat_channel7 = Adafruit_MQTT_Publish(&mqtt, DEVICE_NAME "/stat/channel7");

// Setup a feeds for subscribing to changes.
Adafruit_MQTT_Subscribe cmnd_channel1 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel1");
Adafruit_MQTT_Subscribe cmnd_channel2 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel2");
Adafruit_MQTT_Subscribe cmnd_channel3 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel3");
Adafruit_MQTT_Subscribe cmnd_channel4 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel4");
Adafruit_MQTT_Subscribe cmnd_channel5 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel5");
Adafruit_MQTT_Subscribe cmnd_channel6 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel6");
Adafruit_MQTT_Subscribe cmnd_channel7 = Adafruit_MQTT_Subscribe(&mqtt, DEVICE_NAME "/cmnd/channel7");

static void set_channel_1(uint32_t enable);
static void set_channel_2(uint32_t enable);
static void set_channel_3(uint32_t enable);
static void set_channel_4(uint32_t enable);
static void set_channel_5(uint32_t enable);
static void set_channel_6(uint32_t enable);
static void set_channel_7(uint32_t enable);
static void set_channel_8(uint32_t enable);

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

  cmnd_channel1.setCallback(set_channel_1);
  cmnd_channel2.setCallback(set_channel_2);
  cmnd_channel3.setCallback(set_channel_3);
  cmnd_channel4.setCallback(set_channel_4);
  cmnd_channel5.setCallback(set_channel_5);
  cmnd_channel6.setCallback(set_channel_6);
  cmnd_channel7.setCallback(set_channel_7);
  
  // Setup MQTT subscription feeds.
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

#define PIN_CHANNEL1_SRC 0
#define PIN_CHANNEL2_SRC 0
#define PIN_CHANNEL3_SRC 0
#define PIN_CHANNEL4_SRC 0
#define PIN_CHANNEL5_SRC 0
#define PIN_CHANNEL6_SRC 0
#define PIN_CHANNEL7_SRC 0

#define PIN_CHANNEL1_ENABLE 0
#define PIN_CHANNEL2_ENABLE 0
#define PIN_CHANNEL3_ENABLE 0
#define PIN_CHANNEL4_ENABLE 0
#define PIN_CHANNEL5_ENABLE 0
#define PIN_CHANNEL6_ENABLE 0
#define PIN_CHANNEL7_ENABLE 0

  // OUTPUTS
  pinMode(PIN_CHANNEL1_SRC, OUTPUT);
  pinMode(PIN_CHANNEL2_SRC, OUTPUT);
  pinMode(PIN_CHANNEL3_SRC, OUTPUT);
  pinMode(PIN_CHANNEL4_SRC, OUTPUT);
  pinMode(PIN_CHANNEL5_SRC, OUTPUT);
  pinMode(PIN_CHANNEL6_SRC, OUTPUT);
  pinMode(PIN_CHANNEL7_SRC, OUTPUT);

  pinMode(PIN_CHANNEL1_ENABLE, OUTPUT);
  pinMode(PIN_CHANNEL2_ENABLE, OUTPUT);
  pinMode(PIN_CHANNEL3_ENABLE, OUTPUT);
  pinMode(PIN_CHANNEL4_ENABLE, OUTPUT);
  pinMode(PIN_CHANNEL5_ENABLE, OUTPUT);
  pinMode(PIN_CHANNEL6_ENABLE, OUTPUT);
  pinMode(PIN_CHANNEL7_ENABLE, OUTPUT);
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
  mqtt.processPackets(10);
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

#ifdef ENABLE_SERIAL
  Serial.print("Connecting to MQTT... ");
#endif

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
#ifdef ENABLE_SERIAL
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
#endif
    mqtt.disconnect();
    delay(250);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
#ifdef ENABLE_SERIAL
  Serial.println("MQTT Connected!");
#endif
}



static void set_channel_1(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL1_SRC, LOW);
    digitalWrite(PIN_CHANNEL1_ENABLE, LOW);
    stat_channel1.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL1_SRC, LOW);
    digitalWrite(PIN_CHANNEL1_ENABLE, LOW);
    stat_channel1.publish(1);
  } else {
    // Original programmer
    digitalWrite(PIN_CHANNEL1_SRC, HIGH);
    digitalWrite(PIN_CHANNEL1_ENABLE, LOW);
    stat_channel1.publish(-1);
  }
}

static void set_channel_2(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL2_SRC, LOW);
    digitalWrite(PIN_CHANNEL2_ENABLE, LOW);
    stat_channel2.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL2_SRC, LOW);
    digitalWrite(PIN_CHANNEL2_ENABLE, LOW);
    stat_channel2.publish(1);
  } else {
    // Original programmer
    digitalWrite(PIN_CHANNEL2_SRC, HIGH);
    digitalWrite(PIN_CHANNEL2_ENABLE, LOW);
    stat_channel2.publish(-1);
  }
}

static void set_channel_3(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL3_SRC, LOW);
    digitalWrite(PIN_CHANNEL3_ENABLE, LOW);
    stat_channel3.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL3_SRC, LOW);
    digitalWrite(PIN_CHANNEL3_ENABLE, LOW);
    stat_channel3.publish(1);
  } else {
    // Original programmer
    digitalWrite(PIN_CHANNEL3_SRC, HIGH);
    digitalWrite(PIN_CHANNEL3_ENABLE, LOW);
    stat_channel3.publish(-1);
  }
}

static void set_channel_4(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL4_SRC, LOW);
    digitalWrite(PIN_CHANNEL4_ENABLE, LOW);
    stat_channel4.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL4_SRC, LOW);
    digitalWrite(PIN_CHANNEL4_ENABLE, LOW);
    stat_channel4.publish(1);
  } else {
    // Original programmer
    digitalWrite(PIN_CHANNEL4_SRC, HIGH);
    digitalWrite(PIN_CHANNEL4_ENABLE, LOW);
    stat_channel4.publish(-1);
  }
}

static void set_channel_5(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL5_SRC, LOW);
    digitalWrite(PIN_CHANNEL5_ENABLE, LOW);
    stat_channel5.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL5_SRC, LOW);
    digitalWrite(PIN_CHANNEL5_ENABLE, LOW);
    stat_channel5.publish(1);
  } else {
    // Original programme
    digitalWrite(PIN_CHANNEL5_SRC, HIGH);
    digitalWrite(PIN_CHANNEL5_ENABLE, LOW);
    stat_channel5.publish(-1);
  }
}

static void set_channel_6(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL6_SRC, LOW);
    digitalWrite(PIN_CHANNEL6_ENABLE, LOW);
    stat_channel6.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL6_SRC, LOW);
    digitalWrite(PIN_CHANNEL6_ENABLE, LOW);
    stat_channel6.publish(1);
  } else {
    // Original programmer
    digitalWrite(PIN_CHANNEL6_SRC, HIGH);
    digitalWrite(PIN_CHANNEL6_ENABLE, LOW);
    stat_channel6.publish(-1);
  }
}

static void set_channel_7(uint32_t enable) {
  if (enable == 0) {
    // OFF
    digitalWrite(PIN_CHANNEL7_SRC, LOW);
    digitalWrite(PIN_CHANNEL7_ENABLE, LOW);
    stat_channel7.publish(0);
  } else if (enable == 1) {
    // ON
    digitalWrite(PIN_CHANNEL7_SRC, LOW);
    digitalWrite(PIN_CHANNEL7_ENABLE, LOW);
    stat_channel7.publish(1);
  } else {
    // Original programmer
    digitalWrite(PIN_CHANNEL7_SRC, HIGH);
    digitalWrite(PIN_CHANNEL7_ENABLE, LOW);
    stat_channel7.publish(-1);
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
