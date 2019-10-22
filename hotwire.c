#include <LiquidCrystal.h>
#include <EEPROMex.h>
#include <Button.h>        //https://github.com/JChristensen/Button

// ---> Cutter Setup
#define NUMBER_OF_MODELS 6
#define EEPROM_MODEL_NUMBER_ADDRESS 0
#define EEPROM_MODEL0_ADDRESS 2
#define DEFAULT_TEMPERATURE 7
#define PWM_PIN 9
#define LED_EEPROM 6
#define ENABLE 7
// ---> End Cutter Setup

// ---> Buttons definition
#define DN_PIN 16           //Connect two tactile button switches (or something similar)
#define UP_PIN 17           //from Arduino pin 7 to ground and from pin 8 to ground.
#define P_PIN 15          //Connect two tactile button switches (or something similar)
#define M_PIN 14          //from Arduino pin A0 to ground and from pin A1 to ground.
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
#define EEPROM_LOOP 10000

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

int prevent_first_write =0;
int actual =0;
int bright=0;
int enable=0;
unsigned long hold=0;
unsigned long unhold=0;

// ---> EEPROM def and hotwire related stuff
//LiquidCrystal lcd(rs, e, d5, d4, d3, d2);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
// eeprom start address
int model_num = 0;
int address = EEPROM_MODEL0_ADDRESS+2*model_num;
int current_temperature =0;
int output = 0;
int val=0;
int cls=1;
int temperature[6]; // massive that stores temperature value for each model
int temperaturechenge[6]; // massive that stores temperature value for each model
int modelchanged = 0;
// ---> End of EEPROM def and hotwire related stuff

void setup(void)
{
    Serial.begin(9600);  
    Serial.println();
    // ---> Cutter Setup
    Serial.println("Setup IO Ports");
    Serial.println();
    pinMode(PWM_PIN, OUTPUT);
    analogWrite(PWM_PIN, 0);
    pinMode(LED_EEPROM, OUTPUT);
    pinMode(ENABLE, INPUT);
    digitalWrite(ENABLE, HIGH); 
    // ---> End Cutter Setup

    // ---> EEPROM def and hotwire related stuff
    // Set number of laters in the string
    Serial.println("Init LCD");
    Serial.println();
    lcd.begin(8, 2);
    lcd.print("TeamSeal");
    //Далее перемещаемся в начало второй строки
    lcd.setCursor(0,1);
    for (int a=0; a<50; a++) { 
        delay(30);
        // lcd.print("-");
        analogWrite(LED_EEPROM, a);
        switch (a) {
            case 7: lcd.print("W");break;
            case 14: lcd.print("a");break;   //execution starts at this case label
            case 21: lcd.print("r");break;
            case 28: lcd.print("m");break;
            case 35: lcd.print("i");break;
            case 42: lcd.print("n");break;
            case 49: lcd.print("g"); delay(400);break;
        }
    }
    analogWrite(LED_EEPROM, 0);
    lcd.clear();
    lcd.setCursor(0,0); lcd.print("Type t"); lcd.setCursor(0,1); lcd.print("        ");
    EEPROM.setMaxAllowedWrites(20); // STOP people of rewriting EEPROM more than 10 times
    Serial.println("Read Values from EEPROM");
    Serial.println();
    // Reading model number
    model_num = EEPROM.readInt(EEPROM_MODEL_NUMBER_ADDRESS);
    Serial.println("CurrentModel");
    Serial.println(model_num);
    Serial.println();
    current_temperature = EEPROM.readInt(EEPROM_MODEL0_ADDRESS+2*model_num);
    Serial.println("CurrentTemp");
    Serial.println(current_temperature);
    Serial.println();
    // Zeroing model number
    if(model_num < 0 || model_num > NUMBER_OF_MODELS) {
        Serial.println("Model number out of Range. Setting model_num to 0");
        Serial.println();
        model_num=0;
        // EEPROM.writeInt(EEPROM_MODEL_NUMBER_ADDRESS, model_num);
        analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);delay(100);
        analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);delay(100);
        analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);
    }
    int address=EEPROM_MODEL0_ADDRESS;
    // Reading model temperatures
    for(int i=0; i<NUMBER_OF_MODELS; i++) {
        address=EEPROM_MODEL0_ADDRESS+2*i;
        temperature[i]=EEPROM.readInt(address);
        temperaturechenge[i]=0;
        Serial.print("Model Numder:"); Serial.print(i); Serial.print("  ");
        Serial.print("Temp:"); Serial.print(temperature[i]);
        Serial.println();
    }
    // Zeroing model temperatures
    address=EEPROM_MODEL0_ADDRESS;
    for(int i=0; i<NUMBER_OF_MODELS; i++) {
        address=EEPROM_MODEL0_ADDRESS+2*i;
        if (temperature[i]<0 || temperature[i]>255) {
            Serial.println("Temperature is out of range. Setting temperature to default value");
            Serial.println();
            EEPROM.writeInt(address, DEFAULT_TEMPERATURE);
            digitalWrite(LED_EEPROM, HIGH); delay(150);digitalWrite(LED_EEPROM, LOW);delay(150);
            digitalWrite(LED_EEPROM, HIGH); delay(150);digitalWrite(LED_EEPROM, LOW);delay(150);
            digitalWrite(LED_EEPROM, HIGH); delay(150);digitalWrite(LED_EEPROM, LOW);delay(150);
        }
    }
//    int addressInt       = EEPROM.getAddress(sizeof(int));
//    Serial.print(addressInt);       Serial.print(" \t\t\t "); Serial.print(sizeof(int));  Serial.println(" (int)");
    lcd.setCursor(0,1);
    switch (model_num) {
            case 0: lcd.print("Foam");break;
            case 1: lcd.print("Foa+");break;   //execution starts at this case label
            case 2: lcd.print("EPP");break;
            case 3: lcd.print("EPP+");break;
            case 4: lcd.print("EPO");break;
            case 5: lcd.print("EPO+");break;
        }
    // if(model_num==0) {lcd.print("Foam");}
    // if(model_num==1) {lcd.print("Foa+");}
    // if(model_num==2) {lcd.print("EPP");}
    // if(model_num==3) {lcd.print("EPP+");}
    // if(model_num==4) {lcd.print("EPO");}
    // if(model_num==5) {lcd.print("EPO+");}
    countbi=model_num; // 5
    count=temperature[model_num]; // 7
    lcd.setCursor(5,1);
    lcd.print(temperature[model_num]);
    // ---> End of EEPROM def and hotwire related stuff
}

void loop(void)
{
    btnUP.read();                             //read the buttons
    btnDN.read();
    btnP.read();
    btnM.read();
    address=EEPROM_MODEL0_ADDRESS+model_num*2;
    
    if (count != lastCount || countbi != lastCountbi) {                 //print the count if it has changed
        if (countbi != lastCountbi && prevent_first_write ==1){
            Serial.println("Switching to model number");
            Serial.println(countbi);
            Serial.println();
            Serial.println("Reading model temperature");
            Serial.println(temperature[model_num]);
            Serial.println();
            modelchanged =1;
            model_num=countbi;
            address=EEPROM_MODEL0_ADDRESS+model_num*2;
            //temperature[model_num]=EEPROM.readInt(address);
            // Updating count value
            count=temperature[model_num];
            lastCount = count;
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Type t");
            lcd.setCursor(0,1);
            if(model_num==0) {lcd.print("Foam");}
            if(model_num==1) {lcd.print("Foa+");}
            if(model_num==2) {lcd.print("EPP");}
            if(model_num==3) {lcd.print("EPP+");}
            if(model_num==4) {lcd.print("EPO");}
            if(model_num==5) {lcd.print("EPO+");}
            // Updating last used model
            Serial.println("WRITING EEPROM!");
            EEPROM.updateInt(EEPROM_MODEL_NUMBER_ADDRESS, model_num); // run it rarely
            actual=count;
            EEPROM.updateInt(address, actual);
            digitalWrite(LED_EEPROM, HIGH); digitalWrite(LED_EEPROM, LOW);
        }
        if (count != lastCount && prevent_first_write ==1) {
            temperature[model_num]=count;
            actual=count;
            temperaturechenge[model_num]=1;
            Serial.println("updating model temperature");
            Serial.println(temperature[model_num]);
            Serial.println();
            //EEPROM.updateInt(address, temperature[model_num]);
        }

        lastCount = count; 
        lastCountbi=countbi;
    static const unsigned long REFRESH_INTERVAL = 300; // ms
        static unsigned long lastRefreshTime = 0;
        if(millis() - lastRefreshTime >= REFRESH_INTERVAL) {
            lastRefreshTime += REFRESH_INTERVAL;    
            if (temperature[model_num]==99 || temperature[model_num]==9) {
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Type t");
                lcd.setCursor(0,1);
                if(model_num==0) {lcd.print("Foam");}
                if(model_num==1) {lcd.print("Foa+");}
                if(model_num==2) {lcd.print("EPP");}
                if(model_num==3) {lcd.print("EPP+");}
                if(model_num==4) {lcd.print("EPO");}
                if(model_num==5) {lcd.print("EPO+");}
                Serial.println("clearing screen");
            }
            lcd.setCursor(5,1);
            lcd.print(temperature[model_num]); 
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
    enable=digitalRead(ENABLE);
    // delay(500);
    if (enable==1)    {
        hold=millis();
        analogWrite(PWM_PIN, count );
        if (count >0 && count < 17)     {bright=1;}
        else if(count >=17)             { bright = map(count, 0, 255, 0, 16); }
        else                            { bright = 0; }
        analogWrite(LED_EEPROM, bright);
    }
    else {
        analogWrite(PWM_PIN, 0 );
        analogWrite(LED_EEPROM, 0);
        if (hold-unhold >= 400 && hold-unhold <= 1000) {
            
            if (modelchanged ==1){
                Serial.println("Writing new model number");
                Serial.println(model_num);
                modelchanged=0;
                EEPROM.writeInt(EEPROM_MODEL_NUMBER_ADDRESS, model_num);
                analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);delay(100);
                analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);delay(100);
            }
            for (int v=0;v<NUMBER_OF_MODELS;v++) {
                if (temperaturechenge[v]==1) {
                    Serial.print("Writing new temperature for model ");
                    Serial.print(v);
                    Serial.print(" ");
                    Serial.print("Temperature=");
                    Serial.print(temperature[v]);
                    address = EEPROM_MODEL0_ADDRESS+2*v;
                    temperaturechenge[v]=0;
                    Serial.print("ModelAddress = ");
                    Serial.print(address);
                    Serial.println("");
                    EEPROM.writeInt(address, temperature[v]);
                    analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);delay(100);
                    analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);delay(100);
                    analogWrite(LED_EEPROM, 25); delay(100);analogWrite(LED_EEPROM, 0);
                }
            }
        }
        unhold=millis();
    }
    prevent_first_write =1;
}