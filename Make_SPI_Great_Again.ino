
/*
 * Clock:   13  B00100000
 * MISO:    12  B00010000
 * MOSI:    11  B00001000
 * SS:      10  B00000100
 */
#define DEBUG 0
  /*
   * 3: FULL
   * 2: RDSR FULL
   * 1: RDSR FAST      <-- Choose this if you want serial output
   * 0: NONE            
   * 
   */

// const int slaveSelectPin = 10;
char storeReadData[2] = {0x00, 0x00};
char storeRDSR = 0x00;
boolean WEL = false;
boolean WIP = false;

void setup() {
  DDRB = DDRB|B00101111; // Set input/output
  
  
  Serial.begin(250000);
  //delay(1000);
  /*
  writeStuff();
  //delay(2000);
  readStuff();
  //delay(2000);
  Serial.print("storeReadData[0]: "); Serial.println(storeReadData[0], HEX);
  Serial.print("storeReadData[1]: "); Serial.println(storeReadData[1], HEX);
  */
}

void loop() {
  /* VIRKER
  for(int i = 0; i < 0xFF; i ++){
    transmitOneByteSPI(i);
  }
  */

  storeReadData[0] = 0x00;
  storeReadData[1] = 0x00;
  storeRDSR = 0x00;
  

  writeStuff();
  //delay(2000);
  readStuff();
  //delay(2000);
  Serial.print("storeReadData[0]:\t"); Serial.println(storeReadData[0], HEX);
  Serial.print("storeReadData[1]:\t"); Serial.println(storeReadData[1], HEX);

  Serial.print("storeRDSR:\t\t"); Serial.println(storeRDSR, BIN);
  Serial.println("");

  delay(200);

  
}

void readStatusRegister(){
  setSSHigh();  // Cycle
  setSSLow();   // Slave-select

  transmitOneByteSPI(0x05); // RDSR
  storeRDSR = readOneByteSPI();
  if(bitRead(storeRDSR, 1) == 1){
    WEL = true;
  } else {
    WEL = false;
  }
  if(bitRead(storeRDSR, 0) == 1){
    WIP = true;
  } else {
    WIP = false;
  }
  setSSHigh();
}

void WRDI(){
  setSSHigh();
  transmitOneByteSPI(0x04); // WREN = 0
  setSSLow();
  
}

char readOneByteSPI()
{
  char tempInputData = 0x00;
  // Læs data
  lowMosi();
  for(int k = 7; k >= 0; k--){
    highClock();
    delayMicroseconds(1);
    lowClock();
    delayMicroseconds(1);
    
    // Hvis data in er HIGH efter falling
    if(bitRead(PINB, 4) == 1){
      bitSet(tempInputData, k);  
    }// if
  }// for
  return tempInputData;
}
char transmitOneByteSPI(char data){
  // DDRB = DDRB|B00101111; // Set as output
  
  // lowSS(); -- Nah, it's allready done
  // delayMicroseconds(1);
  for(int i = 7; i >= 0; i--){
    if(bitRead(data,i) == 1){
      highMosi();
    } else {
      lowMosi();
    }
    highClock();
    delayMicroseconds(1);
    lowClock();
    delayMicroseconds(1);    
  }// for

 
}

void readStuff()
{
  /*
   * Read instruction     0x03
   * 
   * Adresse              ADD(24)
   *                      
   * Data                 Kommer ind
   * 
   * SS high for at slutte
   * 
   */
  setSSLow(); // Gør klar
  
  transmitOneByteSPI(0x03);

  transmitOneByteSPI(0x7E);// ----|
  transmitOneByteSPI(0x80);//     |-> Adressen i 24 bit
  transmitOneByteSPI(0x00);// ----|     á 8 bit pr. gang

  storeReadData[0] = readOneByteSPI(); 
  storeReadData[1] = readOneByteSPI();

  setSSHigh(); // Vi er done
}


void writeStuff()
{
  /* SÅDAN SER WRITE-CYCLEN UD!
   * ----------------------------
   * WREN     0x06
   * 
   * RDSR     0x05
   * 
   * WREN=1?  0x06
   * 
   * Program command (how?)   fyr data                      |
   *                                                        |----> 0xAD => ADD(24) + DATA(16)
   * Write program data (Write erase adress)    fyr data    |
   * 
   * RDSR command   0x05
   * 
   * WIP = 0?       
   * 
   * Read array data
   * 
   * Verify OK?
   * 
   * WREN = 0   0x04
   * 
   */
   
    // No need for delays, the chip is WAY 
    // than the arduino
    
    char tempRDSR = 0x00;
    
    setSSLow(); // Gør klar
   
    transmitOneByteSPI(0x06); // WREN = 1
    readStatusRegister();     // read dat shit
    
  
    transmitOneByteSPI(0x7E);// ----|
    transmitOneByteSPI(0x80);//     |-> Adressen i 24 bit
    transmitOneByteSPI(0x00);// ----|     á 8 bit pr. gang
  
    transmitOneByteSPI(0xAD);// ----|-> Data i 16 bit
    transmitOneByteSPI(0xAD);// ----|     á 8 bit pr. gang
  
    readStatusRegister();     // read dat shit
    WRDI();
  
    setSSHigh(); // Vi er done nu

}





















void setSSLow(){
  PORTB =     B00000000; // Set ALL low
}
void setSSHigh(){
  PORTB =     B00000100; // Set SS high  
}

void lowMosi(){
  //DDRB = DDRB|B00101111;  // Set as output
  PORTB &=    B11110111;   // Set low
}
void highMosi(){
  //DDRB = DDRB|B00101111; // Set as output
  PORTB |=    B00001000;  // Set it to high
}
void lowSS(){
  //DDRB = DDRB|B00101111;  // Set as output
  PORTB &=    B11111011;   // Set low
}
void highSS(){
  //DDRB = DDRB|B00101111; // Set as output
  PORTB |=    B00000100;  // Set it to high
}
void highClock(){
  //DDRB = DDRB|B00101111; // Set as output
  PORTB |=    B00100000;  // Set it to high
}
void lowClock(){
  //DDRB = DDRB|B00101111;  // Set as output
  PORTB &=    B11011111;   // Set low
}


