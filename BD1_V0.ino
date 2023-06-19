#include <Wire.h>
#include <hd44780.h>                       // main hd44780 header
#include <hd44780ioClass/hd44780_I2Cexp.h> // i2c expander i/o class header

/*

ASCII Mode Example:

PIN_CLEAR
PIN_X_ENTER, DATA=<x coordinate>
PIN_Y, DATA=<y coordinate>
PIN_RECORD, DATA=<ASCII character> (repeat until msg complete. Note: Autoadvances cursor)

NUMERIC Mode Example:

PIN_CLEAR
PIN_Y, DATA=<y coordinate>
PIN_RECORD, DATA=<first bit>
PIN_RECORD, DATA=<bit 2>
PIN_RECORD, DATA=<bit 3>
...
PIN_RECORD, DATA=<bit N>
PIN_X_ENTER, DATA=<Encoding Scheme. See Macros below.>

*/

// Encoding Scheme IDs
#define ENCODING_INT     0b00
#define ENCODING_FLOAT32 0b01
#define ENCODING_FIXED32 0b10

#define PIN_RECORD 10 // Tells MCU to record the value
#define PIN_X_ENTER 11 // (ASCII) Tells MCU to put cursor on 'x' coordinate specified on data lines. (NUM) Tells the MCU the numeric entry is complete.
#define PIN_Y 12 // Tells MCU to put cursor on 'y' coordinate specified on data lines

#define PIN_CLEAR 13 // Tells MCU to clear display buffer
#define PIN_FLAG_ASCII 17 // CPU flag: If ON, sets display to ASCII mode
#define PIN_ENABLE_FLAG 16 // CPU flag: (NUM) If ON, interprets as floating point

#define PIN_BTN_INFO 15 // Hardware button to display software version / system info / mode

hd44780_I2Cexp lcd(0x27); // declare lcd object: auto locate & auto config expander chip

int data_pins[8] = {2, 3, 4, 5, 6, 7, 8, 9};

// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 04;

void setup() {
	int status;
	
	// Configure LCD
	status = lcd.begin(LCD_COLS, LCD_ROWS);
	if(status) {
		hd44780::fatalError(status); // does not return
	}
	
	// Configure pins
	for (int d = 0; d < 8 ; d++){
		pinMode(data_pins[d], INPUT);
	}
	pinMode(PIN_RECORD, INPUT);
	pinMode(PIN_X_ENTER, INPUT);
	pinMode(PIN_Y, INPUT);
	pinMode(PIN_CLEAR, INPUT);
	pinMode(PIN_ENABLE_FLAG, INPUT);
	pinMode(PIN_FLAG_ASCII, INPUT);
	
	
	// // Print a message to the LCD
	// lcd.print("Hello, World!");
	
	// lcd.backlight();
	// lcd.setCursor(0, 0); //X, Y
	// lcd.print("A");
	// lcd.print("B");
	// lcd.print("C");
	// lcd.setCursor(0,3);
	// lcd.print("K");
	
}

int x = 0;
int y = 0;

int read_data_lines(){
	float data_float = 0;
	for (int i = 7 ; i >= 0 ; i--){
		if (digitalRead(data_pins[i]) == HIGH) data_float += (pow(2.0, float(i)));	
	}
	
	return round(data_float);
}

// Global Variables
bool disp_enabled = false;
bool ascii_mode = false;
int last_record_status = HIGH;
bool show_info = false;
bool info_button_last_state = LOW;

// Display Values - Numeric Mode (by row)
float val0;
float val1;
float val2;
float val3;

// Which values are valid - Numberic Mode (by row)
bool init0 = false;
bool init1 = false;
bool init2 = false;
bool init3 = false;

bool isint0 = false;
bool isint1 = false;
bool isint2 = false;
bool isint3 = false;

char buffer[20];
String str;

void loop() {
	
	int val;
	
	//--------------------------- Check for basic inputs ----------------------------//
	
	// Check CPU enabled flag
	if (digitalRead(PIN_ENABLE_FLAG) == HIGH){
		disp_enabled = true;
		lcd.backlight();
	}else{
		disp_enabled = false;
		lcd.noBacklight();
	}
	
	// Check display mode
	if (digitalRead(PIN_FLAG_ASCII) == HIGH){
		
		// Clear display if mode changed
		if (ascii_mode == false){
			lcd.clear();
		}
		
		ascii_mode = true;
	}else{
		
		// Clear display if mode changed
		if (ascii_mode == true){
			lcd.clear();
		}
		
		ascii_mode = false;
	}
	
	// Check for info button toggle
	if (digitalRead(PIN_BTN_INFO) == HIGH){
		if (info_button_last_state == LOW){
			show_info = !show_info;
			lcd.clear();
		}
		info_button_last_state = HIGH;
	}else{
		info_button_last_state = LOW;
	}
	
	// Check for clear signal
	if (digitalRead(PIN_CLEAR)){
		
		// Clear display
		lcd.clear();
		
		// Clear buffers (Numeric mode)
		init0 = false;
		init1 = false;
		init2 = false;
		init3 = false;
	}
	
	// Check for data input
	if (digitalRead(PIN_RECORD)){
		val0 = read_data_lines();
		init0 = true;
		isint0 = true;
	}
	
	//--------------------------- Update Display and Interpret Serial -------------------------//
	
	// Show DISPLAY INFO if requested
	if (show_info){
		lcd.setCursor(0, 0);
		lcd.print("Blinken Display-PT");
		lcd.setCursor(0, 1);
		lcd.print("Firmware: v0.0");
		lcd.setCursor(0, 3);
		if (ascii_mode){
			lcd.print("Display Mode: ASCII");
			//         12345678901234567890
		}else{
			lcd.print("Display Mode: Num.");
			//         12345678901234567890
		}
		return;
	}
	
	if (ascii_mode){ //------- ASCII Mode -----------
		
		lcd.setCursor(0, 0);
		lcd.print("ERROR: ASCII mode");
		//         12345678901234567890
		lcd.setCursor(0, 1);
		lcd.print("not implemented.");
		lcd.setCursor(0, 3);
		lcd.print("Use numeric mode.");
		//         12345678901234567890
		
	}else{ //------------ Numeric Mode --------------
		
		if (init0){
			if (isint0){
				str = String(int(val0));
			}else{
				str = String(val0);
			}
			lcd.setCursor(20-str.length(), 0);
			lcd.print(str);
			
		}
	}
	
	// Check 'record to buffer' signal
	if (digitalRead(PIN_RECORD)){
		
		// Check that signal is new
		if (!last_record_status){
			
			// Get data and display character
			val = read_data_lines();
			//TODO: Translate val to char
			// lcd.print(ascii_character);
			
			last_record_status = HIGH;
		}
		
	}else{
		if (last_record_status){
			last_record_status = LOW;
		}
	}

	
}






