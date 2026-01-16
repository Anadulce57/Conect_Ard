#include <WiFi.h>                                     
#include <WebServer.h>                                
#include <WebSocketsServer.h>  
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
// Declaração para o mq-7
const int mq7 = 35;
int ppm;
// Declaração para o mq-135
const int mq135 = 32;
int qualidadeAr;
//Declarando tippo de LCD
LiquidCrystal_I2C lcd(0x27, 16, 2);
//Declarando buzzer 
const int buzzer = 15;
// Inicializar a variável msg com uma string vazia  
String msg = "";                      
// SSID and password of Wifi connection:
const char* ssid = "---";
const char* password = "---";
// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
String website = "<!DOCTYPE html><html> <head> <title>Monitoramento ESP32</title> <meta name='viewport' content='width=device-width, initial-scale=1' /> <link rel='stylesheet' href='https://fonts.googleapis.com/css2?family=Material+Symbols+Outlined:opsz,wght,FILL,GRAD@24,400,0,0&icon_names=videocam' /> <style> body { margin: 0; padding: 0; background-color: #1a222c; color: #e0e0e0; font-family: Arial, sans-serif; } .header-bar { background-color: #4f6b4f; color: white; padding: 15px 10px; text-align: center; font-size: 20px; font-weight: bold; position: relative; } .content { padding: 20px; } h1 { display: none; } h2 { color: #a0a0a0; margin-top: 30px; } #commandInput { width: 90%; padding: 10px; margin-bottom: 15px; border: 1px solid #4f6b4f; border-radius: 5px; background-color: #2d333b; color: #e0e0e0; } button { background-color: #4f6b4f; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; } button:hover { background-color: #6a876a; } a { color: white; } #responseStatus { margin-top: 20px; font-style: italic; color: #707070; } </style> </head> <body> <div class='header-bar'>Monitoramento Pet</div> <div class='content'> <h1>Monitoramento Pet</h1> <h2>Controle de Comandos</h2> <input type='text' id='commandInput' placeholder='Ex: alertar ou desligar' /> <button onclick='sendCommand()'>Enviar Comando</button> <p id='responseStatus'></p> <a href='http://172.20.10.13/' ><span class='material-symbols-outlined'>videocam</span></a > </div> </body> <script> var Socket; function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function (event) { processCommand(event); }; } function sendCommand() { var command = document.getElementById('commandInput').value; if (Socket && Socket.readyState === 1) { Socket.send(command); document.getElementById('responseStatus').innerHTML = 'Comando enviado: ' + command; document.getElementById('commandInput').value = ''; } else { document.getElementById('responseStatus').innerHTML = 'Erro: Conexão WebSocket não está aberta (Estado: ' + (Socket ? Socket.readyState : 'Não inicializado') + ')'; console.error('Tentativa de envio antes da conexão OPEN.'); } } function processCommand(event) { document.getElementById('responseStatus').innerHTML = 'Resposta do Servidor: ' + event.data; console.log('Dado Recebido: ' + event.data); } window.onload = function (event) { init(); }; </script></html>";                     
// Initialization of webserver and websocket
WebServer server(80);                                 
WebSocketsServer webSocket = WebSocketsServer(81);  
//Declarando função webSocketEvent 
void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length);

void setup() {
  //Chamando 
  dht.begin();
  lcd.init();
  lcd.backlight();
  pinMode(mq7, INPUT);
  pinMode(mq135, INPUT);
  pinMode(buzzer, OUTPUT);
  // init serial port for debugging and start WiFi interface
  Serial.begin(115200);                                         
  WiFi.begin(ssid, password);                        
  Serial.println("Establishing connection to WiFi with SSID: " + String(ssid));     // print SSID to the serial interface for debugging
 // wait until WiFi is connected
  while (WiFi.status() != WL_CONNECTED) {             
    delay(1000);
    Serial.print(".");
  }
  Serial.print("Connected to network with IP address: ");
  Serial.println(WiFi.localIP());                     // show IP address that the ESP32 has received from router
  server.on("/", []() {                               // define here wat the webserver needs to do
    server.send(200, "text/html", website);           //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();                                     // start server
  webSocket.begin();                                  // start websocket
  webSocket.onEvent(webSocketEvent);                // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"
}

void loop() {
  server.handleClient();                              // Needed for the webserver to handle all clients
  webSocket.loop();                                   // Update function for the webSockets 
  delay(2000);
  lcd.setBacklight(HIGH);
  float h = dht.readHumidity(); // Umidade
  float t = dht.readTemperature(); // Temperatura
  if (isnan(h) || isnan(t)) {
    lcd.setCursor(0, 0);
    Serial.print(("Falha"));
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("Umidade: "));
    lcd.setCursor(10, 0);
    lcd.print(round(h));
    lcd.setCursor(12, 0);
    lcd.print(F(" %"));
    delay(3000);

    lcd.setCursor(0, 1);
    lcd.print(F("Clima: "));
    lcd.setCursor(7, 1);
    lcd.print(round(t));
    lcd.setCursor(9, 1);
    lcd.write(32);  // Caracter espaço
    lcd.write(223); // Caracter °
    lcd.print(F("C"));
    delay(3000);  
  }
}

void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length) {      // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                     // switch on the type of information sent
    case WStype_DISCONNECTED:                         // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:                            // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected"); //Code here what to do when connected
      break;
    case WStype_TEXT:                                // if a client has sent data, then type == WStype_TEXT
      String msg = String((char*)payload, length); 
      msg.trim();
      Serial.print("Recebido via WS: ");
      Serial.println(msg);
      if (msg == "responde") {
          webSocket.sendTXT(num, "Olá, respondo");
          Serial.println("Olá, respondo!");
        } else if (msg == "temperatura") {
          // Leitura do sensor DHT11
          float h = dht.readHumidity(); // Umidade
          float t = dht.readTemperature(); // Temperatura
          if (isnan(h) || isnan(t)) {
          Serial.println(F("Falha de leitura do sensor DHT!"));
        } else {
          Serial.println("Umidade: " + String(h) + "%");
          Serial.println("Temperatura: " + String(t) + " C");
          webSocket.sendTXT(num, "Umidade: " + String(h) + "%");
          webSocket.sendTXT(num, "Temperatura: " + String(t) + " C");
        }
        } else if (msg == "gas") {    // MQ-7
          ppm = analogRead(mq7);
          if (ppm > 70) {
            Serial.println("Gás vazando!");
            webSocket.sendTXT(num, "Gás vazando");
          } else {
            Serial.println("Tudo normal");
            webSocket.sendTXT(num, "Tudo normal");
          }
          } else if (msg == "qualidade") {   // MQ-135
              qualidadeAr = analogRead(mq135);
             if (qualidadeAr > 560) {
              Serial.print("ALERTA: Qualidade do ar ruim");
              webSocket.sendTXT(num, "ALERTA: Qualidade do ar ruim");
          } else {
              Serial.print("Qualidade do ar boa");
              webSocket.sendTXT(num, "Qualidade do ar boa");
          }
          } else if (msg == "alertar"){
              tone(buzzer, 300);
              delay(500);
              Serial.print("Alarme ligado!");
              webSocket.sendTXT(num, "Alarme ligado!");
          } else if (msg == "desligar"){
              noTone(buzzer);
              Serial.print("Alarme desligado...");
              webSocket.sendTXT(num, "Alarme desligado...");
          }
      break;
  }
}