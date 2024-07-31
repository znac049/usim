//
//
//	memory.c
//
//	(C) R.P.Bellis 2021
//
//

#include <cstdio>
#include <cstdlib>
#include <strings.h>
#include <string.h>
#include "memory.h"

Word fread_word(FILE *fp) {
  int c1 = fgetc(fp);
  int c2 = fgetc(fp);

  if ((c1 == EOF) || (c2 == EOF)) {
    return -1;
  }

  return (c1<<8) | c2;
}

Byte fread_hex_byte(FILE *fp)
{
	char			str[3];
	long			l;

	str[0] = fgetc(fp);
	str[1] = fgetc(fp);
	str[2] = '\0';

	l = strtol(str, NULL, 16);
	return (Byte)(l & 0xff);
}

Word fread_hex_word(FILE *fp)
{
	Word		ret;

	ret = fread_hex_byte(fp);
	ret <<= 8;
	ret |= fread_hex_byte(fp);

	return ret;
}

void ROM::load(const char *filename, Word base)
{
	char *c = strrchr((char *)filename, '.');

	if (c == NULL) {
		FILE *fp = fopen(filename, "r");
		Byte c;

		if (!fp) {
			perror("filename");
			exit(EXIT_FAILURE);
		}

		c = fgetc(fp);
		fclose(fp);

		if (c == ':') {	// Assume Intel Hex		
			load_intelhex(filename, base);
		}
		else {
			fprintf(stderr, "Can't determine file format of \"%s\"\n", filename);
			exit(EXIT_FAILURE);
		}
	}
	else {
		if (strcasecmp(c, ".ihex") == 0) {
			load_intelhex(filename, base);
		}
		else if (strcasecmp(c, ".decb") == 0) {
			load_decb(filename, base);
		}
		else {
			fprintf(stderr, "Can't determine file format of \"%s\"\n", filename);
			exit(EXIT_FAILURE);
		}
	}
}

void ROM::load_intelhex(const char *filename, Word base)
{
	FILE		*fp;
	int		done = 0;

	fp = fopen(filename, "r");
	if (!fp) {
		perror("filename");
		exit(EXIT_FAILURE);
	}

	while (!done) {
		Byte		n, t;
		Word		addr;
		Byte		b;

		(void)fgetc(fp);
		n = fread_hex_byte(fp);
		addr = fread_hex_word(fp);
		t = fread_hex_byte(fp);
		if (t == 0x00) {
			while (n--) {
				b = fread_hex_byte(fp);
				if ((addr >= base) && (addr < ((DWord)base + size))) {
					memory[addr - base] = b;
				}
				++addr;
			}
		} else if (t == 0x01) {
			done = 1;
		}
		// Read and discard checksum byte
		(void)fread_hex_byte(fp);
		if (fgetc(fp) == '\r') (void)fgetc(fp);
	}
}

void ROM::load_decb(const char *filename, Word base)
{
	FILE		*fp;
	int		done = 0;

	fp = fopen(filename, "r");
	if (!fp) {
		perror("filename");
		exit(EXIT_FAILURE);
	}

	while (!done) {
		Byte hdr;
		Word len;
		Word addr;
		Byte b;

		hdr = fgetc(fp);

		if (hdr == 0) {
			len = fread_word(fp);
			addr = fread_word(fp);

			for (int i=0; i<len; i++) {
				b = fgetc(fp);
				if ((addr >= base) && (addr < ((DWord)base + size))) {
					memory[addr - base] = b;
				}
				addr++;
			}
		}
		else if (hdr == 0xff) {
			// End of data marker
			return;
		}
		else {
			// shouldn't happen
			;
		}
	}
}
