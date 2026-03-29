/*
  Specialized only for The ESP32 Robot Build

  __________________________________________
 | VL53L0X I2C : SCL = 19 | SDA = 18 | 0x29 |
 |________________________|__________|______|
*/

#include "BluetoothSerial.h"
#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>

#define I2C_SCL 19
#define I2C_SDA 18

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run 'make menuconfig' to enable it
#endif

BluetoothSerial SerialBT;
byte bt_data;

Servo myservo;

Adafruit_VL53L0X lox = Adafruit_VL53L0X();
//--------------------------------------------------------------------------------
const int motor1a = 32, motor1b = 33, motor2a = 25, motor2b = 26, servoPin = 12;
const int trigPin = 21, echoPin = 22;
int duration, sonic_distance, laserDist;
int distanceLeft=0, distanceRight=0;
//--------------------------------------------------------------------------------
const int adcPin=14;
const float referenceVoltage=3.3;
const float dividerRatio=1.27;
const float minVoltage=3.0;
const float maxVoltage=4.2;
//--------------------------------------------------------------------------------
void controlMotors(int m1a, int m1b, int m2a, int m2b) {
  digitalWrite(motor1a, m1a);
  digitalWrite(motor1b, m1b);
  digitalWrite(motor2a, m2a);
  digitalWrite(motor2b, m2b);
}

int ultrasonic_distance() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  return (duration * 0.034) / 2;
}

int laser_distance() {
  if (lox.isRangeComplete()) {
    return lox.readRange() / 10;  // Convert to cm
  }
}

int leftsee() {
  myservo.write(150); 
  delay(1000);
  distanceLeft = laser_distance();
  delay(100);
  myservo.write(74);  // Reset servo to center
  delay(1000);
  return distanceLeft;
}

int rightsee() {
  myservo.write(0);
  delay(1000);
  distanceRight = laser_distance(); 
  delay(100);
  myservo.write(74);  // Reset servo to center
  delay(1000);
  return distanceRight;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  SerialBT.begin("samurai_edge");

  //Voltage Reading
  pinMode(adcPin,INPUT);
  analogSetAttenuation(ADC_11db); // Set ADC range to 3.3V

  // Motor driver pins
  pinMode(motor1a, OUTPUT);
  pinMode(motor1b, OUTPUT);
  pinMode(motor2a, OUTPUT);
  pinMode(motor2b, OUTPUT);

  // Servo motor
  myservo.attach(servoPin);
  myservo.write(74);

  // Ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize VL53L0X
  if (!lox.begin()) {
    while (1) delay(1000);  // Halt execution
  }
  lox.startRangeContinuous();
}

void loop() {

  if (SerialBT.available()) {
    bt_data = SerialBT.read();
    Serial.write(bt_data);
  }

  int adcValue=analogRead(adcPin);
  float voltage=(adcValue/4095.0)*referenceVoltage*dividerRatio;
  int voltage_percent=(voltage-minVoltage)/(maxVoltage-minVoltage)*100;

  // Continuously check for obstacles
  sonic_distance = ultrasonic_distance();
  laserDist = laser_distance();

  int random = 1+(rand()%500);  

  // Debugging: Print sensor readings
  Serial.print("Ultrasonic Distance: ");
  Serial.println(sonic_distance);
  Serial.print("Laser Distance: ");
  Serial.println(laserDist);

  // Stop and avoid obstacles if detected
  if ((sonic_distance > 0 && sonic_distance < 15 ) || (laserDist > 0 && laserDist < 20)) {
    Serial.println("Obstacle detected! Stopping motors.");
    controlMotors(LOW, LOW, LOW, LOW);
    delay(200);

    // Check distances to the left and right
    int leftdistance = leftsee();
    int rightdistance = rightsee();

    Serial.print("Left Distance: ");
    Serial.println(leftdistance);
    Serial.print("Right Distance: ");
    Serial.println(rightdistance);

    if (leftdistance>=rightdistance) {
      // Turn left
      Serial.println("Turning left.");
      controlMotors(LOW, HIGH, HIGH, LOW);
      delay(800);
    } else {
      // Turn right
      Serial.println("Turning right.");
      controlMotors(HIGH, LOW, LOW, HIGH); 
      delay(800);
    }

    // Stop motors after turning
    controlMotors(LOW, LOW, LOW, LOW);
    delay(300);
  }
   else {
        if(random>0 && random<30){
          if(random<=10){
          controlMotors(HIGH, LOW, LOW, HIGH); //right
          delay(100);
          }
          else{
          controlMotors(LOW, HIGH, HIGH, LOW); //left
          delay(100);  
          } 
        }
        else{
          controlMotors(HIGH, LOW, HIGH, LOW); //forward
        }
      }

  // Send distance data via Bluetooth
  SerialBT.print("Battery Voltage: ");
  SerialBT.print(voltage);
  SerialBT.println(" V");

  SerialBT.print("Battery Percentage: ");
  SerialBT.print(voltage_percent);
  SerialBT.println(" %");

  delay(1000);
}
