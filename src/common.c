/***************************************************************************/
/*
** ECM - Encoder for ECM (Error Code Modeler) format.
** Version 1.0
** Copyright (C) 2002 Neill Corlett
** Copyright (c) 2020 Azamat H. Hackimov
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

#include "unecm.h"

/* Init routine */
void eccedc_init(void) {
  ecc_uint32 i, j, edc;
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
ecc_uint32 edc_partial_computeblock(ecc_uint32 edc, const ecc_uint8 *src,
                                    ecc_uint16 size) {
  while (size--)
    edc = (edc >> 8) ^ edc_lut[(edc ^ (*src++)) & 0xFF];
  return edc;
}
