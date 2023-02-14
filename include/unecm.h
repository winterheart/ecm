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

#ifndef ECM_UNECM_H
#define ECM_UNECM_H

#include <stdbool.h>
#include <stdint.h>

#if defined(WIN32) || defined(WIN64)
#define strcasecmp _stricmp
#endif

// Sector 1 (0x930)
#define SECTOR_1_SIZE 2352
// Can be sector 2 and 3 (0x920)
#define SECTOR_2_SIZE 2336

/* Init routine */
void eccedc_init(void);

/* Compute EDC for a block */
uint32_t edc_partial_computeblock(uint32_t edc, const uint8_t *src,
                                  uint16_t size);

/* Compute ECC for a block (can do either P or Q) */
bool ecc_computeblock_encode(uint8_t *src, uint32_t major_count,
                             uint32_t minor_count, uint32_t major_mult,
                             uint32_t minor_inc, uint8_t *dest);

void ecc_computeblock_decode(uint8_t *src, uint32_t major_count,
                             uint32_t minor_count, uint32_t major_mult,
                             uint32_t minor_inc, uint8_t *dest);

/* Generate ECC P and Q codes for a block */
int ecc_generate_encode(uint8_t *sector, bool zeroaddress, uint8_t *dest);

/* Generate ECC P and Q codes for a block */
void ecc_generate_decode(uint8_t *sector, bool zeroaddress);


/* Reset all counters */
void resetcounter(unsigned total);

/* Set counters on analyze */
void setcounter_analyze(unsigned n);

/* Set counters on encode */
void setcounter_encode(unsigned n);

/* Set counters on decode */
void setcounter_decode(unsigned n);

#endif //ECM_UNECM_H
