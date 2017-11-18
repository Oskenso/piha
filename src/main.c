#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include <math.h>
#include <osk_image.h>


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

void print_hex_memory(void *mem, int size) {
	int i;
	unsigned char *p = (unsigned char *)mem;
	for (i = 0; i < size; i++) {
		printf("0x%02x ", p[i]);
		if (( i % 16 == 0) && i)
		printf("\n");
	}
	printf("\n");
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
	// 8_bit 	Triple transfer, 262k support
	//displaySend(DATA, 0x66); // 8_bit 	Dual transfer, 65k support
	//displaySend(DATA, 0x46); // 9_bit Dual transfer, 262k support
	//displaySend(DATA, 0x26); // 16_bit 	Single transfer, 65k support

	/*

	MEMORY_WRITE_MODE (16h)
			Bit 7	Bit6	Bit5	Bit4	Bit3	Bit2	Bit1	Bit0
	R/W		  - 	DFM1	DFM0	TRI 	 - 		HC		 VC		 HV
	Default	  0		 0		 0		 0		 0 		 1 		  1		  0


	DFM1	DFM0	TRI		BIT		Result
	 0		 0		 X		18_bit 	Single transfer, 262k support
	 0		 1		 X		16_bit 	Single transfer, 65k support
	 1		 0		 X		9_bit 	Dual transfer, 262k support
	 1		 1		 0		8_bit 	Dual transfer, 65k support
	 1		 1		 1		8_bit 	Triple transfer, 262k support

	HC : Horizontal address increment/decrement.
		When HC= 0, Horizontal address counter is decreased
		When HC= 1, Horizontal address counter is increased
	VC : Vertical address increment/decrement.
		When VC= 0, Vertical address counter is decreased
		When VC= 1, Vertical address counter is increased
	HV : Set the automatic update method of the AC after the data is written to the DDRAM.
		When HV= 0, The data is continuously written horizontally
		When HV= 1, The data is continuously written vertically
*/

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

void drawImage(Image *img) {
	int i;
	uint8_t *data = img->data;

	if (data == NULL) return;

	screenSetPos(0, 0);
    screenWriteMemoryStart();

	for (i = 0; i < (160 * 128 * 3); i++) {
		digitalWrite(P_CS, 0);
		digitalWrite(P_RS, 1);
		digitalWriteByte(data[i]);
		digitalWrite(P_RW, 0);
		digitalWrite(P_E, 1);
		digitalWrite(P_E, 0);
		digitalWrite(P_CS, 1);

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


void drawFramebuffer() {
    FILE *sysfb;
    uint8_t *buffer;

    sysfb = fopen("/dev/fb0/", "r");
    if (sysfb != NULL)
    {
        buffer = malloc (sizeof(uint8_t) * 160*128*4);

        result = fread(buffer, 160*128*4, 1, sysfb);
        if (result != lSize) {
            puts("Reading error", stderr);
            exit(3);
        }
        fclose (sysfb);

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

        free(buffer);
    }
}

int main() {
	struct dirent *dir;
	int imageCount = 0;
	void **imageList;

	signal(SIGINT, intHandler);

	DIR *d = opendir("./images/");
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if (dir->d_name[0] != '.') {
				imageCount++;
				//printf("%s - %d\n", dir->d_name, imageCount);

			}
		}
		rewinddir(d);

		imageList = malloc(sizeof(void *) * imageCount);
		int i = 0;

		while ((dir = readdir(d)) != NULL) {
			if (dir->d_name[0] != '.') {
				char fName[255];
				strcpy(fName, "images/");
				strcat(fName, dir->d_name);
				//printf("%s\n", fName);
				imageList[i] = CreateImageFromFile(fName);
				i++;
				//printf("%s - %d\n", dir->d_name, imageCount);
			}
		}

		closedir(d);
	}

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

	//clearScreen();

	int i;
	ImageList **fadeSequences;
	fadeSequences = malloc(sizeof(ImageList) * imageCount);

	double fadeTable[FADE_FRAMES];

	puts("Generate fade table");
	for (i = 0; i < FADE_FRAMES; i++) {
		//fadeTable[i] = pow(i, FADE_RATE);
		fadeTable[i] = pow(i, 5.5412635451584261 / log(FADE_FRAMES));
	}

	puts("Generating sequences");
	for (i = 0; i < imageCount; i++) {
		fadeSequences[i] = CreateFadeSequence(imageList[i], &fadeTable);
	}

	puts("Draw!");

	i = -1;
	int nextFrame = 0;
	while (keepRunning) {
		i++;
		if (i >= imageCount) {
			i = 0;
		}
		drawImage(imageList[i]);
		delay(2000);
		nextFrame = (i+1) % imageCount;

		int step;
		for  (step = 0; step < fadeSequences[i]->count; step++) {
			//drawImage(fadeSequences[i]->images[step]);
            drawFramebuffer();
		}
		for (step = fadeSequences[nextFrame]->count-1; step >= 0; step--) {
			drawImage(fadeSequences[nextFrame]->images[step]);
		}

	}

	//digitalWrite(P_RES, 0);
	//delay(500);

	displaySend(COMMAND, 0x04);//power save
	displaySend(DATA, 0x05);// 1/2 driving current, display off
	//delay(2);
	exit(EXIT_SUCCESS);

}
