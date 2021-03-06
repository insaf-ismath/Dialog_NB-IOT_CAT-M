//***Update  DEVICE_MAC_ADDRESS, EVENT_TOPIC,SUB_TOPIC Network Mode with your parameters*****//

const String DEVICE_MAC_ADDRESS = "6569395562831978";   //your device's mac address
const String EVENT_TOPIC   = "ideamartirblasterv1/common";  //your EVENT url/topic as shown under your device definition
const String SUB_TOPIC   = "+/%parentMac%/ideamart/irblaster/v1/sub";  //your subscribe url/topic as shown under your device definition, replace 'parentmac' with your device MAC

//Network mode  GSM/NB-IOT
const String mode = "GSM";
       
//Dialog GPRS APN Credentials
#define DIALOG_GSM_APN   "dialogbb"
#define DIALOG_NBIOT_APN "dialogbb"
#define DIALOG_USERNAME  ""
#define DIALOG_PASSWORD  ""

// ideaBoard pin configuration
#define ideaBoard_PWRKEY 13
#define ideaBoard_RX 8
#define ideaBoard_TX 7
#define ideaBoard_RST 11

//****You don't have to change anything below this***//
#include "Dialog_NBIOT.h"
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include <ArduinoJson.h>

#define halt(s) { Serial.println(F( s )); while(1);  }

SoftwareSerial DialogSerial = SoftwareSerial(ideaBoard_TX, ideaBoard_RX);
Adafruit_FONA_LTE ideaBoard = Adafruit_FONA_LTE(); // For SIM7000

Adafruit_MQTT_FONA mqtt(&ideaBoard, "mqtt.iot.ideamart.io", 1883, DEVICE_MAC_ADDRESS.c_str(), "rabbit", "rabbit");

boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password);

/****************************** PUB/SUB ***************************************/

// Setup a feed called 'sensor' for publishing.
Dialog_MQTT_Publish sensor = Dialog_MQTT_Publish(&mqtt,EVENT_TOPIC.c_str());  
Dialog_MQTT_Subscribe command = Dialog_MQTT_Subscribe(&mqtt,SUB_TOPIC.c_str());

/*************************** Sketch Code ************************************/
// How many transmission failures in a row we're willing to be ok with before reset
uint8_t txfailures = 0;

void setup() {
  while (!Serial);
  pinMode(ideaBoard_RST, OUTPUT);
  digitalWrite(ideaBoard_RST, LOW); // Default state

  pinMode(ideaBoard_PWRKEY, OUTPUT);
  digitalWrite(ideaBoard_PWRKEY, LOW);
  delay(200);

  digitalWrite(ideaBoard_PWRKEY, HIGH);
  delay(180);
  digitalWrite(ideaBoard_PWRKEY, LOW);

  Serial.begin(115200);
  Serial.println(F("********** Dialog NB-IOT Begin **********"));

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();

  // Initialise the ideaBoard module
  if(mode=="NB-IOT"){
    while (! FONAconnect(F(DIALOG_NBIOT_APN), F(DIALOG_USERNAME), F(DIALOG_PASSWORD))) {
      Serial.println("Retrying ideaBoard");  
      
   }
  }
  else if(mode=="GSM"){
    while (! FONAconnect(F(DIALOG_GSM_APN), F(DIALOG_USERNAME), F(DIALOG_PASSWORD))) {
      Serial.println("Retrying ideaBoard");
   }
  }
  
  Serial.println(F("Connected to cellular!"));
  ideaBoard.getfullstatus();
  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
  mqtt.subscribe(&command);  // uncomment this to enable topic subscription
}

void loop() {
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();
  MQTT_connect(); //initial connect & autoreconnect
  Watchdog.reset();
 // createSendEvent();    //uncomment this to enable event publish
 // Watchdog.reset();
  waitForMessage();      //uncomment this to receive messages from subscribed topic
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care of connecting.
void MQTT_connect() {
  int8_t ret;
  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }
  Serial.println(F("Connecting to MQTT server... "));

  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
  }
  Serial.print(F("*******Connection established to mqtt.iot.ideamart.io*********"));

}

boolean FONAconnect(const __FlashStringHelper *apn, const __FlashStringHelper *username, const __FlashStringHelper *password) {
  Watchdog.reset();

  Serial.println(F("Initializing Dialog NB-IoT DevBoard....(May take 3 seconds)"));
  DialogSerial.begin(115200); // Default SIM7000 shield baud rate

  Serial.println(F("Configuring to 4800 baud"));
  DialogSerial.println("AT+IPR=4800");
  DialogSerial.begin(4800);
  if (! ideaBoard.begin(DialogSerial)) {
    Serial.println(F("Couldn't find Dialog NB-IoT DevBoard"));
    return false; // Don't proceed if it couldn't find the device
  }

  DialogSerial.println("AT+CMEE=2"); // Verbose mode to enable error explanation
  Serial.println(F("Dialog NB-IoT DevBoard detected"));
  Watchdog.reset();
  Serial.println(F("Initially setting up Dialog GSM network and checking connectivity..."));
  ideaBoard.connectDialogGsm();
  while (ideaBoard.getNetworkStatus() != 1 && ideaBoard.getNetworkStatus() != 5) {
    delay(500);  //loop if, not in registered(1) or roaming(5) mode
  }

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  
  //connect to Dialog NB-IOT
  if (mode=="NB-IOT") {
    if (ideaBoard.getMode()!= 38) {
      if (!ideaBoard.connectDialogNBIoT()) {
        delay(1000);
        Serial.println(F("Failed to configure Dialog NB-IoT"));
      }
    }
    else {
      if (ideaBoard.getNB_IoT_status() == 9) 
        Serial.println(F("Status : Dialog NB-IoT Online..."));
      else
        Serial.println(F("Status : Unable to connect Dialog NB-IoT network"));
    }
  }

  //connect to Dialog GSM
  else if (mode=="GSM") {
    if (int(ideaBoard.getMode()) != 13 ) {
      if (! ideaBoard.connectDialogGsm()) {
        Serial.println(F("Failed to configure Dialog GSM"));
      }
    }
    else {
      if (ideaBoard.getNB_IoT_status() == 0)
        Serial.println(F("Status : Dialog GSM Online..."));
      else
        Serial.println(F("Status : Unable to connect Dialog GSM network"));
    }
  }

  ideaBoard.setGPRSNetworkSettings(apn, username, password);

  Serial.println(F("Disabling GPRS...."));
  ideaBoard.enableGPRS(false);

  Watchdog.reset();
  delay(5000);  // wait a few seconds to stabilize connection
  Watchdog.reset();
  Serial.println(F("Enabling GPRS...."));
  if (!ideaBoard.enableGPRS(true)) {
    Serial.println(F("Failed to turn GPRS on"));
    return false;
  }
  Watchdog.reset();
  return true;
}

void waitForMessage(){
  Dialog_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(2000))) {
    if (subscription == &command) {
      Serial.print(F("Received: "));
      Serial.println((char *)command.lastread);
      do_actions((char *)command.lastread);
    }
  }
}

