// Libraries needed for this program
#include <Arduino_FreeRTOS.h>
#include <IPAddress.h>
#include <EtherCard.h>
#include <Servo.h>

// Define global variables for base, arm, wrist and gripper positions
int i = 0, j = 0, k = 0; // Base
int x = 0, y = 0, z = 0; // Arm
int w = 0, r = 0;       // Wrist
int g = 0;              // Gripper

// Create servo objects for different robot parts
Servo BaseServo, ArmServo, WristServo, GripperServo;

// Define pins to which servos are connected
int pinServo_Base = 3;
int pinServo_Arm = 5;
int pinServo_Wrist = 6;
int pinServo_Gripper = 9;

// Define pin for the IR sensor used in sorting
int pinIR_Magazine = 8; 

// Initialize sensor readings to zero
int valIR_Magazine = 0;
int valIR_Sorting = 0;

// Counter variable, probably for tracking number of processed items
int Counter = 0;

// Task handle for FreeRTOS task
TaskHandle_t RoboticArmTask_handle = NULL;

// Ethernet specific configurations and variables
const int n; 
static byte mymac[] = { 0x70,0x69,0x69,0x2D,0x30,0x31 };
static byte myip[] = { 192, 168, 1, 3 };
static byte gwip[] = { 192, 168, 1, 1 };
static byte mask[] = { 255, 255, 255, 0 };
const int dstPort PROGMEM = 1234;
byte Ethernet::buffer[700];

void initializeEthernet() {
  // Initialize Ethernet communication and set up IP configurations
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0) {
    Serial.println("Failed to access Ethernet controller"); 
    return;
  }
  ether.staticSetup(myip, gwip, 0, mask);
  Serial.println(F("\n[Receiver]")); 
  ether.printIp("IP: ", ether.myip); 
  ether.printIp("GW: ", ether.gwip);
}

void udpSerialPrint(uint16_t dest_port, uint8_t src_ip[IP_LEN], uint16_t src_port, const char *data, uint16_t len) {
  // Callback function to print received UDP packets
  IPAddress src(src_ip[0], src_ip[1], src_ip[2], src_ip[3]);
  Serial.println(data); 
  if(data=='NO') {
    int n = 0; 
    Serial.println(n);
  }
}

void setup() {
  // Initialize serial communication and the Ethernet interface
  Serial.begin(9600);
  initializeEthernet();
  ether.udpServerListenOnPort(&udpSerialPrint, dstPort);

  // Attach servos to their respective pins
  BaseServo.attach(pinServo_Base);
  ArmServo.attach(pinServo_Arm);
  WristServo.attach(pinServo_Wrist);
  GripperServo.attach(pinServo_Gripper);

  // Define the pin mode for IR sensor
  pinMode(pinIR_Magazine, INPUT);

  // Create a FreeRTOS task for the robotic arm operation
  xTaskCreate(RoboticArmTask, "Robotic Arm Task", 1000, NULL, 1, &RoboticArmTask_handle); 
}

void loop() { 
  // Intentionally left empty, as operations are managed via FreeRTOS tasks
}

// function that's responsible for taking products to sorting 
static void RoboticArmTask(void* parameters) {
  while(1) {
  int p = 0;
  // receive packets, get ready for callbacks 
  ether.packetLoop(ether.packetReceive()); 
  valIR_Magazine = digitalRead(pinIR_Magazine); 
  if(valIR_Magazine == 1) {
    Counter++;
    
    // between magazine and sorting 
    for (i = j; i <= 120; i++) {
      BaseServo.write(i);
      vTaskDelay(60/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS);
    
    // arm gonna move down 
    for the magazine for (x = y; x <= 20; x++) {
      ArmServo.write(x);
      vTaskDelay(20/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); 
    
    // base centering into the magazine
    for (j = i; j <= 133; j++) {
      BaseServo.write(j);
      vTaskDelay(60/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); 
    i = 0;
    
    // gripper grips
    for (g = p; g <= 48.5; g++) {
      GripperServo.write(g);
      vTaskDelay(20/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); 
    
    // arm gonna move up to pick the product
    for (y = x; y >= 0; y--) {
      ArmServo.write(y);
      vTaskDelay(20/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); 
    
    // wrist rotating 90
    for (w = r; w <= 90; w++) {
      WristServo.write(w);
      vTaskDelay(20/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); 
    
    // base going to sorting
    for (k = j; k >= 60; k--) {
      BaseServo.write(k);
      vTaskDelay(60/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); 
    j = 0;
    
    // arm moving down again for sorting
    for (x = 0; x <= 15; x++) {
      ArmServo.write(x);
      vTaskDelay(20/portTICK_PERIOD_MS); }
    vTaskDelay(3000/portTICK_PERIOD_MS); 
    
    // gripper releases
    for ( p = g; p >= 0; p--) {
      GripperServo.write(p);
      vTaskDelay(20/portTICK_PERIOD_MS); }
    vTaskDelay(3000/portTICK_PERIOD_MS);
    
    WristServo.write(0);
    } // Taking product to sorting 
    
    else { 
      // everything sets to zero
      for (int a = k; a >= 0; a--) { 
        k--;
        BaseServo.write(a);
        vTaskDelay(60/portTICK_PERIOD_MS); }
        
      for (int a = y; a >= 0; a--) { 
        y--;
        ArmServo.write(a);
        vTaskDelay(20/portTICK_PERIOD_MS); }
        
      for (int a = w; a >= 0; a--) { 
        w--;
        WristServo.write(a);
        vTaskDelay(20/portTICK_PERIOD_MS); }
    } // no product to be found
    
    BaseServo.write(0); 
    ArmServo.write(0); 
    WristServo.write(0); 
    GripperServo.write(0);
    
    if(Counter > 7) 
    { 
      vTaskDelete(NULL);
  } 
  }
} // END of Base Task function
