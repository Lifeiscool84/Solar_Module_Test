#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <ArduinoBLE.h>
#include <Arduino.h>
#include <SparkFun_RV8803.h> // Include the SparkFun_RV8803 library
#include <SparkFun_u-blox_GNSS_v3.h>
#include <SparkFun_BNO08x_Arduino_Library.h>
#include "WDT.h"

RV8803 rtc; // Create an instance of the RV8803 class
/**
 * \brief
 */
SFE_UBLOX_GNSS myGNSS;
BNO08x bno085;

// For the most reliable interaction with the SHTP bus, we need
// to use hardware reset control, and to monitor the H_INT pin.
// The H_INT pin will go low when its okay to talk on the SHTP bus.
// Note, these can be other GPIO if you like.
// Define as -1 to disable these features.
#define BNO08X_INT  -1
#define BNO08X_RST  -1
#define BNO08X_ADDR 0x4A
#define BUFFER_SIZE 5000 // Define buffer size
#define myWire Wire1 // Define the I2C bus to use initialized I2C2 solder the pin6(SDA3) and pin7(SCL3)
#define gnssAddress 0x42 // Default the GNSS address

// Constants
const char* SERVICE_UUID = "0000";
const char* CHARACTERISTIC_UUID = "1111";
const int CHARACTERISTIC_MAX_LENGTH = 100;
const int DELAY_BETWEEN_PACKETS = 500; // milliseconds
const int DELAY_BETWEEN_ACK_CHECKS = 10; // milliseconds
const char* ACK_MESSAGE = "ACK";

// Global variables
BLEService testService(SERVICE_UUID);
BLEStringCharacteristic testCharacteristic(CHARACTERISTIC_UUID, BLERead | BLEWrite | BLENotify | BLEIndicate, CHARACTERISTIC_MAX_LENGTH);
File dataFile;
int fileNumber = 0;
long gpsReadInterval = -10000;
bool isStoringData = false; // Whether the device is currently storing data to the SD card
bool commandReceived = false; // Whether a command has been received from the central device "S"
bool isCommandSActive = false; // Whether the command "S" is active


//bn00858x variables
uint8_t eventIDType;
uint8_t eventID[]=
{
    SENSOR_REPORTID_ACCELEROMETER,
    SENSOR_REPORTID_GYROSCOPE_CALIBRATED,
    SENSOR_REPORTID_MAGNETIC_FIELD,
    SENSOR_REPORTID_LINEAR_ACCELERATION,
    SENSOR_REPORTID_ROTATION_VECTOR,
    SENSOR_REPORTID_GRAVITY,
    SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR,
    SENSOR_REPORTID_GYRO_INTEGRATED_ROTATION_VECTOR
};
uint16_t timeBetweenReports = 20; // bno085 report rate in ms
unsigned long lastUpdateTime = 0; // Last time the data was updated for GNSS timer
static uint64_t initialTimestamp = 0; // Initial timestamp from BNO085 sensor
bool isLoggingData = false; // Whether the device is currently logging data to the SD card
unsigned long initialMillis = 0; // Global variable to store the initial millis() value
unsigned long gnssUpdateInterval = 2000;//gnss update interval global variable default 2 seconds
unsigned long dataCollectionStartTime = 0; // Time when data collection started
unsigned long dataCollectionDuration = 10 * 60 * 1000; // Duration of data collection in milliseconds (default is 10 minutes)


//bno085 data struct
struct Data_2 {
    // Accelerometer, eventID = SENSOR_REPORTID_ACCELEROMETER
    float acc_x;
    float acc_y;
    float acc_z;
    uint8_t acc_acu{};

    // Gyroscope Calibrated, eventID = SENSOR_REPORTID_GYROSCOPE_CALIBRATED
    float gyro_cal_x;
    float gyro_cal_y;
    float gyro_cal_z;
    uint8_t gyro_cal_acu{};

    // Magnetic Field, eventID = SENSOR_REPORTID_MAGNETIC_FIELD
    float mag_x;
    float mag_y;
    float mag_z;
    uint8_t mag_acu{};

    // Linear Acceleration, eventID = SENSOR_REPORTID_LINEAR_ACCELERATION
    float lin_acc_x;
    float lin_acc_y;
    float lin_acc_z;
    uint8_t lin_acc_acu{};

    // Rotation Vector, eventID = SENSOR_REPORTID_ROTATION_VECTOR
    float quatI;
    float quatJ;
    float quatK;
    float quatReal;
    float quatRadianAccuracy{};
    uint8_t quatAccuracy{};

    // Gravity, eventID = SENSOR_REPORTID_GRAVITY
    float grav_x;
    float grav_y;
    float grav_z;
    uint8_t grav_acu{};

    // Geomagnetic Rotation Vector euler angles, eventID = SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR
    float roll; //later, make sure to: roll = (bno085.getRoll()) * 180.0 / PI; // Convert roll to degrees
    float pitch; //later, make sure to: pitch = (bno085.getPitch()) * 180.0 / PI; // Convert pitch to degrees
    float yaw; //later, make sure to: yaw= (bno085.getYaw()) * 180.0 / PI; // Convert yaw / heading to degrees

    // Gyro Integrated Rotation Vector, eventID = SENSOR_REPORTID_GYRO_INTEGRATED_ROTATION_VECTOR
    float RVI; //later, make sure to:RVI= bno085.getGyroIntegratedRVI();
    float RVJ; //later, make sure to:RVJ= bno085.getGyroIntegratedRVJ();
    float RVK; //later, make sure to:RVK= bno085.getGyroIntegratedRVK();
    float RVReal; //later, make sure to:RVReal= bno085.getGyroIntegratedRVReal();
    float gyroX; //later, make sure to:gyroX= bno085.getGyroIntegratedRVangVelX();
    float gyroY; //later, make sure to:gyroY= bno085.getGyroIntegratedRVangVelY();
    float gyroZ; //later, make sure to:gyroZ= bno085.getGyroIntegratedRVangVelZ();

    // Fields for the time data
    uint16_t year{};
    uint8_t month{};
    uint8_t day{};
    uint8_t hour{};
    uint8_t minute{};
    uint8_t second{};
    unsigned long Millisecond{};
    uint64_t timeStamp{};
    unsigned long timeLapse{};
    unsigned long lastUpdateTime{};
    uint64_t lastTimeStamp{};
    unsigned long millisTimestamp;

    // Validity flags for the sensor data
    bool acc_valid{};
    bool gyro_cal_valid{};
    bool mag_valid{};
    bool lin_acc_valid{};
    bool quat_valid{};
    bool grav_valid{};
    bool euler_valid{};
    bool gyro_integrated_valid{};

    Data_2(float acc_x, float acc_y, float acc_z, float gyro_cal_x, float gyro_cal_y, float gyro_cal_z, float mag_x,
    float mag_y, float mag_z, float lin_acc_x, float lin_acc_y, float lin_acc_z, float quat_i, float quat_j,
    float quat_k, float quat_real, float grav_x, float grav_y, float grav_z, float roll, float pitch, float yaw,
    float rvi, float rvj, float rvk, float rv_real, float gyro_x, float gyro_y, float gyro_z)
    : acc_x(acc_x),
      acc_y(acc_y),
      acc_z(acc_z),
      gyro_cal_x(gyro_cal_x),
      gyro_cal_y(gyro_cal_y),
      gyro_cal_z(gyro_cal_z),
      mag_x(mag_x),
      mag_y(mag_y),
      mag_z(mag_z),
      lin_acc_x(lin_acc_x),
      lin_acc_y(lin_acc_y),
      lin_acc_z(lin_acc_z),
      quatI(quat_i),
      quatJ(quat_j),
      quatK(quat_k),
      quatReal(quat_real),
      grav_x(grav_x),
      grav_y(grav_y),
      grav_z(grav_z),
      roll(roll),
      pitch(pitch),
      yaw(yaw),
      RVI(rvi),
      RVJ(rvj),
      RVK(rvk),
      RVReal(rv_real),
      gyroX(gyro_x),
      gyroY(gyro_y),
      gyroZ(gyro_z) {}
};

Data_2 dataBNO085(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); //initialize the struct

struct Data_1{
    uint8_t hour, minute, second, month, day, siv;
    uint16_t year;
    int32_t alt, speed, head, lat, lng;
    //constructore definition
    Data_1(uint8_t hour, uint8_t minute, uint8_t second, uint8_t month, uint8_t day, uint8_t siv, uint16_t year,
           int32_t alt, int32_t speed, int32_t head, int32_t lat, int32_t lng) :
            hour(hour), minute(minute), second(second), month(month), day(day), siv(siv), year(year),
            alt(alt), speed(speed), head(head), lat(lat), lng(lng) {}
};
Data_1 dataGNSS(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); //initialize the struct

// Function prototypes
void handleClient(BLEDevice central);
void listFiles(BLEDevice central);
void sendFile(BLEDevice central, String fileName);
void logData_2(const String& name, const Data_2* data);
void waitForAck(BLEDevice central);
void gnssData(String fileName);
void logData(const String& name, const Data_1* data);
String createGnssDataString(const Data_1* data);
template<typename T> String formatZero(const T &value);
String formatTimeValue(int value);
void setRTCTimeFromGNSS();
void setRTCTimeFromGNSS_local();
String formatTimestamp_bno085();
void bno085Data(const String& fileName);
void readSensorData(uint8_t eventIDType, const String& fileName);
String formatAccelData(float acc_x, float acc_y, float acc_z);
void rotateDataFile();

void setup() {
    Wire.begin(); // Initialize I2C1 connector pin
    myWire.begin(); // Initialize I2C2 connector pin
    Serial.begin(115200);
    //while (!Serial);

    // Initialize and configure the Watchdog Timer
    wdt.configure(WDT_1HZ, 30, 35); // Configure the Watchdog Timer with a 1Hz clock, 30 ticks for interrupt, and 35 ticks for reset
    wdt.start(); // Start the Watchdog Timer

    if (!BLE.begin()) {
        Serial.println("Starting BLE failed!");
        //while (1);
    }

    if (!SD.begin(8)) {
        Serial.println("Starting SD failed!");
        //while (1);
    }

    if (!rtc.begin()) {
        Serial.println("Starting RTC failed!");        //while (1);
    }

    if (!myGNSS.begin(myWire, gnssAddress)) //Connect to the u-blox module using Wire port and default I2C address
        Serial.println("u-blox GNSS not detected at default I2C address");

    if (!bno085.begin(BNO08X_ADDR, Wire, BNO08X_INT, BNO08X_RST)) {
        Serial.println("BNO08x not detected. Please check wiring. Freezing...");
    }

    //BNO08x Configuration
     bno085.enableAccelerometer(timeBetweenReports);
     bno085.enableGravity(timeBetweenReports);
     bno085.enableGyro(timeBetweenReports);
     bno085.enableLinearAccelerometer(timeBetweenReports);
     bno085.enableMagnetometer(timeBetweenReports);
     bno085.enableRotationVector(timeBetweenReports);
     bno085.enableGeomagneticRotationVector(timeBetweenReports);
//     bno085.enableGyroIntegratedRotationVector(timeBetweenReports);

    // GNSS Configuration
    myGNSS.setI2COutput(COM_TYPE_UBX);
    myGNSS.setMeasurementRate(500,VAL_LAYER_FLASH,2000); //rate (ms),config mode,maxWait(ms)
    myGNSS.setNavigationRate(1);
    myGNSS.setHighPrecisionMode(true);
    myGNSS.setDynamicModel(DYN_MODEL_PORTABLE);
    myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);

    // Set up the BLE characteristics
    BLE.setLocalName("CrocImuGpsTest");
    BLE.setAdvertisedService(testService);
    testService.addCharacteristic(testCharacteristic);
    BLE.addService(testService);
    BLE.advertise();

    //Set rtc to 24 hour mode
    rtc.set24Hour();
    Wire.setClock(400000); //Increase I2C data rate to 400kHz
    Serial.println("Bluetooth device active, waiting for connections...");
}


void loop() {
    wdt.restart();
    BLEDevice central = BLE.central();
    if (central) {
        handleClient(central);
    } else if (isStoringData) {
        // Write to file only when not connected and file is open
        if (dataFile) {
            gnssData(dataFile.name());
            bno085Data(dataFile.name());
            rotateDataFile(); // Rotate the data file if the duration has passed
        }
    } else {
        commandReceived = false; // Reset commandReceived when not connected and not storing data
    }
    wdt.restart();
}

void handleClient(BLEDevice central) {
    dataFile.close(); // Close the file when a central device connects
    Serial.print("Connected to central: ");
    Serial.println(central.address());

    isStoringData = false; // Stop storing data when a central device connects
    commandReceived = false; // Reset commandReceived when a central device connects
    isCommandSActive = false; // Reset the isCommandSActive flag when a central device connects

    // Print the menu options
String menuOptions = "Menu Options:\nL - List Files\nD - Deep Sleep Mode\nS - Start Data Collection\nC - Clear SD Card\nT - Set RTC Time From GNSS\nTL - Set RTC Time From GNSS Local\nR - Reboot and Restart the System\nDG - Disable GNSS Sensor\nDB - Disable BNO085 Sensor\nGU - Change GNSS Update Interval\n";
Serial.println(menuOptions);

    while (central.connected()) {
        wdt.restart(); // Reset the WDT when the central device is connected
        if (testCharacteristic.written()) {
            String command = testCharacteristic.value();
            if (command == "L") {
                listFiles(central);
            } else if (command == "D") {
                // Put the device into deep sleep mode
                //am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
            } else if (command == "S") {
                isCommandSActive = true;
                rtc.updateTime();
                String month = formatTimeValue(rtc.getMonth());
                String day = formatTimeValue(rtc.getDate());
                String hour = formatTimeValue(rtc.getHours());
                String minute = formatTimeValue(rtc.getMinutes());
                String fileName = month + day + hour + minute + ".txt";

                //Serial.print the fine name
                Serial.print("File name: ");
                Serial.println(fileName);

                dataFile = SD.open(fileName, FILE_WRITE);
                if (!dataFile) {
                    Serial.println("Failed to open file for writing");
                    return;
                }
                commandReceived = true; // Set commandReceived to true when "S" command is received
            } else if (command == "C") {
                // Clear the SD card
                File root = SD.open("/");
                if (root) {
                    File entry;
                    while (entry = root.openNextFile()) {
                        if (!entry.isDirectory()) {
                            SD.remove(entry.name());
                        }
                        entry.close();
                    }
                    root.close();
                }
            } else if (command == "T") {// Set RTC time from GNSS
                setRTCTimeFromGNSS();
            } else if (command == "TL") { // Set RTC time from GNSS local time
                setRTCTimeFromGNSS_local();
            } else if (command == "R") { //reboot and restart the system
                am_hal_reset_control(AM_HAL_RESET_CONTROL_SWPOI, NULL);
            } else if (command == "DG") {             //disable GNSS sensor
            myGNSS.powerOff(4294967290);
            }else if (command == "DB") {//disable BNO085 sensor
                bno085.modeSleep();
            } else if (command.startsWith("GU")) { //change the gnssUpdateInterval value of user defined value by using command function. lowest value is 100 ms (10 Hz) and high as it can be. GU and time should be ms unit.
                String intervalString = command.substring(2); // Remove the "GU" from the command
                gnssUpdateInterval = intervalString.toInt(); // Convert the remaining string to an integer and assign it to gnssUpdateInterval
            } else if (command.startsWith("DT")) { // Change the duration of data collection
                String durationString = command.substring(2); // Remove the "DT" from the command
                dataCollectionDuration = durationString.toInt() * 60 * 1000; // Convert the remaining string to an integer, convert it to milliseconds, and assign it to dataCollectionDuration
            }
           else {
                sendFile(central, command);
            }
        }
    }

    if (commandReceived) {
        isStoringData = true; // Start storing data when a central device disconnects
        dataCollectionStartTime = millis(); // Set the data collection start time
        if (isCommandSActive) {
            initialTimestamp = 0; // Reset initial timestamp
        }
    }
    Serial.print("Disconnected from central: ");
    Serial.println(central.address());
}

void rotateDataFile() {
    // Check if the current time is past the data collection start time plus the duration
    if (millis() - dataCollectionStartTime >= dataCollectionDuration) {
        dataFile.close(); // Close the current file

        // Open a new file
        rtc.updateTime();
        String month = formatTimeValue(rtc.getMonth());
        String day = formatTimeValue(rtc.getDate());
        String hour = formatTimeValue(rtc.getHours());
        String minute = formatTimeValue(rtc.getMinutes());
        String fileName = month + day + hour + minute + ".txt";
        dataFile = SD.open(fileName, FILE_WRITE);

        if (!dataFile) {
            Serial.println("Failed to open file for writing");
            return;
        }

        dataCollectionStartTime = millis(); // Update the data collection start time
    }
}

void listFiles(BLEDevice central) {
    File root = SD.open("/");
    if (!root) {
        Serial.println("Failed to open root directory");
        return;
    }

    File entry;
    while ((entry = root.openNextFile()) && testCharacteristic.subscribed()) {
        //reset the wdt timer
        wdt.restart(); //pet the dog
        if (!entry.isDirectory()) {
            if (testCharacteristic.writeValue(entry.name()) == 0) {
                Serial.println("Write to characteristic failed");
            }
            delay(DELAY_BETWEEN_PACKETS);
            waitForAck(central);
        }
        entry.close();
    }
    root.close();
}

void sendFile(BLEDevice central, String fileName) {
    File file = SD.open(fileName.c_str());
    if (!file) {
        Serial.println("Failed to open file: " + fileName);
        return;
    }

    while (file.available() && testCharacteristic.subscribed()) {
        wdt.restart(); //pet the dog
        String data = "";
        for (int i = 0; i < 20 && file.available(); i++) { // Adjusted to maximum send bytes
            char c = file.read();
            data += c;
        }
        if (testCharacteristic.writeValue(data) == 0) { // Send the data
            Serial.println("Write to characteristic failed");
        }
        delay(DELAY_BETWEEN_PACKETS);
        waitForAck(central);
    }
    file.close();
}

void waitForAck(BLEDevice central) {
    while (central.connected() && testCharacteristic.written() && testCharacteristic.value() != ACK_MESSAGE) {
        delay(DELAY_BETWEEN_ACK_CHECKS);
    }
}

/*Data units
myGNSS.getHour(): Returns the current hour in 24-hour format.
myGNSS.getMinute(): Returns the current minute.
myGNSS.getSecond(): Returns the current second.
myGNSS.getMonth(): Returns the current month (1-12).
myGNSS.getDay(): Returns the current day of the month (1-31).
myGNSS.getYear(): Returns the current year.
myGNSS.getSIV(): Returns the number of satellites used in fix.
myGNSS.getAltitudeMSL(): Returns the current altitude in millimeters according to mean sea level.
myGNSS.getGroundSpeed(): Returns the ground speed in millimeters per second.
myGNSS.getHeading(): Returns the heading of motion in degrees * 10^-5.
myGNSS.getLatitude(): Returns the current latitude in degrees *10^-7.
myGNSS.getLongitude(): Returns the current longitude in degrees *10^-7.
*/

void gnssData(String fileName) {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime >= gnssUpdateInterval) { // If defined time has passed (default 2 seconds)
        lastUpdateTime = currentTime; // Update the last update time
        if(myGNSS.getPVT() && myGNSS.getSIV() >1) { // If the GNSS has a fix and is using satellites
            rtc.updateTime();
            dataGNSS = Data_1(
                    myGNSS.getHour(), myGNSS.getMinute(), myGNSS.getSecond(),
                    myGNSS.getMonth(), myGNSS.getDay(), myGNSS.getSIV(),myGNSS.getYear(),
                    myGNSS.getAltitudeMSL(), myGNSS.getGroundSpeed(), myGNSS.getHeading(),
                    myGNSS.getLatitude(), myGNSS.getLongitude()
            );
            logData(fileName, &dataGNSS);
        }
    }
}

template<typename T>
String formatZero(const T &value) {
    return (value < 10) ? "0" + String(static_cast<unsigned long>(value)) : String(static_cast<unsigned long>(value));
}
//Using GNSS time data
String createGnssDataString(const Data_1* data) {
    String stringData = formatZero(data->month) + "/" + formatZero(data->day)    + "/" + formatZero(data->year)   + "," +
                        formatZero(data->hour)  + ":" + formatZero(data->minute) + ":" + formatZero(data->second) + "," +
                        String(data->alt) + "," + String(data->lat)   + "," + String(data->lng)    + "," +
                        String(data->head)  + "," + String(data->speed) + "," + String(data->siv) + ","
                        + formatZero(rtc.getHours())  + ":" + formatZero(rtc.getMinutes()) + ":" + formatZero(rtc.getSeconds()); // Added RTC time data
    return stringData;
}
//Using RTC time data for data timing synchronization
String createGnssDataString_rtc(const Data_1* data) {
    String stringData = "G," + formatZero(rtc.getMonth()) + "/" + formatZero(rtc.getDate())    + "/" + formatZero(rtc.getYear())   + "," +
                        formatZero(rtc.getHours())  + ":" + formatZero(rtc.getMinutes()) + ":" + formatZero(rtc.getSeconds()) + "," +
                            String(data->alt) + "," + String(data->lat)   + "," + String(data->lng)    + "," +
                            String(data->head)  + "," + String(data->speed) + "," + String(data->siv);
    return stringData;
}


void bno085Data(const String& fileName) {
    //if sensor even availabe
    if (bno085.getSensorEvent()) {
        //check event id
        uint8_t eventId = bno085.getSensorEventID();
        // Serial.print("1. Sensor Event Detected bno085Data : "); // Debugging
        // Serial.println(eventId); // Debugging
        // Serial.println("file Name: " + fileName); // Debugging
        // Serial.println(); // Debugging
        // perform actions based on event id
        switch (eventId) {
            case SENSOR_REPORTID_ACCELEROMETER:
                readSensorData(SENSOR_REPORTID_ACCELEROMETER, fileName);
            break;
            case SENSOR_REPORTID_GYROSCOPE_CALIBRATED:
                readSensorData(SENSOR_REPORTID_GYROSCOPE_CALIBRATED, fileName);
            break;
            case SENSOR_REPORTID_MAGNETIC_FIELD:
                readSensorData(SENSOR_REPORTID_MAGNETIC_FIELD, fileName);
            break;
            case SENSOR_REPORTID_LINEAR_ACCELERATION:
                readSensorData(SENSOR_REPORTID_LINEAR_ACCELERATION, fileName);
            break;
            case SENSOR_REPORTID_ROTATION_VECTOR:
                readSensorData(SENSOR_REPORTID_ROTATION_VECTOR,fileName);
            break;
            case SENSOR_REPORTID_GRAVITY:
                readSensorData(SENSOR_REPORTID_GRAVITY, fileName);
            break;
            case SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR:
                readSensorData(SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR, fileName);
            break;
            case SENSOR_REPORTID_GYRO_INTEGRATED_ROTATION_VECTOR:
                readSensorData(SENSOR_REPORTID_GYRO_INTEGRATED_ROTATION_VECTOR, fileName);
            break;
            default:
                break;
        }
    }
}

//store sensor data that is relevant to the event id from the bno085Data()
void readSensorData(uint8_t eventIDType,const String& fileName) {
    switch (eventIDType) {
        case SENSOR_REPORTID_ACCELEROMETER:
            //use Data_2 struct to store the data for getAccel function acc_x, acc_y, acc_z, acc_acu to the function units: m/s^2
                dataBNO085.acc_x = bno085.getAccelX();
        dataBNO085.acc_y = bno085.getAccelY();
        dataBNO085.acc_z = bno085.getAccelZ();
        //dataBNO085.millisTimestamp = millis();
        dataBNO085.acc_valid = true;
        //     Serial.print("2. Moved to function  readSensorData : "); // Debugging
        //     Serial.println("file Name: " + fileName); // Debugging
        //     Serial.print("Accel Data at void readSensorData: "); // Debugging
        //     Serial.print(dataBNO085.acc_x); Serial.print(", ");// Debugging
        //     Serial.print(dataBNO085.acc_y); Serial.print(", ");// Debugging
        //     Serial.println(dataBNO085.acc_z); // Debugging
        //     //check acc_valid
        //     Serial.print("Accel Valid check again: "); Serial.println(dataBNO085.acc_valid);// Debugging
        // Serial.println();// Debugging
        break;
        case SENSOR_REPORTID_GYROSCOPE_CALIBRATED:
            //use Data_2 struct to store the data for getGyro function gyro_cal_x, gyro_cal_y, gyro_cal_z to the function units: rad/s
                dataBNO085.gyro_cal_x = bno085.getGyroX();
        dataBNO085.gyro_cal_y = bno085.getGyroY();
        dataBNO085.gyro_cal_z = bno085.getGyroZ();
        //    dataBNO085.millisTimestamp = millis();
        dataBNO085.gyro_cal_valid = true;
        break;
        case SENSOR_REPORTID_MAGNETIC_FIELD:
            //use Data_2 struct to store the data for getMag function mag_x, mag_y, mag_z to the function units: uT
                dataBNO085.mag_x = bno085.getMagX();
        dataBNO085.mag_y = bno085.getMagY();
        dataBNO085.mag_z = bno085.getMagZ();
        //    dataBNO085.millisTimestamp = millis();
        dataBNO085.mag_valid = true;
        break;
        case SENSOR_REPORTID_LINEAR_ACCELERATION:
            //use Data_2 struct to store the data for getLinAccel function lin_acc_x, lin_acc_y, lin_acc_z to the function units: m/s^2
                dataBNO085.lin_acc_x = bno085.getLinAccelX();
        dataBNO085.lin_acc_y = bno085.getLinAccelY();
        dataBNO085.lin_acc_z = bno085.getLinAccelZ();
        //dataBNO085.millisTimestamp = millis();
        dataBNO085.lin_acc_valid = true;
        break;
        case SENSOR_REPORTID_ROTATION_VECTOR:
            //use Data_2 struct to store the data for getQuat function quatI, quatJ, quatK, quatReal to the function units: unitless
                dataBNO085.quatI = bno085.getQuatI();
        dataBNO085.quatJ = bno085.getQuatJ();
        dataBNO085.quatK = bno085.getQuatK();
        dataBNO085.quatReal = bno085.getQuatReal();
        //dataBNO085.millisTimestamp = millis();
        dataBNO085.quat_valid = true;
        break;
        case SENSOR_REPORTID_GRAVITY:
            //use Data_2 struct to store the data for getGravity function grav_x, grav_y, grav_z to the function units: m/s^2
                dataBNO085.grav_x = bno085.getGravityX();
        dataBNO085.grav_y = bno085.getGravityY();
        dataBNO085.grav_z = bno085.getGravityZ();
        //dataBNO085.millisTimestamp = millis();
        dataBNO085.grav_valid = true;
        break;
        case SENSOR_REPORTID_GEOMAGNETIC_ROTATION_VECTOR:
            //use Data_2 struct to store the data for getGeomag function geomag_x, geomag_y, geomag_z to the function units: degrees
                dataBNO085.roll = static_cast<float>((bno085.getRoll()) * 180.0 / PI); // Convert roll to degrees
        dataBNO085.pitch = static_cast<float>((bno085.getPitch()) * 180.0 / PI); // Convert pitch to degrees
        dataBNO085.yaw = static_cast<float>((bno085.getYaw()) * 180.0 / PI); // Convert yaw / heading to degrees
        //dataBNO085.millisTimestamp = millis();
        dataBNO085.euler_valid = true;
        break;
        case SENSOR_REPORTID_GYRO_INTEGRATED_ROTATION_VECTOR:
            // use Data_2 struct to store the data for getGyroIntegrated function RVI, RVJ, RVK, RVReal, gyroX, gyroY, gyroZ to the function units: unitless
                dataBNO085.RVI = bno085.getGyroIntegratedRVI();
        dataBNO085.RVJ = bno085.getGyroIntegratedRVJ();
        dataBNO085.RVK = bno085.getGyroIntegratedRVK();
        dataBNO085.RVReal = bno085.getGyroIntegratedRVReal();
        dataBNO085.gyroX = bno085.getGyroIntegratedRVangVelX();
        dataBNO085.gyroY = bno085.getGyroIntegratedRVangVelY();
        dataBNO085.gyroZ = bno085.getGyroIntegratedRVangVelZ();
        //dataBNO085.millisTimestamp = millis();
        dataBNO085.gyro_integrated_valid = true;
        break;
        default:
            break;
    }
    //use variables of initailMillis, resetinitialMillis, and currentMillis to store and calculate the time data
    // capture the current time right after the measurement is taken
    unsigned long currentMillis = millis();
    // Check if this is the first measurement
    if (initialMillis == 0) {
        // Update the RTC and store the current date and time in the dataBNO085 structure
        initialMillis = currentMillis;

        rtc.updateTime();
        dataBNO085.year = rtc.getYear();
        dataBNO085.month = rtc.getMonth();
        dataBNO085.day = rtc.getDate();
        dataBNO085.hour = rtc.getHours();
        dataBNO085.minute = rtc.getMinutes();
        dataBNO085.second = rtc.getSeconds();
        dataBNO085.Millisecond = 0; //First data will have 0 milliseconds and next data will be calculated based on initialMillis time

    } else {
        // Calculate the elapsed time in milliseconds
        dataBNO085.Millisecond = currentMillis - initialMillis;

        // Handle the rollover of milliseconds to seconds, seconds to minutes, and so on
        while (dataBNO085.Millisecond >= 1000) {
            dataBNO085.second++;
            dataBNO085.Millisecond -= 1000;
            initialMillis += 1000;
        }
        if (dataBNO085.second >= 60) {
            dataBNO085.minute++;
            dataBNO085.second -= 60;
            if (dataBNO085.minute >= 60) {
                dataBNO085.hour++;
                dataBNO085.minute -= 60;
                if (dataBNO085.hour >= 24) {
                    dataBNO085.day++;
                    dataBNO085.hour -= 24;
                    // Handle the rollover of days to months, months to years, and so on. based on the month, it shuld be either 30 or 31 days,leap year
                    if (dataBNO085.month == 2) {
                        if (dataBNO085.year % 4 == 0) {
                            if (dataBNO085.day > 29) {
                                dataBNO085.month++;
                                dataBNO085.day -= 29;
                            }
                        } else {
                            if (dataBNO085.day > 28) {
                                dataBNO085.month++;
                                dataBNO085.day -= 28;
                            }
                        }
                    } else if (dataBNO085.month == 4 || dataBNO085.month == 6 || dataBNO085.month == 9 || dataBNO085.month == 11) {
                        if (dataBNO085.day > 30) {
                            dataBNO085.month++;
                            dataBNO085.day -= 30;
                        }
                    } else {
                        if (dataBNO085.day > 31) {
                            dataBNO085.month++;
                            dataBNO085.day -= 31;
                        }
                    }
                }
            }
        }
    }

        logData_2(fileName, &dataBNO085);
        //Reset the validity flags to false after logging the data
        dataBNO085.acc_valid = false;
        dataBNO085.gyro_cal_valid = false;
        dataBNO085.mag_valid = false;
        dataBNO085.lin_acc_valid = false;
        dataBNO085.quat_valid = false;
        dataBNO085.grav_valid = false;
        dataBNO085.euler_valid = false;
        dataBNO085.gyro_integrated_valid = false;
}

String createBno085DataString_rtc(const Data_2* data) {
    // Serial.println("4. createBno085DataString_rtc: ");// Debugging
    // Serial.print("Check acc_valid in CreateBno085DataString_rtc: ");Serial.println(data->acc_valid);// Debugging
    String stringData = formatZero(dataBNO085.month) + "/" + formatZero(dataBNO085.day)    + "/" + formatZero(dataBNO085.year)   + "," +
                        formatZero(dataBNO085.hour)  + ":" + formatZero(dataBNO085.minute) + ":" + formatZero(dataBNO085.second) + ":" + formatZero(dataBNO085.Millisecond);
    auto floatToString = [](float value) {
        int intValue = static_cast<int>(value);
        int fracValue = static_cast<int>((value - intValue) * 100);
        return String(intValue) + "." + (fracValue < 10 ? "0" : "") + String(abs(fracValue));
    };

    if(data->acc_valid) {
        stringData = "ac," + stringData + "," + floatToString(data->acc_x) + "," + floatToString(data->acc_y) + "," + floatToString(data->acc_z);
        //stringData += "," + String(data->acc_x) + "," + String(data->acc_y)   + "," + String(data->acc_z);
        //stringData += formatAccelData(data->acc_x, data->acc_y, data->acc_z);
        // Serial.print("Accel Data inside of the if- statement of createBno085DataString_rtc: "); Serial.println(stringData);// Debugging
        // Serial.println();// Debugging
    }
    if(data->gyro_cal_valid) {
        stringData =  "gy," + stringData + "," + floatToString(data->gyro_cal_x) + "," + floatToString(data->gyro_cal_y) + "," + floatToString(data->gyro_cal_z);
    }
    if(data->mag_valid) {
        stringData = "mg," + stringData + "," + floatToString(data->mag_x) + "," + floatToString(data->mag_y) + "," + floatToString(data->mag_z);
    }
    if(data->lin_acc_valid) {
        stringData = "la," +stringData + "," + floatToString(data->lin_acc_x) + "," + floatToString(data->lin_acc_y) + "," + floatToString(data->lin_acc_z);
    }
    if(data->quat_valid) {
        stringData = "qu," + stringData + "," + floatToString(data->quatI) + "," + floatToString(data->quatJ) + "," + floatToString(data->quatK) + "," + floatToString(data->quatReal);
    }
    if(data->grav_valid) {
        stringData = "gr," + stringData + "," + floatToString(data->grav_x) + "," + floatToString(data->grav_y) + "," + floatToString(data->grav_z);
    }
    if(data->euler_valid) {
        stringData = "er,"+ stringData + "," +floatToString(data->roll) + "," + floatToString(data->pitch) + "," + floatToString(data->yaw);
    }
    if(data->gyro_integrated_valid) {
        stringData = "gi," + stringData + "," + floatToString(data->RVI) + "," + floatToString(data->RVJ) + "," + floatToString(data->RVK) + "," + floatToString(data->RVReal) + "," + floatToString(data->gyroX) + "," + floatToString(data->gyroY) + "," + floatToString(data->gyroZ);
    }
    return stringData;
}

String formatAccelData(float acc_x, float acc_y, float acc_z) {
    String acc_x_str = String(acc_x, 2);
    String acc_y_str = String(acc_y, 2);
    String acc_z_str = String(acc_z, 2);

    String formattedData = "," + acc_x_str + "," + acc_y_str + "," + acc_z_str;
    return formattedData;
}
//Gnss data storing SD card
void logData(const String& name, const Data_1* data) {
    if(dataFile) {
        dataFile.println(createGnssDataString_rtc(data));
        //dataFile.flush(); // Ensure data is written to the SD card
    }
    else {
        error("Error -> Couldn't write data to file!!");
    }
}

//BNO085 data storing SD card
void logData_2(const String& name, const Data_2* data) {
    if(dataFile) {
        dataFile.println(createBno085DataString_rtc(data));
        //dataFile.flush(); // Ensure data is written to the SD card
        // Serial.println("logData_2 (storing to SD card data format): ");// Debugging
        // Serial.println(createBno085DataString_rtc(data));// Debugging
        // Serial.println("-----------------------------------------------------------");// Debugging
        // Serial.println();// Debugging
    }
    else {
        error("Error -> Couldn't write data to file!!");
    }
}

String formatTimeValue(int value) {
    if (value < 10) {
        return "0" + String(value);
    } else {
        return String(value);
    }
}

void setRTCTimeFromGNSS() {
    unsigned long startTime = millis();
    while(!myGNSS.getPVT()) { // Keep looping until GNSS has a valid fix and valid time
        if(millis() - startTime > 5000) { // If 5 seconds have passed without a valid fix
            Serial.println("Failed to sync RTC with GNSS within 5 seconds.");
            return;
        }
    }
    // If we reach here, it means we have a valid GNSS fix
    // Get the current GNSS time
    uint8_t hour = myGNSS.getHour();
    uint8_t minute = myGNSS.getMinute();
    uint8_t second = myGNSS.getSecond();
    uint8_t day = myGNSS.getDay();
    uint8_t month = myGNSS.getMonth();
    uint16_t year = myGNSS.getYear();
    uint8_t weekday = 0; // not going to use

    // Set the RTC time using the GNSS time
    rtc.setTime(second, minute, hour, weekday, day, month, year);
    Serial.println("Successfully synced RTC with GNSS.");
}

void setRTCTimeFromGNSS_local() {
    unsigned long startTime = millis();
    while(!myGNSS.getPVT()) { // Keep looping until GNSS has a valid fix and valid time
        if(millis() - startTime > 5000) { // If 5 seconds have passed without a valid fix
            Serial.println("Failed to sync RTC with GNSS within 5 seconds.");
            return;
        }
    }
    // If we reach here, it means we have a valid GNSS fix
    // Get the current GNSS time
    int hour = myGNSS.getHour();
    uint8_t minute = myGNSS.getMinute();
    uint8_t second = myGNSS.getSecond();
    uint8_t day = myGNSS.getDay();
    uint8_t month = myGNSS.getMonth();
    uint16_t year = myGNSS.getYear();
    uint8_t weekday = 0; // not going to use

    // Adjust the hour for the local timezone
    hour -= 6;
    if(hour < 0) {
        hour += 24;
        day--;
        if(day < 1) {
            month--;
            if(month < 1) {
                month = 12;
                year--;
            }
            // Adjust the day based on the month
            if(month == 2) {
                if(year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
                    day = 29;
                } else {
                    day = 28;
                }
            } else if(month == 4 || month == 6 || month == 9 || month == 11) {
                day = 30;
            } else {
                day = 31;
            }
        }
    }

    // Compensate for the one-second delay
    second++;
    if (second >= 60) {
        second = 0;
        minute++;
        if (minute >= 60) {
            minute = 0;
            hour++;
        }
    }

    // Set the adjusted time to the RTC
    rtc.setTime(second, minute, hour, 0 /* weekday */, day, month, year);
    Serial.println("Successfully synced RTC with local time.");
}
