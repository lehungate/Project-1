#include <ESP8266WiFi.h>   // Thư viện để kết nối với WiFi bằng ESP8266
#include <PubSubClient.h>  // Thư viện để kết nối với MQTT Broker
#include <ArduinoJson.h>   // Thư viện để xử lý JSON
#include <SoftwareSerial.h> // Thư viện để sử dụng Serial Software
#include <DHT.h>        // Thư viện để sử dụng cảm biến DHT22
#include <vector>       // Thư viện dùng tách chuỗi cho truyền thông nối tiếp

#define RX_PIN 4 // Chân RX của Serial Software kết nối với D2
#define TX_PIN 5 // Chân TX của Serial Software kết nối với D1
SoftwareSerial mySerial(RX_PIN, TX_PIN); // Tạo đối tượng SoftwareSerial
#define LED_PIN D4 // Định nghĩa chân LED

// Thông tin WiFi
const char *ssid = "YUKOKEISO";  // Tên WiFi "Hoa Biển Hotel - Trệt"
const char *password = "Yuko2023";   // Mật khẩu WiFi "77779999"

// Thông tin MQTT Broker
const char *mqtt_broker = "117.7.60.137";  // Địa chỉ IP của MQTT Broker
const char *mqtt_topic = "ESP8266-001/Data";     // Chủ đề MQTT Publisher
const char *subcruber_topic = "IoT/PCP-201/AI";     // Chủ đề MQTT Publisher
const char *mqtt_username = "";  // Tên đăng nhập MQTT (nếu có)
const char *mqtt_password = "";  // Mật khẩu MQTT (nếu có)
const int mqtt_port = 1883;  // Cổng của MQTT (TCP)

WiFiClient espClient;  // Tạo đối tượng WiFiClient
PubSubClient mqtt_client(espClient);  // Tạo đối tượng PubSubClient để sử dụng MQTT

String myString; // Biến để lưu trữ chuỗi dữ liệu nhận từ Serial
String SubcribeString; // Biến để lưu trữ chuỗi dữ liệu nhận từ MQTT Broker
char rdata;  // Biến để lưu trữ ký tự nhận từ Serial
int ValueV1 = 0; // Biến để lưu trữ các giá trị cảm biến
int ValueV2 = 0;
int ValueV3 = 0;

void connectToWiFi(); // Khai báo hàm kết nối WiFi
void connectToMQTTBroker(); // Khai báo hàm kết nối với MQTT Broker
void mqttCallback(char *topic, byte *payload, unsigned int length); // Khai báo hàm xử lý khi nhận được tin nhắn MQTT
String getValue(String data, char separator, int index); // Khai báo hàm tách chuỗi dữ liệu
std::vector<String> splitString(const String &str, char delimiter); // Hàm tách chuỗi dữ liệu dựa trên dấu phẩy

// Hàm connect wifi của ESP8266 
void connectToWiFi() {
    WiFi.begin(ssid, password); // Bắt đầu kết nối WiFi với SSID và mật khẩu
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) { // Chờ cho đến khi kết nối WiFi thành công
        delay(500); // Dừng 500ms
        Serial.print("."); // In dấu chấm để hiển thị trạng thái đang kết nối
    }
    Serial.println(""); // In thông báo khi kết nối thành công
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// Hàm connect ESP8266 với MQTT Broker
void connectToMQTTBroker() {
    while (!mqtt_client.connected()) { // Chờ cho đến khi kết nối MQTT thành công
        String client_id = "esp8266-client-" + String(WiFi.macAddress()); // Tạo client ID với địa chỉ MAC
        Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str()); // In thông báo kết nối
        if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) { // Kết nối với MQTT Broker
            Serial.println("Connected to MQTT broker"); // In thông báo khi kết nối thành công
            mqtt_client.subscribe(mqtt_topic); // Đăng ký chủ đề MQTT
            mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^"); // Gửi tin nhắn chào mừng
        } else {
            Serial.print("Failed to connect to MQTT broker, rc="); // In thông báo khi kết nối thất bại
            Serial.print(mqtt_client.state()); // In mã lỗi
            Serial.println(" try again in 5 seconds"); // Thử lại sau 5 giây
            delay(5000); // Dừng 5 giây
        }
    }
}

//-----Call back Method for Receiving MQTT massage---------
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for(int i=0; i<length;i++) incommingMessage += (char)payload[i];
  Serial.println("Massage arived ["+String(topic)+"]"+incommingMessage);
}

//-----Method for Publishing MQTT Messages---------
void publishMessage(const char* topic, String payload, boolean retained){
  if(mqtt_client.publish(topic,payload.c_str(),true))
    Serial.println("Message published ["+String(topic)+"]: "+payload);
}


void setup() {
    pinMode(LED_PIN, OUTPUT); // Thiết lập chân LED là OUTPUT
    Serial.begin(9600); // Khởi động Serial với baud rate 115200
    mySerial.begin(9600); // Khởi động Software Serial với baud rate 9600
    connectToWiFi(); // Gọi hàm kết nối WiFi
    mqtt_client.setServer(mqtt_broker, mqtt_port); // Thiết lập MQTT Broker
    mqtt_client.setCallback(mqttCallback); // Thiết lập hàm callback cho MQTT
    connectToMQTTBroker(); // Gọi hàm kết nối MQTT Broker
}

void loop() {
 
  // Nếu mất kết nối MQTT tự động kết nối lại
  if (!mqtt_client.connected()) {
    connectToMQTTBroker(); 
  }
  mqtt_client.loop(); // Xử lý các tác vụ MQTT

  // Đoạn code ghép dữ liệu để gửi lên MQTT
  DynamicJsonDocument doc(1024); // Tạo đối tượng JSON động
  int sensorValues[] = {ValueV1, ValueV2, ValueV3};  // Mảng dữ liệu cảm biến
  JsonArray array = doc.createNestedArray("SensorValues"); // Tạo mảng JSON lồng nhau

  // Thêm các giá trị vào mảng
  JsonObject sensor1 = array.createNestedObject();
  sensor1["id"] = 1;
  sensor1["value"] = ValueV1;

  JsonObject sensor2 = array.createNestedObject();
  sensor2["id"] = 2;
  sensor2["value"] = ValueV2;

  JsonObject sensor3 = array.createNestedObject();
  sensor3["id"] = 3;
  sensor3["value"] = ValueV3;

  char mqtt_message[256]; // Tạo mảng ký tự để chứa tin nhắn MQTT
  serializeJson(doc, mqtt_message); // Chuyển đổi đối tượng JSON thành chuỗi ký tự
  publishMessage(mqtt_topic, mqtt_message, true); // Gửi tin nhắn MQTT

  Serial.print("Message Published: "); // In thông báo khi tin nhắn được gửi
  Serial.println(mqtt_message); // In tin nhắn MQTT

  // Đọc và gửi dữ liệu từ Arduino - ESP8266 qua UART
  if (mySerial.available()) {
    // Xử lý dữ liệu nhận từ Arduino
    String data = mySerial.readStringUntil('\n');
    Serial.println("Received: " + data);
    std::vector<String> values = splitString(data, ','); // Tách dữ liệu thành các giá trị riêng lẻ
    if (values.size() >= 3) {
      for (size_t i = 0; i < values.size(); i++) { // Hiển thị từng giá trị
        Serial.println("Value " + String(i+1) + ": " + values[i]);
      }
      ValueV1 = values[0].toInt();
      ValueV2 = values[1].toInt();
    }
  }
  
  // Tạo chuỗi dự liệu gửi đến Arduino
  String SendData = String(ValueV1) + "," + String(ValueV2) + "," + String(ValueV3) ; // Dấu phẩy được dùng làm dấu phân cách
  Serial.print("String Send = ");
  Serial.println(SendData); // In ra cổng nối tiếp phần cứng
  digitalWrite(LED_PIN, LOW); // Tắt đèn LED trước khi gửi dữ liệu qua UART
  mySerial.println(SendData); // In ra cổng nối tiếp phần mềm
  digitalWrite(LED_PIN, HIGH); // Bật đèn LED sau khi gửi dữ liệu
  SendData = ""; // Xóa chuỗi dữ liệu

  // In ra cổng Serial để check
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Tạo trễ xử lý trong chương trình
  delay(1000); // Đợi 1 giây trước khi đọc lại
}

// Hàm tách chuỗi dữ liệu dựa trên dấu phẩy
std::vector<String> splitString(const String &str, char delimiter) {
  std::vector<String> tokens;
  String token;
  size_t start = 0;
  size_t end = str.indexOf(delimiter);
  while (end != -1) {
    token = str.substring(start, end);
    tokens.push_back(token);
    start = end + 1;
    end = str.indexOf(delimiter, start);
  }
  token = str.substring(start);
  tokens.push_back(token);
  return tokens;
}

// Hàm tách giá trị từ chuỗi dữ liệu dựa trên dấu phân cách
String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i + 1 : i;
        }
    }

    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

