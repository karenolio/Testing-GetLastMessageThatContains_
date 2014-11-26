/* Setup shield-specific #include statements */
#include <SPI.h>
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>
#include <ccspi.h>
#include <Client.h>
#include <Temboo.h>
#include "TembooAccount.h" // Contains Temboo account information


#define ADAFRUIT_CC3000_IRQ 3
#define ADAFRUIT_CC3000_VBAT 5
#define ADAFRUIT_CC3000_CS 10

class TembooCC3KClient : public Client {
  public:
    TembooCC3KClient(Adafruit_CC3000& cc3k) : m_cc3k(cc3k) {
      m_cached = -1;
    }
    
    int connect(IPAddress ip, uint16_t port) {
      m_client = m_cc3k.connectTCP((uint32_t)ip, port); 
      m_cached = -1; 
      return m_client.connected()?1:0;
    }

    int connect(const char* host, uint16_t port) {
      uint32_t ip=0;
      if (m_cc3k.getHostByName(const_cast<char*>(host), &ip)) {
        m_client = m_cc3k.connectTCP(ip, port);
        m_cached = -1;
        return m_client.connected()?1:0;
      }
      return 0;
    }

    size_t write(uint8_t data) { 
      return m_client.write(data);
    }
    
    size_t write(const uint8_t* buf, size_t size) {
      return m_client.write((void*)buf, (uint16_t)size); 
    }
    
    int available() {
      return m_cached < 0 ? m_client.available():1; 
    }
    
    int read() {
      int c = m_cached < 0 ? m_client.read() : m_cached; 
      m_cached = -1; 
      return c; 
    }
    
    int read(uint8_t* buf, size_t size) {
      if (size <= 0) { return 0; }
      if (NULL == buf) { return -1; }
      if (m_cached >= 0) {
        buf[0] = (uint8_t)m_cached;
        m_cached = -1;
        buf++;
        size--;
      }
      return m_client.read((void*) buf, size);
    }

    int peek() {
      if (m_cached < 0) {
        m_cached = (available())?(read()):-1; 
      } 
      return m_cached;
    }

    void flush() {
      m_cached = -1;
      while(available()) {
        read(); 
      }
    }

    void stop() {
      while (m_client.connected()) {
        m_client.close(); 
        delay(10);
      }
      m_cached = -1;
    }
    
    uint8_t connected() {
      return (uint8_t)m_client.connected();
    }
    
    operator bool() {
      return true;
    } 
  
  private:
    Adafruit_CC3000& m_cc3k;
    Adafruit_CC3000_Client m_client;
    int16_t m_cached;
};

Adafruit_CC3000 cc3k = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT);

TembooCC3KClient client(cc3k);

int numRuns = 1;   // Execution count, so this doesn't run forever
int maxRuns = 10;   // Maximum number of times the Choreo should be executed


void setup() {
  Serial.begin(9600);
  
  // For debugging, wait until the serial console is connected.
  delay(4000);
  while(!Serial);

  status_t wifiStatus = STATUS_DISCONNECTED;
  while (wifiStatus != STATUS_CONNECTED) {
    Serial.print("WiFi:");
    if (cc3k.begin()) {
      if (cc3k.connectToAP(WIFI_SSID, WPA_PASSWORD, WLAN_SEC_WPA2)) {
        wifiStatus = cc3k.getStatus();
      }
    }
    if (wifiStatus == STATUS_CONNECTED) {
      Serial.println("OK");
    } else {
      Serial.println("FAIL");
    }
    delay(5000);
  }

  Serial.println("Setup complete.\n");
}

void loop() {
  if (numRuns <= maxRuns) {
    Serial.println("Running GetLastMessageThatContains - Run #" + String(numRuns++));

    TembooChoreo GetLastMessageThatContainsChoreo(client);

    // Invoke the Temboo client
    GetLastMessageThatContainsChoreo.begin();

    // Set Temboo account credentials
    GetLastMessageThatContainsChoreo.setAccountName(TEMBOO_ACCOUNT);
    GetLastMessageThatContainsChoreo.setAppKeyName(TEMBOO_APP_KEY_NAME);
    GetLastMessageThatContainsChoreo.setAppKey(TEMBOO_APP_KEY);

    // Set profile to use for execution
    GetLastMessageThatContainsChoreo.setProfile("TwilioAccount");

    // Set Choreo inputs
    String FilterValue = "lights";
    GetLastMessageThatContainsChoreo.addInput("Filter", FilterValue);

    // Identify the Choreo to run
    GetLastMessageThatContainsChoreo.setChoreo("/Library/Twilio/SMSMessages/GetLastMessageThatContains");

    // Run the Choreo; when results are available, print them to serial
    GetLastMessageThatContainsChoreo.run();

    while(GetLastMessageThatContainsChoreo.available()) {
      char c = GetLastMessageThatContainsChoreo.read();
      Serial.print(c);
    }
    GetLastMessageThatContainsChoreo.close();
  }

  Serial.println("\nWaiting...\n");
  delay(30000); // wait 30 seconds between GetLastMessageThatContains calls
}
