#include <KonnektingDevice.h>

#ifdef __SAMD21G18A__
#include <FlashAsEEPROM.h> // http://librarymanager/All#FlashStorage
#endif

// ################################################
// ### KNX DEVICE CONFIGURATION
// ################################################
word individualAddress  = P_ADDR(1,1,199);
word groupAddressInput  = G_ADDR(7,7,7);
word groupAddressOutput = G_ADDR(7,7,8);

// ################################################
// ### DEBUG CONFIGURATION
// ################################################
#define KDEBUG // comment this line to disable DEBUG mode
#ifdef KDEBUG
#include <DebugUtil.h>

// Get correct serial port for debugging

#ifdef __AVR_ATmega32U4__
// Leonardo/Micro/ProMicro: USB serial port
#define DEBUGSERIAL Serial

#elif ARDUINO_ARCH_STM32
// STM32 NUCLEO Boards: USB port
#define DEBUGSERIAL Serial

#elif __SAMD21G18A__ 
// Zero: native USB port
#define DEBUGSERIAL SerialUSB

#elif ESP8266
// ESP8266 uses the 2nd serial port with TX only (GPIO2)
#define DEBUGSERIAL Serial1

#else
// All other, (ATmega328P f.i.) use software serial
#include <SoftwareSerial.h>
SoftwareSerial softserial(11, 10); // RX, TX
#define DEBUGSERIAL softserial
#endif
// end of debugging defs
#endif

// ################################################
// ### IO Configuration
// ################################################
#ifdef ESP8266
//LED_BUILTIN on ESP8266 cannot be used, because this pin is allready used for Debug-Serial
#define TEST_LED 16
#else
// default on-board LED on most Arduinos
#define TEST_LED LED_BUILTIN //or change it to another pin


#endif

// Define KONNEKTING Device related IDs
#define MANUFACTURER_ID 57005
#define DEVICE_ID 255
#define REVISION 0

// ################################################
// ### KONNEKTING Configuration
// ################################################
#ifdef __AVR_ATmega328P__
// Uno/Nano/ProMini: use Serial D0=RX/D1=TX
#define KNX_SERIAL Serial

#elif ARDUINO_ARCH_STM32
//STM32 NUCLEO-64 with Arduino-Header: D8(PA9)=TX, D2(PA10)=RX
#define KNX_SERIAL Serial1

#elif ESP8266
// ESP8266: swaped Serial on GPIO13=RX, GPIO15=TX
#define KNX_SERIAL Serial

#else
// Leonardo/Micro/Zero etc.: Serial1 D0=RX/D1=TX
#define KNX_SERIAL Serial1
#endif

// Definition of the Communication Objects attached to the device
KnxComObject KnxDevice::_comObjectsList[] = {
    /* Suite-Index 0 : */ KnxComObject(KNX_DPT_1_001, COM_OBJ_LOGIC_IN),
    /* Suite-Index 1 : */ KnxComObject(KNX_DPT_1_001, COM_OBJ_SENSOR),
};
const byte KnxDevice::_numberOfComObjects = sizeof (_comObjectsList) / sizeof (KnxComObject); // do no change this code

// Definition of parameter size
byte KonnektingDevice::_paramSizeList[] = {
    /* Param Index 0 */ PARAM_UINT16
};
const int KonnektingDevice::_numberOfParams = sizeof (_paramSizeList); // do no change this code

//we do not need a ProgLED, but this function muss be defined
void progLed (bool state){ 
    //nothing to do here
    Debug.println(F("Toggle ProgLED, actual state: %d"),state);
}

unsigned long sendDelay = 2000;
unsigned long lastmillis = millis(); 
bool laststate = false;


// ################################################
// ### KNX EVENT CALLBACK
// ################################################

void knxEvents(byte index) {
    switch (index) {

        case 0: // object index has been updated

            if (Knx.read(0)) {
                digitalWrite(TEST_LED, HIGH);
                Debug.println(F("Received new state, turning LED ON"));
            } else {
                digitalWrite(TEST_LED, LOW);
                Debug.println(F("Received new state, turning LED OFF"));
            }
            break;

        default:
            break;
    }
}

byte readMemory(int index){
    return EEPROM.read(index);
}

void writeMemory(int index, byte val) {
    EEPROM.write(index,val);
}

void updateMemory(int index, byte val) {
    if (readMemory(index) != val) {
        writeMemory(index, val);
    }
}

void commitMemory() {
#if defined(ESP8266) || defined(__SAMD21G18A__)
    EEPROM.commit();
#endif
}

void setup() {

    Konnekting.setMemoryReadFunc(&readMemory);
    Konnekting.setMemoryWriteFunc(&writeMemory);
    Konnekting.setMemoryUpdateFunc(&updateMemory);
    Konnekting.setMemoryCommitFunc(&commitMemory);

    //simulating allready programmed Konnekting device:
    //write hardcoded PA and GAs
    writeMemory(0,  0x7F);  //Set "not factory" flag
    writeMemory(1,  (byte)(individualAddress >> 8));
    writeMemory(2,  (byte)(individualAddress));
    writeMemory(10, (byte)(groupAddressInput >> 8));
    writeMemory(11, (byte)(groupAddressInput));
    writeMemory(12, 0x80);  //activate InputGA
    writeMemory(13, (byte)(groupAddressOutput >> 8));
    writeMemory(14, (byte)(groupAddressOutput));
    writeMemory(15, 0x80);  //activate OutputGA


    // debug related stuff
#ifdef KDEBUG

    // Start debug serial with 115200 bauds
    DEBUGSERIAL.begin(115200);

    //waiting 3 seconds, so you have enough time to start serial monitor
    delay(3000);

    // make debug serial port known to debug class
    // Means: KONNEKTING will sue the same serial port for console debugging
    Debug.setPrintStream(&DEBUGSERIAL);
#endif

    // Initialize KNX enabled Arduino Board
    Konnekting.init(KNX_SERIAL,
            &progLed,
            MANUFACTURER_ID,
            DEVICE_ID,
            REVISION);

    pinMode(TEST_LED,OUTPUT);
    
    Debug.println(F("Toggle LED every %d ms."), sendDelay);
    digitalWrite(TEST_LED,HIGH);
    Debug.println(F("Setup is ready. Turning LED on and going to loop..."));

}

void loop() {

    // Do KNX related stuff (like sending/receiving KNX telegrams)
    // This is required in every KONNEKTING aplication sketch
    Knx.task();

    unsigned long currentmillis = millis();

    /*
     * only do measurements and other sketch related stuff if not in programming mode
     * means: only when konnekting is ready for appliction
     */
    if (Konnekting.isReadyForApplication()) {

        if (currentmillis - lastmillis >= sendDelay) {

            Debug.println(F("Sending: %d"), laststate);
            Knx.write(1, laststate);
            laststate = !laststate;
            lastmillis = currentmillis;
            Debug.println(F("DONE\n"));

        }

    }

}
