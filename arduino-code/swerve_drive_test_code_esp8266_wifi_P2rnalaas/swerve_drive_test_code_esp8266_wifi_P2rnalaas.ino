/* Kasutame plaati "NodeMCU Lolin V3".
 Pinout: https://www.google.com/url?sa=i&url=https%3A%2F%2Fwww.theengineeringprojects.com%2F2018%2F10%2Fintroduction-to-nodemcu-v3.html&psig=AOvVaw2rf1HHs6BeR9AY0TZf059I&ust=1682944000922000&source=images&cd=vfe&ved=0CBEQjRxqFwoTCKi76IzN0f4CFQAAAAAdAAAAABAE
 Kasutame mootori draiverit "L298N DC motor driver module"
 Kontrolleri Kasutamise näide: https://www.electronicwings.com/nodemcu/nodemcu-gpio-with-arduino-ide
 Koos kasutamine: https://lastminuteengineers.com/l298n-dc-stepper-driver-arduino-tutorial/?utm_content=cmp-true 
 */
/*Mikroprotsessori võimalikud ühendused: GPIO-d D0, D1, D2, D3, D4, D5, D6, D7, D8 ja lisaks S2(D9), S3(D10)
Kontroller on ühendatud kolme draiveriga.
Kõik kolm draiverit jagavad pine In1-In4, et neid korraga kontrollida lülitada. Samuti selleks, et GPIO pin-e plaadil kokku hoida.
Kuid sellise ühenduse puhul liiguvad kõik mootorid ühes suunas.
*/
#include <ESP8266WiFi.h>

// In1 ja In2 kontrollivad liikumismootorite suunda (A mootorid) ning In3 ja In4 kontrollivad pööramismootorite suunda (B mootorid).
#define In1 D1
#define In2 D2 
#define In3 D3
#define In4 D4
// Mootorite kontrollimiseks PWM ühendused. A on liikumiseks, B on pööramiseks.
// viide : https://cyberblogspot.com/nodemcu-v3-esp8266-pinout-and-configuration/
#define CNTR1_ENA D0
#define CNTR1_ENB D5
#define CNTR2_ENA D6
#define CNTR2_ENB D7
#define CNTR3_ENA D8
#define CNTR3_ENB 9 // GPIO9

IPAddress    apIP(12, 12, 12, 12);  // Defineerime staatilise IP aadressi: lokaalne ja pääsupunkt.
// Vaikimisi IP pääsupunktiks seadistatuna on 192.168.4.1

// Wifi AP seadistus:
const char *ssid = "PilleswerveAP";
const char *password = "parool1234";
// Seadistatud veebiserver pordil 80 HTTP jaoks.
WiFiServer server(80);

// Ühekordne konfigureerimis funktsioon.
void setup() {
  // Seadistame draiverite pinid väljundiks.
  pinMode(In1, OUTPUT);
  pinMode(In2, OUTPUT);
  pinMode(In3, OUTPUT);
  pinMode(In4, OUTPUT);
  pinMode(CNTR1_ENA, OUTPUT);
  pinMode(CNTR1_ENB, OUTPUT);
  pinMode(CNTR2_ENA, OUTPUT);
  pinMode(CNTR2_ENB, OUTPUT);
  pinMode(CNTR3_ENA, OUTPUT);
  pinMode(CNTR3_ENB, OUTPUT);
  delay(1000);
  // Jadamisühenduse suhtlus kiirusel 115200 bps.
  Serial.begin(115200);
  Serial.println("Serial init");
  Serial.println();
  Serial.println();
  Serial.print("Configuring WiFi access point...");
  delay(10);

  WiFi.softAP(ssid, password);
  delay(1000);
  IPAddress myIP = WiFi.softAPIP();

  Serial.print("Host IP address: ");
  Serial.println(myIP);
  Serial.print("AP name: ");
  Serial.println(ssid);
  Serial.print("AP password: ");
  Serial.println(password);

  server.begin();
  Serial.println("Server started");
  // Lülita mootorid välja, see on esialgne seis.
  digitalWrite(In1, LOW);
	digitalWrite(In2, LOW);
	digitalWrite(In3, LOW);
	digitalWrite(In4, LOW);
}

// analogWrite väärtused on vahemikus 0 to 255.
int speed_drive = 0;
int speed_turn = 0;
// Muutuja direction, et kontrollida liikumissuunda.
int direction = 0;
// Muutujad määramaks, kas kontrollime hetkel pööramise või liikumise funktsionaalsust.
int drive = 1;
int turn = 0;
// Muutujad rataste kiiruse salvestamiseks.
int wheel1 = 0;
int wheel2 = 0;
int wheel3 = 0;
// Muutuja jälgimiseks, kas kiirust on saadud käsus muudetud.
bool change_speed=false;

// Lõputu tsükkel.
void loop() {
  // Kontrollime, kas wifi AP-ga on klient ühendatud.
  WiFiClient client = server.available();
  if (client) {
    Serial.println("new client");
    while (client.connected()) {
      String myRequest = " ";
      if (client.available()){
        // Loeme myRequest esimest rida saadud infost.
        myRequest = client.readStringUntil('\r');
        Serial.print("request: ");
        Serial.println(myRequest);
      }
      client.flush();

      // Olekumasin käitumaks vastavalt nutiseadmest tulevale käsule.
      if (myRequest.equals("DRIVE")){
        Serial.println("DRIVE");
        // lõpeta pööramine enne liikumise alustamist
        speed_control(0, 0, 0, 0);
        drive=1;
        turn=0;
      }
      else if (myRequest.equals("TURN")){
        Serial.println("TURN");
        // lõpeta sõitmine enne pööramise alustamist
        speed_control(0, 0, 0, 1);
        drive=0;
        turn=1;
      }
      else if (myRequest.equals("FORWARD")){
        Serial.println("FORWARD");
        direction=0;
        if (drive==1){
          turn_motors_on(direction, 1);
        }
        else if (turn==1){
          turn_motors_on(direction, 0);
        }
      }
      else if (myRequest.equals("BACK")){
        Serial.println("BACK");
        direction=1;
        if (drive==1){
          turn_motors_on(direction, 1);
        }
        else if (turn==1){
          turn_motors_on(direction, 0);
        }
      }
      else if (myRequest.charAt(0)=='A'){
        Serial.print("wheel-1 ");
        myRequest.remove(0, 1);
        wheel1=myRequest.toInt();
        Serial.print(wheel1);
        change_speed=true;
      }
      else if (myRequest.charAt(0)=='B'){
        Serial.print("wheel-2 ");
        myRequest.remove(0, 1);
        wheel2=myRequest.toInt();
        Serial.print(wheel2);
        change_speed=true;
      }
      else if (myRequest.charAt(0)=='C'){
        Serial.print("wheel-3 ");
        myRequest.remove(0, 1);
        wheel3=myRequest.toInt();
        Serial.print(wheel3);
        change_speed=true;
      }
      else if (myRequest.equals("STOP")){
        Serial.println("STOP");
        turn_motors_off();
        for(;;);
      }
      
      if (change_speed==true){
        Serial.println(" speed change");
        if (drive==1){
          speed_control(wheel1, wheel2, wheel3, 1);
        }
        else if (turn==1){
          speed_control(wheel1, wheel2, wheel3, 0);
        }
        change_speed=false;
      }
    }
  }  
}

/*Funktsioon mootorite sisse lülitamiseks. 
 Muutuja direction määrab liikumissuuna.
 Muutuja turn_or_drive määrab, kas lülitame keeramismootoreid (0) või liikumismootoreid (1).
*/ 
void turn_motors_on(int direction, int turn_or_drive){
  // suund 1
  if (direction==1){
    // liikumismootoid
    if (turn_or_drive==1){
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, HIGH);
      digitalWrite(In4, LOW);
    }
    // keeramismootorid
    else{
      digitalWrite(In1, HIGH);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
    }
  }
  // suund 2
  else{
    // liikumismootorid
    if (turn_or_drive==1){
      digitalWrite(In1, LOW);
      digitalWrite(In2, LOW);
      digitalWrite(In3, LOW);
      digitalWrite(In4, HIGH);
    }
    // keeramismootorid
    else{
      digitalWrite(In1, LOW);
      digitalWrite(In2, HIGH);
      digitalWrite(In3, LOW);
      digitalWrite(In4, LOW);
    }
  }
}
/* 
Funktsioon mootorite välja lülitamiseks.
*/
void turn_motors_off(){
  digitalWrite(In1, LOW);
	digitalWrite(In2, LOW);
	digitalWrite(In3, LOW);
	digitalWrite(In4, LOW);
}

/*
Funktsioon mootoritele kiiruse andmiseks.
Muutujad W1, W2, W3 määravad ära, millisele rattale ning kui palju kiirust anname.
Muuutuja turn_or_drive määrab, kas kontrollime pööramist või sõitmist.
Kui turn_or_drive on 1, siis liikumist, kui 0, siis pööramist.
*/
void speed_control(int W1, int W2, int W3, int turn_or_drive){
  // liikumine - mootorid B
  if (turn_or_drive==1){
    if (W1>0){
      analogWrite(CNTR1_ENB, W1);
    }
    if (W2>0){
      analogWrite(CNTR2_ENB, W2);
    }
    if (W3>0){
      analogWrite(CNTR3_ENB, W3);
    }
  }
  // pööramine - mootorid A
  else if (turn_or_drive==0){
    if (W1>0){
      analogWrite(CNTR1_ENA, W1);
    }
    if (W2>0){
      analogWrite(CNTR2_ENA, W2);
    }
    if (W3>0){
      analogWrite(CNTR3_ENA, W3);
    }
  }
}