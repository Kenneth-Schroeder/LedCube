#ifndef Cube_h
#define Cube_h	// verhindert Mehrfacheinbindung
#include "Arduino.h"
#include "types.h"

class CubeClass
{
	private:
		CubeClass();
		static byte loopCount;                   // Variable für Schleifen
		static byte anode[5];                   // Anoden Bits
		static byte green[4][25];               // 4 Bits für jede LED im Würfel zur Helligkeitsregulierung
		static byte output[4];                  // Transfer Bytes

		static byte level;
		static byte BAM_Bit, BAM_Counter;		// Bitangle Modulation Zähler
		static byte whichbyte;
		static byte whichbit;
		static byte pos;
	public:
		static void SPI_Init();
		static void uebertragung();
		static void LED(byte, byte, byte, byte);
		static void revLED(byte, byte, byte, byte, byte);
		static void PAUSE(int);
		static void clean();
		static void heart();
		static void edgeFill(int);
		static void edgeClear(int);
		static void pulse();
		static void crossDemo();
		static void splitter(int);
		static void wallsBeta();
		static void spirale(int);
		static void _print(String, int);
		static void heartbeat(byte);
		static void flash(byte, int);
		static void box(byte, int);
		static void vave(byte, int, int);
		static void scan(byte, int);
		static void scanner(int);
		static void loading(int);
};

extern CubeClass Cube;
#endif