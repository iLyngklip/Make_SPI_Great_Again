/*  PINOUT
 * ------------------------------------------
 * |     Arduino  T Port      T  Papilio    |
 * |  Clock:   13 | B00100000 |  13         |
 * |  MISO:    12 | B00010000 |  12         |
 * |  MOSI:    11 | B00001000 |  11         |
 * |  SS:      10 | B00000100 |  09         |
 * ------------------------------------------
 */

#define BASIC_ADRESS  0x7E8000


// int16_t arrayToSaveToFlash[] = {0x0000, 0x0000, 0x0000, 0x0000};
int16_t arrayToSaveToFlash[] = {0x0000, 0x0000};

char storeReadData[2] = {0x00, 0x00};
char storeRDSR = 0x00;
char RDSCUR = 0x00;

boolean WEL = true;
boolean WIP = false;

void setup() {
  DDRB = DDRB|B00101111; // Set input/output
  
  
  Serial.begin(250000);
  //delay(1000);
  /*
  writeStuff();
  //delay(2000);
  readTwoBytes();
  //delay(2000);
  Serial.print("storeReadData[0]: "); Serial.println(storeReadData[0], HEX);
  Serial.print("storeReadData[1]: "); Serial.println(storeReadData[1], HEX);
  */
  delay(2000);
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
  RDSCUR = 0x00;
  

  writeStuff();
  //delay(2000);
  readTwoBytes();
  //delay(2000);
  Serial.print("storeReadData[0]:\t"); Serial.println(storeReadData[0], HEX);
  Serial.print("storeReadData[1]:\t"); Serial.println(storeReadData[1], HEX);

  Serial.print("storeRDSR:\t\t"); Serial.println(storeRDSR, BIN);
  Serial.println("");

  delay(1000);
}
















// ######################################
// ####   SPECIFIC WRITE FUNCTIONS   ####
// ######################################

  /* SÅDAN SER WRITE-CYCLEN UD!
   * ----------------------------
   * WREN                                         0x06
   * RDSR                                         0x05
   *  L_ WREN=1?                                    Bit 1 fra RDSR
   * Contenious program mode                      0xAD 
   *  L_ Adressen                                   ADD(24)
   *  L_ Write data                                 DATA(16)
   * RDSR command                                 0x05
   *  L_ WIP = 0?                                   Bit 0 fra RDSR      
   * RDSCUR command - Tjek om det lykkedes        0x2B
   *  L_ P_FAIL / E_FAIL = 1?                       FORFRA! ALT ER DONE! :o
   * WREN = 0   0x04
   */


void writeEnable(){
  highSS();
  transmitOneByteSPI(0x06); // WEL = 1
  lowSS();
  Serial.println("WE");
}

void writeDisable(){
  highSS();
  transmitOneByteSPI(0x04); // WEL = 0
  lowSS();
}

void readStatusRegister(){
  highSS();  // Cycle
  lowSS();   // Slave-select

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
  highSS(); 
}

void sendContinouslyProgramCommand(){
  transmitOneByteSPI(0xAD); // CP command
}

void continouslyProgram(){
/* The sequence of issuing CP instruction is: 
 *  1 → CS# goes low  
 *  2 → sending CP instruction code
 *  3 → 3-byte address on SI pin
 *  4 → two data bytes on SI
 *  5 → CS# goes high to low 
 *  6 → sending CP instruction and then continue two data bytes are programmed
 *  7 → CS# goes high to low
 *  8 → till last desired two data bytes are programmed
 *  9 → CS# goes high to low
 * 10 → sending WRDI (Write Disable) instruction to end CP mode
 *   
 *  → send RDSR instruction to verify if CP mode word program ends, or send RDSCUR to check bit4 to verify if CP mode ends. 
 */
  
  lowSS();                            // Step 1
  sendContinouslyProgramCommand();    // Step 2
  sendAdress(BASIC_ADRESS);           // Step 3 

  // Herefter bliver al dataen sendt afsted
  for(int i = 0; i < sizeof(arrayToSaveToFlash); i = i++){
    // Først skal dataen opdeles, da de ligger i 16-bit samples
    // og det kun er muligt at smide 1 byte afsted ad gangen.
    transmitOneByteSPI(arrayToSaveToFlash[i]>>8);       // First data byte
    transmitOneByteSPI(arrayToSaveToFlash[i]&0x00FF);   // Second

    // Cycle Slave-Select (HIGH → LOW)
    cycleSS();  // Step 5

    // Hvis der er mere data der skal afsted (aka min. 1 sample mere)
    if(i < sizeof(arrayToSaveToFlash) - 1){
      sendContinouslyProgramCommand(); // Step 6
    }// if
  }// for

  // Cycle Slave-Select (HIGH → LOW)
  cycleSS();      // Step 9 -> Jeg er ikke sikker på denne egentlig skal være her? 
  writeDisable(); // Step 10
  
}// continouslyProgram















// #####################################
// ####   SPECIFIC READ FUNCTIONS   ####
// #####################################
  /*  The sequence of issuing READ instruction is: 
   *   1 → CS# goes low
   *   2 → sending READ instruction code
   *   3 → 3-byte address on SI 
   *   4 → data out on SO
   *   5 → to end READ operation can use CS# to high at any time during data out. 
   *   
   *   Kilde: Datablad pp. 19
   */

  void sendReadInstruction(){
    /*  Send read-kommandoen til flashen
     */
    transmitOneByteSPI(0x03); // Read command
  }

  void readData(char lenghtInBytes){
    /*  Denne funktion skal læse lengthInBytes-antal bytes
     *   fra flash'en
     */
  }













// ##################################
// ####   READ/WRITE FUNCTIONS   ####
// ##################################

void sendAdress(uint32_t adress){
  /* Send adressen over SPI
   *  
   */
  transmitOneByteSPI((adress&0xFF0000) >> 16);  // ----|
  transmitOneByteSPI((adress&0x00FF00) >> 8);   //     |-> START Adressen i 24 bit
  transmitOneByteSPI(adress&0x0000FF);          // ----|     á 8 bit pr. gang
}


char readOneByteSPI(){
  char tempInputData = 0x00;
  // Læs data
  lowMosi();
  for(int k = 7; k >= 0; k--){
    highClock();
    delayMicroseconds(1);
    lowClock();
    delayMicroseconds(1);
    
    // Hvis data in er HIGH efter falling-edge clock
    if(bitRead(PINB, 4) == 1){
      bitSet(tempInputData, k);  // Sæt den pågældende bit high
    } 
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


void readRDSCUR(){
  lowSS();
  transmitOneByteSPI(0x2B);
  
  RDSCUR = readOneByteSPI();
  highSS();
}













// ##################################
// ####   READ/WRITE FUNCTIONS   ####
// ##################################
void readTwoBytes(){
  /*  The sequence of issuing READ instruction is: 
   *   1 → CS# goes low
   *   2 → sending READ instruction code
   *   3 → 3-byte address on SI 
   *   4 → data out on SO
   *   5 → to end READ operation can use CS# to high at any time during data out. 
   *   
   *   Kilde: Datablad pp. 19
   */

  lowSS();               // Step 1
  sendReadInstruction();    // Step 2
  sendAdress(BASIC_ADRESS); // Step 3

  /*
    transmitOneByteSPI(0x7E);// ----|
    transmitOneByteSPI(0x80);//     |-> Adressen i 24 bit
    transmitOneByteSPI(0x00);// ----|     á 8 bit pr. gang
  */

  // Step 4 
  storeReadData[0] = readOneByteSPI();//  ---|-> Læser 2x8 bit og gemmer dem
  storeReadData[1] = readOneByteSPI();//  ---|    i en global variabel, for nu.
                                      //  ---| Skift denne til at bruge readData() i stedet
  
  highSS();  // Step 5
  // Vi er done
}

void writeStuff(){
  /* SÅDAN SER WRITE-CYCLEN UD!
   * ----------------------------
   * WREN                                         0x06
   * RDSR                                         0x05
   *  L_ WREN=1?                                    Bit 1 fra RDSR
   * Contenious program mode                      0xAD 
   *  L_ Adressen                                   ADD(24)
   *  L_ Write data                                 DATA(16)
   * RDSR command                                 0x05
   *  L_ WIP = 0?                                   Bit 0 fra RDSR      
   * RDSCUR command - Tjek om det lykkedes        0x2B
   *  L_ P_FAIL / E_FAIL = 1?                       FORFRA! ALT ER DONE! :o
   * WREN = 0   0x04  Denne er skrevet!
   */
   
    writeEnable();          // Step 1
    
    do{                     // Step 2
      readStatusRegister(); // Step 2
      Serial.println("WR: 2");
    }while(!WEL);           // Step 2

    continouslyProgram();   // Step 3

    do{                     // Step 4
      readStatusRegister(); // Step 4
      Serial.println("WR: 4");
    }while(WIP);           // Step 4

    readRDSCUR();           // Step 5
    // Her tjekker vi om det faktisk lykkedes
    if(bitRead(RDSCUR, 5) == 1 || bitRead(RDSCUR, 6) == 1){
      // The programming failed! 
      throwErrorMessage();
    } else {
      // Ting virkede! :D
      Serial.println("Ting virkede!");
    }

  /* Bruges ikke mere
    transmitOneByteSPI(0x7E);// ----|
    transmitOneByteSPI(0x80);//     |-> Adressen i 24 bit
    transmitOneByteSPI(0x00);// ----|     á 8 bit pr. gang
  
    transmitOneByteSPI(0xAD);// ----|-> Data i 16 bit
    transmitOneByteSPI(0xAD);// ----|     á 8 bit pr. gang
  
    readStatusRegister();     // read dat shit

    
    writeDisable();
  */
    
    highSS(); // Vi er done nu

}
















// #############################
// ####   OTHER FUNCTIONS   ####
// #############################
void throwErrorMessage(){
  Serial.println("-------------------------------------------");
  Serial.println("Something went wrong because you are stupid");
  Serial.println("-------------------------------------------");
  Serial.print("RDSCUR:\t\t"); Serial.println(RDSCUR, BIN);
  Serial.print("storeRDSR:\t"); Serial.println(storeRDSR, BIN);
  Serial.println("-------------------------------------------");
}














// #############################
// ####   BASIC FUNCTIONS   ####
// #############################
void cycleSS(){
  highSS();
  lowSS();
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

