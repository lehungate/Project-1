#include <Wire.h>
#include <LiquidCrystal_I2C.h>       // Thư viện điều khiển màn hình LCD
LiquidCrystal_I2C lcd(0x3F, 16, 2);  // Đặt địa chỉ (0x3F,16,2) cho màn hình LCD 16x2

#include <SoftwareSerial.h> // Thư viện cho cổng nối tiếp mềm, tx-11, rx-10, gnd-gnd
#define RX_PIN 10 // Chân RX của cổng nối tiếp mềm
#define TX_PIN 11 // Chân TX của cổng nối tiếp mềm
SoftwareSerial mySerial(RX_PIN, TX_PIN); // Tạo đối tượng cổng nối tiếp mềm

const int analogInPin = A0;  // Chân đầu vào analog kết nối với chiết áp
const int analogOutPin = 9;  // Chân đầu ra analog kết nối với LED

const int buttonPin = 2;  // Số chân của nút nhấn
const int ledPin = 13;    // Số chân của đèn LED

int sensorValue = 0;  // Giá trị đọc từ chiết áp
int outputValue = 0;  // Giá trị đầu ra cho PWM (analog out)
int ADCPercent = 0;   // Giá trị điện áp trong khoảng 0~5V (đơn vị: 0.01V)
int SubcribeV1 = 0;   // Giá trị SubcibeV1
int buttonState = 0;  // Biến lưu trạng thái của nút nhấn
String SendData;   // Dữ liệu gửi đi UART
String ReceiveData; // Dữ liệu sau khi nhận từ UART
String ReadData; // Dữ liệu đọc từ UART

// Hàm tách chuỗi dữ liệu theo ký tự phân cách và trả về phần tử thứ index
String getValue(String data, char separator, int index){
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
      if (data.charAt(i) == separator || i == maxIndex) {
          found++;
          strIndex[0] = strIndex[1] + 1;
          strIndex[1] = (i == maxIndex) ? i+1 : i;
      }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void setup() {
  Serial.begin(9600); // Khởi tạo cổng nối tiếp phần cứng với tốc độ baud là 9600
  mySerial.begin(9600); // Khởi tạo cổng nối tiếp phần mềm với tốc độ baud là 9600
  lcd.init(); // Khởi tạo màn hình LCD
  //lcd.backlight();
  pinMode(buttonPin, INPUT);  // Khởi tạo chân nút nhấn là đầu vào
  pinMode(ledPin, OUTPUT);    // Khởi tạo chân đèn LED là đầu ra
}

void loop() {
          
  sensorValue = Sub_ADC(A0, 0, 1023); // Gọi hàm Sub_ADC với tham số chân analog, giá trị min và max

  ADCPercent = map(sensorValue, 0, 1023, 0, 1000); // Chuyển đổi giá trị ADC sang phần trăm (0.0~100.0%)
  outputValue = Sub_DAC(analogOutPin, ADCPercent, 0, 1000); // Gọi hàm Sub_DAC với các tham số cần thiết

  lcd.setCursor(0, 0);
  lcd.print("SENSOR:");  // Hiển thị "SENSOR:" trên màn hình LCD
  lcd.setCursor(9, 0);
  lcd.print(sensorValue);  // Hiển thị giá trị Sensor

  lcd.setCursor(0, 1);
  lcd.print("OUTPUT%:");  // Hiển thị "OUTPUT%:" trên màn hình LCD
  lcd.setCursor(10, 1);
  lcd.print(outputValue);  // Hiển thị giá trị outputValue

  buttonState = digitalRead(buttonPin);  // Đọc trạng thái của nút nhấn

  // Kiểm tra trạng thái của nút nhấn
  if (buttonState == HIGH) {
    digitalWrite(ledPin, HIGH);  // Bật đèn LED
  } else {
    digitalWrite(ledPin, LOW);  // Tắt đèn LED
  }
  // Đọc và gửi dữ liệu từ Arduino - ESP8266 qua UART
  
  // Tạo chuỗi dữ liệu gửi đến ESP8266 qua UART
    int sdata1 = sensorValue;
    int sdata2 = outputValue;
    int sdata3 = ADCPercent;
    SendData = String(sdata1) + "," + String(sdata2) + "," + String(sdata3); // Dấu phẩy được dùng làm dấu phân cách
    Serial.print("String Send = ");
    Serial.println(SendData); // In ra cổng nối tiếp phần cứng
    mySerial.println(SendData); // In ra cổng nối tiếp phần mềm
    SendData = ""; // Xóa chuỗi dữ liệu

     // Đọc dữ liệu từ từ ESP8266 qua UART
    if (mySerial.available()){
     ReadData = mySerial.readString(); // Đọc chuỗi dữ liệu
     ReceiveData += ReadData;
     int rdata1 = getValue(ReceiveData, ',', 0).toInt(); // Lấy giá trị cảm biến thứ nhất
     int rdata2 = getValue(ReceiveData, ',', 1).toInt(); // Lấy giá trị cảm biến thứ hai
     int rdata3 = getValue(ReceiveData, ',', 2).toInt(); // Lấy giá trị cảm biến thứ ba
     SubcribeV1 = rdata3;
     Serial.print("String Revice = ");
     Serial.println(ReceiveData); // In ra chuỗi dữ liệu
     Serial.print("rdata1 = ");
     Serial.println(rdata1);
     Serial.print("rdata2 = ");
     Serial.println(rdata2);
     Serial.print("rdata3 = ");
     Serial.println(rdata3);
     ReceiveData = ""; // Xóa chuỗi dữ liệu
     }

  delay(1000); // Đợi 1000 milli giây (1 giây)
}

// Hàm xử lý giá trị Analog Input
int Sub_ADC(int AnalogInputPIN, int SensorMinValve, int SensorMaxValue) {
  int ADCValue = analogRead(AnalogInputPIN); // Đọc giá trị từ chân analog
  int Result = map(ADCValue, 0, 1023, SensorMinValve, SensorMaxValue);  // Chuyển đổi giá trị về dải đầu vào của cảm biến
  return Result;
}

// Hàm xử lý giá trị Analog Output (0.0~100.0% Output)
int Sub_DAC(int AnalogOutputPIN, int OutputValue, int OutputMinValve, int OutputMaxValue) {
  int PMWValue = map(OutputValue, OutputMinValve, OutputMaxValue, 0, 255); // Chuyển đổi giá trị về dải đầu ra analog
  analogWrite(AnalogOutputPIN, PMWValue); // Thay đổi giá trị đầu ra analog
  int Result = map(PMWValue, 0, 255, 0, 1000);
  return Result;
}
