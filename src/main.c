#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <math.h>
//#include <osk_image.h>


#define P_RS 22 //D/C
#define P_RW 23
#define P_E 24
#define P_RES 25
#define P_PS 27
#define P_CS 28

#define COMMAND 0
#define DATA 1
#define CHANNEL 1

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    keepRunning = 0;
}

void displaySend(uint8_t t, uint8_t d) {
	digitalWrite(P_CS, 0);
	digitalWrite(P_RS, t);
	digitalWriteByte(d);
    digitalWrite(P_RW, 0);
    digitalWrite(P_E, 1);
    digitalWrite(P_E, 0);
    digitalWrite(P_CS, 1);
}

void displaySetReg(uint8_t c, uint8_t d) {
	displaySend(COMMAND, c);
	displaySend(DATA, d);
}

void displaySetColumnAddress(uint8_t startX, uint8_t endX) {
	displaySetReg(0x17, startX);
	displaySetReg(0x18, endX);
}

void displaySetRowAddress(uint8_t startY, uint8_t endY) {
	displaySetReg(0x19, startY);
	displaySetReg(0x1A, endY);
}

//Begin write to RAM
void screenWriteMemoryStart() {
	displaySend(COMMAND, 0x22);
}

void initDisplay() {
	//digitalWrite(P_CPU, 0);
	//digitalWrite(P_PS, 0);

	digitalWrite(P_RES, 0);
	delay(100);
	digitalWrite(P_RES, 1);
	delay(100);

	//displaySend(COMMAND, 0x04);
	//displaySend(DATA, 0x03);
	//delay(2);

	//half driving current
	displaySetReg(0x04, 0x04);

	displaySetReg(0x3B, 0x00);
	displaySetReg(0x02, 0x01);//export1 pin at interal

	displaySetReg(0x03, 0x90);//120hz
	displaySetReg(0x80, 0x01);//ref voltage

	displaySetReg(0x08, 0x04);
	displaySetReg(0x09, 0x05);
	displaySetReg(0x0A, 0x05);

	displaySetReg(0x0B, 0x9D);
	displaySetReg(0x0C, 0x8C);
	displaySetReg(0x0D, 0x57);//pre charge of blue
	displaySetReg(0x10, 0x56);
	displaySetReg(0x11, 0x4D);
	displaySetReg(0x12, 0x46);
	displaySetReg(0x13, 0x00);//set color sequence

	displaySetReg(0x14, 0x01);// Set MCU Interface Mode
	displaySetReg(0x16, 0x76);// Set Memory Write Mode

	//shift mapping ram counter
	displaySetReg(0x20, 0x00);
	displaySetReg(0x21, 0x00);
	displaySetReg(0x28, 0x7F);// 1/128 Duty (0x0F~0x7F)
	displaySetReg(0x29, 0x00);// Set Mapping RAM Display Start Line (0x00~0x7F)
	displaySetReg(0x06, 0x01);// Display On (0x00/0x01)

	// Set All Internal Register Value as Normal Mode
	displaySetReg(0x05, 0x00); // Disable Power Save Mode

	// Set RGB Interface Polarity as Active Low
	displaySetReg(0x15, 0x00);

	displaySetColumnAddress(0, 159);
	displaySetRowAddress(0, 127);
}

void screenSetPos(uint8_t x, uint8_t y) {
	displaySetReg(0x20, x);
	displaySetReg(0x21, y);
}

void screenSetPixel(unsigned long color) {
	uint8_t buffer[3];
	//displaySend(DATA, color >> 16);
	//displaySend(DATA, color >> 8);
	//displaySend(DATA, color);
	buffer[0] = color >> 16;
	buffer[1] = color >> 8;
	buffer[2] = color;
	//digitalWrite(P_RS, 1);
	//write(spiHandle, buffer, 3);
	for (int i = 0; i < 3; i++) {
		digitalWrite(P_CS, 0);
		digitalWrite(P_RS, 1);
		digitalWriteByte(buffer[i]);
		digitalWrite(P_RW, 0);
		digitalWrite(P_E, 1);
		digitalWrite(P_E, 0);
		digitalWrite(P_CS, 1);
	}


}

void screenFill(unsigned long color) {
	unsigned int i;

	screenSetPos(0, 0);
	screenWriteMemoryStart();
	uint8_t buffer[20480];

	digitalWrite(P_RS, 1);
	for (i = 0; i < (160 * 128); i++) {
		screenSetPixel(color);
	}
}

void setFirstWindowPosition(uint8_t x, uint8_t y) {
	displaySetReg(0x2e, x);
	displaySetReg(0x2f, y);
}
void setSecondWindowPosition(uint8_t x, uint8_t y) {

	displaySend(COMMAND, 0x31);
	displaySend(DATA, x);
	displaySend(COMMAND, 0x32);
	displaySend(DATA, y);
}

//#define COLOR_MODE_1 0x76; // triple transfer 262k
//#define COLOR_MODE_2 0x66; // dual transfer 65k
void clearScreen(uint16_t color) {
	displaySend(COMMAND, 0x16);// Set Memory Write Mode
	displaySend(DATA, 0x66); // 8_bit 	Dual transfer, 65k support

	screenWriteMemoryStart();

	for (int i = 0; i < (160 * 128); i++) {
		digitalWrite(P_CS, 0);
		digitalWrite(P_RS, 1);
		digitalWriteByte(color & 0xFF);
	    digitalWrite(P_RW, 0);
	    digitalWrite(P_E, 1);
	    digitalWrite(P_E, 0);
	    digitalWrite(P_CS, 1);

		digitalWrite(P_CS, 0);
		digitalWrite(P_RS, 1);
		digitalWriteByte(color >> 8);
	    digitalWrite(P_RW, 0);
	    digitalWrite(P_E, 1);
	    digitalWrite(P_E, 0);
	    digitalWrite(P_CS, 1);
	}

	displaySend(COMMAND, 0x16);// Set Memory Write Mode
	displaySend(DATA, 0x76); // 8_bit 	Triple transfer, 262k support
}

void screenSetColorMode(uint8_t colorMode) {
	displaySend(COMMAND, 0x16);// Set Memory Write Mode
	displaySend(DATA, 0x66); // 8_bit 	Dual transfer, 65k support
	displaySend(DATA, 0x76); // 8_bit 	Triple transfer, 262k support

}

void drawFramebuffer(uint8_t *buffer) {
    screenWriteMemoryStart();
    for (int i = 0; i < (160*128); i++) {
        //uint8_t color = buffer[i+0] << 16 | buffer[i+1] << 8 | buffer[i+2];

        digitalWrite(P_CS, 0);
        digitalWrite(P_RS, 1);
        digitalWriteByte(buffer[(i*4)+0]);
        digitalWrite(P_RW, 0);
        digitalWrite(P_E, 1);
        digitalWrite(P_E, 0);
        digitalWrite(P_CS, 1);

        digitalWrite(P_CS, 0);
        digitalWrite(P_RS, 1);
        digitalWriteByte(buffer[(i*4)+1]);
        digitalWrite(P_RW, 0);
        digitalWrite(P_E, 1);
        digitalWrite(P_E, 0);
        digitalWrite(P_CS, 1);

        digitalWrite(P_CS, 0);
        digitalWrite(P_RS, 1);
        digitalWriteByte(buffer[(i*4)+2]);
        digitalWrite(P_RW, 0);
        digitalWrite(P_E, 1);
        digitalWrite(P_E, 0);
        digitalWrite(P_CS, 1);
    }
}


void shrinkImage(uint8_t *in, uint8_t *out) {
	int offset = 0;
	int w = 160;
	int h = 144;

	for (int i = 0; i < w * h*4; i++) {
		int line = floor(i / w);
		if ( ((i % w) == 0) && ((line % 8) == 1) && (line != 0)) {
			offset += w;
		}
		//uint8_t r,g,b;
		//r = (((in[i+offset] >> 16) & 0xFF) + ((in[i+offset-160] >> 16) & 0xFF)) / 2;
		//g = (((in[i+offset] >> 8) & 0xFF) + ((in[i+offset-160] >> 8) & 0xFF)) / 2;
		//b = (((in[i+offset]) & 0xFF) + ((in[i+offset-160]) & 0xFF)) / 2;
		//out[i] = r << 16 | g << 8 | b;
        if (offset >= w) {
            out[i] = (in[i+offset] + in[i+offset-160]) / 2;
        } else {
            out[i] = in[i+offset];
        }

	}

}

int main() {

	signal(SIGINT, intHandler);



	wiringPiSetup();
	//pinMode(P_CPU, OUTPUT);
	pinMode(P_PS, OUTPUT);
	pinMode(P_RES, OUTPUT);
	pinMode(P_RS, OUTPUT);

	pinMode(P_RW, OUTPUT);
	pinMode(P_E, OUTPUT);
	pinMode(P_CS, OUTPUT);
	pinMode(0, OUTPUT);
	pinMode(1, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(3, OUTPUT);
	pinMode(4, OUTPUT);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);

	digitalWrite(P_PS, 1);

	initDisplay();

	//#define debug 1

	#ifdef debug
	//power save
	displaySetReg(0x04, 0x05);// 1/2 driving current, display off
	return 0;
	#endif


    //FILE *sysfb;
    //sysfb = fopen("/dev/fb0", "r");
	int sysfb;
	sysfb = open("/dev/fb0", O_RDONLY);
    //uint8_t *buffer = malloc(160*144*4);
    uint8_t *toOled = malloc(160*128*4);
	while (keepRunning) {
        //rewind(sysfb);
        //uint8_t result = fread(buffer, 160*128*4, 1, sysfb);
		pread(sysfb, toOled, 160*128*4, 0);
        //shrinkImage(buffer, toOled);
        drawFramebuffer(toOled);
        delay(16);
	}
	//digitalWrite(P_RES, 0);
	//fclose(sysfb);
    free(toOled);
	close(sysfb);
	//delay(500);

	displaySend(COMMAND, 0x04);//power save
	displaySend(DATA, 0x05);// 1/2 driving current, display off
	//delay(2);
	exit(EXIT_SUCCESS);

}
