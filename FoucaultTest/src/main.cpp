#include <Arduino.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <EEPROM.h>
#include <WiFi.h>
#include <DNSServer.h>
#include "CaptiveRequestHandler.h"
#include "esp_task_wdt.h"
#include <iostream>
#include <vector>
// #include <FreeRTOS.h>

#define FORMAT_LITTLEFS_IF_FAILED true

//------------- VARIABLE -------------\\

//-----GROS DRIVER-----\\

// #define config 2
#define cur_senseA 33
#define EndRaceA1 13
#define EndRaceA2 12
#define IN1A 26
#define IN2A 14
#define lockPin 15
#define Ouverture 2
#define Fermeture 1
#define Bouton 34
#define Reset 2

#define ACTION_A 1
#define ACTION_C 2
#define ACTION_D 3

// TaskHandle_t xHandle = NULL;
TaskHandle_t xHandleConfig = NULL;

bool stop = false;
int sizeArray = 0;
int adresseFirstConfig = 0;
bool firstConfig = false;
int startAddress = 3;
int CruiseSpeed = 244;
int EngageSpeed = 130;
int CurrentSpeed = 0;
int VitessePorte = 1;
int sens = 1;
int timeBetweenCurrentValue = 50;
bool btnDisabled = 0;

std::vector<int> delaysConfig = {0,1000};
std::vector<int> actionsConfig = {ACTION_A,ACTION_C};

// Variable pour le restart config
const unsigned long restartDelay = 3000;
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
bool restartTriggered = false;

const char *ssid = "Foucault Box";
const char *psk = NULL;

// const int Led = 27;

DNSServer dnsServer;
AsyncWebServer server(80);

//------------- FUNCTION -------------\\

bool EndRacePressed(int capteur)
{
  return digitalRead(capteur) == LOW;
}

void LockDoor()
{
  if (digitalRead(EndRaceA1) == LOW) // Detect pin optional --> Endrace deja verif
  {
    delay(500);
    digitalWrite(lockPin, LOW);
    Serial.println("Porte Fermé et Verouillé");
    return;
  }

  else
  {
    digitalWrite(lockPin, HIGH);
    Serial.println("Attention, porte non-verrouillée");
  }
}

void UnlockDoor()
{
  digitalWrite(lockPin, HIGH);
  Serial.println("Porte Déverrouillée");
  delay(333);
}

void motorMove(int pinPWM, int pinDIR, int speed, int sens)
{
  if (sens == Fermeture)
  {
    digitalWrite(pinDIR, LOW);
    analogWrite(pinPWM, speed);
  }

  if (sens == Ouverture)
  {
    digitalWrite(pinPWM, LOW);
    analogWrite(pinDIR, speed);
  }
  CurrentSpeed = speed;
}

void StopMotor()
{
  Serial.println("Stop motor");
  motorMove(IN1A, IN2A, 0, sens);
  CurrentSpeed = 0;
  sens = sens == Fermeture ? Ouverture : Fermeture;
  LockDoor();
  return;
}

void putIntArrayFromEEPROM(std::vector<int>& dataDelays,std::vector<int>& dataActions ,int dataSize, int address) {

int sizeData = 0;

  for (int i = 0; i < dataSize; i++) {
    int delayValue;
    EEPROM.get(address + i * sizeof(dataDelays[i]), delayValue);
    dataDelays.push_back(delayValue);
    sizeData = address + i * sizeof(dataDelays[i]);
  }

  sizeData += 4;

  for (int i = 0; i < dataSize -1; i++) {
    int actionValue;
    EEPROM.get(sizeData  + i*sizeof(int), actionValue);
    dataActions.push_back(actionValue);
  }
}

void clearEEPROM() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0);
    EEPROM.commit();
  }
  Serial.println("EEPROM clear !");
}

void ResetConfig(){
  Serial.println("Restart...");
  clearEEPROM();
  firstConfig = false;
  sizeArray = 0;
  CurrentSpeed = 0;
  delaysConfig = {0,1000};
  actionsConfig = {ACTION_A,ACTION_C};
  Serial.println("Reset Config OK");
  ESP.restart();
}

void UseDelay(){delay(10);}
void UseDelay100(){delay(100);}

static void Start(void *pvParameters)
{
  stop = false;
  Serial.println("SENS : " + String(sens == Fermeture ? "Fermeture" : "Ouverture"));
  UnlockDoor();

  std::vector<int> delays = {};
  std::vector<int> actions = {};
  
  putIntArrayFromEEPROM(delays, actions, sizeArray ,startAddress);

  int timeStart = millis();
  int timeLastCurrentValue = millis();
  int currentValue = 0;

  int getDelaysCount = sizeof(delays) / sizeof(int);

  Serial.println("MOUVEMENTS PREVUS : ");
  for (int i = 0; i < delays.size() - 1; i++)
  {
    Serial.println(String(i) + " - de " + String(delays[i]) + "ms à " + String(delays[i + 1]) + "ms : " + String(actions[i]));
  }

  while (!stop)
  {
    int timeElapsed = millis() - timeStart;
    int timeElapsedLastCurrentValue = millis() - timeLastCurrentValue;
    int actualCurrentValue = analogRead(cur_senseA);
    int delay = 0;

    for (int i = 0; i < delays.size(); i++)
    {
      if (timeElapsed >= delays[i])
      {
        delay = i;
      }
    }

    int action = actions[delay];

    if (action == ACTION_A)
    {
      int speed = map(timeElapsed, delays[delay], delays[delay + 1], CurrentSpeed, CruiseSpeed);
      motorMove(IN1A, IN2A, speed, sens);
      UseDelay();
    }

    if (action == ACTION_D)
    {
      int speed = map(timeElapsed, delays[delay], delays[delay + 1], CurrentSpeed, EngageSpeed);
      motorMove(IN1A, IN2A, speed, sens);
      UseDelay();
    }

    bool endRaceA1Pressed = EndRacePressed(EndRaceA1);
    bool endRaceA2Pressed = EndRacePressed(EndRaceA2);

    if (timeElapsed >= delays[delays.size() - 1] ||
        (sens == Ouverture && endRaceA1Pressed) ||
        (sens == Fermeture && endRaceA2Pressed) ||
        (digitalRead(Bouton)==LOW)
        )
    {
      Serial.println("STOP");
      stop = true;
    }

    if (timeElapsedLastCurrentValue >= timeBetweenCurrentValue)
    {
      timeLastCurrentValue = millis();
      currentValue = actualCurrentValue;
    }
  }
  StopMotor();
}

//------------- START CONFIG -------------\\
 
bool click = false;

void writeDelaysActionsToEEPROM(std::vector<int>& dataDelays,std::vector<int>& dataActions, int address) {

int sizeDataDelays = 0;

  for (int i = 0; i < dataDelays.size(); i++) {
    EEPROM.put(address + i * sizeof(dataDelays[i]), dataDelays[i]);
    EEPROM.commit();

    sizeDataDelays = address + i * sizeof(dataDelays[i]);
  }

  sizeDataDelays += 4;

  for (int i = 0; i < dataActions.size(); i++) {
    EEPROM.put(sizeDataDelays + i * sizeof(int), dataActions[i]);

    EEPROM.commit();
  }
}

void changeStateBt() {
    btnDisabled = !btnDisabled;
}

void StopConfig()
{
  Serial.println("Stop Config");
  firstConfig = true;
  EEPROM.put(adresseFirstConfig,firstConfig);
  EEPROM.commit();
  motorMove(IN1A, IN2A, 0, sens);
  sens = sens == Fermeture ? Ouverture : Fermeture;
  LockDoor();
  
  return;
}

void StartConfig(void *pvParameters)

{

  stop = false;
  Serial.println("Start configuration");
  Serial.println("SENS : " + String(sens == Fermeture ? "Fermeture" : "Ouverture"));

  UnlockDoor();

  int timeStart = millis();
  int timeLastCurrentValue = millis();
  int timeActionAD = 1000;
  int switchAD = ACTION_D;
  int lastChangeAD = ACTION_A;

  while(!stop)
  {

    int timeElapsed = millis() - timeStart;
    int timeElapsedLastCurrentValue = millis() - timeLastCurrentValue;
    int actualCurrentValue = analogRead(cur_senseA);
    int delay = 0;

    for (int i = 0; i < delaysConfig.size(); i++)
    {
      if (timeElapsed >= delaysConfig[i])
      {
        delay = i;
      }
    }
    int action = actionsConfig[delay];

    if (action == ACTION_A)
    {
      int speed = map(timeElapsed, delaysConfig[delay], delaysConfig[delay + 1], CurrentSpeed, CruiseSpeed);
      motorMove(IN1A, IN2A, speed, sens);
      UseDelay();
    }


    if (action == ACTION_D)
    {
      int speed = map(timeElapsed, delaysConfig[delay], delaysConfig[delay + 1], CurrentSpeed, EngageSpeed);
      motorMove(IN1A, IN2A, speed, sens);
      UseDelay();
    }
     
    bool endRaceA1Pressed = EndRacePressed(EndRaceA1);
    bool endRaceA2Pressed = EndRacePressed(EndRaceA2);

    if (
        (sens == Ouverture && endRaceA1Pressed) ||
        (sens == Fermeture && endRaceA2Pressed)
        )
    {
      Serial.println("STOP");
      stop = true;
    }
 
    if((click == true))
      {
        Serial.println("C :" + String(timeElapsed));
        delaysConfig.push_back(timeElapsed);
        delaysConfig.push_back(timeElapsed+ timeActionAD);

        if(switchAD == ACTION_A){
          actionsConfig.push_back(ACTION_A);
          actionsConfig.push_back(ACTION_C);
          switchAD = ACTION_D;
        }else{
          actionsConfig.push_back(ACTION_D);
          actionsConfig.push_back(ACTION_C);
          switchAD = ACTION_A;
        }
        click = false;
        UseDelay();
        esp_task_wdt_reset();
      }

      if(stop){
        delaysConfig.push_back(timeElapsed);
      }
   }
  Serial.println("MOUVEMENTS PREVUS : ");
  for (int i = 0; i < delaysConfig.size() - 1; i++)
 
  {
    Serial.println(String(i) + " - de " + String(delaysConfig[i]) + " ms à " + String(delaysConfig[i + 1]) + " ms : " + String(actionsConfig[i]));
  }

  clearEEPROM();
  UseDelay();
  writeDelaysActionsToEEPROM(delaysConfig, actionsConfig, startAddress);
  sizeArray = delaysConfig.size();
  EEPROM.put(adresseFirstConfig + 1, sizeArray);
  StopConfig();
  ESP.restart();
  Serial.println("Restart auto après ");
  xHandleConfig = NULL;
  vTaskSuspend(xHandleConfig);
  
  
}

void InitWifi(){
  //------------- FILE -------------\\

  if (!SPIFFS.begin(FORMAT_LITTLEFS_IF_FAILED))
  {
    Serial.println("Erreur de lecture des documents");
    return;
  }

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file)
  {
    Serial.print("File : ");
    Serial.println(file.name());
    file.close();
    file = root.openNextFile();
  }

  //------------- SERVER -------------\\

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html"); });

  server.on("/jquery-3.7.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/jquery-3.7.1.min.js", "text/javascript"); });

  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/script.js", "text/javascript"); });

  server.on("/w3.css", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/w3.css", "text/css"); });

// Requête de Js -> Cpp : click , stop
  server.on("/btnActive", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("stateBtnActive",true))
    {
      String message;
      message = request->getParam("stateBtnActive",true)->value();
      click = message.toInt();
      UseDelay();
      click = false;
    }
      request->send(200); });

      server.on("/btnStop", HTTP_POST, [](AsyncWebServerRequest *request){
    if(request->hasParam("btnStop",true))
    {
      String message;
      message = request->getParam("btnStop",true)->value();
      stop = message.toInt();
      UseDelay();
    }
      request->send(200); });

  server.on("/push", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        if (firstConfig == false && xHandleConfig == NULL)
    {
       xTaskCreate(StartConfig, "StartConfig", 10000, NULL, 2, &xHandleConfig);
    }

    request->send(200); });

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, psk);
  dnsServer.start(53, "*", WiFi.softAPIP());
  Serial.print("Access Point Mode - IP Address: ");
  Serial.println(WiFi.softAPIP());
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
  Serial.println("Server Activated.");

}

void setup()
{
  esp_task_wdt_init(120, true);

  //------------- GPIO -------------\\

  pinMode(IN1A, OUTPUT);
  pinMode(IN2A, OUTPUT);
  pinMode(cur_senseA, INPUT);
  pinMode(lockPin, OUTPUT);
  pinMode(EndRaceA1, INPUT_PULLUP);
  pinMode(EndRaceA2, INPUT_PULLUP);
  pinMode(Bouton, INPUT_PULLUP);
  pinMode(Reset, INPUT_PULLUP);

  LockDoor();

  delay(50);

  //------------- SERIAL -------------\\

  Serial.begin(115200);
  EEPROM.begin(512);
  firstConfig = EEPROM.read(adresseFirstConfig);
  sizeArray = EEPROM.read(adresseFirstConfig + 1);
  
  Serial.println("Communication Serie OK");
  Serial.println("\n");

  delay(50);

  if(firstConfig == false)
  {
    InitWifi();
  }
  else{
    dnsServer.stop();
    server.end();
    WiFi.mode(WIFI_OFF);
  }

}

void loop()
{


  if ( digitalRead(Reset)== LOW) { 
    if (!buttonPressed) { 
      buttonPressStartTime = millis(); 
      buttonPressed = true;
    }
    
    if (millis() - buttonPressStartTime == restartDelay && !restartTriggered) {
      
      ResetConfig(); 
      restartTriggered = true;
    }

  } else { 
    buttonPressed = false; 
    restartTriggered = false;
  }

  

  if (digitalRead(Bouton) == LOW)
  {
    
    delay(1200);
    Serial.println("Bouton Start");
    Start(NULL);
    delay(1000);
  }

  if(firstConfig == false){
    dnsServer.processNextRequest();
  }
  
}
