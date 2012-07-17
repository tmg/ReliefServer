#ifndef _CONSTANTS
#define _CONSTANTS	
//#define SPAN_SCREEN

// David settings
/*
#define SCREEN_WIDTH 2704
#define SCREEN_HEIGHT 768

#define PROJECTION_RECT_X 2029
#define PROJECTION_RECT_Y 223
#define PROJECTION_RECT_WIDTH 414
#define PROJECTION_RECT_HEIGHT 414
*/

// Matt settings
//#define SCREEN_WIDTH 2304
//#define SCREEN_HEIGHT 768

//#define PROJECTION_RECT_X 1533
//#define PROJECTION_RECT_Y 129


// Tony settings
/*
#define SCREEN_WIDTH 2464
#define SCREEN_HEIGHT 768

#define PROJECTION_RECT_X 1679
#define PROJECTION_RECT_Y 310
#define PROJECTION_RECT_WIDTH 420
#define PROJECTION_RECT_HEIGHT 420
*/

// Matthew Vertical Screen Settings
#define SCREEN_WIDTH  800
#define SCREEN_HEIGHT 400


#define FPS 60
#define	RELIEF_SIZE_X 12 
#define	RELIEF_SIZE_Y 12

#define GRID_WIDTH 43
#define FRAME_WIDTH (RELIEF_SIZE_X * GRID_WIDTH)

#define NUM_SERIAL_CONNECTIONS 3
#define MAX_NUM_ARDUINOS_PER_CONNECTION 12
#define NUM_ARDUINOS 32
#define NUM_PINS_ARDUINO 4

#define SERIAL_PORT_2 "/dev/tty.usbserial-A900ceuT"
#define SERIAL_PORT_1 "/dev/tty.usbserial-A800etID"
#define SERIAL_PORT_0 "/dev/tty.usbserial-A900cedr"
#define SERIAL_PORT_0_FIRST_ID 0
#define SERIAL_PORT_1_FIRST_ID 10
#define SERIAL_PORT_2_FIRST_ID 22
#define SERIAL_PORT_0_NUMEROFARDUINOS 10
#define SERIAL_PORT_1_NUMEROFARDUINOS 12
#define SERIAL_PORT_2_NUMEROFARDUINOS 10
#define SERIAL_BAUD_RATE 115200

#define ARDUINO_GAIN_P 150
#define ARDUINO_GAIN_I 35
#define ARDUINO_MAX_I  60
#define ARDUINO_DEADZONE 0

#define RELIEF_CONNECTED 1
#define DIRECT_MANIPULATION_DELAY 4

//#define PI 3.14159265

#define RELIEFSETTINGS "reliefSettings.xml"
#define RELIEF_FLOOR 110 //the pin setting corresponding to the lowest pin state
#define RELIEF_CEIL 0 //the pin setting corresponding to the highest pin state

#endif