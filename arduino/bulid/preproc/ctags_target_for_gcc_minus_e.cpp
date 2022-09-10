# 1 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino"
/*

This sketch demonstrates the capabilities of the pubsub library in combination

with the ESP8266 board/library.

It connects to an MQTT server then:

- publishes "hello world" to the topic "outTopic" every two seconds

- subscribes to the topic "inTopic", printing out any messages

  it receives. NB - it assumes the received payloads are strings not binary

- If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,

  else switch it off

It will reconnect to the server if the connection is lost using a blocking

reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to

achieve the same result without blocking the main loop.

To install the ESP8266 board, (using Arduino 1.6.4+):

- Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":

     http://arduino.esp8266.com/stable/package_esp8266com_index.json

- Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"

- Select your ESP8266 in "Tools -> Board"

*/
# 20 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino"
# 21 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino" 2
# 22 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino" 2

# 24 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino" 2

// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
# 28 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino" 2

// Update these with values suitable for your network.

const char *ssid = "NKUST_Aimo"; // WiFi ssid
const char *password = "0931992966"; // WiFi pwd
// const char *ssid = "Aimo"; // WiFi ssid
// const char *password = "45556140"; // WiFi pwd
# 36 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino" 2
const char *mqtt_server = "mqtt.54ucl.com";
const char *user_name = "flask"; // 連接 MQTT broker 的帳號密碼
const char *user_password = "flask";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
// MSG Len
#define MSG_BUFFER_SIZE (255)
char msg[(255)];
int value = 0;

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

float Axyz[3];
float Gxyz[3];
float Mxyz[3];

float SVM = 0;

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

void setup()
{

  // pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // mqttPublish();

  // join I2C bus (I2Cdev library doesn't do this automatically)
  Wire.begin();

  // initialize serial communication
  // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
  // it's really up to you depending on your project)
  // Serial.begin(115200);

  // initialize device
  while (!Serial)
    ;
  Serial.println("Initializing I2C devices...");
  accelgyro.initialize();

  // verify connection
  Serial.println("Testing device connections...");
  // Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

  delay(5000);
  Mxyz_init_calibrated();
}

void loop()
{

  getAccel_Data();
  getGyro_Data();
  getCompassDate_calibrated(); // compass data has been calibrated here
  getHeading(); // before we use this function we should run 'getCompassDate_calibrated()' frist, so that we can get calibrated data ,then we can get correct angle .
  getTiltHeading();

  Serial.print("calibration parameter: ");
  Serial.print(mx_centre);
  Serial.print("         ");
  Serial.print(my_centre);
  Serial.print("         ");
  Serial.println(mz_centre);
  Serial.println("     ");

  Serial.print("Acceleration(g) of X,Y,Z: ");
  Serial.print(Axyz[0]);
  Serial.print(", ");
  Serial.print(Axyz[1]);
  Serial.print(", ");
  Serial.println(Axyz[2]);
  /*

  Serial.print("Gyro(degrees/s) of X,Y,Z: ");

  Serial.print(Gxyz[0]);

  Serial.print(", ");

  Serial.print(Gxyz[1]);

  Serial.print(", ");

  Serial.println(Gxyz[2]);

  Serial.print("Compass Value of X,Y,Z: ");

  Serial.print(Mxyz[0]);

  Serial.print(", ");

  Serial.print(Mxyz[1]);

  Serial.print(", ");

  Serial.println(Mxyz[2]);

  Serial.print("The clockwise angle between the magnetic north and X-Axis: ");

  Serial.println(heading);

  Serial.print("The clockwise angle between the magnetic north and the projection of the positive X-Axis in the horizontal plane: ");

  Serial.println(tiltheading);

  */
# 162 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino"
  Serial.println();

  // SVM = sqrt(pow(Axyz[0], 2) + pow(Axyz[0], 2) + pow(Axyz[0], 2));
  SVM = 10;

  if (SVM >= 10)
  {
    mqttPublish(Axyz[0], Axyz[1], Axyz[2]);
  }

  delay(1000);
}

void mqttPublish(float x, float y, float z)
{
  if (!client.connected())
  {
    reconnect();
  };
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000)
  {
    lastMsg = now;
    ++value;
    snprintf(msg, (255), "{Msg_id #%ld, Acceleration(g) of X,Y,Z: %2f, %2f, %2f.}", value, x, y, z);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("demo/SmartRC/down/", msg);
    Serial.println();
  }
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network

  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.println(" ...");
  }

  randomSeed(micros());

  Serial.print("WiFi connected. ");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void getHeading(void)
{
  heading = 180 * atan2(Mxyz[1], Mxyz[0]) / 3.1415926535897932384626433832795;
  if (heading < 0)
  {
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
  tiltheading = 180 * atan2(yh, xh) / 3.1415926535897932384626433832795;
  if (yh < 0)
  {
    tiltheading += 360;
  }
}

void Mxyz_init_calibrated()
{

  Serial.println(((reinterpret_cast<const __FlashStringHelper *>(("Before using 9DOF,we need to calibrate the compass frist,It will takes about 2 minutes. \n" "During calibratting ,you should rotate and turn the 9DOF all the time within 2 minutes. \n" "If you are ready ,please sent a command data 'ready' to start sample and calibrate."))))

                                                                                                         );
  // Serial.println(F("During calibratting ,you should rotate and turn the 9DOF all the time within 2 minutes."));
  // while (!Serial.find("ready"));
  Serial.println("ready");
  Serial.println("Sample starting......");
  Serial.println("waiting ......");

  get_calibration_Data();

  Serial.println("compass calibration parameter ");
  Serial.print(mx_centre);
  Serial.print("     ");
  Serial.print(my_centre);
  Serial.print("     ");
  Serial.println(mz_centre);
}

void get_calibration_Data()
{
  for (int i = 0; i < 5000; i++)
  {
    get_one_sample_date_mxyz();
    /*

        Serial.print(mx_sample[2]);

        Serial.print(" ");

        Serial.print(my_sample[2]);                            //you can see the sample data here .

        Serial.print(" ");

        Serial.println(mz_sample[2]);

    */
# 278 "c:\\Users\\user\\Downloads\\UCL2023\\110-2激勵計畫\\code\\arduino\\test\\test.ino"
    if (mx_sample[2] >= mx_sample[1])
    {
      mx_sample[1] = mx_sample[2];
    }
    if (my_sample[2] >= my_sample[1])
    {
      my_sample[1] = my_sample[2]; // find max value
    }
    if (mz_sample[2] >= mz_sample[1])
    {
      mz_sample[1] = mz_sample[2];
    }

    if (mx_sample[2] <= mx_sample[0])
    {
      mx_sample[0] = mx_sample[2];
    }
    if (my_sample[2] <= my_sample[0])
    {
      my_sample[0] = my_sample[2]; // find min value
    }
    if (mz_sample[2] <= mz_sample[0])
    {
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
  Axyz[0] = (double)ax / 16384;
  Axyz[1] = (double)ay / 16384;
  Axyz[2] = (double)az / 16384;
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
  I2C_M.writeByte(0x0C, 0x0A, 0x01); // enable the magnetometer
  delay(10);
  I2C_M.readBytes(0x0C, 0x03, 6, buffer_m);

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

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1')
  {
    // digitalWrite(BUILTIN_LED, LOW); // Turn the LED on (Note that LOW is the voltage level
    Serial.println("F");
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  }
  else
  {
    // digitalWrite(BUILTIN_LED, HIGH); // Turn the LED off by making the voltage HIGH
    Serial.println("T");
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), 16);
    // Attempt to connect
    if (client.connect(clientId.c_str(), user_name, user_password))
    {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("inTopic");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds.");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
