#ifndef ECM_UNECM_H
#define ECM_UNECM_H

#include <stdbool.h>
#include <stdint.h>

#if defined(WIN32) || defined(WIN64)
#define strcasecmp _stricmp
#endif

/* Data types */
#define ecc_uint8 uint8_t
#define ecc_uint16 uint16_t
#define ecc_uint32 uint32_t

/* LUTs used for computing ECC/EDC */
ecc_uint8 ecc_f_lut[256];
ecc_uint8 ecc_b_lut[256];
ecc_uint32 edc_lut[256];

/* Counters for analyze / encode / decode / total */
unsigned mycounter_analyze;
unsigned mycounter_encode;
unsigned mycounter_total;

/* Init routine */
void eccedc_init(void);

/* Compute EDC for a block */
ecc_uint32 edc_partial_computeblock(ecc_uint32 edc, const ecc_uint8 *src,
                                    ecc_uint16 size);

/* Compute ECC for a block (can do either P or Q) */
bool ecc_computeblock_encode(ecc_uint8 *src, ecc_uint32 major_count,
                      ecc_uint32 minor_count, ecc_uint32 major_mult,
                      ecc_uint32 minor_inc, ecc_uint8 *dest);

void ecc_computeblock_decode(ecc_uint8 *src, ecc_uint32 major_count,
                             ecc_uint32 minor_count, ecc_uint32 major_mult,
                             ecc_uint32 minor_inc, ecc_uint8 *dest);

/* Reset all counters */
void resetcounter(unsigned total);

/* Set counters on analyze */
void setcounter_analyze(unsigned n);

/* Set counters on encode */
void setcounter_encode(unsigned n);

/* Set counters on decode */
void setcounter_decode(unsigned n);

#endif //ECM_UNECM_H
