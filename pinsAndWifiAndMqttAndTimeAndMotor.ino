#include <Adafruit_MotorShield.h>

#include <WiFi.h>
#include "time.h"

WiFiClient espClient;
#include "Esp32MQTTClient.h"

// Please input the SSID and password of WiFi
const char* ssid = "myNetwork";
const char* password = "3866159019";

const long  gmtOffset_sec = (-5 * 3600);
const int   daylightOffset_sec = (-4 * 3600);

const char* ntpServer = "pool.ntp.org";

static const char* connectionString ="";
  
static bool hasIoTHub = false;

//////// MOTOR ////////////
Adafruit_MotorShield AFMS = Adafruit_MotorShield();


// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *motorLeft = AFMS.getMotor(1);
Adafruit_DCMotor *motorRight = AFMS.getMotor(3);

/////////////// ULTRASONIC  HR-SR04 ////////////////////
#define HC_Trig1 14  // trigger reading 
#define HC_Data1 32  // data input     
#define HC_Trig2 15  // data input     
#define HC_Data2 33  // trigger reading 
#define HC_Trig3 27  // data input     
#define HC_Data3 13  // trigger reading 

const int vals = 1;
int d1Vals[vals], d2Vals[vals], d3Vals[vals];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  // init arrays
  for (int x = 0; x < vals; x++) {
    d1Vals[x] = 0;
    d2Vals[x] = 0;
    d3Vals[x] = 0;
  }

  Serial.begin(115200);           // startup serial for debugging

  // wait for serial port to open on native usb devices
  while (!Serial) {
    delay(1);
  }

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Establishing connection to WiFi..");
  }
  Serial.println("Connectedx");

  Serial.println(WiFi.macAddress());
  Serial.println(WiFi.localIP());

  if (!Esp32MQTTClient_Init((const uint8_t*)connectionString))
  {
    hasIoTHub = false;
    Serial.println("Initializing IoT hub failed.");
    return;
  }
  hasIoTHub = true;

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // configure pins for HC
  pinMode(HC_Trig1, OUTPUT);
  pinMode(HC_Data1, INPUT);

  pinMode(HC_Trig2, OUTPUT);
  pinMode(HC_Data2, INPUT);

  pinMode(HC_Trig3, OUTPUT);
  pinMode(HC_Data3, INPUT);

  digitalWrite(HC_Trig1, LOW);
  digitalWrite(HC_Trig2, LOW);
  digitalWrite(HC_Trig3, LOW);

  AFMS.begin();  // create with the default frequency 1.6KHz

  // Set the speed to start, from 0 (off) to 255 (max speed)
  motorLeft->setSpeed(0);
  motorLeft->run(FORWARD);
  motorRight->setSpeed(0);
  motorRight->run(FORWARD);

  // turn on motor
  motorRight->run(RELEASE);
  motorLeft->run(RELEASE);
}

// do actual sensor reading
int getReading(int trig, int echo)
{
  digitalWrite(trig, LOW);
  delay(1);
  digitalWrite(trig, HIGH);
  delay(1);
  digitalWrite(trig, LOW);
  long duration = pulseIn(echo, HIGH);
  int d = duration * 0.034 / 2;
  if (d > 200)
    d = 200;

  return d;
}

void printLocalTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, ">>>>>>>>> %A, %B %d %Y %H:%M:%S");

  time_t now;
  time(&now);
  Serial.print(">>>>> EPOCH >>>>>  ");
  Serial.println(now);

}

void sendData(int d1, int d2, int d3)
{
  if (hasIoTHub)
  {
    char buff[200];
    time_t now;
    time(&now);
    sprintf(buff, "{\"topic\":\"lieb\", \"d1\":\"%d\", \"d2\":\"%d\",\"d3\":\"%d\",\"e\":\"%d\" }", d1, d3, d2, now);

    if (Esp32MQTTClient_SendEvent(buff))
    {
      Serial.println("Sending data succeed");
    }
    else
    {
      Serial.println("Failure...");
    }
  }
}

void adjustMotor(int d1, int d2, int cntr) {

  if (cntr > 100)
  {
    motorLeft->setSpeed(200);
    motorLeft->run(FORWARD);
    motorRight->setSpeed(200);
    motorRight->run(FORWARD);
  }
  else if (cntr < 40)
  {
    Serial.println("Slow roll");
    if (d1 > d2)
    {
      motorLeft->setSpeed(d2);
      motorLeft->run(FORWARD);

      motorRight->setSpeed(50);
      motorRight->run(BACKWARD);
    }
    else
    {
      motorLeft->setSpeed(50);
      motorLeft->run(BACKWARD);

      motorRight->setSpeed(d2);
      motorRight->run(FORWARD);
    }
  }
  else if (cntr < 75)
  {
    Serial.println("Mid roll");
    if (d1 > d2)
    {
      motorLeft->setSpeed(d2);
      motorLeft->run(FORWARD);

      motorRight->setSpeed(d1 * 1.2);
      motorRight->run(FORWARD);
    }
    else
    {
      Serial.println("Fast roll");
      motorLeft->setSpeed(d1);
      motorLeft->run(FORWARD);

      motorRight->setSpeed(d2 * 1.2);
      motorRight->run(FORWARD);
    }
  }
  else
  {
    if (d1 > d2)
    {
      motorLeft->setSpeed(d2);
      motorLeft->run(FORWARD);

      motorRight->setSpeed(200);
      motorRight->run(FORWARD);
    }
    else
    {
      motorLeft->setSpeed(d1);
      motorLeft->run(FORWARD);

      motorRight->setSpeed(200);
      motorRight->run(FORWARD);
    }
  }
}

// add value to array, get moving average
int getMovingValue(int *values, int newValue)
{
  long total = newValue;
  for (int x = (vals - 1); x > 0; x--) {
    total = values[x - 1];
    values[x] = values[x - 1];
  }
  values[0] = newValue;
  return total / vals;
}

// get sensor avg readings and display
void readAllSensors()
{
  int d1 = getMovingValue(d1Vals, getReading(HC_Trig1, HC_Data1));
  int d2 = getMovingValue(d2Vals, getReading(HC_Trig2, HC_Data2));

  d3Vals[2] = d3Vals[1];
  d3Vals[1] = d3Vals[0];
  d3Vals[0] = getReading(HC_Trig3, HC_Data3);
  int d3 = (d3Vals[0] + d3Vals[1] + d3Vals[2]) / 3;

  long st = millis();
  // sendData(d1, d2, d3);
  Serial.println( (millis() - st) / 1000.0);

  adjustMotor(d1, d2, d3);



  Serial.print(d1);
  Serial.print("\t\t");
  Serial.print(d3);
  Serial.print("\t\t");
  Serial.println(d2);
}

int loops = 1000;

void loop() {
  readAllSensors();
  loops++;
  if (loops > 50) {
    printLocalTime();
    loops = 0;
  }
}
