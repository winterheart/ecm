/***************************************************************************/
/*
** ECM - Encoder for ECM (Error Code Modeler) format.
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

/*
** sector types:
** 00 - literal bytes
** 01 - 2352 mode 1         predict sync, mode, reserved, edc, ecc
** 02 - 2336 mode 2 form 1  predict redundant flags, edc, ecc
** 03 - 2336 mode 2 form 2  predict redundant flags, edc
*/

int check_type(unsigned char *sector, bool canbetype1) {
  bool canbetype2 = true;
  bool canbetype3 = true;
  uint32_t myedc;
  /* Check for mode 1 */
  if (canbetype1) {
    if ((sector[0x00] != 0x00) || (sector[0x01] != 0xFF) ||
        (sector[0x02] != 0xFF) || (sector[0x03] != 0xFF) ||
        (sector[0x04] != 0xFF) || (sector[0x05] != 0xFF) ||
        (sector[0x06] != 0xFF) || (sector[0x07] != 0xFF) ||
        (sector[0x08] != 0xFF) || (sector[0x09] != 0xFF) ||
        (sector[0x0A] != 0xFF) || (sector[0x0B] != 0x00) ||
        (sector[0x0F] != 0x01) || (sector[0x814] != 0x00) ||
        (sector[0x815] != 0x00) || (sector[0x816] != 0x00) ||
        (sector[0x817] != 0x00) || (sector[0x818] != 0x00) ||
        (sector[0x819] != 0x00) || (sector[0x81A] != 0x00) ||
        (sector[0x81B] != 0x00)) {
      canbetype1 = false;
    }
  }
  /* Check for mode 2 */
  if ((sector[0x0] != sector[0x4]) || (sector[0x1] != sector[0x5]) ||
      (sector[0x2] != sector[0x6]) || (sector[0x3] != sector[0x7])) {
    canbetype2 = false;
    canbetype3 = false;
    if (!canbetype1)
      return 0;
  }

  /* Check EDC */
  myedc = edc_partial_computeblock(0, sector, 0x808);
  if (canbetype2)
    if ((sector[0x808] != ((myedc >> 0) & 0xFF)) ||
        (sector[0x809] != ((myedc >> 8) & 0xFF)) ||
        (sector[0x80A] != ((myedc >> 16) & 0xFF)) ||
        (sector[0x80B] != ((myedc >> 24) & 0xFF))) {
      canbetype2 = false;
    }
  myedc = edc_partial_computeblock(myedc, sector + 0x808, 8);
  if (canbetype1)
    if ((sector[0x810] != ((myedc >> 0) & 0xFF)) ||
        (sector[0x811] != ((myedc >> 8) & 0xFF)) ||
        (sector[0x812] != ((myedc >> 16) & 0xFF)) ||
        (sector[0x813] != ((myedc >> 24) & 0xFF))) {
      canbetype1 = false;
    }
  myedc = edc_partial_computeblock(myedc, sector + 0x810, 0x10C);
  if (canbetype3)
    if ((sector[0x91C] != ((myedc >> 0) & 0xFF)) ||
        (sector[0x91D] != ((myedc >> 8) & 0xFF)) ||
        (sector[0x91E] != ((myedc >> 16) & 0xFF)) ||
        (sector[0x91F] != ((myedc >> 24) & 0xFF))) {
      canbetype3 = false;
    }
  /* Check ECC */
  if (canbetype1) {
    if (!(ecc_generate_encode(sector, false, sector + 0x81C))) {
      canbetype1 = false;
    }
  }
  if (canbetype2) {
    if (!(ecc_generate_encode(sector - 0x10, true, sector + 0x80C))) {
      canbetype2 = false;
    }
  }
  if (canbetype1)
    return 1;
  if (canbetype2)
    return 2;
  if (canbetype3)
    return 3;
  return 0;
}

/***************************************************************************/
/*
** Encode a type/count combo
*/
void write_type_count(FILE *out, unsigned type, unsigned count) {
  count--;
  fputc(((count >= 32) << 7) | ((count & 31) << 2) | type, out);
  count >>= 5;
  while (count) {
    fputc(((count >= 128) << 7) | (count & 127), out);
    count >>= 7;
  }
}


/***************************************************************************/
/*
** Encode a run of sectors/literals of the same type
*/
unsigned in_flush(unsigned edc, unsigned type, unsigned count, FILE *in,
                  FILE *out) {
  unsigned char buf[SECTOR_1_SIZE];
  write_type_count(out, type, count);
  if (!type) {
    while (count) {
      unsigned b = count;
      if (b > SECTOR_1_SIZE)
        b = SECTOR_1_SIZE;
      fread(buf, 1, b, in);
      edc = edc_partial_computeblock(edc, buf, b);
      fwrite(buf, 1, b, out);
      count -= b;
      setcounter_encode(ftell(in));
    }
    return edc;
  }
  while (count--) {
    switch (type) {
    case 1:
      fread(buf, 1, SECTOR_1_SIZE, in);
      edc = edc_partial_computeblock(edc, buf, SECTOR_1_SIZE);
      fwrite(buf + 0x00C, 1, 0x003, out);
      fwrite(buf + 0x010, 1, 0x800, out);
      setcounter_encode(ftell(in));
      break;
    case 2:
      fread(buf, 1, SECTOR_2_SIZE, in);
      edc = edc_partial_computeblock(edc, buf, SECTOR_2_SIZE);
      fwrite(buf + 0x004, 1, 0x804, out);
      setcounter_encode(ftell(in));
      break;
    case 3:
      fread(buf, 1, SECTOR_2_SIZE, in);
      edc = edc_partial_computeblock(edc, buf, SECTOR_2_SIZE);
      fwrite(buf + 0x004, 1, 0x918, out);
      setcounter_encode(ftell(in));
      break;
    }
  }
  return edc;
}

/***************************************************************************/

int ecmify(FILE *in, FILE *out) {
  unsigned char inputqueue[1048576 + 4];
  unsigned inedc = 0;
  int curtype = -1;
  int curtypecount = 0;
  int curtype_in_start = 0;
  int detecttype;
  int incheckpos = 0;
  int inbufferpos = 0;
  int intotallength;
  int inqueuestart = 0;
  int dataavail = 0;
  int typetally[4];
  fseek(in, 0, SEEK_END);
  intotallength = ftell(in);
  resetcounter(intotallength);
  typetally[0] = 0;
  typetally[1] = 0;
  typetally[2] = 0;
  typetally[3] = 0;
  /* Magic identifier */
  fputc('E', out);
  fputc('C', out);
  fputc('M', out);
  fputc(0x00, out);
  for (;;) {
    if ((dataavail < SECTOR_1_SIZE) && (dataavail < (intotallength - inbufferpos))) {
      int willread = intotallength - inbufferpos;
      if (willread > ((sizeof(inputqueue) - 4) - dataavail))
        willread = (sizeof(inputqueue) - 4) - dataavail;
      if (inqueuestart) {
        memmove(inputqueue + 4, inputqueue + 4 + inqueuestart, dataavail);
        inqueuestart = 0;
      }
      if (willread) {
        setcounter_analyze(inbufferpos);
        fseek(in, inbufferpos, SEEK_SET);
        fread(inputqueue + 4 + dataavail, 1, willread, in);
        inbufferpos += willread;
        dataavail += willread;
      }
    }
    if (dataavail <= 0)
      break;
    if (dataavail < SECTOR_2_SIZE) {
      detecttype = 0;
    } else {
      detecttype = check_type(inputqueue + 4 + inqueuestart, false);
    }
    if (detecttype != curtype) {
      if (curtypecount) {
        fseek(in, curtype_in_start, SEEK_SET);
        typetally[curtype] += curtypecount;
        inedc = in_flush(inedc, curtype, curtypecount, in, out);
      }
      curtype = detecttype;
      curtype_in_start = incheckpos;
      curtypecount = 1;
    } else {
      curtypecount++;
    }
    switch (curtype) {
    case 0:
      incheckpos += 1;
      inqueuestart += 1;
      dataavail -= 1;
      break;
    case 1:
      incheckpos += SECTOR_1_SIZE;
      inqueuestart += SECTOR_1_SIZE;
      dataavail -= SECTOR_1_SIZE;
      break;
    case 2:
    case 3:
      incheckpos += SECTOR_2_SIZE;
      inqueuestart += SECTOR_2_SIZE;
      dataavail -= SECTOR_2_SIZE;
      break;
    }
  }
  if (curtypecount) {
    fseek(in, curtype_in_start, SEEK_SET);
    typetally[curtype] += curtypecount;
    inedc = in_flush(inedc, curtype, curtypecount, in, out);
  }
  /* End-of-records indicator */
  write_type_count(out, 0, 0);
  /* Input file EDC */
  fputc((inedc >> 0) & 0xFF, out);
  fputc((inedc >> 8) & 0xFF, out);
  fputc((inedc >> 16) & 0xFF, out);
  fputc((inedc >> 24) & 0xFF, out);
  /* Show report */
  fprintf(stderr, "Literal bytes........... %10d\n", typetally[0]);
  fprintf(stderr, "Mode 1 sectors.......... %10d\n", typetally[1]);
  fprintf(stderr, "Mode 2 form 1 sectors... %10d\n", typetally[2]);
  fprintf(stderr, "Mode 2 form 2 sectors... %10d\n", typetally[3]);
  fprintf(stderr, "Encoded %d bytes -> %ld bytes\n", intotallength, ftell(out));
  fprintf(stderr, "Done.\n");
  return 0;
}

/***************************************************************************/

int main(int argc, char **argv) {
  FILE *fin, *fout;
  char *infilename;
  char *outfilename;

  fprintf(stderr, "ECM - Encoder for Error Code Modeler format v1.0\n"
                  "Copyright (C) 2002 Neill Corlett\n\n");

  /*
  ** Initialize the ECC/EDC tables
  */
  eccedc_init();
  /*
  ** Check command line
  */
  if ((argc != 2) && (argc != 3)) {
    fprintf(stderr, "usage: %s cdimagefile [ecmfile]\n", argv[0]);
    return 1;
  }
  infilename = argv[1];
  /*
  ** Figure out what the output filename should be
  */
  if (argc == 3) {
    outfilename = argv[2];
  } else {
    outfilename = malloc(strlen(infilename) + 5);
    if (!outfilename)
      abort();
    sprintf(outfilename, "%s.ecm", infilename);
  }
  fprintf(stderr, "Encoding %s to %s.\n", infilename, outfilename);
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
  ** Encode
  */
  ecmify(fin, fout);
  /*
  ** Close everything
  */
  fclose(fout);
  fclose(fin);
  return 0;
}
