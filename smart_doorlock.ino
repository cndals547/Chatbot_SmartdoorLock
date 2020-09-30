#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <Key.h>
#include <Keypad.h>

#define LED_YELLOW 15
#define LED_GREEN 2

const char *ssid = "knucse";
const char *passwd = "knucse9216";

int loopCount = 0;
int delayTime0 = 0;
int validateOTPTIme = -1;
int OTPNum = -1;
#define OTP_LENGTH 4
ESP8266WebServer server(80);

WiFiClientSecure client;
const char* HOST_FINGERPRINT = "88 BB 44 52 4A 4D F2 66 F3 24 9E 0C 03 EC C6 EB FE 3F CF C7";

bool isDoorLocked = true;
int validOpenDoorTime = -1;

String inputPwd = "";

char keys[4][3] = {
   { '1','2','3' },
   { '4','5','6' },
   { '7','8','9' },
   { '*','0','#' }
};

byte rowPins[4] = { 0, 13, 12, 14 }; byte colPins[3] = { 4,5,16 }; 
Keypad customKeypad = Keypad(makeKeymap(keys), rowPins, colPins, 4, 3);
void indexed();
void genOTP();
void openDoor();
void handler404();

void genPwd();
void resetPwd();
void pushMsg(String msg);

void connectHost() {
   Serial.print("connecting to ");
   Serial.println(HOST);
   if (!client.connect(HOST, HTTPS_PORT)) {
      Serial.println("connection failed");
      return;
   }

   if (client.verify(HOST_FINGERPRINT, HOST)) {
      Serial.println("certificate matches");
   }
   else {
      Serial.println("certificate doesn't match");
   }
}

void setup() {
   
   Serial.begin(115200);


            pinMode(LED_YELLOW, OUTPUT);
   pinMode(LED_GREEN, OUTPUT);

   Serial.print("connecting to ");
   Serial.println(ssid);

   WiFi.begin(ssid, passwd);
   while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }

   Serial.println("");
   Serial.println("WiFi connected");
   Serial.println("IP address: ");
   Serial.println(WiFi.localIP());

   connectHost();

   server.on("/", indexed);
   server.on("/genOTP", genOTP);
   server.on("/openDoor", openDoor);
   server.onNotFound(handler404);
   server.begin();

   Serial.println("server run...");

}

void setColor(bool g, bool y) {
   if (g) {
      digitalWrite(LED_GREEN, HIGH);
   }
   else {
      digitalWrite(LED_GREEN, LOW);
   }

   if (y) {
      digitalWrite(LED_YELLOW, HIGH);
   }
   else {
      digitalWrite(LED_YELLOW, LOW);
   }
}

void loop() {
   int currentTime = millis();
   if (currentTime - delayTime0 >= 100) {
   
      delayTime0 = currentTime;

      server.handleClient();

      char customKey = customKeypad.getKey();
      if (customKey) {
         OTPRoutine(customKey);
      }

      if (isDoorLocked) {
                  setColor(true, false);

      }
      else {
         setColor(false, true);
               }

      if (client.connected() == false) {
         Serial.print("커넥션 끊김 : ");
         Serial.println(currentTime);
         connectHost();
      }

                  int statePush = LOW;
      int stateOTP = LOW;

      if (validateOTPTIme >= 0 && currentTime - validateOTPTIme >= (15 * 1000)) {
         Serial.println("password expired..");
         resetPwd();
         pushMsg("OTP 번호가 만료되었습니다..");
      }

      if (validOpenDoorTime >= 0 && currentTime - validOpenDoorTime >= (10 * 1000)) {
         isDoorLocked = true;
         validOpenDoorTime = -1;
         pushMsg("문이 자동으로 닫혔습니다.");

      }

      if (stateOTP == HIGH) {
         genPwd();
         String temp = String(OTPNum);
         Serial.println(temp);

         for (int a = 0; a< OTP_LENGTH - temp.length(); a++) {
              temp = "0" + temp;
            }
         pushMsg(temp);
         }
      if (statePush == HIGH) {
         Serial.println("test Push");
         pushMsg("성공!!");
      }
   }      
}
void indexed() {
   Serial.println("=======index");
   String returnData = "";
   returnData += "<html><body>hi</body></html>";
   server.send(200, "text/html", returnData);
}

void genOTP() {
   Serial.println("=====genOTP");

   genPwd();
   String temp = String(OTPNum);

   for (int a = 0; a< OTP_LENGTH - temp.length(); a++) {
      temp = "0" + temp;
   }
   Serial.println(temp);

   pushMsg(temp);

   String returnData = "";
   returnData += "<html><body>Try OTP Generating..</body></html>";
   server.send(200, "text/html", returnData);
}

void openDoor() {
   Serial.println("=====openDoor");
   String returnData = "";
   returnData += "<html><body>Door Opened!</body></html>";

   validOpenDoorTime = millis();
   isDoorLocked = false;

   pushMsg("문이 열렸습니다");
   server.send(200, "text/html", returnData);
}

void handler404() {
   String message = "File Not Found\n\n";
   message += "URI: ";
   message += server.uri();
   message += "\nMethod: ";
   message += (server.method() == HTTP_GET) ? "GET" : "POST";
   message += "\nArguments: ";
   message += server.args();
   message += "\n";

   for (uint8_t i = 0; i < server.args(); i++) {
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
   }

   server.send(404, "text/plain", message);
}

void genPwd() {
   validateOTPTIme = millis();
   OTPNum = random(0, pow(10, OTP_LENGTH - 1));
}

void resetPwd() {
   validateOTPTIme = -1;
   OTPNum = -1;
}

void pushMsg(String msg) {
   if (client.connected() == false) {
      Serial.print("커넥션 끊김 : ");
      Serial.println(millis());
      connectHost();
   }

   String postData = "msg=" + msg + "&type=text";
   String url = "/api/v1.0/pushall";
   client.print(String("POST ") + url + " HTTP/1.1\r\n" +
      "Host: " + HOST + "\r\n" +
      "Content-Type: application/x-www-form-urlencoded;\r\n" +
      "Connection: Keep-Alive\r\n");    client.print("Content-Length: ");
   client.print(postData.length());
   client.print("\r\n\r\n");
   client.print(postData);
      client.connected();

   Serial.println("==========");
   while (true) {
      String li = client.readStringUntil('\n');
      Serial.println(li);
      if (li == "") {
         break;
      }
   }
   Serial.println("==========");
}

bool checkPwd(String tmp) {
   int number = tmp.toInt();
   if (number == OTPNum) {
      return true;
   }
   else {
      return false;
   }
}

void OTPRoutine(char keyC) {
   if (inputPwd.length() > OTP_LENGTH) {
      Serial.println("입력한 길이가 초과했습니다.");
      inputPwd = "";
      return;
   }

   switch (keyC) {
   case '#':
            if (checkPwd(inputPwd)) {
         isDoorLocked = false;
         validOpenDoorTime = millis();
         Serial.println("올바른 비밀번호");
         pushMsg("문이 열렸습니다");
      }
      else {
         Serial.println("틀린 비밀번호! : " + inputPwd);
         pushMsg("허가되지않은 비밀번호! : " + inputPwd);
      }
      inputPwd = "";

      break;

   case '*':
      break;

   default:
      inputPwd += keyC;
      break; 

   }
   Serial.println(keyC);
}
