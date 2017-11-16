#include <LiquidCrystal.h>
#include <EEPROMex.h>

// ---> Buttons definition
#include <Button.h>        //https://github.com/JChristensen/Button

#define DN_PIN 7           //Connect two tactile button switches (or something similar)
#define UP_PIN 8           //from Arduino pin 2 to ground and from pin 3 to ground.
#define PULLUP true        //To keep things simple, we use the Arduino's internal pullup resistor.
#define INVERT true        //Since the pullup resistor will keep the pin high unless the
                           //switch is closed, this is negative logic, i.e. a high state
                           //means the button is NOT pressed. (Assuming a normally open switch.)
#define DEBOUNCE_MS 20     //A debounce time of 20 milliseconds usually works well for tactile button switches.

#define REPEAT_FIRST 500   //ms required before repeating on long press
#define REPEAT_INCR 100    //repeat interval for long press
#define MIN_COUNT 0
#define MAX_COUNT 255

Button btnUP(UP_PIN, PULLUP, INVERT, DEBOUNCE_MS);    //Declare the buttons
Button btnDN(DN_PIN, PULLUP, INVERT, DEBOUNCE_MS);

enum {WAIT, INCR, DECR};              //The possible states for the state machine
uint8_t STATE;                        //The current state machine state
int count;                            //The number that is adjusted
int lastCount = -1;                   //Previous value of count (initialized to ensure it's different when the sketch starts)
unsigned long rpt = REPEAT_FIRST;     //A variable time that is used to drive the repeats for long presses
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
    Serial.begin(115200);
    // ---> EEPROM def and hotwire related stuff
    // Определение количество символов в строке и количество строк:
    lcd.begin(8, 2);
    lcd.print("TeamSeal");
    //Далее перемещаемся в начало второй строки
    lcd.setCursor(0,1);
    //int a=0;
    for (int a=0; a<8; a++) { delay(200);lcd.print("-"); }
    lcd.setCursor(0,0); lcd.print("Set Temp."); lcd.setCursor(0,1); lcd.print("        ");

    // Uncomment next line to write first value to EEPROM and 
    // comment it back when switch to new Arduino
    //EEPROM.writeInt(address, input);
    output = EEPROM.readInt(address);
    //if (output != 0) {EEPROM.writeInt(address, input);}
    val=output;
    count=output;
    lcd.setCursor(0,1);
    lcd.print(output);
    // ---> End of EEPROM def and hotwire related stuff
}

void loop(void)
{
    btnUP.read();                             //read the buttons
    btnDN.read();
    if (count != lastCount) {                 //print the count if it has changed
        lastCount = count;
//        Serial.println(count, DEC);
        static const unsigned long REFRESH_INTERVAL = 300; // ms
        static unsigned long lastRefreshTime = 0;  
        if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
        {
            lastRefreshTime += REFRESH_INTERVAL;    
            if (count==99 || count==9) {cls=1;}
            if (cls=1) {lcd.clear();lcd.print("Set Temp."); cls=0;}
            lcd.setCursor(0,1);
            lcd.print (count);
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
}