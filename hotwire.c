#include <LiquidCrystal.h>
#include <EEPROMex.h>

// ---> Cutter Setup
#define PWM_PIN 9
#define LED_EEPROM 19
// ---> End Cutter Setup

// ---> Buttons definition
#include <Button.h>        //https://github.com/JChristensen/Button

#define DN_PIN 7           //Connect two tactile button switches (or something similar)
#define UP_PIN 8           //from Arduino pin 7 to ground and from pin 8 to ground.
#define P_PIN 14          //Connect two tactile button switches (or something similar)
#define M_PIN 15          //from Arduino pin A0 to ground and from pin A1 to ground.
#define PULLUP true        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
                           //switch is closed, this is negative logic, i.e. a high state
                           //means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.

#define REPEAT_FIRST 500   //ms required before repeating on long press
#define REPEAT_INCR 100    //repeat interval for long press
#define MIN_COUNT 0
#define MAX_COUNT 255
#define MAX_COUNTBI 5

Button btnUP(UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the buttons
Button btnDN(DN_PIN, PULLUP, INVERT, DEBOUNCE_MS);
Button btnP(P_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the buttons
Button btnM(M_PIN, PULLUP, INVERT, DEBOUNCE_MS);

enum {WAIT, INCR, DECR};              //The possible states for the state machine
uint8_t STATE;                        //The current state machine state
int count;                            //The number that is adjusted
int lastCount = -1;                   //Previous value of count (initialized to ensure it's different when the sketch starts)
unsigned long rpt = REPEAT_FIRST;     //A variable time that is used to drive the repeats for long presses

// ---> Second pair of buttons
uint8_t STATEBI;                        //The current state machine BI state
int countbi;                            //The number that is adjusted
int lastCountbi = -1;                   //Previous value of count (initialized to ensure it's different when the sketch starts)
unsigned long rptbi = REPEAT_FIRST;     //A variable time that is used to drive the repeats for long presses
// ---> End of Buttons definition

// ---> EEPROM def and hotwire related stuff
//LiquidCrystal lcd(rs, e, d5, d4, d3, d2);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
// eeprom start address
int address = 0;
int input = 255; // default value for arduino EEPROM wire temp.
int output = 0;
int val=0;
int cls=1;
// ---> End of EEPROM def and hotwire related stuff

void setup(void)
{
    //Serial.begin(115200);
    Serial.begin(9600);  
    Serial.println();
    // ---> Cutter Setup
    pinMode(PWM_PIN, OUTPUT);
    analogWrite(PWM_PIN, 0);
    pinMode(LED_EEPROM, OUTPUT);
    // ---> End Cutter Setup

    // ---> EEPROM def and hotwire related stuff
    // Определение количество символов в строке и количество строк:
    lcd.begin(8, 2);
    lcd.print("TeamSeal");
    //Далее перемещаемся в начало второй строки
    lcd.setCursor(0,1);
    //int a=0;
    for (int a=0; a<8; a++) { delay(200);lcd.print("-"); }
    lcd.setCursor(0,0); lcd.print("Mat Temp"); lcd.setCursor(0,1); lcd.print("        ");
    // Uncomment next line to write first value to EEPROM and 
    // comment it back when switch to new Arduino
    //EEPROM.writeInt(address, input);
    EEPROM.setMaxAllowedWrites(10); // STOP people of rewriting EEPROM more than 10 times
    output = EEPROM.readInt(address);    digitalWrite(LED_EEPROM, HIGH); delay(200);digitalWrite(LED_EEPROM, LOW);
    int addressInt       = EEPROM.getAddress(sizeof(int));
        Serial.print(addressInt);       Serial.print(" \t\t\t "); Serial.print(sizeof(int));  Serial.println(" (int)");

    //if (output != 0) {EEPROM.writeInt(address, input);}
    val=output;
    count=output;
    lcd.setCursor(5,1);
    lcd.print(output);
    // ---> End of EEPROM def and hotwire related stuff
}

void loop(void)
{
    btnUP.read();                             //read the buttons
    btnDN.read();
    btnP.read();
    btnM.read();
    if (count != lastCount || countbi != lastCountbi) {                 //print the count if it has changed
        lastCount = count; lastCountbi=countbi;
//        Serial.println(count, DEC);
        static const unsigned long REFRESH_INTERVAL = 300; // ms
        static unsigned long lastRefreshTime = 0;  
        if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
        {
            lastRefreshTime += REFRESH_INTERVAL;    
            if (count==99 || count==9 || countbi != lastCountbi) {cls=1;}
            if (cls=1) {lcd.clear();lcd.print("Mat Temp"); cls=0;}
            lcd.setCursor(5,1);
            lcd.print (count);

            if(countbi==0) {lcd.setCursor(0,1); lcd.print("Foam"); EEPROM.updateInt(address, count); digitalWrite(LED_EEPROM, HIGH);digitalWrite(LED_EEPROM, LOW); } 
            if(countbi==1) {lcd.setCursor(0,1); lcd.print("Foa+");} 
            if(countbi==2) {lcd.setCursor(0,1); lcd.print("EPO");} 
            if(countbi==3) {lcd.setCursor(0,1); lcd.print("EPO+");} 
            if(countbi==4) {lcd.setCursor(0,1); lcd.print("EPP");} 
            if(countbi==5) {lcd.setCursor(0,1); lcd.print("EPP+");}

        }
    }
    
    switch (STATE) {        
        case WAIT:                                //wait for a button event
            if (btnUP.wasPressed())
                STATE = INCR;
            else if (btnDN.wasPressed())
                STATE = DECR;
            else if (btnUP.wasReleased())         //reset the long press interval
                rpt = REPEAT_FIRST;
            else if (btnDN.wasReleased())
                rpt = REPEAT_FIRST;
            else if (btnUP.pressedFor(rpt)) {     //check for long press
                rpt += REPEAT_INCR;               //increment the long press interval
                STATE = INCR;
            }
            else if (btnDN.pressedFor(rpt)) {
                rpt += REPEAT_INCR;
                STATE = DECR;
            }
            break;
        case INCR:                                //increment the counter
            count = min(count++, MAX_COUNT);      //but not more than the specified maximum
            STATE = WAIT;
            break;
        case DECR:                                //decrement the counter
            count = max(count--, MIN_COUNT);      //but not less than the specified minimum
            STATE = WAIT;
            break;
    }

    switch (STATEBI) {        
        case WAIT:                                //wait for a button event
            if (btnP.wasPressed())
                STATEBI = INCR;
            else if (btnM.wasPressed())
                STATEBI = DECR;
            else if (btnP.wasReleased())         //reset the long press interval
                rptbi = REPEAT_FIRST;
            else if (btnM.wasReleased())
                rptbi = REPEAT_FIRST;
            else if (btnP.pressedFor(rpt)) {     //check for long press
                rptbi += REPEAT_INCR;               //increment the long press interval
                STATEBI = INCR;
            }
            else if (btnM.pressedFor(rpt)) {
                rptbi += REPEAT_INCR;
                STATEBI = DECR;
            }
            break;
        case INCR:                                //increment the counter
            countbi = min(countbi++, MAX_COUNTBI);      //but not more than the specified maximum
            STATEBI = WAIT;
            break;
        case DECR:                                //decrement the counter
            countbi = max(countbi--, MIN_COUNT);      //but not less than the specified minimum
            STATEBI = WAIT;
            break;
    }
    analogWrite(PWM_PIN, count );
}