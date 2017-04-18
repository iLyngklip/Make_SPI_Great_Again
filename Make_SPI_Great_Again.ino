#include "AudioSamples.h"

/*  Dette program sender indholdet af "arrayToSaveToFlash[]" over SPI
 *   til MX25L6445E flashen som sidder på Papilioen. 
 *   
 * 
 */




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

// Da flashen er fyldt med 1'ere, skriver vi bevidst 0'ere
// til den, så vi tjekker om vi rammer rigtigt.
  int16_t arrayToSaveToFlash[] = {0x0000, 0x0000};
  // int16_t arrayToSaveToFlash[] = {0x0000, 0x0000, 0x0000, 0x0000};
int16_t arrayOfArrays[] = {
  &arrayToSaveToFlash
          };



char storeReadData[2] = {0x00, 0x00}; // Her gemmer vi de to byte vi læser
char storeRDSR = 0x00;                // RDSR gemmes her. Omskriv til lokal variabel
char RDSCUR = 0x00;                   // RDSCUR gemmes her. Omskriv til lokal variabel

boolean WEL = true;   // Write Enable Latch - bit
boolean WIP = false;  // Write In Progress - bit

void setup() {
  DDRB = DDRB|B00101111; // Set input/output pinmodes

  Serial.begin(250000);
  delay(2000);
}

void loop() {
  // Resetter globale variabler
  storeReadData[0] = 0x00;
  storeReadData[1] = 0x00;
  storeRDSR = 0x00;
  RDSCUR = 0x00;
  


  
  writeStuff();   // Først skriver vi ting
  //delay(2000);
  readTwoBytes(); // Herefter læser vi ting
  //delay(2000);
  Serial.print("storeReadData[0]:\t"); Serial.println(storeReadData[0], HEX); // og printer
  Serial.print("storeReadData[1]:\t"); Serial.println(storeReadData[1], HEX); // hvad vi har læst

  Serial.print("storeRDSR:\t\t"); Serial.println(storeRDSR, BIN); // print RDSR
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
   *  ↳ WREN=1?                                    Bit 1 fra RDSR
   * Contenious program mode                      0xAD 
   *  ↳ Adressen                                   ADD(24)
   *  ↳ Write data                                 DATA(16)
   * RDSR command                                 0x05
   *  ↳ WIP = 0?                                   Bit 0 fra RDSR      
   * RDSCUR command - Tjek om det lykkedes        0x2B
   *  ↳ P_FAIL / E_FAIL = 1?                       FORFRA! ALT ER DONE! :o
   * WREN = 0   0x04
   */


void writeEnable(){
  /* The sequence of issuing WREN instruction is: 
   *  1 → CS# goes low
   *  2 → sending WREN instruction code
   *  3 → CS# goes high.
   * 
   *  Kilde: Datablad pp. 16
   */
   
  lowSS();                  // Step 1
  transmitOneByteSPI(0x06); // Step 2
  highSS();                 // Step 3
} // writeEnable

void writeDisable(){
  /* The sequence of issuing WRDI instruction is: 
   *  1 → CS# goes low
   *  2 → sending WRDI instruction code
   *  3 → CS# goes high. 
   *  
   *  Kilde: Datablad pp. 16
   */
   
  lowSS();
  transmitOneByteSPI(0x04); // WEL = 0
  highSS();
} // writeDisable

void readStatusRegister(){
  /* The sequence of issuing RDSR instruction is: 
   *  1 → CS# goes low
   *  2 → sending RDSR instruction code
   *  3 → Status Register data out on SO 
   *  
   *  Kilde: Databled pp. 17
   */
  cycleSS();                    // Step 1
  transmitOneByteSPI(0x05);     // Step 2
  storeRDSR = readOneByteSPI(); // Step 3
  
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
  highSS(); // High SS afterwards
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
 * 10.5 → send RDSR instruction to verify if CP mode word program ends, or send RDSCUR to check bit4 to verify if CP mode ends. 
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
  // Send adressen over SPI
  transmitOneByteSPI(adress&0xFF0000 >> 16);  // ----|
  transmitOneByteSPI((adress >> 8) & 0xFF);   //     |-> START Adressen i 24 bit
  transmitOneByteSPI(adress&0x0000FF);        // ----|     á 8 bit pr. gang
}


char readOneByteSPI(){
  char tempInputData = 0x00;
  // Læs data
  lowMosi();
  for(int k = 7; k >= 0; k--){
    cycleClock();
    
    // Hvis data in er HIGH efter falling-edge clock
    if(bitRead(PINB, 4) == 1){
      bitSet(tempInputData, k);  // Sæt den pågældende bit high
    } 
  }// for
  return tempInputData;
}

char transmitOneByteSPI(char data){
  // DDRB = DDRB|B00101111; // Set as output - Overflødig
  for(int i = 7; i >= 0; i--){
    if(bitRead(data,i) == 1){
      highMosi();
    } else {
      lowMosi();
    }
    cycleClock();   
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

  lowSS();                  // Step 1
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
   * Continously program mode                     0xAD 
   *  L_ Adressen                                   ADD(24)
   *  L_ Write data                                 DATA(16)
   * RDSR command                                 0x05
   *  L_ WIP = 0?                                   Bit 0 fra RDSR      
   * RDSCUR command - Tjek om det lykkedes        0x2B
   *  L_ P_FAIL / E_FAIL = 1?                       FORFRA! ALT ER DONE! :o
   * WREN = 0   0x04  Denne er skrevet!
   * 
   * ------------------------------------------------------------------------------
   * | TEKST-version:                                                             |
   * ------------------------------------------------------------------------------
   * || Write-Enable sendes (step 1) hvorefter vi afventer chippen er klar        |
   * || til at modtage data (step 2). Dette tjekkes med bit 1 fra                 |
   * || statusregistret (RDSR). Nu sættes chippen i Continously-Program (CP) mode |
   * || (step 3). Hvordan denne fungerer kan læses i funktionen                   |
   * || continouslyProgram(). Når continouslyProgram() er færdig med at skrive    |
   * || data til flash'en tjekkes bit 0 fra RDSR (step 4) Er denne = 0, er        |
   * || chippen færdig med at gemme og derfor klar til nye ting.                  |
   * || For at tjekke om dataen blev skrevet til flashen tjekkes P_FAIL & E_FAIL  |
   * || som er bit 5 & 6 fra RDSR. Er disse HIGH mislykkedes hele operationen og  |
   * || hele writeStuff() skal køres forfra.                                      |
   * ------------------------------------------------------------------------------
   */
   
    writeEnable();          // Step 1

    // Vent på Write-Enable-Latch bliver 1
    do{                     // Step 2
      readStatusRegister(); // Step 2
      Serial.println("WR: 2");
    }while(!WEL);           // Step 2

    // Fyr data afsted
    continouslyProgram();   // Step 3


    // Vent på Write-In-Progress bitten bliver 0 igen
    do{                     // Step 4
      readStatusRegister(); // Step 4
      Serial.println("WR: 4");
    }while(WIP);            // Step 4

    readRDSCUR();           // Step 5
    
    // Her tjekker vi om det faktisk lykkedes
    if(bitRead(RDSCUR, 5) == 1 || bitRead(RDSCUR, 6) == 1){
      // The programming failed! 
      throwErrorMessage();
    } else {
      // Ting virkede!
      // Denne else er overflødig
      Serial.println("Ting virkede!");
    }
    writeDisable();
    highSS(); // Vi er done nu
}
















// #############################
// ####   OTHER FUNCTIONS   ####
// #############################
void throwErrorMessage(){
  Serial.println("-------------------------------------------");
  Serial.println("Something went wrong because you are stupid");
  Serial.println("-------------------------------------------");
  Serial.print("RDSCUR:\t\t");      Serial.println(RDSCUR, BIN);
  Serial.print("storeRDSR:\t");  Serial.println(storeRDSR, BIN);
  Serial.println("-------------------------------------------");
}














// #############################
// ####   BASIC FUNCTIONS   ####
// #############################
void cycleSS(){
  highSS();
  lowSS();
}

void cycleClock(){
  highClock();
  delayMicroseconds(1);
  lowClock();
  delayMicroseconds(1);
}

void lowMosi(){
  //DDRB = DDRB|B00101111;  // Set as output - Overflødig
  PORTB &=    B11110111;   // Set low
}

void highMosi(){
  //DDRB = DDRB|B00101111; // Set as output - Overflødig
  PORTB |=    B00001000;  // Set it to high
}

void lowSS(){
  //DDRB = DDRB|B00101111;  // Set as output - Overflødig
  PORTB &=    B11111011;   // Set low
}

void highSS(){
  //DDRB = DDRB|B00101111; // Set as output - Overflødig
  PORTB |=    B00000100;  // Set it to high
}

void highClock(){
  //DDRB = DDRB|B00101111; // Set as output - Overflødig
  PORTB |=    B00100000;  // Set it to high
}

void lowClock(){
  //DDRB = DDRB|B00101111;  // Set as output - Overflødig
  PORTB &=    B11011111;   // Set low
}

