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

#define DEBOUNCE_DELAY_MS 300 // Time (ms) to pause for debouncing

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

int read_data_lines(int byte_no=0){
	float data_float = 0;
	for (int i = 7 ; i >= 0 ; i--){
		if (digitalRead(data_pins[i]) == HIGH) data_float += (pow(2.0, float(i+8*byte_no)));	
	}
	
	return round(data_float);
}

// Global Variables
bool disp_enabled = false;
bool ascii_mode = false;
int last_record_status = HIGH;
bool show_info = false;
bool info_button_last_state = LOW;
bool RECORD_last_state = LOW;
bool ENTER_last_state = LOW;

// Display Values - Numeric Mode (by row)
float val0;
float val1;
float val2;
float val3;

// Cursor Position
int cursor_y = 0;
int byte_no = 0;

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
			delay(DEBOUNCE_DELAY_MS);
		}
		info_button_last_state = HIGH;
	}else{
		if (info_button_last_state == HIGH){
			delay(DEBOUNCE_DELAY_MS);
		}
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
		
		return;
	}
	
	// Cursor Y position signal
	if (digitalRead(PIN_Y)){
		lcd.setCursor(0, 0);
		lcd.print("Y-set:");
		cursor_y = read_data_lines();
		if (cursor_y > 3){
			cursor_y = 3;
		}
		lcd.print(String(cursor_y));
		return;
	}
	
	// Check for --RECORD-- key, --> data input
	if (digitalRead(PIN_RECORD)){
		
		if (RECORD_last_state == LOW){
			if (cursor_y == 0){
				if (byte_no == 0){
					val0 = 0;
					lcd.setCursor(0, 0);
					lcd.print("                    ");
					//         12345678901234567890
					init0 = false;
				}
				val0 += read_data_lines(byte_no);
				byte_no++;
			}else if(cursor_y == 1){
				if (byte_no == 0){
					val1 = 0;
					lcd.setCursor(0, 1);
					lcd.print("                    ");
					//         12345678901234567890
					init1 = false;
				}
				val1 += read_data_lines(byte_no);
				byte_no++;
			}else if(cursor_y == 2){
				if (byte_no == 0){
					val2 = 0;
					lcd.setCursor(0, 2);
					lcd.print("                    ");
					//         12345678901234567890
					init2 = false;
				}
				val2 += read_data_lines(byte_no);
				byte_no++;
			}else if(cursor_y == 3){
				if (byte_no == 0){
					val3 = 0;
					lcd.setCursor(0, 3);
					lcd.print("                    ");
					//         12345678901234567890
					init3 = false;
				}
				val3 += read_data_lines(byte_no);
				byte_no++;
			}
			
			lcd.setCursor(0, 0);
			lcd.print("BN: ");
			lcd.print(String(byte_no));
			
			//Wait. This should be reduced in final version and is set high for breadboard use.
			delay(DEBOUNCE_DELAY_MS);
		}
		
		RECORD_last_state = HIGH;

	}else{
		if (RECORD_last_state == HIGH){
			delay(DEBOUNCE_DELAY_MS);
		}
		RECORD_last_state = LOW;
	}
	
	// Check for --ENTER-- key
	if (digitalRead(PIN_X_ENTER)){
		
		if (ENTER_last_state == LOW){
			// Get encoding code
			int encoding_code = read_data_lines();
			
			if (cursor_y == 0){
				if (encoding_code == 0){
					isint0 = true;
				}else{
					isint0 = false;
				}
				init0 = true;
			}else if(cursor_y == 1){
				if (encoding_code == 0){
					isint1 = true;
				}else{
					isint1 = false;
				}
				init1 = true;
			}else if(cursor_y == 2){
				if (encoding_code == 0){
					isint2 = true;
				}else{
					isint2 = false;
				}
				init2 = true;
			}else if(cursor_y == 3){
				if (encoding_code == 0){
					isint3 = true;
				}else{
					isint3 = false;
				}
				init3 = true;
			}
			
			
			// Reset byte count
			byte_no = 0;
			
			delay(DEBOUNCE_DELAY_MS);
		}
		
		ENTER_last_state = HIGH;
	}else{
		if (ENTER_last_state == HIGH){
			delay(DEBOUNCE_DELAY_MS);
		}
		ENTER_last_state = LOW;
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
		if (init1){
			if (isint1){
				str = String(int(val1));
			}else{
				str = String(val1);
			}
			lcd.setCursor(20-str.length(), 1);
			lcd.print(str);
		}
		if (init2){
			if (isint2){
				str = String(int(val2));
			}else{
				str = String(val2);
			}
			lcd.setCursor(20-str.length(), 2);
			lcd.print(str);
		}
		if (init3){
			if (isint3){
				str = String(int(val3));
			}else{
				str = String(val3);
			}
			lcd.setCursor(20-str.length(), 3);
			lcd.print(str);
		}
	}

	
}






