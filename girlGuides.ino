/*  
 *  This is the main codebase for the robot's controls and perception of traffic light states (through messages over wifi), ultimately we want this (or maybe another file with only the functions we want the girls to interact with) in a header file
 *  Written by: Roberto Ruiz, Evan Arsenault, Karen Rees-Milton (Add your names as you clone and update this code?)
*/
#include <complex.h> 
#include <ArduinoJson.h>
#include <Servo.h> 
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//TIME_TO_TURN90 is a constant, time taken in milliseconds to turn 90 degrees
//TIME_SERVO_ON is a constant, time taken in milliseconds to move from 1 square to another
//*******TURN_TIME and MOVE_TIME will need to be determined********** 
#define TIME_TO_TURN90 175
#define TIME_SERVO_ON 500
StaticJsonDocument<200> jsonLightStatus; // create JSON object to deserialize (decode) and store the recieved message

Servo leftServo;  // create servo object to control a servo
Servo rightServo;

//all our functions
void drive(); 
void turn(char dir);
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void reconnect();
void get_message();
String check_light();

//Complex pos(1,1); //don't worry about these, just a method of accounting position and orientation
//Complex orientation(0,1);

//start position and orientation
int x = 1;
int y = 1;
char orientation = 'N';

//RR - used in checklight - TODO: change all instances of this variable with whatever position variable we're gonna use
int mypos[] = {0,0};

//WiFi connection details
const char* ssid = "CACGuest";
const char* password = "quickiris517";
const char* mqtt_server = "192.168.12.179";

//Create WiFi and MQTT objects to interact with WiFi and MQTT (lib-name: PubSubClient) libraries
WiFiClient espClient; 
PubSubClient client(espClient);

//MQTT 
long lastMsg = 0; //for MQTT messaging
char msg[50];
int value = 0;

int light_state[9]; //an array of 9 characters, r for red, g for green. Each index corresponds to the position of light in light_position below
int light_position[9][2] = {{3,3},{3,5},{3,7},{5,3},{5,5},{5,7},{7,3},{7,5},{7,7}}; //positions of each traffic light

void setup() {
  Serial.begin(115200); //sets up Baud rate for Serial monitor
  pinMode(13, OUTPUT); //turns pin 13 into an output LED

  //MQTT Set-up
  setup_wifi(); //connects to Wifi, sets up callback which is ran whenever new message comes in
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  client.subscribe("jsonTest"); //This is the 'topic' from which we will recieve the traffic light status

  //Setting Servo objects to the GPIO pins the physical servos are connected to
  leftServo.attach(12); // Moves forward with 0
  rightServo.attach(14); // Moves forward with 1

  //Sequence of moves goes here
  drive();
  turn('L');
  turn('R');
  turn('R');
}

//Not used, but just in case, keep asking for messages recieved
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

//Calibration happens here
//----------------------------------------------------------------------------//
int timeServoOn = 500;  //Manages step size: keep the servos on for 0.5 seconds
int timeToTurn90 = 250;  // Manages the time spent turning (TODO: Calibrate to 90 degrees): Keep the servos on for .25 seconds
//----------------------------------------------------------------------------//

//Function to drive 'one square' forwards
void drive() {
  // Since we are not using the loop() function, and we need to constantly ask for MQTT messages
    // every function should be calling client.loop() and attempting to reconnect.
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //RR - Robot as left by me on wednesday (Jul-31st) has the leftServo connected to the left servo
  // and the left servo will drive forwards with 0. Viceversa for rightServo (180 to go forwards)
  leftServo.write(0);
  rightServo.write(180);

  //This is how long the servo spends with the servos on
  delay(TIME_SERVO_ON);

  //Writing 90 to the servos will cause them to stop
  leftServo.write(90);
  rightServo.write(90);

  //update position (x and y values)
  switch(orientation){
    case 'N':
      y++;
      break;
    case 'E':
      x++;
      break;
    case 'S':
      y--;
      break;
    case 'W':
      x--;
      break;
  }
}

void turn(char dir) {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
//  Complex turn(0,1);

  // RR - As I left it on wednesday, it will turn left with both servos written to 180 and right with both servos written to 0
  if (dir == 'L') {
    Serial.println("TURN LEFT");
    leftServo.write(180);
    rightServo.write(180);

    // Similar to drive(), time spent with the servos on but with the goal to turn 90 degrees
    delay(TIME_TO_TURN90);

    leftServo.write(90);
    rightServo.write(90);
    
//    orientation = orientation * turn;
//    Serial.println(orientation);
//update orientation (N, E, S, W)
    switch(orientation){
      case 'N':
        orientation = 'W';
        break;
      case 'E':
        orientation = 'N';
        break;
      case 'S':
        orientation = 'E';
        break;
      case 'W':
        orientation = 'S';
        break;
    }
  } 
  else if (dir == 'R') {
    Serial.println("TURN RIGHT");
    leftServo.write(0);
    rightServo.write(0);

    // Similar to drive(), time spent with the servos on but with the goal to turn 90 degrees
    delay(TIME_TO_TURN90);

    leftServo.write(90);
    rightServo.write(90);
    
//    orientation = orientation * (-turn);
//    Serial.println(orientation);
//update orientation (N, E, S, W)
    switch(orientation){
      case 'N':
        orientation = 'E';
        break;
      case 'E':
        orientation = 'S';
        break;
      case 'S':
        orientation = 'W';
        break;
      case 'W':
        orientation = 'N';
        break;
    }
  }
}

void setup_wifi() { //this establishes wifi
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  // The actual attempt to connect
  WiFi.begin(ssid, password);

  // A check to wait while it tries to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // I don't even know
  randomSeed(micros());

  // Helpful info printed to the monitor
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// When an MQTT message is recieved, this function is automatically called.
// The MQTT messages we will be recieving is the traffic lights' state
void callback(char* topic, byte* payload, unsigned int length) { //modify so that it changes light_state array
  
  Serial.print("Message recieved");
  // Decode the message and turn it into a JSON object
  DeserializationError error = deserializeJson(jsonLightStatus,payload);

  Serial.print("Message arrived [");
  Serial.print(topic); // Topic where the message came from
  Serial.print("] ");
  Serial.println("");
  
  if (error) {
    Serial.print(F("deserializeJSON() failed: "));
    Serial.println(error.c_str());
    return;
  }
  // Change the light state
  light_state[0] = jsonLightStatus["d"]["light0"];
  light_state[1] = jsonLightStatus["d"]["light1"];
  light_state[2] = jsonLightStatus["d"]["light2"];
  light_state[3] = jsonLightStatus["d"]["light3"];
  light_state[4] = jsonLightStatus["d"]["light4"];
  light_state[5] = jsonLightStatus["d"]["light5"];
  light_state[6] = jsonLightStatus["d"]["light6"];
  light_state[7] = jsonLightStatus["d"]["light7"];
  light_state[8] = jsonLightStatus["d"]["light8"];

  Serial.println("Light state changed successfully");

// Uncomment to check the values being recieved
//  Serial.println("The light[0] will be switched to");
//  Serial.println(jsonLightStatus["d"]["light6"]);
//  Serial.println(light_state[6]);
//  Serial.println("");
}

//Attempt to reconnect to MQTT
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("jsonTest", "hello world");
      // ... and resubscribe
      client.subscribe("jsonTest");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// RR - We probably don't need this
//void get_message() {
//  if (!client.connected()) {
//    reconnect();
//  }
//  client.loop();
//
//  long now = millis();
//  if (now - lastMsg > 2000) {
//    lastMsg = now;
//    ++value;
//    snprintf (msg, 50, "hello world #%ld", value);
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish("outTopic", msg);
//  }
//}

//Function that checks whether the light corresponding to the spot the robot is in is Red or Green
//The function will return "Green" or "Red"
String check_light() {
//  Complex next_pos = pos + orientation;
  int index = -1;

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //Loop over each light and check whether the robot is standing on the spot corresponding to any of the lights' location
  for(int i=0; i<9; i++){
    if(light_position[i][0] == mypos[0]){
      if(light_position[i][1] == mypos[1]){

        //We are at a light
        Serial.println("The light we are facing is at");
        Serial.print(light_position[i][0]);
        Serial.println("");
        Serial.println(light_position[i][1]);
        Serial.println("The light status is");

        //If the lights' state is 1, that translates to green. Viceversa for 0 (Red)
        if(light_state[i] == 1){
          Serial.println("Green");
          return "Green";
        }
        else if(light_state[i] == 0){
          Serial.println("Red");
          return "Red";
        }
      }
    }
  }
}
