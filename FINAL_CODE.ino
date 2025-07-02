#include <ezButton.h>
#include <AccelStepper.h>

// Motor 1 pin definitions
#define motor1DirPin 5
#define motor1StepPin 3
#define motor1EnPin 6

// Motor 2 pin definitions
#define motor2DirPin 41
#define motor2StepPin 39
#define motor2EnPin 43

// Motor 3 pin definitions
#define motor3DirPin 11
#define motor3StepPin 10
#define motor3EnPin 12

// Motor 4 pin definitions Extra
#define motor4DirPin 47
#define motor4StepPin 49
#define motor4EnPin 51

#define switchPin 31
#define switchPin2 8

// Extra
#define switchPin3 32
#define switchPin4 33
#define switchPin5 34

bool motor1Direction = true; // Variable to track motor direction
bool motor2Direction = true; // Variable to track motor direction
bool motor3Direction = true; // Variable to track motor direction

// Create AccelStepper object for motor
AccelStepper motor1(AccelStepper::DRIVER, motor1StepPin, motor1DirPin);

// Create AccelStepper object for motor
AccelStepper motor2(AccelStepper::DRIVER, motor2StepPin, motor2DirPin);

// Create AccelStepper object for motor
AccelStepper motor3(AccelStepper::DRIVER, motor3StepPin, motor3DirPin);

ezButton limitSwitch1(switchPin);
ezButton limitSwitch2(switchPin2);
ezButton limitSwitch3(switchPin3);
ezButton limitSwitch4(switchPin4);

// UART Parsing

#define BUFFER_SIZE 9
uint8_t rx_buffer[BUFFER_SIZE];
uint8_t buffer_index = 0; // Tracks the index of received bytes
uint16_t VP_Address;
//char Page_Change[10] = {0x5A,0XA5,0X07,0X82,0X00,0X84,0X5A,0X01,0X00,0X01};

// Movement parameters for motor 1
const int maxSpeed1 = 800;
const int acceleration1 = 100;
int speedMotor1 = 150; // Constant high speed for motor 1

// Movement parameters for motor 2
const int maxSpeed2 = 1500;
const int acceleration2 = 400;
int speedMotor2 = 800; // Constant high speed for motor 2

// Parameters for Linear Guide Motor direction change
unsigned long motor2PreviousMillis = 0;
const unsigned long motor2ChangeDirectionInterval = 5000; // Time in milliseconds (5 seconds)
bool motor2ChangeDirection = true; // true for forward, false for reverse

// Parameters for motor 3 gradual speed increase
const int stepsPerRevolution = 200; // Adjust based on your motor
const int microstepping = 8;        // Adjust based on your driver
unsigned long previousMillis = 0;   // Last update time for motor 3 speed
const long interval = 200;          // Speed update interval for motor 3
float motor3Speed = 0;              // Current speed for motor 3 (steps/sec)
const int maxSpeed3 = 1500;         // Max speed for motor 3
const int acceleration3 = 400;// Constant high speed for motor 3

const int mosfetPin = 13;

// Timing for linear guide motor switching
//unsigned long linearPrevMillis = 0;
//const long linearInterval = 2000;  
//bool linearState = false;          


// Timing for 1-second timer
unsigned long startTime = 0; // To track the start time of the timer
bool timerActive = false;    // To indicate if the timer is active
const long timerDuration = 60000; // 1 minute in milliseconds

void setup() {
  Serial1.begin(115200);
  Serial.begin(115200); // Debugging on Serial monitor

  // Configure motor 1
  motor1.setMaxSpeed(maxSpeed1);
  motor1.setAcceleration(acceleration1);
  motor1.setSpeed(speedMotor1);

  pinMode(motor1EnPin, OUTPUT);
  digitalWrite(motor1EnPin, 0); // Initially disable motor

  // Configure motor 2
  motor2.setMaxSpeed(maxSpeed2);
  motor2.setAcceleration(acceleration2);
  motor2.setSpeed(speedMotor2);

  pinMode(motor2EnPin, OUTPUT);
  digitalWrite(motor2EnPin, 0); // Initially disable motor

  //Configure motor 3
  motor3.setMaxSpeed(maxSpeed3);
  motor3.setAcceleration(acceleration3);
  pinMode(motor3EnPin, OUTPUT);
  digitalWrite(motor3EnPin, 0); // Initially disable motor

  // Configure the switches
  limitSwitch1.setDebounceTime(50); // Set debounce time to 50 milliseconds
  limitSwitch2.setDebounceTime(50); 
  limitSwitch3.setDebounceTime(50); 
  limitSwitch4.setDebounceTime(50); 

  // Set the MOSFET pin as output
  pinMode(mosfetPin, OUTPUT);
  digitalWrite(mosfetPin, LOW);

  Serial.println("Setup Complete");
}

void loop() {
  unsigned long currentMillis = millis();
  // Update switch state
  limitSwitch1.loop();
  limitSwitch2.loop();

  if (timerActive && (currentMillis - startTime >= timerDuration)) {
    stopAllMotors();
    timerActive = false;
    Serial.println("Timer expired: All motors stopped");
  }


   if (digitalRead(motor1EnPin) == HIGH) {
    motor1.runSpeed();
  }
  else {
    motor1.setSpeed(0);
  }

  // Run motor 2 if enabled
  if (digitalRead(motor2EnPin) == HIGH) {
   // if (currentMillis - motor2PreviousMillis >= motor2ChangeDirectionInterval) {
    //  motor2PreviousMillis = currentMillis;

      // Toggle direction
    //  motor2ChangeDirection = !motor2ChangeDirection;
   //   motor2.setSpeed(motor2ChangeDirection ? -speedMotor2 : speedMotor2);
      //Serial.print("Motor 2 Direction changed: ");
      //Serial.println(motor2ChangeDirection ? "Forward" : "Reverse");
   // }
    motor2.runSpeed();
  } else {
    motor2.setSpeed(0); // Ensure motor stops when disabled
  }

// Motor 3 gradual speed increase logic
  if (digitalRead(motor3EnPin) == HIGH) {
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      if (motor3Speed < maxSpeed3) {
        motor3Speed += 100; // Gradual increase in speed
        motor3.setSpeed(motor3Speed);

        // Calculate and display RPM for debugging
        float rpm = (motor3Speed * 60.0) / (stepsPerRevolution * microstepping);
       // Serial.print("Motor 3 Speed: ");
       // Serial.print(motor3Speed);
       // Serial.print(" steps/sec, ");
       // Serial.print("RPM: ");
       // Serial.println(rpm);
      }
    }
    motor3.runSpeed();
  } else {
    motor3Speed = 0; // Reset speed if motor 3 is disabled
    motor3.setSpeed(0);
  }

  // Handle UART data
  while (Serial1.available()) {
    int byteReceived = Serial1.read();
    Serial.print(byteReceived,HEX);
    Serial.println(byteReceived);

    // Add byte to buffer
    rx_buffer[buffer_index++] = byteReceived;

    // Check if buffer is full
    if (buffer_index == BUFFER_SIZE) {
      buffer_index = 0; // Reset buffer index

      // Debug: Print full buffer
      Serial.print("Full Buffer: ");
      for (int i = 0; i < BUFFER_SIZE; i++) {
        Serial.print("0x");
        Serial.print(rx_buffer[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      // Check for VP_Address in all valid positions
      for (int i = 0; i <= BUFFER_SIZE - 2; i++) {
        VP_Address = (rx_buffer[i] << 8) | rx_buffer[i + 1];
        Serial.print("VP_Address: ");
        Serial.println(VP_Address, HEX);

        // Process VP_Address if it matches a known value
        if (VP_Address == 0x2030 || VP_Address == 0x5015 || VP_Address == 0x2910) {
          handleVPAddress(VP_Address, rx_buffer[8]); // Pass address and command byte
        }
       /* else if (VP_Address == 0x5015) {
          handleVPAddress(VP_Address, rx_buffer[8]); // Pass address and command byte
        }
        else if(VP_Adress == 0x2910) {
          handleVPAddress(VP_Address, rx_buffer[8]); // Pass address and command byte
        }*/
      }

      // Clear buffer for next command
      memset(rx_buffer, 0, BUFFER_SIZE);
    }
  }

  // Run motor at constant speed
  motor1.runSpeed();
  motor2.runSpeed();
  motor3.runSpeed();



  if (limitSwitch1.isPressed()) {
    Serial.println("Motor 1: Enable and Anti-clockwise");
    digitalWrite(motor1EnPin, 1); // Enable motor
    motor1.setSpeed(-speedMotor1); // Set speed anticlockwise
    motor1Direction = false;
    digitalWrite(mosfetPin,LOW);
  }
  else if (limitSwitch2.isPressed()) {
    Serial.println("Motor 1: Stop");
    digitalWrite(motor1EnPin, 0); // Enable motor
    motor1.stop(); // Set speed anticlockwise
    motor1.setSpeed(0);
  }
  
  if(limitSwitch3.isPressed()){
    Serial.println("Motor 2: Anticlockwise");
    digitalWrite(motor2EnPin, 1);
    motor2.setSpeed(-speedMotor2);
    motor2Direction = false;
  }
  else if (limitSwitch4.isPressed()){
    Serial.println("Motor 2: Clocwise");
    digitalWrite(motor2EnPin, 1);
    motor2.setSpeed(speedMotor2);
    motor2Direction = true;
  }
    //motor1Direction = false;
}


void handleVPAddress(uint16_t address, uint8_t command) {
  if (address == 0x2030) {
        delay(50);
        Serial.println("Motor 1: Start");
        motor1.run();
        digitalWrite(motor1EnPin, 1); // Enable motor with switches
        motor1.setSpeed(speedMotor1); 
        motor1Direction = true;
        Serial.println("Motor 2: Start");
        motor2.run();
        digitalWrite(motor2EnPin, 1); // Enable Linear Guide Motor
        motor2.setSpeed(speedMotor2); 
        motor2Direction = true;
        Serial.println("Motor 3: Start");
        motor3.run();
        digitalWrite(motor3EnPin, 1); // Enable Motor with Gradual Acceleration
        motor3.setSpeed(0); 
        motor3Direction = true;
        digitalWrite(mosfetPin,HIGH);

        // Start the timer
        startTime = millis();
        timerActive = true;

  }
  else if (address == 0x5015) {
        delay(50);
        stopAllMotors();
        timerActive = false;    
    }
  
  //else if (address == 0x2910)
  /*{
      // ADD additional Functionality For Homing
      delay(50);
      timerActive = false;
      digitalWrite(motor1EnPin, 1); // Enable motor
      motor1.setSpeed(-speedMotor1); // Set speed anticlockwise
      motor1Direction = false;
      digitalWrite(motor2EnPin, 1); // Enable motor
      motor1.setSpeed(-speedMotor2); // Set speed anticlockwise
      motor2Direction = false;
      if (limitSwitch4.isPressed()){
        Serial.println("Motor 2: Stop");
        digitalWrite(motor2EnPin, 0);
        motor2.stop();
        motor2.setSpeed(0);
      }
      timerActive = false;
  }*/

}


void stopAllMotors() {
  Serial.println("Stopping all motors");

  motor1.stop();
  digitalWrite(motor1EnPin, 0);
  motor1.setSpeed(0);

  motor2.stop();
  digitalWrite(motor2EnPin, 0);
  motor2.setSpeed(0);

  motor3.stop();
  digitalWrite(motor3EnPin, 0);
  motor3.setSpeed(0);

  digitalWrite(mosfetPin, LOW);
}


