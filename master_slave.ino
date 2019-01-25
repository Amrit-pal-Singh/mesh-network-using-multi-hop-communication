#include <esp_now.h>
#include <WiFi.h>
#include "DHT.h"
#include <string>
#define DHTPIN 23
#define DHTTYPE DHT11   // DHT 11

DHT dht(DHTPIN, DHTTYPE);
#define LED_PIN 2

// Global copy of slave
#define NUMSLAVES 20
esp_now_peer_info_t slaves[NUMSLAVES] = {};
int SlaveCnt = 0;

#define CHANNEL_MASTER 3
#define CHANNEL_SLAVE 1
#define PRINTSCANRESULTS 0
#define DATASIZE 48

// Init ESP Now with fallback
void InitESPNow() {
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  }
  else {
    Serial.println("ESPNow Init Failed");
    // Retry InitESPNow, add a counte and then restart?
    // InitESPNow();
    // or Simply Restart
    ESP.restart();
  }
}

// Scan for slaves in AP mode
void ScanForSlave() {
  WiFi.disconnect();   ///G
  int8_t scanResults = WiFi.scanNetworks();
  //reset slaves
  memset(slaves, 0, sizeof(slaves));
  SlaveCnt = 0;
  Serial.println("");
  if (scanResults == 0) {
    Serial.println("No WiFi devices in AP Mode found");
  } else {
    Serial.print("Found "); Serial.print(scanResults); Serial.println(" devices ");
    for (int i = 0; i < scanResults; ++i) {
      // Print SSID and RSSI for each device found
      String SSID = WiFi.SSID(i);
      int32_t RSSI = WiFi.RSSI(i);
      String BSSIDstr = WiFi.BSSIDstr(i);

      if (PRINTSCANRESULTS) {
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); 
        Serial.print(BSSIDstr); Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); 
        Serial.println("");
      }
      delay(10);
      // Check if the current device starts with `Slave`
      if (SSID.indexOf("ESPNOW") == 0) {
        // SSID of interest
        Serial.print(i + 1); Serial.print(": "); Serial.print(SSID); Serial.print(" ["); Serial.print(BSSIDstr);
        Serial.print("]"); Serial.print(" ("); Serial.print(RSSI); Serial.print(")"); Serial.println("");
        // Get BSSID => Mac Address of the Slave
        int mac[6];

        if ( 6 == sscanf(BSSIDstr.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",  &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5] ) ) {
          for (int ii = 0; ii < 6; ++ii ) {
            slaves[SlaveCnt].peer_addr[ii] = (uint8_t) mac[ii];
          }
        }
        slaves[SlaveCnt].channel = CHANNEL_MASTER; // pick a channel
        slaves[SlaveCnt].encrypt = 0; // no encryption
        SlaveCnt++;
      }
    }
  }

  if (SlaveCnt > 0) {
    Serial.print(SlaveCnt); Serial.println(" Slave(s) found, processing..");
  } else {
    Serial.println("No Slave Found, trying again.");
  }

  // clean up ram
  WiFi.scanDelete();
}

// Check if the slave is already paired with the master.
// If not, pair the slave with master
void manageSlave() {
  if (SlaveCnt > 0) {
    for (int i = 0; i < SlaveCnt; i++) {
      const esp_now_peer_info_t *peer = &slaves[i];
      const uint8_t *peer_addr = slaves[i].peer_addr;
      Serial.print("Processing: ");
      for (int ii = 0; ii < 6; ++ii ) {
        Serial.print((uint8_t) slaves[i].peer_addr[ii], HEX);
        if (ii != 5) Serial.print(":");
      }
      Serial.print(" Status: ");
      // check if the peer exists
      bool exists = esp_now_is_peer_exist(peer_addr);
      if (exists) {
        // Slave already paired.
        Serial.println("Already Paired");
      } else {
        // Slave not paired, attempt pair
        esp_err_t addStatus = esp_now_add_peer(peer);
        if (addStatus == ESP_OK) {
          // Pair success
          Serial.println("Pair success");
        } else if (addStatus == ESP_ERR_ESPNOW_NOT_INIT) {
          // How did we get so far!!
          Serial.println("ESPNOW Not Init");
        } else if (addStatus == ESP_ERR_ESPNOW_ARG) {
          Serial.println("Add Peer - Invalid Argument");
        } else if (addStatus == ESP_ERR_ESPNOW_FULL) {
          Serial.println("Peer list full");
        } else if (addStatus == ESP_ERR_ESPNOW_NO_MEM) {
          Serial.println("Out of memory");
        } else if (addStatus == ESP_ERR_ESPNOW_EXIST) {
          Serial.println("Peer Exists");
        } else {
          Serial.println("Not sure what happened");
        }
        delay(100);
      }
    }
  } else {
    // No slave found to process
    Serial.println("No Slave found to process");
  }
}

int MAX_NO_OF_PARENTS = 2;
uint8_t data[DATASIZE] = {-1};
uint8_t data1[DATASIZE] = {-1};
int check = 0;
uint64_t pos=0;
int layer_n = 3;
int node_rank = 1;

// send data
void sendData() {
  int i = 0; 
  //sprintf((char *)data,"%lld", pos);
  data[i++] = 'layer_n';    // layer number
  data[i++] = '_';
  data[i++] = 'node_rank';    // node rank in that layer
  data[i++] = 'L';
  Serial.print("--------------");
  Serial.print(i);
  float h = dht.readHumidity();
  int hi = (int)h;
  float t = dht.readTemperature();
  int ti = (int)t;
  float f = dht.readTemperature(true);
  int fi = (int)f;
  
  char humidity[3];
  char celcius[2];
  char farenhiet[3];
  itoa (hi,humidity,10);
  itoa (ti, celcius, 10);
  itoa (fi, farenhiet, 10);
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    i = 4;
    String str = " No data";
    for(int j = 0; j < 8; j++){
      data[i++] = str[j];
    }
  }
  else{
    for(int j = 0 ; j < 3; j++){   // Humidity
      if(humidity[j] !=  NULL)
        data[i++] = humidity[j];
    }
    data[i++] = 'H';  
    for(int j = 0; j < 2 ; j++){   // Celcius
      if(celcius[j] != NULL)
        data[i++] = celcius[j];
    }
    data[i++] = 'C';
    for(int j = 0; j < 3; j++){    // Farenhiet
      if(farenhiet[j] != NULL)
        data[i++] = farenhiet[j];
    }  
    data[i++] = 'F';
  }
  //path
  data[i++] = 'layer_n';    // layer number
  data[i++] = ','; 
  data[i++] = 'node_rank';    // node number   
  data[i++] = '>';
  data[i++] = 'P';    
  
  Serial.print((char*) data);
  for (int i = 0; i < SlaveCnt; i++) {
    const uint8_t *peer_addr = slaves[i].peer_addr;
    Serial.println("PEER ADRRESS");
    for(int jj = 0; jj < 6; jj++){
      Serial.print(peer_addr[jj]);
      Serial.print(" ");
    }
    Serial.println();
    if (i == 0) { // print only for first slave
      Serial.print("Sending: ");
      Serial.println((char *)data);
    }
      int flag = 1;
      int my_arr[MAX_NO_OF_PARENTS][6] = {{48, 174, 164, 117, 91, 149},
                                         {48, 174, 164, 25, 63, 81}} ;
      //remeber  you have to give ap address and not device address!!!!!
      for(int loop_j = 0; loop_j < MAX_NO_OF_PARENTS; loop_j++){
        flag = 1;
        for(int loop_i = 0; loop_i < 6; loop_i++){
          if(my_arr[loop_j][loop_i] != peer_addr[loop_i])
            flag = 0;
        }
        if(flag){
          break;
        }
      }
    if(flag){
      Serial.print("inside flag1");
      esp_err_t result = esp_now_send(peer_addr, data, DATASIZE);
      Serial.print("Send Status: ");
      if (result == ESP_OK) {
        Serial.println("Success");
      } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
        // How did we get so far!!
        Serial.println("ESPNOW not Init.");
      } else if (result == ESP_ERR_ESPNOW_ARG) {
        Serial.println("Invalid Argument");
      } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
        Serial.println("Internal Error");
      } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
        Serial.println("ESP_ERR_ESPNOW_NO_MEM");
      } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
        Serial.println("Peer not found.");
      } else {
        Serial.println("Not sure what happened");
      }
    }
    delay(100);
  }
  if(check == 1){
    delay(100);
    check = 0;
    for(int i = 0 ; i < DATASIZE; i++){
      if(i == 47){                 // if data is too long have 'L' for flag
        data1[i] = 'L';
      }
      else if((char)data1[i] == 'P'){   
        // when encounter path end 
        // shift p and add current layer number to it
        data1[i++] = 'layer_n';    // layer number
        data1[i++] = ','; 
        data1[i++] = 'node_rank';    // node number   
        data1[i++] = '>';
        data1[i++] = 'P';
      }
    }
    for (int i = 0; i < SlaveCnt; i++) {
      const uint8_t *peer_addr = slaves[i].peer_addr;
      if (i == 0) { // print only for first slave
        Serial.print("Sending: ");
        Serial.println((char *)data1);
      }
      int flag = 1;
      int my_arr[MAX_NO_OF_PARENTS][6] = {{48, 174, 164, 117, 91, 149},
                                         {48, 174, 164, 25, 63, 81}} ;
      //remeber  you have to give ap address and not device address!!!!!
      for(int loop_j = 0; loop_j < MAX_NO_OF_PARENTS; loop_j++){
        flag = 1;
        for(int loop_i = 0; loop_i < 6; loop_i++){
          if(my_arr[loop_j][loop_i] != peer_addr[loop_i])
            flag = 0;
        }
        if(flag){
          break;
        }
      }
    if(flag){
      esp_err_t result = esp_now_send(peer_addr, data1, DATASIZE);
      Serial.print("Send Status: ");
      if (result == ESP_OK) {
        Serial.println("Success");
      } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
        // How did we get so far!!
        Serial.println("ESPNOW not Init.");
      } else if (result == ESP_ERR_ESPNOW_ARG) {
        Serial.println("Invalid Argument");
      } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
        Serial.println("Internal Error");
      } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
        Serial.println("ESP_ERR_ESPNOW_NO_MEM");
      } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
        Serial.println("Peer not found.");
      } else {
        Serial.println("Not sure what happened");
      }
    }
    delay(100);
  }
  }
}


// callback when data is sent from Master to Slave
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: "); Serial.println(macStr);
  Serial.print("Last Packet Send Status: "); Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

// callback when data is recv from Master
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  digitalWrite(LED_PIN, 1);
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("\t\tLast Packet Recv from: "); Serial.println(macStr);
  Serial.print("\t\tLast Packet Recv Data: "); Serial.println((char *)data);
  Serial.println("");
  delay(10); // just a little bit
  Serial.println("------------DDAATAA Recieved-------------");
  for(int i = 0; i < DATASIZE; i++){
      data1[i] = data[i];
  }
  delay(500);
  Serial.print("\t\tData Copied in data1: "); Serial.println((char *)data1);
  check = 1;
  digitalWrite(LED_PIN, 0);
}


// config AP SSID
void configDeviceAP() {
  String Prefix = "ESPNOW:";
  String Mac = WiFi.macAddress();
  String SSID = Prefix + Mac;
  String Password = "123456789";
  bool result = WiFi.softAP(SSID.c_str(), Password.c_str(), CHANNEL_SLAVE, 0);
  if (!result) {
    Serial.println("AP Config failed.");
  } else {
    Serial.println("AP Config Success. Broadcasting with AP: " + String(SSID));
  }
}

void setup() {
  dht.begin();
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  //Set device in STA mode to begin with
  WiFi.mode(WIFI_MODE_APSTA);
  Serial.println("ESPNow/Multi-Slave/Master Example");
  
  // configure device AP mode
  configDeviceAP();
  
  // This is the mac address of the Master in Station Mode
  Serial.print("STA MAC: "); Serial.println(WiFi.macAddress());
  Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
  // Init ESPNow with a fallback logic
  InitESPNow();
  // Once ESPNow is successfully Init, we will register for Send CB(Cell Broadcast) to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // In the loop we scan for slave
  ScanForSlave();
  // If Slave is found, it would be populate in `slave` variable
  // We will check if `slave` is defined and then we proceed further
  if (SlaveCnt > 0) { // check if slave channel is defined
    // `slave` is defined
    // Add slave as peer if it has not been added already
    manageSlave();
    // pair success or already paired
    // Send data to device
    sendData();
  } else {
    // No slave found to process
  }

  // wait for 3seconds to run the logic again
  delay(2000);
}
