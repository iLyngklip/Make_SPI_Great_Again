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

#define BASIC_ADRESS      0x010000
#define BASIC_ADRESS_OLD  0x7E8000
uint32_t adressenViHusker = BASIC_ADRESS;
const uint32_t startAdressen = BASIC_ADRESS;

char antalBytes = 2;


// Da flashen er fyldt med 1'ere, skriver vi bevidst 0'ere
// til den, så vi tjekker om vi rammer rigtigt.
  //byte arrayToSaveToFlash[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  const byte arrayToSaveToFlash[] = {
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA,
    0xAA, 0x10, 0x20, 0x30, 0x40, 0x50, 0x60, 0x70, 0x80, 0x90, 0xA0, 0xB0, 0xC0, 0xD0, 0xE0, 0xFA
  };
  // int16_t arrayToSaveToFlash[] = {0x0000, 0x0000, 0x0000, 0x0000};




byte storeReadData[sizeof(arrayToSaveToFlash)] = {0x00, 0x00}; // Her gemmer vi de to byte vi læser
byte storeRDSR = 0x00;                // RDSR gemmes her. Omskriv til lokal variabel
byte RDSCUR = 0x00;                   // RDSCUR gemmes her. Omskriv til lokal variabel

boolean WEL = true;   // Write Enable Latch - bit
boolean WIP = false;  // Write In Progress - bit
boolean BP0 = true;//   |
boolean BP1 = true;//   |-> Protection
boolean BP2 = true;//   |

void setup() {
  DDRB = DDRB|B00101111; // Set input/output pinmodes

  Serial.begin(250000);
  blockErase(adressenViHusker);
  blockErase(adressenViHusker);
  blockErase(adressenViHusker);
  delay(2000);
}

void loop() {
  // Resetter globale variabler
  // storeReadData[0]  = 0x00;
  // storeReadData[1]  = 0x00;
  for(int i = 0; i < sizeof(arrayToSaveToFlash); i++){
    storeReadData[i] = 0x00;
  }
  storeRDSR         = 0x00;
  RDSCUR            = 0x00;
  
  //while(1){
  // readStatusRegister(); 
  // readRDSCUR();
  //}
    
  
  // blockErase(adressenViHusker);
  delay(100);
  // Serial.print("adressenViHusker:\t"); Serial.println(adressenViHusker, HEX);
  pageProgram();  // Page program først!
  // pageProgram();  // Page program først!
      // writeStuff();   // Først skriver vi ting
  // delay(2000);
  checkBP();
  
  readTwoBytes(); // Herefter læser vi ting
  //delay(2000);
/*     
  for(int i = 0; i < sizeof(arrayToSaveToFlash); i+=4){
    Serial.print("SRD["); Serial.print(i); Serial.print("]:\t"); Serial.print(storeReadData[i], HEX); Serial.print("\t");
    Serial.print("SRD["); Serial.print(i+1); Serial.print("]:\t"); Serial.print(storeReadData[i+1], HEX); Serial.print("\t");
    Serial.print("SRD["); Serial.print(i+2); Serial.print("]:\t"); Serial.print(storeReadData[i+2], HEX); Serial.print("\t");
    Serial.print("SRD["); Serial.print(i+3); Serial.print("]:\t"); Serial.println(storeReadData[i+3], HEX);
  
  }
*/
  for(int i = 0; i < sizeof(arrayToSaveToFlash); i++){  
    if(storeReadData[i] != arrayToSaveToFlash[i]){
      Serial.print("storeReadData["); Serial.print(i); Serial.print("] IKKE ENS\n");
    }
  }
  /*
  Serial.print("storeReadData[0]:\t"); Serial.println(storeReadData[0], HEX); // og printer
  Serial.print("storeReadData[1]:\t"); Serial.println(storeReadData[1], HEX); // hvad vi har læst
  */
  // Serial.print("storeRDSR:\t\t"); Serial.println(storeRDSR, BIN); // print RDSR
  // Serial.println("");
  
  delay(100);
  if(adressenViHusker >= BASIC_ADRESS + (0x10000 - (2*sizeof(arrayToSaveToFlash)))){
      adressenViHusker = BASIC_ADRESS;
    } else {
      adressenViHusker += (sizeof(arrayToSaveToFlash)*2);
    }
    Serial.print("adressenViHusker:\t"); Serial.println(adressenViHusker, HEX);
}


void checkBP(){
  if(BP0 ||  BP1 || BP2){
    Serial.println("Something is protected");
  } else {
    // Serial.println("NOT protected");
  }
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
   *  3 → Status Register data out on MISO 
   *  
   *  Kilde: Databled pp. 17
   */
  highSS();                     // Cycle
  lowSS();                      // Slave-select
  transmitOneByteSPI(0x05);     // Step 2
  storeRDSR = readOneByteSPI(); // Step 3
  
  if(bitRead(storeRDSR, 1) == 1){
    WEL = 1;
  } else {
    WEL = 0;
  }
  if(bitRead(storeRDSR, 0) == 1){
    WIP = 1;
    // Serial.println("WIP = 1");
  } else {
    WIP = 0;
    // Serial.println("WIP = 0");
  }
  if(bitRead(storeRDSR, 2) == 1){
    BP0 = 1;
  } else {
    BP0 = 0;
  }
  if(bitRead(storeRDSR, 3) == 1){
    BP1 = 1;
  } else {
    BP1 = 0;
  }
  if(bitRead(storeRDSR, 4) == 1){
    BP2 = 1;
  } else {
    BP2 = 0;
  }
  if(bitRead(storeRDSR, 5) == 1){
    
  }
  if(bitRead(storeRDSR, 6) == 1){
    
  }
  if(bitRead(storeRDSR, 7) == 1){
    // SRWD
    // Serial.println("RDSR: SRWD = 1");
  } else {
    // Serial.println("RDSR: SRWD = 0");
  }
  
  // highSS(); // High SS afterwards
  // highSS(); // High SS afterwards
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
  highSS();                           // just to be safe
  lowSS();                            // Step 1
// Serial.println("sendContinouslyProgramCommand");
  sendContinouslyProgramCommand();    // Step 2
  sendAdress(adressenViHusker);           // Step 3 
// Serial.println("Adress sent");
  // Herefter bliver al dataen sendt afsted
  for(int i = 0; i < 2; i++){
  // Serial.println("For-loop");
    // Først skal dataen opdeles, da de ligger i 16-bit samples
    // og det kun er muligt at smide 1 byte afsted ad gangen.
    transmitOneByteSPI((arrayToSaveToFlash[i]>>8)&0xFF);       // First data byte
    transmitOneByteSPI(arrayToSaveToFlash[i]&0xFF);   // Second

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

  transmitOneByteSPI((adress >> 16) & 0xFF);  // ----|
  transmitOneByteSPI((adress >> 8 ) & 0xFF);   //     |-> START Adressen i 24 bit
  transmitOneByteSPI(adress&0x0000FF);        // ----|     á 8 bit pr. gang

/*
    transmitOneByteSPI(0x7E);// ----|
    transmitOneByteSPI(0x80);//     |-> Adressen i 24 bit
    transmitOneByteSPI(0x00);// ----|     á 8 bit pr. gang
*/
}


byte readOneByteSPI(){
  byte tempInputData = 0x00;
  // Læs data
  lowMosi();
  // cycleClock();    DA DER LIGE ER BLEVET SENDT EN KOMMANDO ER DER
  //                  IKKE BEHOV FOR AT CYCLE CLOCKEN
  
  for(int k = 7; k >= 0; k--){
    highClock();
    // Hvis data in er HIGH efter falling-edge clock
    if(bitRead(PINB, 4)){
      bitSet(tempInputData, k);  // Sæt den pågældende bit high
    } 
    lowClock();
  }// for
  return tempInputData;
}

void transmitOneByteSPI(char data){
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
  // Read Security Register
  lowSS();
  transmitOneByteSPI(0x2B);
  RDSCUR = readOneByteSPI();
  highSS();
  
  if(bitRead(RDSCUR, 7) == 1){
    // Serial.println("RDSCUR: WPSEL = 1");
  } else {
    // Serial.println("RDSCUR: WPSEL = 0");
  }
}

void RDBLOCK(){
  /*  Send en RDBLOCK kommando og gem svaret
   *  
   *  → CS# goes low 
   *  → send RDBLOCK (3Ch) instruction 
   *  → send 3 address bytes to assign one block on SI pin 
   *  → read block's protection lock status bit on SO pin 
   *  → CS# goes high. 
   */
  lowSS();
  transmitOneByteSPI(0x3C);
  sendAdress(adressenViHusker);
  byte tempRDBLOCK = 0x00;
  tempRDBLOCK = readOneByteSPI();
  tempRDBLOCK = tempRDBLOCK & 0xFF;
  
  if(tempRDBLOCK == 0xFF){
    // Der er låst!
    Serial.print("L--RDBLOCK:\t"); Serial.println(tempRDBLOCK, BIN);
  } else {
    // Der er IKKE låst!
    Serial.print("UL-RDBLOCK:\t"); Serial.println(tempRDBLOCK, BIN);
  }
  highSS();
}

void blockErase(uint32_t adress){
  lowSS();
  writeEnable();
  lowSS();
  transmitOneByteSPI(0xD8);
  sendAdress(adress);
  highSS();
  delay(50);
  
}


void setGBULK(){
  /*  Flow: 
   * → CS# goes low 
   * → send GBULK (0x98) instruction 
   * → CS# goes high <- DEN SKAL VÆRE SAMMEN MED DEN SIDSTE DATA-BIT!
   */

  lowSS();

  highMosi();//   |
  cycleClock();// |-> Bit 7

  lowMosi();//    |
  cycleClock();// |-> Bit 6

  lowMosi();//    |
  cycleClock();// |-> Bit 5

  highMosi();//   |
  cycleClock();// |-> Bit 4
  
  highMosi();//   |
  cycleClock();// |-> Bit 3

  lowMosi();//    |
  cycleClock();// |-> Bit 2

  lowMosi();//    |
  cycleClock();// |-> Bit 1

  lowMosi();//    |-> Bit 0
  PORTB |=    B00100100;// Her settes clocken OG SS samtidig
  PORTB &=    B11011111;// Low clock
  
  // No need for highSS();


  /* Bruges til skabelon til Bit 0
    PORTB &=    B11110111;   // Set low MOSI
    PORTB |=    B00100000;  // Set it to high // CLOCK
    PORTB |=    B00000100;  // Set it to high // SS
  */
  
}

void setGBLK(){
  /*  Flow: 
   * → CS# goes low 
   * → send GBLK (0x7E) instruction 
   * → CS# goes high <- DEN SKAL VÆRE SAMMEN MED DEN SIDSTE DATA-BIT!
   */

  lowSS();

  lowMosi();//    |
  cycleClock();// |-> Bit 7

  highMosi();//   |
  cycleClock();// |-> Bit 6

  highMosi();//   |
  cycleClock();// |-> Bit 5

  highMosi();//   |
  cycleClock();// |-> Bit 4
  
  highMosi();//   |
  cycleClock();// |-> Bit 3

  highMosi();//   |
  cycleClock();// |-> Bit 2

  highMosi();//   |
  cycleClock();// |-> Bit 1

  lowMosi();//    |-> Bit 0
  PORTB |=    B00100100;// Her settes clocken OG SS samtidig
  PORTB &=    B11011111;// Low clock
  
  // No need for highSS();


  /* Bruges til skabelon til Bit 0
    PORTB &=    B11110111;   // Set low MOSI
    PORTB |=    B00100000;  // Set it to high // CLOCK
    PORTB |=    B00000100;  // Set it to high // SS
  */
  
}

void writeStatusRegister(){
  // Write Status Register
  
  /* The sequence of issuing WRSR instruction is: 
   *  → CS# goes low
   *  → sending WRSR instruction code
   *  → Status Register data on SI
   *  → CS# goes high.
   * 
   */
  lowSS();
  do{                     
      writeEnable();        //   
      readStatusRegister(); // 
    // Serial.print("RDSR: "); Serial.println(storeRDSR, BIN);
    }while(!WEL);           // 
  
  highSS();
  lowSS();
  
  transmitOneByteSPI(0x01);
  

  highMosi();//   |
  cycleClock();// |-> Bit 7

  lowMosi();//    |
  cycleClock();// |-> Bit 6

  lowMosi();//    |
  cycleClock();// |-> Bit 5

  lowMosi();//    |
  cycleClock();// |-> Bit 4
  
  lowMosi();//    |
  cycleClock();// |-> Bit 3

  lowMosi();//    |
  cycleClock();// |-> Bit 2

  highMosi();//   |
  cycleClock();// |-> Bit 1

  lowMosi();//    |-> Bit 0
  PORTB |=    B00100100;// Her settes clocken OG SS samtidig
  PORTB &=    B11011111;// Low clock
  // SS er high

  
}








// ##################################
// ####   READ/WRITE FUNCTIONS   ####
// ##################################
void readTwoBytes(){
  /*  The sequence of issuing READ instruction is: 
   *   1 → CS# goes low
   *   2 → sending READ instruction code
   *   3 → 3-byte address on SI 
   *   4 → data out on MISO
   *   5 → to end READ operation can use CS# to high at any time during data out. 
   *   
   *   Kilde: Datablad pp. 19
   */
// Serial.println("readTwoBytes");
  lowSS();                  // Step 1
  sendReadInstruction();    // Step 2
  sendAdress((adressenViHusker)); // Step 3
  

  /*
    transmitOneByteSPI(0x7E);// ----|
    transmitOneByteSPI(0x80);//     |-> Adressen i 24 bit
    transmitOneByteSPI(0x00);// ----|     á 8 bit pr. gang
  */

  // Step 4 
  
  for(int i = 0; i < sizeof(arrayToSaveToFlash); i++){
    storeReadData[i] = readOneByteSPI();//  ---|-> Læser 2x8 bit og gemmer dem
    // Serial.print("Read:\t\t"); Serial.println(storeReadData[i], HEX);
  }
  
  
  highSS();  // Step 5
  // Vi er done
}

void writeStuff(){
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
   
    

    // Vent på Write-Enable-Latch bliver 1
    do{                     
      writeEnable();        // Step 1  
      writeStatusRegister();
      readStatusRegister(); // Step 2
    // Serial.print("RDSR: "); Serial.println(storeRDSR, BIN);
    }while(!WEL);           // Step 2

      // setGBULK();
    RDBLOCK();



    
    // Fyr data afsted
    continouslyProgram();   // Step 3


    // Vent på Write-In-Progress bitten bliver 0 igen
    do{                     // Step 4
      readStatusRegister(); // Step 4
      if(WIP){
        Serial.println("WIP 1");
      }
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
    // setGBLK();
    writeDisable();
    highSS(); // Vi er done nu
}

void pageProgram(){
  /* The sequence of issuing PP instruction is: 
   *  → Write Enable
   *  
   *  → CS# goes low
   *  → sending PP instruction code
   *  → 3-byte address on SI
   *    → at least 1-byte on data on SI
   *  → CS# goes high.  
   */

    //    De adresse-variabler vi har
    // uint32_t adressenViHusker = BASIC_ADRESS;
    // const uint32_t startAdressen = BASIC_ADRESS;

    lowSS();
    // Gør klar til at sende data afsted
    // writeEnable();
    readStatusRegister();
    highSS();
    do{                     
      // lowSS();
      writeEnable();        // Write Enable 
      // writeEnable();        // Write Enable 
      // writeEnable();        // Write Enable 
      lowSS();
      readStatusRegister(); // Write Enable 
      highSS();
    
      delay(5);
    // Serial.print("RDSR: "); Serial.println(storeRDSR, BIN);
    }while(!WEL && WIP);           // Write Enable 
      
    highSS();
    lowSS();

    transmitOneByteSPI(0x02);       // Sender kommandoen om Page Program
    

    
    sendAdress(adressenViHusker);   // Sender adressen
    
    for(int i = 0; i < sizeof(arrayToSaveToFlash); i++){
      transmitOneByteSPI(arrayToSaveToFlash[i]);
      // Serial.print("Skriver:\t"); Serial.println(arrayToSaveToFlash[i], HEX);
    }
/*
    for(int k = 0; k < 2048 - sizeof(arrayToSaveToFlash); k++){
      emptyBytes();
    }
*/
    

    // Sender den sidste byte!
    highMosi();
    for(int k = 7; k > 0; k--){
      cycleClock();
    }
    PORTB = B00100000; // HIGH CLOCK
    PORTB = B00000100; // HIGH SS + LOW CLOCK
    
    
    // Serial.print("PORTB:\t"); Serial.println(PORTB, BIN);
    // Færdig med at sende den sidste byte her
    
    
    // SS er høj på dette tidspunkt
    // Nu gemmer chippen sager!
    delay(5); // Ifølge databladet (tPP) er chippen max 5 ms om at gemme

    
    // Vent på Write-In-Progress bitten bliver 0 igen
    do{                     // Step 4
      writeDisable();
      readStatusRegister(); // Step 4
      readRDSCUR();           // Step 5
      // Serial.println("Venter slut");
      delay(1);
      /*
      if(WIP){
        Serial.println("WIP 1");
      }
      */
    }while(WIP && WEL);            // Step 4
  
    
    
    // Her tjekker vi om det faktisk lykkedes
    if(bitRead(RDSCUR, 5) == 1 || bitRead(RDSCUR, 6) == 1){
      // The programming failed! 
      throwErrorMessage();
    } else {
      // Ting virkede!
      // Denne else er overflødig
      // Serial.println("Ting virkede!");
    }
    // setGBLK();
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

void emptyBytes(){
  lowMosi();
  for(int i = 8; i > 0; i--){
    cycleClock();
  }
}


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
}   // JEG ER RET SEJ!
