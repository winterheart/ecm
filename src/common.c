/***************************************************************************/
/*
** ECM - Encoder for ECM (Error Code Modeler) format.
** Version 1.0
** Copyright (C) 2002 Neill Corlett
** Copyright (c) 2020-2023 Azamat H. Hackimov
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

#include <stdint.h>
#include <stdio.h>
#include "unecm.h"

/* LUTs used for computing ECC/EDC */
uint8_t ecc_f_lut[256];
uint8_t ecc_b_lut[256];
uint32_t edc_lut[256];

/* Counters for analyze / encode / decode / total */
unsigned mycounter_analyze;
unsigned mycounter_encode;
unsigned mycounter_total;

/* Init routine */
void eccedc_init(void) {
  uint32_t i, j, edc;
  for (i = 0; i < 256; i++) {
    j = (i << 1) ^ (i & 0x80 ? 0x11D : 0);
    ecc_f_lut[i] = j;
    ecc_b_lut[i ^ j] = i;
    edc = i;
    for (j = 0; j < 8; j++)
      edc = (edc >> 1) ^ (edc & 1 ? 0xD8018001 : 0);
    edc_lut[i] = edc;
  }
}

/* Compute EDC for a block */
uint32_t edc_partial_computeblock(uint32_t edc, const uint8_t *src,
                                  uint16_t size) {
  while (size--)
    edc = (edc >> 8) ^ edc_lut[(edc ^ (*src++)) & 0xFF];
  return edc;
}

/* Compute ECC for a block (can do either P or Q) */
bool ecc_computeblock_encode(uint8_t *src, uint32_t major_count,
                             uint32_t minor_count, uint32_t major_mult,
                             uint32_t minor_inc, uint8_t *dest) {
  uint32_t size = major_count * minor_count;
  uint32_t major, minor;
  for (major = 0; major < major_count; major++) {
    uint32_t index = (major >> 1) * major_mult + (major & 1);
    uint8_t ecc_a = 0;
    uint8_t ecc_b = 0;
    for (minor = 0; minor < minor_count; minor++) {
      uint8_t temp = src[index];
      index += minor_inc;
      if (index >= size)
        index -= size;
      ecc_a ^= temp;
      ecc_b ^= temp;
      ecc_a = ecc_f_lut[ecc_a];
    }
    ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
    if (dest[major] != (ecc_a))
      return false;
    dest[major] = ecc_a;
    if (dest[major + major_count] != (ecc_a ^ ecc_b))
      return false;
    dest[major + major_count] = ecc_a ^ ecc_b;
  }
  return true;
}

/* Compute ECC for a block (can do either P or Q) */
void ecc_computeblock_decode(uint8_t *src, uint32_t major_count,
                             uint32_t minor_count, uint32_t major_mult,
                             uint32_t minor_inc, uint8_t *dest) {
  uint32_t size = major_count * minor_count;
  uint32_t major, minor;
  for (major = 0; major < major_count; major++) {
    uint32_t index = (major >> 1) * major_mult + (major & 1);
    uint8_t ecc_a = 0;
    uint8_t ecc_b = 0;
    for (minor = 0; minor < minor_count; minor++) {
      uint8_t temp = src[index];
      index += minor_inc;
      if (index >= size)
        index -= size;
      ecc_a ^= temp;
      ecc_b ^= temp;
      ecc_a = ecc_f_lut[ecc_a];
    }
    ecc_a = ecc_b_lut[ecc_f_lut[ecc_a] ^ ecc_b];
    dest[major] = ecc_a;
    dest[major + major_count] = ecc_a ^ ecc_b;
  }
}

/* Generate ECC P and Q codes for a block */
int ecc_generate_encode(uint8_t *sector, bool zeroaddress, uint8_t *dest) {
  int r;
  uint8_t address[4], i;
  /* Save the address and zero it out */
  if (zeroaddress)
    for (i = 0; i < 4; i++) {
      address[i] = sector[12 + i];
      sector[12 + i] = 0;
    }
  /* Compute ECC P code */
  if (!(ecc_computeblock_encode(sector + 0xC, 86, 24, 2, 86,
                                dest + 0x81C - 0x81C))) {
    if (zeroaddress)
      for (i = 0; i < 4; i++)
        sector[12 + i] = address[i];
    return 0;
  }
  /* Compute ECC Q code */
  r = ecc_computeblock_encode(sector + 0xC, 52, 43, 86, 88,
                              dest + 0x8C8 - 0x81C);
  /* Restore the address */
  if (zeroaddress)
    for (i = 0; i < 4; i++)
      sector[12 + i] = address[i];
  return r;
}

/* Generate ECC P and Q codes for a block */
void ecc_generate_decode(uint8_t *sector, bool zeroaddress) {
  uint8_t address[4], i;
  /* Save the address and zero it out */
  if (zeroaddress)
    for (i = 0; i < 4; i++) {
      address[i] = sector[12 + i];
      sector[12 + i] = 0;
    }
  /* Compute ECC P code */
  ecc_computeblock_decode(sector + 0xC, 86, 24, 2, 86, sector + 0x81C);
  /* Compute ECC Q code */
  ecc_computeblock_decode(sector + 0xC, 52, 43, 86, 88, sector + 0x8C8);
  /* Restore the address */
  if (zeroaddress)
    for (i = 0; i < 4; i++)
      sector[12 + i] = address[i];
}

/* Reset all counters */
void resetcounter(unsigned total) {
  mycounter_analyze = 0;
  mycounter_encode = 0;
  mycounter_total = total;
}

/* Set counters on analyze */
void setcounter_analyze(unsigned n) {
  if ((n >> 20) != (mycounter_analyze >> 20)) {
    unsigned a = (n + 64) / 128;
    unsigned e = (mycounter_encode + 64) / 128;
    unsigned d = (mycounter_total + 64) / 128;
    if (!d)
      d = 1;
    fprintf(stderr, "Analyzing (%02d%%) Encoding (%02d%%)\r", (100 * a) / d,
            (100 * e) / d);
  }
  mycounter_analyze = n;
}

/* Set counters on encode */
void setcounter_encode(unsigned n) {
  if ((n >> 20) != (mycounter_encode >> 20)) {
    unsigned a = (mycounter_analyze + 64) / 128;
    unsigned e = (n + 64) / 128;
    unsigned d = (mycounter_total + 64) / 128;
    if (!d)
      d = 1;
    fprintf(stderr, "Analyzing (%02d%%) Encoding (%02d%%)\r", (100 * a) / d,
            (100 * e) / d);
  }
  mycounter_encode = n;
}

/* Set counters on decode */
void setcounter_decode(unsigned n) {
  if ((n >> 20) != (mycounter_analyze >> 20)) {
    unsigned a = (n + 64) / 128;
    unsigned d = (mycounter_total + 64) / 128;
    if (!d)
      d = 1;
    fprintf(stderr, "Decoding (%02d%%)\r", (100 * a) / d);
  }
  mycounter_analyze = n;
}
