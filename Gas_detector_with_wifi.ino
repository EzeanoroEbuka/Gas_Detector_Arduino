#include "credentials.h"
#include <Base64.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// Pin Definitions
const int mq2Pin = A0; // MQ2 sensor analog pin
const int ledGasPin = D1; // LED for gas detection
const int ledSafePin = D2; // LED for safe condition
const int buzzerPin = D3; // Buzzer pin

// Threshold value for gas detection (adjust this as needed)
const int threshold = 950; // Example threshold for gas detection

// Wi-Fi Credentials
const char* ssid  = "DataBase Server";
const char* password = "password";

//credentials for sending Email
const char* gmailServer = "smtp.gmail.com"; 
const int smtpPort = 465; 
const char* senderEmail = SENDEREMAIL;
const char* emailPassword = EMAILPASSWORD; 
const char* recipientEmail = RECIPIENTEMAIL;

ESP8266WebServer server(80);

WiFiClientSecure client;

void handleRoot() {
    int gasLevel = analogRead(mq2Pin);
    String message = "<h1>Gas Leakage Detector</h1>";
    message += "<p>Current Gas Level: " + String(gasLevel) + "</p>";
    if (gasLevel > threshold) {
        message += "<p><b>Warning: High Gas Level Detected!</b></p>";
    } else {
        message += "<p>Gas levels are safe.</p>";
    }
    
    server.sendHeader("Access-Control-Allow-Origin", "*"); // Allow all origins
    server.send(200, "text/html", message); // Send response
}

void setup() {
    Serial.begin(115200);
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    pinMode(ledGasPin, OUTPUT);
    pinMode(ledSafePin, OUTPUT);
    pinMode(buzzerPin, OUTPUT);

    // Connect to Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Wi-Fi...");
    }
    Serial.println("Connected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());


    // Start the server
    server.on("/", handleRoot); // Define route for "/"
    server.begin();
    Serial.println("HTTP server started");
}
bool emailNotSent = true;
void loop() {
  
    server.handleClient(); // Handle HTTP client requests
    int gasLevel = analogRead(mq2Pin);
    Serial.print("Gas Level: ");
    Serial.println(gasLevel);

    if (gasLevel > threshold) {
        digitalWrite(ledGasPin, HIGH);
        digitalWrite(buzzerPin, HIGH);
        digitalWrite(ledSafePin, LOW);  
        if(emailNotSent){
          verifyWifi();
          sendEmail();
          emailNotSent = false;
        }      
    } else {
        digitalWrite(ledGasPin, LOW);
        digitalWrite(ledSafePin, HIGH);
        digitalWrite(buzzerPin, LOW);
        if(gasLevel == threshold - 50){
           emailNotSent = true;
        }
    }
    delay(1000);
    server.handleClient(); // Handle incoming HTTP requests
}

void verifyWifi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Cannot send email.");
        return;
    }
}

byte sendEmail() {
    
     Serial.println("Attempting to connect to Gmail Server");
    if(client.connect(gmailServer,smtpPort) == 1){
      Serial.println(F("connected"));
    }
    else{
      Serial.println(F("connection Failed: "));
      return 0;
    }

    if(!response())
      return 0;
    

    Serial.println(F("Sending EHLO"));
    client.println("EHLO gmail.com");
    
    if(!response())
      return 0;
    Serial.println(F("Sending auth login"));
    client.println("auth login");
    if(!response())
      return 0;
   Serial.println(F("Sending User"));
    client.println(base64::encode(senderEmail));
    if(!response())
      return 0;

  Serial.println(F("Sending Password"));
    client.println(base64::encode(emailPassword));
    if(!response())
      return 0;    
   Serial.println(F("Sending From"));
      char* senderContent;
       snprintf(senderContent, sizeof(senderContent), "MAIL FROM: <%s>", senderEmail);
      client.println(F(senderContent));
      if(!response())
        return 0; 
  Serial.println(F("Sending To"));
    client.println(F("RCPT TO: <recipientEmail>"));
    if(!response())
      return 0;    
   Serial.println(F("Sending DATA"));
    client.println(F("DATA"));
    if(!response())
      return 0;    

  client.println(F("From: ebest"));
  client.println(F("Subject: Gas Detector Alert\r\n"));
  client.println(F("This email was sent securely via an encrypted mail link.\n"));
  client.println(F("In the last few Minutes Gas leaks was detected. Please check your Gas cylinders and other Gas Equipment."));
  client.println(F("This email was sent from an unmonitored email account - please do not reply."));
  client.println(F("Appluade for Ebuka and Tomi.They wrote this sketch."));
  client.println(F("."));
  
  if (!response())
    return 0;
  Serial.println(F("Sending QUIT"));
  client.println(F("QUIT"));
  if (!response())
    return 0;
  client.stop();
  Serial.println(F("Disconnected"));
  return 1;
}


byte response()
{
 
  int loopCount = 0;
  while (!client.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if (loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }
    // Take a snapshot of the response code
  byte respCode = client.peek();
  while (client.available())
  {
    Serial.write(client.read());
  }
  if (respCode >= '4')
  {
    Serial.print("Failed in eRcv with response: ");
    Serial.print(respCode);
    return 0;
  }
  return 1;
}
