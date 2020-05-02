/***************************************************************************/
/*
** UNECM - Decoder for ECM (Error Code Modeler) format.
** Version 1.0
** Copyright (C) 2002 Neill Corlett
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/***************************************************************************/
/*
** Portability notes:
**
** - Assumes a 32-bit or higher integer size
** - No assumptions about byte order
** - No assumptions about struct packing
** - No unaligned memory access
*/
/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unecm.h"


/***************************************************************************/

void edc_computeblock(const uint8_t *src, uint16_t size, uint8_t *dest) {
  uint32_t edc = edc_partial_computeblock(0, src, size);
  dest[0] = (edc >> 0) & 0xFF;
  dest[1] = (edc >> 8) & 0xFF;
  dest[2] = (edc >> 16) & 0xFF;
  dest[3] = (edc >> 24) & 0xFF;
}

/***************************************************************************/
/*
** Generate ECC/EDC information for a sector (must be 2352 = 0x930 bytes)
** Returns 0 on success
*/
void eccedc_generate(uint8_t *sector, int type) {
  uint32_t i;
  switch (type) {
  case 1: /* Mode 1 */
    /* Compute EDC */
    edc_computeblock(sector + 0x00, 0x810, sector + 0x810);
    /* Write out zero bytes */
    for (i = 0; i < 8; i++)
      sector[0x814 + i] = 0;
    /* Generate ECC P/Q codes */
    ecc_generate_decode(sector, false);
    break;
  case 2: /* Mode 2 form 1 */
    /* Compute EDC */
    edc_computeblock(sector + 0x10, 0x808, sector + 0x818);
    /* Generate ECC P/Q codes */
    ecc_generate_decode(sector, true);
    break;
  case 3: /* Mode 2 form 2 */
    /* Compute EDC */
    edc_computeblock(sector + 0x10, 0x91C, sector + 0x92C);
    break;
  }
}

int unecmify(FILE *in, FILE *out) {
  unsigned checkedc = 0;
  unsigned char sector[SECTOR_1_SIZE];
  unsigned type;
  unsigned num;
  fseek(in, 0, SEEK_END);
  resetcounter(ftell(in));
  fseek(in, 0, SEEK_SET);
  if ((fgetc(in) != 'E') || (fgetc(in) != 'C') || (fgetc(in) != 'M') ||
      (fgetc(in) != 0x00)) {
    fprintf(stderr, "Header not found!\n");
    goto corrupt;
  }
  for (;;) {
    int c = fgetc(in);
    int bits = 5;
    if (c == EOF)
      goto uneof;
    type = c & 3;
    num = (c >> 2) & 0x1F;
    while (c & 0x80) {
      c = fgetc(in);
      if (c == EOF)
        goto uneof;
      num |= ((unsigned)(c & 0x7F)) << bits;
      bits += 7;
    }
    if (num == 0xFFFFFFFF)
      break;
    num++;
    if (num >= 0x80000000)
      goto corrupt;
    if (!type) {
      while (num) {
        int b = num;
        if (b > SECTOR_1_SIZE)
          b = SECTOR_1_SIZE;
        if (fread(sector, 1, b, in) != b)
          goto uneof;
        checkedc = edc_partial_computeblock(checkedc, sector, b);
        fwrite(sector, 1, b, out);
        num -= b;
        setcounter_decode(ftell(in));
      }
    } else {
      while (num--) {
        memset(sector, 0, sizeof(sector));
        memset(sector + 1, 0xFF, 10);
        switch (type) {
        case 1:
          sector[0x0F] = 0x01;
          if (fread(sector + 0x00C, 1, 0x003, in) != 0x003)
            goto uneof;
          if (fread(sector + 0x010, 1, 0x800, in) != 0x800)
            goto uneof;
          eccedc_generate(sector, 1);
          checkedc = edc_partial_computeblock(checkedc, sector, SECTOR_1_SIZE);
          fwrite(sector, SECTOR_1_SIZE, 1, out);
          setcounter_decode(ftell(in));
          break;
        case 2:
          sector[0x0F] = 0x02;
          if (fread(sector + 0x014, 1, 0x804, in) != 0x804)
            goto uneof;
          sector[0x10] = sector[0x14];
          sector[0x11] = sector[0x15];
          sector[0x12] = sector[0x16];
          sector[0x13] = sector[0x17];
          eccedc_generate(sector, 2);
          checkedc = edc_partial_computeblock(checkedc, sector + 0x10, SECTOR_2_SIZE);
          fwrite(sector + 0x10, SECTOR_2_SIZE, 1, out);
          setcounter_decode(ftell(in));
          break;
        case 3:
          sector[0x0F] = 0x02;
          if (fread(sector + 0x014, 1, 0x918, in) != 0x918)
            goto uneof;
          sector[0x10] = sector[0x14];
          sector[0x11] = sector[0x15];
          sector[0x12] = sector[0x16];
          sector[0x13] = sector[0x17];
          eccedc_generate(sector, 3);
          checkedc = edc_partial_computeblock(checkedc, sector + 0x10, SECTOR_2_SIZE);
          fwrite(sector + 0x10, SECTOR_2_SIZE, 1, out);
          setcounter_decode(ftell(in));
          break;
        }
      }
    }
  }
  if (fread(sector, 1, 4, in) != 4)
    goto uneof;
  fprintf(stderr, "Decoded %ld bytes -> %ld bytes\n", ftell(in), ftell(out));
  if ((sector[0] != ((checkedc >> 0) & 0xFF)) ||
      (sector[1] != ((checkedc >> 8) & 0xFF)) ||
      (sector[2] != ((checkedc >> 16) & 0xFF)) ||
      (sector[3] != ((checkedc >> 24) & 0xFF))) {
    fprintf(stderr, "EDC error (%08X, should be %02X%02X%02X%02X)\n", checkedc,
            sector[3], sector[2], sector[1], sector[0]);
    goto corrupt;
  }
  fprintf(stderr, "Done; file is OK\n");
  return 0;
uneof:
  fprintf(stderr, "Unexpected EOF!\n");
corrupt:
  fprintf(stderr, "Corrupt ECM file!\n");
  return 1;
}

/***************************************************************************/

int main(int argc, char **argv) {
  FILE *fin, *fout;
  char *infilename;
  char *outfilename;
  char *cuefilename;
  char createcue = 0;

  fprintf(stderr, "UNECM - Decoder for Error Code Modeler format v1.0\n"
                  "Copyright (C) 2002 Neill Corlett\n\n");
  /*
  ** Initialize the ECC/EDC tables
  */
  eccedc_init();
  /*
  ** Check command line
  */
  if ((argc != 2) && (argc != 3) && (argc != 4)) {
    fprintf(stderr, "usage: %s [--cue] ecmfile [outputfile]\n", argv[0]);
    return 1;
  }
  /*
  ** Get cur generation status
  */
  if (!strcasecmp(argv[1], "--cue")) {
    createcue = 1;
  }
  /*
  ** Verify that the input filename is valid
  */
  infilename = argv[1 + createcue];
  if (strlen(infilename) < 5) {
    fprintf(stderr, "filename '%s' is too short\n", infilename);
    return 1;
  }
  if (strcasecmp(infilename + strlen(infilename) - 4, ".ecm")) {
    fprintf(stderr, "filename must end in .ecm\n");
    return 1;
  }
  /*
  ** Figure out what the output filename should be
  */
  if (argc == 3 + createcue) {
    outfilename = argv[2 + createcue];
  } else {
    outfilename = malloc(strlen(infilename) - 3);
    if (!outfilename)
      abort();
    memcpy(outfilename, infilename, strlen(infilename) - 4);
    outfilename[strlen(infilename) - 4] = 0;
  }
  fprintf(stderr, "Decoding %s to %s.\n", infilename, outfilename);
  /*
  ** Open both files
  */
  fin = fopen(infilename, "rb");
  if (!fin) {
    perror(infilename);
    return 1;
  }
  fout = fopen(outfilename, "wb");
  if (!fout) {
    perror(outfilename);
    fclose(fin);
    return 1;
  }
  /*
  ** Decode
  */
  unecmify(fin, fout);
  /*
  ** Close everything
  */
  fclose(fout);
  fclose(fin);
  /*
  ** Write cue file
  */
  if (createcue) {
    cuefilename = malloc(strlen(outfilename));
    if (!cuefilename)
      abort();
    memcpy(cuefilename, outfilename, strlen(outfilename));
    memcpy(cuefilename + strlen(outfilename) - 3, "cue", 3);
    fout = fopen(cuefilename, "wt");
    if (!fout) {
      perror(cuefilename);
      fclose(fout);
      return 1;
    }
    fwrite("FILE \"", 1, 6, fout);
    fwrite(outfilename, 1, strlen(outfilename), fout);
    fwrite("\" BINARY\n  TRACK 01 MODE2/2352\n    INDEX 01 00:00:00\n", 1, 53,
           fout);
    fclose(fout);
  }
  return 0;
}
