/*
  Paint Demonstration
 */
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <MorseEnDecoder.h>  // Morse EnDecoder Library

//#include <EEPROM.h>
#include <Wire.h>
#include <SPI.h>
#include <digitalWriteFast.h>
#include <GraphicsLib.h>
#include <MI0283QT2.h>
#include <MI0283QT9.h>
#include <DisplaySPI.h>
#include <DisplayI2C.h>
#include <PS2Keyboard.h>

//Declare only one display !
 MI0283QT2 lcd;  //MI0283QT2 Adapter v1
 // declare keyboard!
PS2Keyboard keyboard;

// Application preferences global
//  Char set values:
//    1 = 26 alpha characters
//    2 = numbers
//    3 = punctuation characters
//    4 = all characters in alphabetical order
//    5 = all characters in Koch order. Two prefs set range-
//            KOCH_NUM is number to use
//            KOCH_SKIP is number to skip
//    6 = reserved
#define SAVED_FLG 0     // will be 170 if settings have been saved to EEPROM
#define GROUP_NUM 1     // expected number of cw characters to be received
#define GROUP_DLY 2     // delay before sending (in 0.01 sec increments)
#define KEY_SPEED 3     // morse keying speed (WPM)
#define CHAR_SET  4     // defines which character set to send the student.
#define KOCH_NUM  5     // how many character to use
#define KOCH_SKIP 6     // characters to skip in the Koch table
#define OUT_MODE  7     // 0 = Key, 1 = Key + Speaker
#define NUM_PREFS 8     // number of entries in the preference list
byte prefs[NUM_PREFS];  // Table of preference values

//=========================================
int Key_speed_adj = -2; // correction for keying speed

// IO definitions
const byte morseInPin = 2; // Pin for key or tone input
const byte beep_pin = 10;  // Pin for speaker
const byte key_pin = 12;   // Pin for CW digital output
int unit_delay = 250;
const int DataPin = 2; //Data pin for the Keyboard
const int IRQpin =  3;

char randomWord = '/';

void setup()
{
  delay(1000);
  keyboard.begin(DataPin, IRQpin);
  Serial.begin(9600);
  
  //init display
  lcd.begin(7); //spi-clk=SPI_CLOCK_DIV4
  lcd.begin(0x20); //I2C Displays: addr=0x20

  //set touchpanel calibration data
  lcd.touchRead();
  if(lcd.touchZ())  // || readCalData()) //calibration data in EEPROM?
  {
    lcd.touchStartCal(); //calibrate touchpanel
//    writeCalData(); //write data to EEPROM
  }

  //clear screen
  lcd.fillScreen(RGB(255,255,255));

  //show backlight power and cal text
  lcd.led(50); //backlight 0...100%
  lcd.drawText(2, 2, "BL    ", RGB(255,0,0), RGB(255,255,255), 1);
  lcd.drawInteger(20, 2, 50, DEC, RGB(255,0,0), RGB(255,255,255), 1);
  lcd.drawText(lcd.getWidth()-30, 2, "CAL", RGB(255,0,0), RGB(255,255,255), 1);

   // Initialize application preferences
  prefs_init();
  
}


void loop()
{
  char tmp[128];
  static uint16_t last_x=0, last_y=0;
  static uint8_t led=60;
  unsigned long m;
  static unsigned long prevMillis=0;

  //service routine for touch panel
  lcd.touchRead();
  if(lcd.touchZ()) //touch press?
  {
    //change backlight power
    if((lcd.touchX() < 45) && (lcd.touchY() < 15))
    {
      m = millis();
      if((m - prevMillis) > 800) //change only every 800ms
      {
        prevMillis = m;

        led += 10;
        if(led > 100)
        {
          led = 10;
        }
        lcd.led(led);
        lcd.drawText(2, 2, "BL    ", RGB(255,0,0), RGB(255,255,255), 1);
        lcd.drawInteger(20, 2, led, DEC, RGB(255,0,0), RGB(255,255,255), 1);
        lcd.drawText(lcd.getWidth()-30, 2, "CAL", RGB(255,0,0), RGB(255,255,255), 1);
      }
    }

    //calibrate touch panel
    else if((lcd.touchX() > (lcd.getWidth()-30)) && (lcd.touchY() < 15))
    {
      lcd.touchStartCal();
//      writeCalData();
      lcd.drawText(2, 2, "BL    ", RGB(255,0,0), RGB(255,255,255), 1);
      lcd.drawInteger(20, 2, led, DEC, RGB(255,0,0), RGB(255,255,255), 1);
      lcd.drawText(lcd.getWidth()-30, 2, "CAL", RGB(255,0,0), RGB(255,255,255), 1);
    }

    //draw line
    else if((last_x != lcd.touchX()) || (last_y != lcd.touchY()))
    {
      sprintf(tmp, "X:%03i Y:%03i P:%03i", lcd.touchX(), lcd.touchY(), lcd.touchZ());
      lcd.drawText(50, 2, tmp, RGB(0,0,0), RGB(255,255,255), 1);

      if(last_x == 0)
      {
        lcd.drawPixel(lcd.touchX(), lcd.touchY(), RGB(0,0,0));
      }
      else
      {
        lcd.drawLine(last_x, last_y, lcd.touchX(), lcd.touchY(), RGB(0,0,0));
      }
      last_x = lcd.touchX();
      last_y = lcd.touchY();
    }
  }
  else
  {
    last_x = 0;
  }
 morse_trainer();
  
}


//====================
// Morse code trainer function
//====================
void morse_trainer()
{ 
  const static char wordsToMorse[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G',
  'H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',',','.','/','?','\0'};
  if(randomWord == '/'){
   randomWord = wordsToMorse[random(41)];
  Serial.print(randomWord);
     String stringinValue = "sound of random char  "+ String(randomWord);
  lcd.drawText((lcd.getWidth()/4), (lcd.getHeight()/2),stringinValue , RGB(255,0,0), RGB(255,255,255), 1);
  generate_morse_sound(randomWord);
  }
   if (keyboard.available()) {
    
    // read the next key
    char c = keyboard.read();
    // check for some of the special keys
    if (c == PS2_ENTER) {
      Serial.println();
    } else if (c == PS2_TAB) {
      Serial.print("[Tab]");
    } else if (c == PS2_ESC) {
      Serial.print("[ESC]");
    } else if (c == PS2_PAGEDOWN) {
      Serial.print("[PgDn]");
    } else if (c == PS2_PAGEUP) {
      Serial.print("[PgUp]");
    } else if (c == PS2_LEFTARROW) {
      Serial.print("[Left]");
    } else if (c == PS2_RIGHTARROW) {
      Serial.print("[Right]");
    } else if (c == PS2_UPARROW) {
      Serial.print("[Up]");
    } else if (c == PS2_DOWNARROW) {
      Serial.print("[Down]");
    } else if (c == PS2_DELETE) {
      Serial.print("[Del]");
    } else {
      if(c == randomWord){
        Serial.print(randomWord);
        Serial.print("==");
      Serial.print(c);
        lcd.drawText((lcd.getWidth()/4), (lcd.getHeight()/2), "congratulations               ", RGB(255,0,0), RGB(255,255,255), 1);
        randomWord = '/';
      }
      else {
        Serial.print(randomWord);
        Serial.print("!=");
        Serial.print(c);
        lcd.drawText((lcd.getWidth()/4), (lcd.getHeight()/2), "Bad luck                     ", RGB(255,0,0), RGB(255,255,255), 1);
        randomWord = '/';
      }
      // otherwise, just print all normal characters
    }
  }
    delay(3000);  //0.1 sec pause at the end of the loop.

}  // end morse_trainer()


// different Tone for everyword method
void dot()
{
Serial.print(".");
tone(beep_pin, 1000);
delay(unit_delay);
tone(beep_pin, 400);
delay(unit_delay);
noTone(beep_pin);
}
void dash()
{
Serial.print("-");
tone(beep_pin, 1000);
delay(unit_delay * 3);
tone(beep_pin, 400);
delay(unit_delay);
noTone(beep_pin);
}
void A()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void B()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void C()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
}
void D()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void E()
{
dot();
delay(unit_delay);
}
void f()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
}
void G()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
}
void H()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void I()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void J()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void K()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void L()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void M()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void N()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
}
void O()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void P()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
}
void Q()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void R()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
}
void S()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void T()
{
dash();
delay(unit_delay);
}
void U()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void V()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void W()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void X()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void Y()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void Z()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void one()
{
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void two()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void three()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}
void four()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dash();
delay(unit_delay);
}
void five()
{
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void six()
{
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void seven()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void eight()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
dot();
delay(unit_delay);
}
void nine()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dot();
delay(unit_delay);
}
void zero()
{
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
dash();
delay(unit_delay);
}



// Morse sounds
void generate_morse_sound(char ch)
{
if (ch == 'A' || ch == 'a')
{
A();
Serial.print(" ");
}
else if (ch == 'B' || ch == 'b')
{
B();
Serial.print(" ");
}
else if (ch == 'C' || ch == 'c')
{
C();
Serial.print(" ");
}
else if (ch == 'D' || ch == 'd')
{
D();
Serial.print(" ");
}
else if (ch == 'E' || ch == 'e')
{
E();
Serial.print(" ");
}
else if (ch == 'f' || ch == 'f')
{
f();
Serial.print(" ");
}
else if (ch == 'G' || ch == 'g')
{
G();
Serial.print(" ");
}
else if (ch == 'H' || ch == 'h')
{
H();
Serial.print(" ");
}
else if (ch == 'I' || ch == 'i')
{
I();
Serial.print(" ");
}
else if (ch == 'J' || ch == 'j')
{
J();
Serial.print(" ");
}
else if (ch == 'K' || ch == 'k')
{
K();
Serial.print(" ");
}
else if (ch == 'L' || ch == 'l')
{
L();
Serial.print(" ");
}
else if (ch == 'M' || ch == 'm')
{
M();
Serial.print(" ");
}
else if (ch == 'N' || ch == 'n')
{
N();
Serial.print(" ");
}
else if (ch == 'O' || ch == 'o')
{
O();
Serial.print(" ");
}
else if (ch == 'P' || ch == 'p')
{
P();
Serial.print(" ");
}
else if (ch == 'Q' || ch == 'q')
{
Q();
Serial.print(" ");
}
else if (ch == 'R' || ch == 'r')
{
R();
Serial.print(" ");
}
else if (ch == 'S' || ch == 's')
{
S();
Serial.print(" ");
}
else if (ch == 'T' || ch == 't')
{
T();
Serial.print(" ");
}
else if (ch == 'U' || ch == 'u')
{
U();
Serial.print(" ");
}
else if (ch == 'V' || ch == 'v')
{
V();
Serial.print(" ");
}
else if (ch == 'W' || ch == 'w')
{
W();
Serial.print(" ");
}
else if (ch == 'X' || ch == 'x')
{
X();
Serial.print(" ");
}
else if (ch == 'Y' || ch == 'y')
{
Y();
Serial.print(" ");
}
else if (ch == 'Z' || ch == 'z')
{
Z();
Serial.print(" ");
}
else if (ch == '0')
{
zero();
Serial.print(" ");
}
else if (ch == '1')
{
one();
Serial.print(" ");
}
else if (ch == '2')
{
two();
Serial.print(" ");
}
else if (ch == '3')
{
three();
Serial.print(" ");
}
else if (ch == '4')
{
four();
Serial.print(" ");
}
else if (ch == '5')
{
five();
Serial.print(" ");
}
else if (ch == '6')
{
six();
Serial.print(" ");
}
else if (ch == '7')
{
seven();
Serial.print(" ");
}
else if (ch == '8')
{
eight();
Serial.print(" ");
}
else if (ch == '9')
{
nine();
Serial.print(" ");
}
else if(ch == ' ')
{
delay(unit_delay*7);
Serial.print("/ ");
}
else
Serial.println("Unknown symbol!");
}



//===========================
// Restore app preferences from EEPROM if
// values are saved, else set to defaults.
// Send values to serial port on debug. 
//===========================
void prefs_init()
{
  // Restore app settings from the EEPROM if the saved
  // flag value is 170, otherwise init to defaults.
  if (EEPROM.read(0) == 170)
  {
    for (int idx = 0; idx < NUM_PREFS; idx++)
    {
      prefs_set(idx,EEPROM.read(idx));
    }
  }
  else
  {
    prefs_set(SAVED_FLG, 0);  // Prefs not saved
    prefs_set(GROUP_NUM, 1);  // Send/receive groups of 1 char to start
    prefs_set(GROUP_DLY, 0);  // Send/receive with no delay
    prefs_set(KEY_SPEED, 25); // Send at 25 wpm to start
    prefs_set(CHAR_SET, 5);   // Use Koch order char set
    prefs_set(KOCH_NUM, 5);   // Use first 5 char in Koch set
    prefs_set(KOCH_SKIP, 0);  // Don't skip over any char to start
    prefs_set(OUT_MODE, 1);   // Output to speaker
  }
}

//========================
// Set preference specified in arg1 to value in arg2
// Constrain prefs values to defined limits
// Echo value to serial port
//========================
byte prefs_set(byte pref, int val)
{
  const byte lo_lim[] {0, 1, 0, 10, 1, 1, 0, 0};  // Table of lower limits of preference values
  const byte hi_lim[] {170, 15, 30, 30, 6, 40, 39, 1};  // Table of uppper limits of preference values
  byte new_val;
  byte indx;


  
  // Set new value
  indx = constrain(pref, 0, NUM_PREFS-1);                // Constrain index, just to be safe
  new_val = constrain(val, lo_lim[indx], hi_lim[indx]);  // Set new value within defined limits

  // Dispatch on preference index to do debug print
  switch (indx) {
    case SAVED_FLG:
      Serial.print("Saved flag = ");
      break;
    case GROUP_NUM:
      Serial.print("Group size = ");
      break;
    case GROUP_DLY:
      Serial.print("Inter-character Delay = ");
      break; 
    case KEY_SPEED:
      Serial.print("Key speed = ");
      break;
    case CHAR_SET:
      Serial.print("Character set = ");
      break;
    case KOCH_NUM:
      Serial.print("Koch number = ");
      break;
    case KOCH_SKIP:
      if (new_val >= prefs[KOCH_NUM]) new_val = prefs[KOCH_NUM]-1;
      Serial.print("Skip = ");
      break;
    case OUT_MODE:
      Serial.print("Output mode = ");
      break;
    default:
      Serial.print("Preference index out of range\n");
      return new_val;
  }

  // Print and save new value before returning it
  Serial.println(new_val);
  prefs[indx] = new_val;
  return new_val;
}
