#include "Arduino.h"
#include "Cube.h"
#include "SPI.h"

#define sck_clock 15             //
#define mosi_data 16             //
#define ss_speicher 17           //
#define miso_disable 14          // Definierung der Pins für die SPI Library

byte CubeClass::loopCount;
byte CubeClass::anode[5] = { B10000, B01000, B00100, B00010, B00001 };  // Anoden Bits
byte CubeClass::green[4][25];											// 4 Bits für jede LED im Würfel zur Helligkeitsregulierung
byte CubeClass::output[4];												// Transfer Bytes

byte CubeClass::level = 0;
byte CubeClass::BAM_Bit = 0;
byte CubeClass::BAM_Counter = 0;				// Bitangle Modulation Zähler
byte CubeClass::whichbyte;
byte CubeClass::whichbit;
byte CubeClass::pos;

CubeClass::CubeClass() {}

void CubeClass::SPI_Init() {
	noInterrupts();						// Verhindern von Unterbrechungen beim Setup
	SPIClass::setBitOrder(MSBFIRST);    // Setzen der Bit-Reihenfolge

	TCCR1A = B00000000;
	TCCR1B = B00001011;
	TIMSK1 = B00000010;
	OCR1A = 9;

	pinMode(ss_speicher, OUTPUT);		// *
	pinMode(mosi_data, OUTPUT);			// *
	pinMode(sck_clock, OUTPUT);			// *
	pinMode(miso_disable, OUTPUT);		// Setzen der Pins auf Output
	SPIClass::begin();					// Starten der SPI Library
	Serial.begin(9600);
	interrupts();
}

void CubeClass::uebertragung() {
	if (BAM_Counter == 8)
		BAM_Bit++;
	else if (BAM_Counter == 24)
		BAM_Bit++;
	else if (BAM_Counter == 56)
		BAM_Bit++;                      // Erhöhung des BAM_Bit nach bestimmten Größen des BAM_Counters (Durchläufe: 8; 16; 32; 64)

	BAM_Counter++;                      // Erhöhung des BAM_Counters

	if (BAM_Counter == 120) {
		BAM_Counter = 0;
		BAM_Bit = 0;
	}

	pos = level * 5;
	output[0] = (anode[level] << 3) + (green[BAM_Bit][pos] >> 2);
	output[1] = (green[BAM_Bit][pos] << 6 & 255) + (green[BAM_Bit][pos + 1] << 1 & 255) + (green[BAM_Bit][pos + 2] >> 4);
	output[2] = (green[BAM_Bit][pos + 2] << 4 & 255) + (green[BAM_Bit][pos + 3] >> 1);
	output[3] = (green[BAM_Bit][pos + 3] << 7 & 255) + (green[BAM_Bit][pos + 4] << 2);

	for (loopCount = 0; loopCount < 4; loopCount++) {
		SPIClass::transfer(output[loopCount]);
	}

	PORTB |= _BV(PB0);                   // Aktualisieren der Schieberegister ON
	PORTB &= ~_BV(PB0);                  // OFF

	level++;                             // Erhöhung des Ebenen-Zählers

	if (level == 5)                      // Rücksetzen des Ebenen-Zählers
		level = 0;

	for (loopCount = 0; loopCount < 4; loopCount++) {
		output[loopCount] = 0;           // Rücksetzen der Output-Bytes
	}
}

ISR(TIMER1_COMPA_vect) {				 // Interuptmethode
	CubeClass::uebertragung();
}

void CubeClass::LED(byte column, byte row, byte level, byte brightness) {      // Steuerung beliebiger LED mit Helligkeit --- X - Y - Z
	level--;																//
	row--;																	//
	column--;																// Subtraktion zur Ermöglichung von Eingabe von 1-5

	if (level < 0)
		level = 0;
	if (level > 4)
		level = 4;
	if (row < 0)
		row = 0;
	if (row > 4)
		row = 4;
	if (column < 0)
		column = 0;
	if (column > 4)
		column = 4;
	if (brightness < 0)
		brightness = 0;
	if (brightness > 15)
		brightness = 15;                                                // Fehlervermeidung

	whichbyte = int((level * 25 + row * 5) / 5);						// Byte, das umgeschrieben werden soll
	whichbit = 4 - column;

	for (int i = 0; i < 4; i++){
		bitWrite(green[i][whichbyte], whichbit, bitRead(brightness, i));
	} // Überschreiben der Bits im Speicher Array nach gewählter Helligkeit
}

void CubeClass::revLED(byte x, byte y, byte z, byte brightness, byte version) {
	switch (version) {
		case 1:
			LED(x, z, y, brightness);
			break;
		case 2:
			LED(z, x, y, brightness);
			break;
		case 3:
			LED(z, y, x, brightness);
			break;
		case 4:
			LED(y, x, z, brightness);
			break;
		case 5:
			LED(y, z, x, brightness);
			break;
		default:
			LED(x, y, z, brightness);
			break;
	}
}

void CubeClass::PAUSE(int milli) {	// Pause ohne Unterbrechung des Systems
	long now = millis();
	while (millis() < now + milli) {  }
}

void CubeClass::clean() {
	for (int k = 0; k < 4; k++) {
		for (int m = 0; m < 25; m++) {
			green[k][m] = 0;
		}
	}
}

void CubeClass::heart() {
	byte shape[][10][3] = { { { 5, 2, 3 }, { 5, 4, 3 }, { 4, 1, 3 }, { 4, 3, 3 }, { 4, 5, 3 }, { 3, 1, 3 }, { 3, 5, 3 }, { 2, 2, 3 }, { 2, 4, 3 }, { 1, 3, 3 } },
						    { { 5, 2, 2 }, { 5, 4, 4 }, { 4, 1, 2 }, { 4, 3, 3 }, { 4, 5, 4 }, { 3, 1, 2 }, { 3, 5, 4 }, { 2, 2, 2 }, { 2, 4, 4 }, { 1, 3, 3 } },
						    { { 5, 2, 2 }, { 5, 4, 4 }, { 4, 2, 1 }, { 4, 3, 3 }, { 4, 4, 5 }, { 3, 2, 1 }, { 3, 4, 5 }, { 2, 2, 2 }, { 2, 4, 4 }, { 1, 3, 3 } },
						    { { 5, 3, 2 }, { 5, 3, 4 }, { 4, 3, 1 }, { 4, 3, 3 }, { 4, 3, 5 }, { 3, 3, 1 }, { 3, 3, 5 }, { 2, 3, 2 }, { 2, 3, 4 }, { 1, 3, 3 } },
						    { { 5, 4, 2 }, { 5, 2, 4 }, { 4, 4, 1 }, { 4, 3, 3 }, { 4, 2, 5 }, { 3, 4, 1 }, { 3, 2, 5 }, { 2, 4, 2 }, { 2, 2, 4 }, { 1, 3, 3 } },
						    { { 5, 4, 2 }, { 5, 2, 4 }, { 4, 5, 2 }, { 4, 3, 3 }, { 4, 1, 4 }, { 3, 5, 2 }, { 3, 1, 4 }, { 2, 4, 2 }, { 2, 2, 4 }, { 1, 3, 3 } } };

	for (int k = 1; k < 3; k++) { // VOR
		for (int m = 0; m<10; m++){
			LED(k, shape[0][m][1], shape[0][m][0], 3 * k);
		}
		PAUSE(200);
		for (int m = 0; m<10; m++){
			LED(k, shape[0][m][1], shape[0][m][0], 0);
		}
	}
	for (int i = 0; i<2; i++){     // DREHUNG I
		for (int k = 0; k<6; k++){
			for (int m = 0; m<10; m++){
				LED(shape[k][m][2], shape[k][m][1], shape[k][m][0], 9);
			}
			PAUSE(200);
			for (int m = 0; m<10; m++){
				LED(shape[k][m][2], shape[k][m][1], shape[k][m][0], 0);
			}
		}
	}
	for (int k = 3; k < 6; k++) { // WEITER VOR
		for (int m = 0; m<10; m++){
			LED(k, shape[0][m][1], shape[0][m][0], 3 * k);
		}
		PAUSE(200);
		for (int m = 0; m<10; m++){
			LED(k, shape[0][m][1], shape[0][m][0], 0);
		}
	}
	for (int k = 5; k > 0; k--) { // ZURÜCK
		for (int m = 0; m<10; m++){
			LED(k, shape[0][m][1], shape[0][m][0], 3 * k);
		}
		PAUSE(100);
		for (int m = 0; m<10; m++){
			LED(k, shape[0][m][1], shape[0][m][0], 0);
		}
	}
	PAUSE(300);
	for (int k = 5; k > 0; k--) { // VOR II
		for (int m = 0; m<10; m++){
			LED(shape[0][m][1], k, shape[0][m][0], 3 * (6 - k));
		}
		PAUSE(200);
		for (int m = 0; m<10; m++){
			LED(shape[0][m][1], k, shape[0][m][0], 0);
		}
	}
	for (int k = 1; k < 3; k++) { // ZURÜCK
		for (int m = 0; m<10; m++){
			LED(shape[0][m][1], k, shape[0][m][0], 3 * (6 - k));
		}
		PAUSE(100);
		for (int m = 0; m<10; m++){
			LED(shape[0][m][1], k, shape[0][m][0], 0);
		}
	}
	for (int i = 0; i < 2; i++){  // DREHUNG II
		for (int k = 0; k < 6; k++){
			for (int m = 0; m < 10; m++){
				LED(shape[k][m][1], shape[k][m][2], shape[k][m][0], 9);
			}
			PAUSE(100);
			for (int m = 0; m<10; m++){
				LED(shape[k][m][1], shape[k][m][2], shape[k][m][0], 0);
			}
		}
	}
	for (int k = 3; k < 6; k++) { // WEITER ZURÜCK
		for (int m = 0; m<10; m++){
			LED(shape[0][m][1], k, shape[0][m][0], 3 * (6 - k));
		}
		PAUSE(100);
		for (int m = 0; m<10; m++){
			LED(shape[0][m][1], k, shape[0][m][0], 0);
		}
	}
	PAUSE(300);
}

void CubeClass::edgeFill(int pause){

	for (int i = 0; i < 13; i++){
		for (int k = 0; k < 5; k++){
			if (i - k >= 0){
				for (int m = 0; m < 5 - abs(4 - (i - k)); m++){
					LED(edgeShape[i - k][m][0], 6 - edgeShape[i - k][m][1], 5 - k, 15);
				}
			}
		}
		PAUSE(pause);
	}
}

void CubeClass::edgeClear(int pause){
	for (int i = 0; i < 13; i++){
		for (int k = 0; k < 5; k++){
			if (i - k >= 0){
				for (int m = 0; m < 5 - abs(4 - (i - k)); m++){
					LED(edgeShape[i - k][m][0], edgeShape[i - k][m][1], k + 1, 0);
				}
			}
		}
		PAUSE(pause);
	}
}

void CubeClass::pulse() {
	edgeFill(400);
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 31; x++) {
			for (int a = 1; a < 6; a++) {
				for (int b = 1; b < 6; b++) {
					for (int c = 1; c < 6; c++) {
						LED(a, b, c, abs(15 - x));
					}
				}
			}
			PAUSE(50);
		}
	}
	edgeClear(400);
}

void CubeClass::crossDemo() {
	int lol;
	for (int m = 0; m < 2; m++) {
		if (m % 2 == 0)
			lol = 15;
		else
			lol = 0;
		for (int l = 1; l < 6; l++) {
			LED(l, l, l, lol);
			PAUSE(100);
		}
		for (int l = 1; l < 6; l++) {
			LED(6 - l, l, l, lol);
			PAUSE(100);
		}
		for (int l = 1; l < 6; l++) {
			LED(l, 6 - l, l, lol);
			PAUSE(100);
		}
		for (int l = 1; l < 6; l++) {
			LED(l, l, 6 - l, lol);
			PAUSE(100);
		}
	}
}

void CubeClass::splitter(int ver) {
	byte shape[][3] = { { 1, 1, 1 }, { 1, 1, 2 }, { 1, 1, 3 }, { 1, 1, 4 }, { 1, 1, 5 },
					    { 2, 1, 1 }, { 2, 1, 2 }, { 2, 1, 3 }, { 2, 1, 4 }, { 2, 1, 5 },
					    { 3, 1, 1 }, { 3, 1, 2 }, { 3, 1, 3 }, { 3, 1, 4 }, { 3, 1, 5 },
					    { 4, 1, 1 }, { 4, 1, 2 }, { 4, 1, 3 }, { 4, 1, 4 }, { 4, 1, 5 },
					    { 5, 1, 1 }, { 5, 1, 2 }, { 5, 1, 3 }, { 5, 1, 4 }, { 5, 1, 5 } };

	for (int z = 0; z < 25; z++) {                        // Ebene erstellen
		revLED(1, shape[z][2], shape[z][0], 15, ver);
	}
	PAUSE(300);
	for (int z = 1; z < 5; z++) {                         // Punkte vor
		for (int u = 0; u < 25; u++) {
			if (shape[u][1] == z && random(5 + 1 - z) != 0) { // wenn LED auf derzeitiger Ebene liegt && eine Zufallszahl von 5+1-Ebene (Zufallszahl < 6-Ebene) ungleich 0 ist, befördere LED
				revLED(shape[u][1], shape[u][2], shape[u][0], 0, ver);
				shape[u][1] = shape[u][1] + 1;
				revLED(shape[u][1], shape[u][2], shape[u][0], 15, ver);
			}
		}
		PAUSE(100);
	}
	PAUSE(300);
	for (int z = 1; z < 5; z++) {                         // Ebene nach
		for (int u = 0; u < 25; u++) {
			if (shape[u][1] == z) {
				revLED(shape[u][1], shape[u][2], shape[u][0], 0, ver);
				shape[u][1] = z + 1;
				revLED(shape[u][1], shape[u][2], shape[u][0], 15, ver);
			}
		}
		PAUSE(100);
	}
	PAUSE(300);

	for (int z = 5; z >= 1; z--) {                        // Punkte zurück
		for (int u = 0; u < 25; u++) {
			if (shape[u][1] == z && random(z) != 0) {
				revLED(shape[u][1], shape[u][2], shape[u][0], 0, ver);
				shape[u][1] = z - 1;
				revLED(shape[u][1], shape[u][2], shape[u][0], 15, ver);
			}
		}
		PAUSE(100);
	}
	PAUSE(300);
	for (int z = 5; z >= 2; z--) {                        // Ebene nach
		for (int u = 0; u < 25; u++) {
			if (shape[u][1] == z) {
				revLED(shape[u][1], shape[u][2], shape[u][0], 0, ver);
				shape[u][1] = z - 1;
				revLED(shape[u][1], shape[u][2], shape[u][0], 15, ver);
			}
		}
		PAUSE(100);
	}
	clean();
}

void CubeClass::wallsBeta() {
	byte shape[][25][3] = { { { 5, 1, 5 }, { 5, 1, 4 }, { 5, 1, 3 }, { 5, 1, 2 }, { 5, 1, 1 }, { 4, 1, 5 }, { 4, 1, 4 }, { 4, 1, 3 }, { 4, 1, 2 }, { 4, 1, 1 }, { 3, 1, 5 }, { 3, 1, 4 }, { 3, 1, 3 }, { 3, 1, 2 }, { 3, 1, 1 }, { 2, 1, 5 }, { 2, 1, 4 }, { 2, 1, 3 }, { 2, 1, 2 }, { 2, 1, 1 }, { 1, 1, 5 }, { 1, 1, 4 }, { 1, 1, 3 }, { 1, 1, 2 }, { 1, 1, 1 } },
							{ { 5, 5, 5 }, { 5, 4, 5 }, { 5, 3, 5 }, { 5, 2, 5 }, { 5, 1, 5 }, { 5, 5, 4 }, { 5, 4, 4 }, { 5, 3, 4 }, { 5, 2, 4 }, { 5, 1, 4 }, { 5, 5, 3 }, { 5, 4, 3 }, { 5, 3, 3 }, { 5, 2, 3 }, { 5, 1, 3 }, { 5, 5, 2 }, { 5, 4, 2 }, { 5, 3, 2 }, { 5, 2, 2 }, { 5, 1, 2 }, { 5, 5, 1 }, { 5, 4, 1 }, { 5, 3, 1 }, { 5, 2, 1 }, { 5, 1, 1 } } };

	for (int i = 1; i < 21; i++){
		if (i < 10){
			for (int k = 0; k<25; k++){
				LED(shape[1][k][1], shape[1][k][2], 5 - (i / 3) + 1, 15);
			}
		}
		else if (i < 11){
			for (int k = 0; k<25; k++){
				LED(shape[1][k][1], shape[1][k][2], 4, 15);
			}
		}
		else if (i < 14){
			for (int k = 0; k<25; k++){
				LED(shape[1][k][1], shape[1][k][2], 5 - (i - 8), 15);
			}
		}
		else if (i < 21){
			for (int k = 0; k<25; k++){
				LED(shape[1][k][1], shape[1][k][2], 6 - (4 - (i - 14) / 2), 15);
			}
		}

		if (i < 9){
			for (int k = 0; k<25; k++){
				LED((i + 1) / 2, shape[0][k][2], shape[0][k][0], 15);
			}
		}
		else if (i < 12){
			for (int k = 0; k<25; k++){
				LED(4 - (i - 9), shape[0][k][2], shape[0][k][0], 15);
			}
		}
		else if (i < 21){
			for (int k = 0; k<25; k++){
				LED((i - 3) / 3, shape[0][k][2], shape[0][k][0], 15);
			}
		}

		PAUSE(120);

		clean();
	}
	clean();
}

void CubeClass::spirale(int pause){
	int val;
	byte shape[][5][3] = { { { 1, 2, 1 }, { 2, 3, 1 }, { 3, 4, 1 }, { 4, 5, 2 }, { 5, 5, 3 } },
						   { { 1, 3, 1 }, { 2, 4, 1 }, { 3, 5, 2 }, { 4, 5, 3 }, { 5, 5, 4 } },
						   { { 1, 4, 1 }, { 2, 5, 2 }, { 3, 5, 3 }, { 4, 5, 4 }, { 5, 4, 5 } },
						   { { 1, 5, 2 }, { 2, 5, 3 }, { 3, 5, 4 }, { 4, 4, 5 }, { 5, 3, 5 } },
						   { { 1, 5, 3 }, { 2, 5, 4 }, { 3, 4, 5 }, { 4, 3, 5 }, { 5, 2, 5 } },
						   { { 1, 5, 4 }, { 2, 4, 5 }, { 3, 3, 5 }, { 4, 2, 5 }, { 5, 1, 4 } } };

	for (int i = 0; i < 42; i++){
		val = i % 6;
		for (int k = 0; k < 5; k++){
			LED(shape[val][k][1], shape[val][k][2], shape[val][k][0], 15);
			LED(6 - shape[val][k][1], 6 - shape[val][k][2], shape[val][k][0], 15);
		}
		PAUSE(pause);
		for (int k = 0; k < 5; k++){
			LED(shape[val][k][1], shape[val][k][2], shape[val][k][0], 0);
			LED(6 - shape[val][k][1], 6 - shape[val][k][2], shape[val][k][0], 0);
		}
	}
	for (int i = 0; i < 42; i++){
		val = i % 6;
		for (int k = 0; k < 5; k++){
			LED(shape[5 - val][k][1], shape[5 - val][k][2], shape[5 - val][k][0], 15);
			LED(6 - shape[5 - val][k][1], 6 - shape[5 - val][k][2], shape[5 - val][k][0], 15);
		}
		PAUSE(pause);
		for (int k = 0; k < 5; k++){
			LED(shape[5 - val][k][1], shape[5 - val][k][2], shape[5 - val][k][0], 0);
			LED(6 - shape[5 - val][k][1], 6 - shape[5 - val][k][2], shape[5 - val][k][0], 0);
		}
	}
}

void CubeClass::_print(String message, int time){
	byte mSize = message.length();
	char arr[mSize];
	int ascToInd;

	for (int i = 0; i < mSize; i++){
		arr[i] = message[i];
	}

	for (int x = 0; x < mSize; x++){
		if (arr[x] == 32){
			PAUSE(200);
		}
		else{
			if (arr[x] == 104)
				ascToInd = 0;
			else{
				ascToInd = arr[x] - 47;
				if (ascToInd > 10)
					ascToInd -= 7;
			}
			for (int i = 1; i < 6; i++){
				for (int k = 0; k < 16; k++){
					if (letter[ascToInd][k][0] != 0)
						LED(i, letter[ascToInd][k][1], letter[ascToInd][k][0], i * 3);
				}
				PAUSE(time);
				for (int k = 0; k < 16; k++){
					LED(i, letter[ascToInd][k][1], letter[ascToInd][k][0], 0);
				}
			}
		}
	}
	PAUSE(time);
}

void CubeClass::heartbeat(byte num) {
	for (int j = 0; j < num; j++){
		for (int i = 0; i < 16; i++){
			for (int k = 0; k < 10; k++){	// Herz an bis 15 (224 ms)
				LED(3, letter[0][k][1], letter[0][k][0], i);
			}
			PAUSE(14);
		}
		for (int i = 0; i < 11; i++){
			for (int k = 0; k < 10; k++){	// Herz aus bis 5 (275 ms)
				LED(3, letter[0][k][1], letter[0][k][0], 15 - i);
			}
			PAUSE(25);
		}
		for (int i = 5; i < 16; i++){
			for (int k = 0; k < 10; k++){	// Herz an bis 15 (154 ms)
				LED(3, letter[0][k][1], letter[0][k][0], i);
			}
			PAUSE(14);
		}
		for (int i = 0; i < 14; i++){
			for (int k = 0; k < 10; k++){	// Herz aus bis 2 (770 ms)
				LED(3, letter[0][k][1], letter[0][k][0], 15 - i);
			}
			PAUSE(55);
		}
	}
	for (int i = 14; i < 16; i++){
		for (int k = 0; k < 10; k++){	// Herz ganz aus
			LED(3, letter[0][k][1], letter[0][k][0], 15 - i);
		}
		PAUSE(55);
	}
	clean();
}

void CubeClass::flash(byte num, int pause) {
	PAUSE(500);
	for (int i = 0; i < num; i++){
		for (int k = 0; k < 4; k++) {
			for (int m = 0; m < 25; m++) {
				green[k][m] = 31;
			}
		}
		PAUSE(pause);
		for (int k = 0; k < 4; k++) {
			for (int m = 0; m < 25; m++) {
				green[k][m] = 0;
			}
		}
		PAUSE(pause);
	}
}

void CubeClass::box(byte rep, int pause) {
	int grosse;
	int runde;

	for (int i = 0; i < rep; i++) {
		grosse = 0;
		runde = 0;
		while (runde <= 3) {
			if (runde % 2 == 1)
				grosse = 1;
			else if (runde % 4 == 0)
				grosse = 0;
			else
				grosse = 2;

			for (int x = 1; x <= 5; x++) {
				for (int y = 1; y <= 5; y++) {
					for (int z = 1; z <= 5; z++) {
						int c = 0;
						c += abs(x - 3) == grosse ? 1 : (abs(x - 3) > grosse ? -1 : 0);
						c += abs(y - 3) == grosse ? 1 : (abs(y - 3) > grosse ? -1 : 0);
						c += abs(z - 3) == grosse ? 1 : (abs(z - 3) > grosse ? -1 : 0);
						if (c >= 2)
							LED(x, y, z, 15);
						else
							LED(x, y, z, 0);
					}
				}
			}
			PAUSE(pause);
			runde++;
		}
	}
	clean();
	LED(3, 3, 3, 15);
	PAUSE(pause);
	clean();
}

void CubeClass::vave(byte rep, int pause, int dir) {
	dir = dir / abs(dir);
	for (int x = 0; x < rep; x++) {
		for (int m = 0; m <= 9; m++) {
			for (int i = 1; i <= 5; i++) {
				for (int k = 1; k <= 5; k++) {
					LED(i, k, round(sin((i + m*dir + k)*PI / 5) * 2 + 3), 15); //calc z coordinate according to i and k with 3D sine function adjusted to diagonal cube size // m moves "field of vision" to achieve animation
				}
			}
			PAUSE(pause);
			clean();
		}
	}
}

void CubeClass::scan(byte version, int pause) {
	for (int i = 5; i > 0; i--) {
		for (int k = 1; k < 6; k++) {
			for (int m = 1; m < 6; m++) {
				revLED(k, m, i, 15, version);
			}
		}
		PAUSE(pause);
		clean();
	}
	for (int i = 2; i < 6; i++) {
		for (int k = 1; k < 6; k++) {
			for (int m = 1; m < 6; m++) {
				revLED(k, m, i, 15, version);
			}
		}
		PAUSE(pause);
		clean();
	}
}

void CubeClass::scanner(int pause) {
	scan(0, pause);
	scan(1, pause);
	scan(2, pause);
}

void CubeClass::loading(int pause) {
	for (int i = 0; i <= 2; i++) {    // mittiges Kreuz
		for (int k = 0; k <= 2; k++) {
			revLED(3, 3, 3 + i, 15, k); // 3,3,2 bis 3,3,4 mit 3 Hauptvertauschungen
			revLED(3, 3, 3 - i, 15, k);
		}
		PAUSE(pause);
	}
	for (int i = 1; i <= 2; i++) {
		for (int k = 0; k <= 5; k++) {
			revLED(1, 3, 3 + i, 15, k);
			revLED(1, 3, 3 - i, 15, k);
			revLED(5, 3, 3 + i, 15, k);
			revLED(5, 3, 3 - i, 15, k);
		}
		PAUSE(pause*1.5);
	}
	for (int i = 1; i <= 2; i++) {
		for (int k = 0; k <= 5; k++) {
			revLED(1, 1, 3 + i, 15, k);
			revLED(1, 1, 3 - i, 15, k);
			revLED(1, 5, 3 + i, 15, k);
			revLED(1, 5, 3 - i, 15, k);
			revLED(5, 5, 3 + i, 15, k);
			revLED(5, 5, 3 - i, 15, k);
		}
		PAUSE(pause * 2);
	}
	clean();
}