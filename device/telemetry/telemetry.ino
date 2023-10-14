#include <Wire.h>
#include <WiFi.h>
#include <RingBuf.h>
#include <ArduinoJson.h>
#include <SocketIOclient.h>

/****************************************************************************
 * Hotspot AP configurations                                                *
 ****************************************************************************/
const char ssid[] = NETWORK_SSID;
const char pwd[]  = NETWORK_PASSWORD;

/****************************************************************************
 * telemetry server configuration                                           *
 ****************************************************************************/
const char server[] = SERVER_NAME;
const char url[] = "/socket.io/?EIO=4&device=1&channel="CHANNEL_NAME"&key="CHANNEL_KEY;
const int port = 80; // telemetry port of socket.io server

// GPIO and I2C configurations
#define ESP_COMM 19 // STM32 EXTI GPIO
#define I2C_SDA  22
#define I2C_SCL  21
#define I2C_ADDR (0x0) // generall call address
#define I2C_FREQ 400000 // 400kHz fast mode

// global flags
bool stm_handshake = false;
bool rtc_received = false;
bool server_conn = false;

// socket.io client
SocketIOclient socketIO;

// log buffer
RingBuf<char, 1024> tx_buf;
char log_payload[52] = "[\"tlog\",{\"log\":\"";

// server time
char rtc[19];

void setup() {
  Serial.begin(115200);
  Serial.printf("Wi-Fi:\n\tSSID: %s / PW: %s\nSERVER:\n\tname: %s:%d\n\turl: %s\nI2C:\n\tFREQ: %dHz / ADDR: 0x%02x / SDA: %d / SCL: %d\n", ssid, pwd, server, port, url, I2C_FREQ, I2C_ADDR, I2C_SDA, I2C_SCL);

  // init ESP_COMM
  pinMode(ESP_COMM, OUTPUT);
  digitalWrite(ESP_COMM, HIGH);

  // init I2C slave mode
  Wire.onReceive(i2c_rcv_callback);
  Wire.onRequest(i2c_req_callback);
  Wire.begin(I2C_ADDR, I2C_SDA, I2C_SCL, I2C_FREQ);

  // init Wi-Fi
  WiFi.disconnect();

  if (WiFi.getMode() & WIFI_AP) {
    WiFi.softAPdisconnect(true);
  }

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  // attach socket
  socketIO.begin(server, port, url);
  socketIO.onEvent(socketIOEvent);

  disableCore0WDT();
}

void loop() {
  socketIO.loop();

  // server connection EXTI indicator
  if (stm_handshake && rtc_received) {
    if (WiFi.status() != WL_CONNECTED) {
      server_conn = false;
    }

    digitalWrite(ESP_COMM, server_conn ? HIGH : LOW);
  }

  // flush log buffer to server
  if (server_conn && !tx_buf.isEmpty()) {
    char pop;
    char buf[16];

    for (int x = 0; x < 16; x++) {
      tx_buf.lockedPop(pop);
      buf[x] = pop;
    }

    sprintf((log_payload + 16),
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x\"}]",
        buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
        buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);

    socketIO.sendEVENT(log_payload, 51);
  }
}

void socketIOEvent(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case sIOtype_CONNECT:
      server_conn = true;
      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");
      break;

    case sIOtype_EVENT: {
      // parse server RTC time fix data
      StaticJsonDocument<64> json;
      DeserializationError jsonError = deserializeJson(json, payload, length);

      if (jsonError) {
        return;
      }

      const char *event = json[0];

      if (!rtc_received && (strcmp(event, "rtc_fix") == 0)) {
        strncpy(rtc, json[1]["datetime"], 19);
        Wire.slaveWrite((uint8_t *)rtc, 19);
        rtc_received = true;
      }
      break;
    }

    case sIOtype_DISCONNECT:
      server_conn = false;
      break;

    case sIOtype_ACK:
    case sIOtype_ERROR:
    case sIOtype_BINARY_EVENT:
    case sIOtype_BINARY_ACK:
    default:
      break;
  }
}

void i2c_rcv_callback(int len) {
  int i = 0;
  char buffer[16];

  while (Wire.available()) {
    if (i < 16) {
      buffer[i++] = Wire.read();
    } else {
      Wire.read(); // just flush buffers
    }
  }

  // STM32 handshake sequence
  if (!stm_handshake) {
    if (strncmp(buffer, "READY", 5) == 0) {
      stm_handshake = true;
      digitalWrite(ESP_COMM, LOW);
    }
  }

  // log received
  else if (i == 16) {
    for (int x = 0; x < 16; x++) {
      tx_buf.lockedPushOverwrite(buffer[x]);
    }
  }
}

void i2c_req_callback(void) {
  // nothing to do; write buffer is set at rtc_fix event
}