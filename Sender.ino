// Import necessary libraries
#include <Arduino_FreeRTOS.h> 
#include "LiquidCrystal_I2C.h" 
#include <EtherCard.h>
#include "semphr.h"
#include <Servo.h>

// Flag to determine the type of product to be sent
const int n = 1; // if n = 0, textToSend is sent (Metal), if n = 1, textToSend2 is sent (Non Metal) 

// Network configuration details
static byte mymac[] = { 0x1A,0x2B,0x3C,0x4D,0x5E,0x6F }; // MAC address
static byte myip[] = { 192, 168, 1, 2 }; // Local IP address
static byte gwip[] = { 192, 168, 1, 1 }; // Gateway IP address
static byte mask[] = { 255, 255, 255, 0 }; // Subnet mask
static byte dstIp[] = { 192, 168, 1, 3 }; // Destination IP address

// Ports for communication
const int dstPort PROGMEM = 1234; 
const int srcPort PROGMEM = 4321;

// Messages to send over Ethernet
char noProduct[] = "NO";
char Product[] = "YES";

// Ethernet communication buffer and a timer for delays
byte Ethernet::buffer[700];
static uint32_t timer;

// LCD configuration
LiquidCrystal_I2C lcd(0x3F,16,2); // Address, Number of columns, Number of rows
boolean state = true; // Flag for LCD state

// Function declarations for FreeRTOS tasks
static void SortingTask(void* parameters); 
static void PackagingTask(void* parameters); 
static void LCDTask(void* parameters);

// Global variables for packaging logic
int i = 0; // Counter for Packaging 1
int x = 0; // Counter for Packaging 2

// Servo configurations
Servo SortingServo; 
Servo Packaging1Servo; 
Servo Packaging2Servo;

// Pin assignments for servos
int pinServo_Sorting = 3;
int pinServo_Packaging1 = 5;
int pinServo_Packaging2 = 6;

// Pin assignment for the sorting proximity sensor
int pinPROX = A0;

// Pin assignments for IR sensors
int pinIR_Sorting = 2;  // Sorting IR sensor
int pinIR_Packaging1 = 4;  // Packaging 1 IR sensor 
int pinIR_Packaging2 = 7;  // Packaging 2 IR sensor

// Sensor initial values
int valIR_Packaging1 = 0;
int valIR_Packaging2 = 0;
int valIR_Sorting = 0;
int valPROX = 0;

// Counters for various tasks
int counterSorting = 0;
int counterPackaging = 0;
int counterPackaging1_LCD = 0;
int counterPackaging2_LCD = 0;

// Binary semaphores for task synchronization
SemaphoreHandle_t xBinarySemaphore;  // For sorting to packaging synchronization
SemaphoreHandle_t xBinary1Semaphore; // For packaging to LCD synchronization

// Task handles for future operations or management
TaskHandle_t SortingTask_handle;
TaskHandle_t PackagingTask_handle; 
TaskHandle_t LCDTask_handle;

/* Ethernet initialization function */ 
void initializeEthernet()
{
  // Initialize Ethernet communication
  if (ether.begin(sizeof Ethernet::buffer, mymac, 10) == 0)
  { 
    Serial.println("Failed to access Ethernet controller"); 
    return;
  }
  // Set static IP configuration 
  ether.staticSetup(myip, gwip, 0, mask);

  // Print configuration details to Serial
  Serial.println(F("\n[Sender]")); 
  ether.printIp("IP: ", ether.myip); 
  ether.printIp("GW: ", ether.gwip);
}

void setup() {
  // Begin Serial communication
  Serial.begin(57600);

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  // Initialize Ethernet communication
  initializeEthernet();
  
  // Set LCD display text
  lcd.setCursor(0,0); 
  lcd.print("Metal Count: "); 
  lcd.setCursor(0,1); 
  lcd.print("Artelon Count: ");
  
  // Create semaphores for task synchronization
  xBinarySemaphore = xSemaphoreCreateBinary();
  xBinary1Semaphore = xSemaphoreCreateBinary();
  
  // Attach servos to their respective pins
  SortingServo.attach(pinServo_Sorting); 
  Packaging1Servo.attach(pinServo_Packaging1); 
  Packaging2Servo.attach(pinServo_Packaging2);
  
  // Define input pins for sensors
  pinMode(pinPROX, INPUT);
  pinMode(pinIR_Sorting, INPUT); 
  pinMode(pinIR_Packaging1, INPUT); 
  pinMode(pinIR_Packaging2, INPUT);
  
  // Create FreeRTOS tasks with given priorities
  xTaskCreate(SortingTask, "Sorting_Task", 200, NULL, 1, &SortingTask_handle); 
  xTaskCreate(PackagingTask, "Packaging_Task", 200, NULL, 2, &PackagingTask_handle); 
  xTaskCreate(LCDTask, "LCD_Task", 100, NULL, 3, &LCDTask_handle);
  
  // Start the task scheduler
  vTaskStartScheduler();
}

// Main loop (empty because functionality is in the FreeRTOS tasks)
void loop() { }

// function that's responsible for product sorting 
static void SortingTask(void* parameters) {
  while(1) {
  valIR_Sorting = digitalRead(pinIR_Sorting); 
  valPROX = digitalRead(pinPROX);
  
  // ethernet sending
  for(int i = 0; i <= 3; i++){
    if (millis() > timer) {
      timer = millis() + 100;
      // testing funtion if changed the global variable from the top from zero or one 
      //as to identify metal or non metal this should be your function
      if(valIR_Sorting == 1) {
        ether.sendUdp(noProduct,sizeof(noProduct),srcPort,dstIp,dstPort );} 
        else if(valIR_Sorting == 0) {
          ether.sendUdp(Product,sizeof(Product),srcPort,dstIp,dstPort );}
  } }
  
  // sorting process
  if(valIR_Sorting == 0 && valPROX == 1) {
    counterSorting++;
    for (int i = 94; i >= 45; i--) {
      SortingServo.write(i);
      vTaskDelay(10/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS);
    } // Metal Sorting
    
  else if(valIR_Sorting == 0 && valPROX == 0) {
    counterSorting++;
    for (int i = 94; i <= 150; i++) {
      SortingServo.write(i);
      vTaskDelay(10/portTICK_PERIOD_MS); }
    vTaskDelay(1000/portTICK_PERIOD_MS); } // Artelon Sorting
    
  else {
  SortingServo.write(94); 
  vTaskDelay(1000/portTICK_PERIOD_MS);
  } // no product to be found
  
  SortingServo.write(94); 
  vTaskDelay(1000/portTICK_PERIOD_MS);
  
  // gives semaphore to the packaging 
  xSemaphoreGive(xBinarySemaphore);
  
  // deleting task after 8 products 
  if(counterSorting > 7) {
    vTaskDelete(SortingTask_handle); }
  }
  } // END of SortingTask function
  
  // function that's responsible for Packaging 1 
  static void PackagingTask(void* parameters) {
  while(1) {
    // takes semaphore from the sorting 
    xSemaphoreTake(xBinarySemaphore, portMAX_DELAY); 
    valIR_Packaging1 = digitalRead(pinIR_Packaging1); 
    valIR_Packaging2 = digitalRead(pinIR_Packaging2);
    
    if(valIR_Packaging1 == 0) { 
      counterPackaging++; 
      Packaging2Servo.write(0); 
      vTaskDelay(273/portTICK_PERIOD_MS); 
      Packaging2Servo.write(90);
      vTaskDelay(273/portTICK_PERIOD_MS); }
      
    if(valIR_Packaging2 == 0) { 
      counterPackaging++; 
      Packaging1Servo.write(0); 
      vTaskDelay(273/portTICK_PERIOD_MS); 
      Packaging1Servo.write(90); 
      vTaskDelay(273/portTICK_PERIOD_MS);
    }
    // gives semaphore to the LCD 
    //xSemaphoreGive(xBinary1Semaphore);
    
    if(counterPackaging > 7) { 
      vTaskDelete(PackagingTask_handle);
    } 
    }
  } // END of SortingTask function
  
  // function that's responsible for LCD 
  static void LCDTask (void* parameters) {
    while(1) {
    // takes semaphore from the packaging 
    //xSemaphoreTake(xBinary1Semaphore, portMAX_DELAY); 
    valIR_Packaging1 = digitalRead(pinIR_Packaging1); 
    valIR_Packaging2 = digitalRead(pinIR_Packaging2);
    
    if (valIR_Packaging1 == 0 && state == true){
      counterPackaging1_LCD++;
      state = false;
      lcd.setCursor(15,0); 
      lcd.print(counterPackaging1_LCD); 
      vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    else if (valIR_Packaging2 == 0 && state == true){
      counterPackaging2_LCD++;
      state = false;
      lcd.setCursor(15,1); 
      lcd.print(counterPackaging2_LCD); 
      vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    if (valIR_Packaging1 == 1 || valIR_Packaging2 == 1 ) { 
      state = true; 
      vTaskDelay(1000/portTICK_PERIOD_MS);
    }
    if(counterPackaging1_LCD > 3 && counterPackaging2_LCD > 3 ) { 
      vTaskDelete(LCDTask_handle);
    } }
} // END of LCDTask function
