#ifndef ATTRACT_H
#define ATTRACT_H

const char * ATTRACT_MODES[] = {
	// Moving white peak
"\x63\x21\x0a\x67\x40\x73\x0a\x73\x21\x2a\x54\x5f\x2c\x30\x2e\x31\x0a\x73\x22\x2b\x76\x21\x2c\x50\x5f\x0a\x73\x23\x74\x76\x22\x0a\x73\x24\x2a\x76\x23\x2c\x76\x23\x0a\x73\x25\x2a\x76\x23\x2c\x76\x24\x0a\x73\x26\x2a\x76\x25\x2c\x76\x25\x0a\x73\x27\x2a\x54\x5f\x2c\x30\x2e\x30\x33\x0a\x73\x28\x2d\x31\x2c\x76\x26\x0a\x73\x29\x6c\x30\x2e\x32\x2c\x30\x2e\x35\x2c\x76\x26\x0a\x73\x2a\x5d\x76\x27\x2c\x76\x28\x2c\x76\x29\x0a\x63\x2b\x0a",

	// Rainbow cycle
"\x63\x21\x0a\x67\x41\x5a\x0a\x73\x21\x2a\x54\x5f\x2c\x30\x2e\x33\x0a\x73\x22\x2b\x76\x21\x2c\x50\x5f\x0a\x73\x23\x5d\x76\x22\x2c\x31\x2c\x31\x0a\x63\x24\x0a",

// Red
"\x63\x21\x0a\x67\x40\x73\x0a\x73\x21\x5b\x30\x2e\x33\x0a\x63\x22\x0a",

// Blue waves
/*
led = rgb(0, 0, sin01(T + P))
*/
"\x63\x21\x0a\x67\x40\x68\x0a\x73\x21\x2b\x54\x5f\x2c\x50\x5f\x0a\x73\x22\x73\x76\x21\x0a\x73\x23\x5b\x30\x2c\x30\x2c\x76\x22\x0a\x63\x24\x0a",
};

const uint16_t ATTRACT_MODES_LEN = sizeof(ATTRACT_MODES) / sizeof(*ATTRACT_MODES);

#endif
