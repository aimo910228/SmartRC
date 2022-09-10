
#include "Wire.h"

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

#include <PubSubClient.h>
#include <WiFi.h>
#include <time.h>

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;
I2Cdev I2C_M;

uint8_t buffer_m[6];

int16_t ax, ay, az;
int16_t gx, gy, gz;
int16_t mx, my, mz;

float heading;
float tiltheading;

float Axyz[6] = { 0, 0, 0, 0, 0, 0 };
float dAxyz[3] = { 0, 0, 0 };
float Gxyz[3];
float Mxyz[3];

float SVM = 0;
float ASVM = 0;
float GSVM = 0;
float MSVM = 0;

int afterDroptime = 0;

#define sample_num_mdate 5000

volatile float mx_sample[3];
volatile float my_sample[3];
volatile float mz_sample[3];

static float mx_centre = 0;
static float my_centre = 0;
static float mz_centre = 0;

volatile int mx_max = 0;
volatile int my_max = 0;
volatile int mz_max = 0;

volatile int mx_min = 0;
volatile int my_min = 0;
volatile int mz_min = 0;

// Update these with values suitable for your network.
const char* ssid = "NKUST_Aimo"; // WiFi ssid
const char* password = "0931992966"; // WiFi pwd
// "Aimo" "C217_VPN" WiFi ssid
// "45556140" "" WiFi pwd

const char* ntpServer = "time.google.com";
const long gmtoffset_sec = 8 * 3600;
const int daylightoffset_sec = 0;

const char* mqtt_server = "mqtt.54ucl.com";
const char* user_name = "flask"; // 連接 MQTT broker 的帳號密碼
const char* user_password = "flask";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

// MSG Len
#define MSG_BUFFER_SIZE (255)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup()
{
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(115200);

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    configTime(gmtoffset_sec, daylightoffset_sec, ntpServer);
    printLocalTime();

    // initialize device
    while (!Serial)
        ;
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    // Serial.println(accelgyro.testConnection() ? "MPU6050 connection
    // successful" : "MPU6050 connection failed");

    Mxyz_init_calibrated();
    delay(5000);
}

void loop()
{
    getAccel_Data();
    getGyro_Data();
    getCompassDate_calibrated();
    /* compass data has been calibrated here before we use this function
    we should run 'getCompassDate_calibrated()' frist, so that we can get calibrated data ,
    then we can get correct angle . */
    getHeading();
    getTiltHeading();

    /*
    Serial.print("calibration parameter: ");
    Serial.print(mx_centre);    Serial.print(", ");
    Serial.print(my_centre);    Serial.print(", ");
    Serial.println(mz_centre);
    Serial.print(); Serial.println();
    Serial.print("Acceleration(g) of X,Y,Z: ");
    Axyz[0] ", " Axyz[1] ", " Axyz[2]
    Serial.print("Gyro(degrees/s) of X,Y,Z: ");
    Gxyz[0] ", " Gxyz[1] ", " Gxyz[2]
    Serial.print("Compass Value of X,Y,Z: ");
    Mxyz[0] ", " Mxyz[1] ", " Mxyz[2]
    Serial.print("The clockwise angle between the magnetic north and X-Axis: ");
    Serial.println(heading);
    Serial.print("The clockwise angle between the magnetic north and the
    projection of the positive X-Axis in the horizontal plane: ");
    Serial.println(tiltheading);
    */

    // Serial.print(String("A: ") + "" + Axyz[0] + ", " + Axyz[1] + ", " + Axyz[2] + ", ");
    // Serial.print(String("dA: ") + "" + dAxyz[0] + ", " + dAxyz[1] + ", " + dAxyz[2] + ", ");
    // Serial.print(String("G: ") + "" + Gxyz[0] + ", " + Gxyz[1] + ", " + Gxyz[2] + ", ");
    // Serial.print(String("M: ") + "" + Mxyz[0] + ", " + Mxyz[1] + ", " + Mxyz[2] + ", ");

    SVM = sqrt(pow(dAxyz[0], 2) + pow(dAxyz[0], 2) + pow(dAxyz[0], 2));
    GSVM = sqrt(pow(Gxyz[0], 2) + pow(Gxyz[0], 2) + pow(Gxyz[0], 2));
    // Serial.print(String("SVM: ") + SVM + ", " + GSVM + ", ");

    if (afterDroptime != 0) {
        Serial.print(afterDroptime);
        afterDroptime += 1;
        if (afterDroptime == 30) {
            afterDroptime = 0;
            mqttPublish();
        }
    }

    if (GSVM >= 25) {
        Serial.print("Gdrop ");
        afterDroptime = 0;
    }

    if (SVM >= 0.8) {
        Serial.print("drop ");
        afterDroptime = 1;
    }
    Serial.println(String(".") + afterDroptime);
    delay(300);
}

void getHeading(void)
{
    heading = 180 * atan2(Mxyz[1], Mxyz[0]) / PI;
    if (heading < 0) {
        heading += 360;
    }
}

void getTiltHeading(void)
{
    float pitch = asin(-Axyz[0]);
    float roll = asin(Axyz[1] / cos(pitch));

    float xh = Mxyz[0] * cos(pitch) + Mxyz[2] * sin(pitch);
    float yh = Mxyz[0] * sin(roll) * sin(pitch) + Mxyz[1] * cos(roll) - Mxyz[2] * sin(roll) * cos(pitch);
    float zh = -Mxyz[0] * cos(roll) * sin(pitch) + Mxyz[1] * sin(roll) + Mxyz[2] * cos(roll) * cos(pitch);
    tiltheading = 180 * atan2(yh, xh) / PI;
    if (yh < 0) {
        tiltheading += 360;
    }
}

void Mxyz_init_calibrated()
{
    Serial.println(
        F("Before using 9DOF,we need to calibrate the compass frist,It will "
          "takes about 2 minutes. \nDuring calibratting ,you should rotate and turn the 9DOF all the "
          "time within 2 minutes. \nIf you are ready ,please sent a command data 'ready' to start sample and calibrate."));
    // while (!Serial.find("ready"));
    Serial.println("ready");
    Serial.println("Sample starting......");
    Serial.println("waiting......");

    get_calibration_Data();

    Serial.print("compass calibration parameter: ");
    Serial.print(mx_centre);
    Serial.print(", ");
    Serial.print(my_centre);
    Serial.print(", ");
    Serial.println(mz_centre);
}

void get_calibration_Data()
{
    for (int i = 0; i < sample_num_mdate; i++) {
        get_one_sample_date_mxyz();
        /*
            Serial.print(mx_sample[2]);
            Serial.print(" ");
            Serial.print(my_sample[2]);                            //you can see
           the sample data here . Serial.print(" ");
            Serial.println(mz_sample[2]);
        */

        if (mx_sample[2] >= mx_sample[1]) {
            mx_sample[1] = mx_sample[2];
        }
        if (my_sample[2] >= my_sample[1]) {
            my_sample[1] = my_sample[2]; // find max value
        }
        if (mz_sample[2] >= mz_sample[1]) {
            mz_sample[1] = mz_sample[2];
        }

        if (mx_sample[2] <= mx_sample[0]) {
            mx_sample[0] = mx_sample[2];
        }
        if (my_sample[2] <= my_sample[0]) {
            my_sample[0] = my_sample[2]; // find min value
        }
        if (mz_sample[2] <= mz_sample[0]) {
            mz_sample[0] = mz_sample[2];
        }
    }

    mx_max = mx_sample[1];
    my_max = my_sample[1];
    mz_max = mz_sample[1];

    mx_min = mx_sample[0];
    my_min = my_sample[0];
    mz_min = mz_sample[0];

    mx_centre = (mx_max + mx_min) / 2;
    my_centre = (my_max + my_min) / 2;
    mz_centre = (mz_max + mz_min) / 2;
}

void get_one_sample_date_mxyz()
{
    getCompass_Data();
    mx_sample[2] = Mxyz[0];
    my_sample[2] = Mxyz[1];
    mz_sample[2] = Mxyz[2];
}

void getAccel_Data(void)
{
    accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
    Axyz[3] = Axyz[0];
    Axyz[4] = Axyz[1];
    Axyz[5] = Axyz[2];
    Axyz[0] = (double)ax / 16384;
    Axyz[1] = (double)ay / 16384;
    Axyz[2] = (double)az / 16384;
    dAxyz[0] = abs(Axyz[0]) - abs(Axyz[3]);
    dAxyz[1] = abs(Axyz[1]) - abs(Axyz[4]);
    dAxyz[2] = abs(Axyz[2]) - abs(Axyz[5]);
}

void getGyro_Data(void)
{
    accelgyro.getMotion9(&ax, &ay, &az, &gx, &gy, &gz, &mx, &my, &mz);
    Gxyz[0] = (double)gx * 250 / 32768;
    Gxyz[1] = (double)gy * 250 / 32768;
    Gxyz[2] = (double)gz * 250 / 32768;
}

void getCompass_Data(void)
{
    I2C_M.writeByte(MPU9150_RA_MAG_ADDRESS, 0x0A,
        0x01); // enable the magnetometer
    delay(10);
    I2C_M.readBytes(MPU9150_RA_MAG_ADDRESS, MPU9150_RA_MAG_XOUT_L, 6, buffer_m);

    mx = ((int16_t)(buffer_m[1]) << 8) | buffer_m[0];
    my = ((int16_t)(buffer_m[3]) << 8) | buffer_m[2];
    mz = ((int16_t)(buffer_m[5]) << 8) | buffer_m[4];

    Mxyz[0] = (double)mx * 1200 / 4096;
    Mxyz[1] = (double)my * 1200 / 4096;
    Mxyz[2] = (double)mz * 1200 / 4096;
}

void getCompassDate_calibrated()
{
    getCompass_Data();
    Mxyz[0] = Mxyz[0] - mx_centre;
    Mxyz[1] = Mxyz[1] - my_centre;
    Mxyz[2] = Mxyz[2] - mz_centre;
}

void printLocalTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%F %T %A"); //格式化输出
}

void mqttPublish()
{
    if (!client.connected()) {
        reconnect();
    };
    client.loop();

    char outime[50];
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        String lostinfo = "Failed to obtain time";
        lostinfo.toCharArray(outime, 50);
    } else {
        strftime(outime, sizeof(outime), "%F %T %A", &timeinfo);
    }

    unsigned long now = millis();
    if (now - lastMsg > 2000) {
        lastMsg = now;
        ++value;
        snprintf(msg, MSG_BUFFER_SIZE, "{\"drop_id\": \"#%ld\", \"timestamp\": \"%s\"}",
            value, outime);
        Serial.println(String("") + "Publish message: " + msg);
        client.publish("demo/SmartRC/down/", msg);
        Serial.println();
    }
}

void setup_wifi()
{
    delay(10);
    // We start by connecting to a WiFi network

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println(String("") + "Connecting to " + ssid + " ...");
    }

    randomSeed(micros());

    Serial.print("WiFi connected. ");
    Serial.println(String("") + "IP address: " + WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length)
{
    Serial.print(String("") + "Message arrived [" + topic + "] ");
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') {
        // digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is
        // the voltage level
        Serial.println("F");
        // but actually the LED is on; this is because
        // it is active low on the ESP-01)
    } else {
        // digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the
        // voltage HIGH
        Serial.println("T");
    }
}

void reconnect()
{
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Create a random client ID
        String clientId = "ESP32Client-";
        clientId += String(random(0xffff), HEX);
        // Attempt to connect
        if (client.connect(clientId.c_str(), user_name, user_password)) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish("outTopic", "hello world");
            // ... and resubscribe
            client.subscribe("inTopic");
        } else {
            Serial.println(String("") + "failed, rc=" + client.state() + " try again in 5 seconds.");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}
