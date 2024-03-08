#include <Arduino.h>
#include <Wire.h>
#include <Dps3xx.h>
#include "I2Cdev.h"
#include "MPU6050.h"


// TODO: Pressuresensor: The 0 Ohm resistors are soldered as shown on the right picture
// TODO: Create a pull-up resistor for I2C bus


// Change debug mode | CHANGE TO FALSE WHEN NO COMPUTER CONNECTED
#define DEBUG TRUE



// ========= Constants ==========

// Sensor adresses for I2C
#define IMU_ADR 0x68 //b1101000
//#define PRESSURE_SENSOR_ADR 0x77 // Default and does not need to be given

// Baudrate for serial communication to terminal on computer
#define BAUDRATE 250000 

// LED pin on microcontroller
#define LED_PIN 13

// Timeout to wait before skipping a task
#define TIMEOUT_DURATION 15000000 // 15 seconds



// ========= Variables/Objects ==========

// Pressure sensor object
Dps3xx Dps3xxPressureSensor = Dps3xx();

/*
  * temperature measure rate (value from 0 to 7)
  * 2^temp_mr temperature measurement results per second
  */
int16_t temp_mr = 2;

/*
  * temperature oversampling rate (value from 0 to 7)
  * 2^temp_osr internal temperature measurements per result
  * A higher value increases precision
  */
int16_t temp_osr = 2;
 
/*
  * pressure measure rate (value from 0 to 7)
  * 2^prs_mr pressure measurement results per second
  */
int16_t prs_mr = 2;

/*
  * pressure oversampling rate (value from 0 to 7)
  * 2^prs_osr internal pressure measurements per result
  * A higher value increases precision
  */
int16_t prs_osr = 2;

// IMU object
MPU6050 accelgyro;

// IMU sensor data
int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t mx, my, mz;

// LED pin state
bool blinkState = false;



// ========= Functions ==========

// Initialize pressure sensor
void initDPS310(){

  #if DEBUG
    Serial.println("Initializing Pressuresensor over I2C...");
  #endif
  Dps3xxPressureSensor.begin(Wire);
  #if DEBUG
    Serial.println("Pressuresensor I2C initialized successfully");
  #endif

  /*
   * startMeasureBothCont enables background mode
   * temperature and pressure ar measured automatically
   * High precision and hgh measure rates at the same time are not available.
   * Consult Datasheet (or trial and error) for more information
   */
  int16_t ret = Dps3xxPressureSensor.startMeasureBothCont(temp_mr, temp_osr, prs_mr, prs_osr);
  
  /*
   * Use one of the commented lines below instead to measure only temperature or pressure
   * int16_t ret = Dps3xxPressureSensor.startMeasureTempCont(temp_mr, temp_osr);
   * int16_t ret = Dps3xxPressureSensor.startMeasurePressureCont(prs_mr, prs_osr);
   */
  #if DEBUG
  if (ret != 0)
  {
    Serial.print("Init Dsp310 FAILED! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.println("Init Dsp310 complete!");
  }
  #else
  if (ret != 0)
  {
    // TODO: Initialization failed, send error to computer
  }
  #endif
}

// Initialize IMU
void initIMU(){

 /*  Serial.println("Initializing IMU over I2C...");
  accelgyro.initialize();
  Serial.println("IMU I2C initialized successfully");

  // Verify connection
  Serial.println("Testing IMU connection...");
  Serial.println(accelgyro.testConnection() ? "IMU connection successful" : "IMU connection failed"); */

  Serial.println("Initializing IMU over I2C...");
  unsigned long startTime = millis();
  bool isInitialized = false;

  while(millis() - startTime < TIMEOUT_DURATION) {
    if(accelgyro.testConnection()) {
      accelgyro.initialize();
      isInitialized = true;
      break;
    }
    delay(100); // delay in between attempts
  }

  if (!isInitialized) {
    // Initialization failed, handle accordingly
    // TODO: Send error message to computer
  }
}

// Read sensor data from I2C bus
void readIMU(){
  // read raw accel/gyro measurements from device
  accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);

  // these methods are also available
  //accelgyro.getAcceleration(&ax, &ay, &az);
  //accelgyro.getRotation(&gx, &gy, &gz);

  // display tab-separated accel/gyro x/y/z values
  Serial.print("a/g/m:\t");
  Serial.print(ax); Serial.print("\t");
  Serial.print(ay); Serial.print("\t");
  Serial.print(az); Serial.print("\t");
  Serial.print(gx); Serial.print("\t");
  Serial.print(gy); Serial.print("\t");
  Serial.print(gz); Serial.print("\t");
  Serial.print(mx); Serial.print("\t");
  Serial.print(my); Serial.print("\t");
  Serial.println(mz);  
}

// Read temperature and pressure
void readPS(){

  uint8_t pressureCount = 20;
  float pressure[pressureCount];
  uint8_t temperatureCount = 20;
  float temperature[temperatureCount];

  /*
   * This function writes the results of continuous measurements to the arrays given as parameters
   * The parameters temperatureCount and pressureCount should hold the sizes of the arrays temperature and pressure when the function is called
   * After the end of the function, temperatureCount and pressureCount hold the numbers of values written to the arrays
   * Note: The Dps3xx cannot save more than 32 results. When its result buffer is full, it won't save any new measurement results
   */
  int16_t ret = Dps3xxPressureSensor.getContResults(temperature, temperatureCount, pressure, pressureCount);

  #if DEBUG
  if (ret != 0)
  {
    Serial.println();
    Serial.println();
    Serial.print("FAIL! ret = ");
    Serial.println(ret);
  }
  else
  {
    Serial.println();
    Serial.println();
    Serial.print(temperatureCount);
    Serial.println(" temperature values found: ");
    for (int16_t i = 0; i < temperatureCount; i++)
    {
      Serial.print(temperature[i]);
      Serial.println(" degrees of Celsius");
    }

    Serial.println();
    Serial.print(pressureCount);
    Serial.println(" pressure values found: ");
    for (int16_t i = 0; i < pressureCount; i++)
    {
      Serial.print(pressure[i]);
      Serial.println(" Pascal");
    }
  }
  #else
  // In non-debug mode we can just
  if (ret != 0)
  {
    // TODO: Send error message to computer
  }
  else
  {
    // TODO: Save data in variables to LQR
    Serial.print(temperatureCount);
    for (int16_t i = 0; i < temperatureCount; i++)
    {
      Serial.print(temperature[i]);
    }

    Serial.print(pressureCount);
    for (int16_t i = 0; i < pressureCount; i++)
    {
      Serial.print(pressure[i]);
    }
  }

  #endif

  //Wait some time, so that the Dps310 can refill its buffer
  delay(10000);
}




// ========= Setup ==========

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(BAUDRATE); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  #if DEBUG
  Serial.println("==== Starship model initializing... ====");
  Serial.println("");
  Serial.println("Initializing I2C bus...");
  #endif

  // Initialize I2C bus
  Wire.begin();

  // Initialize sensors
  initIMU();
  initDPS310();

  // Configure microcontroller LED for TX/RX status
  pinMode(LED_PIN, OUTPUT);

  #if DEBUG
  Serial.println("Init complete!");
  #endif
}



// ========= Loop ==========

void loop() {
  readIMU();
  readPS();

  // Blink LED to indicate activity
  blinkState = !blinkState;
  digitalWrite(LED_PIN, blinkState);

}
