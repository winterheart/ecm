#ifndef ECM_UNECM_H
#define ECM_UNECM_H

#include <stdbool.h>
#include <stdint.h>

#if defined(WIN32) || defined(WIN64)
#define strcasecmp _stricmp
#endif

/* LUTs used for computing ECC/EDC */
uint8_t ecc_f_lut[256];
uint8_t ecc_b_lut[256];
uint32_t edc_lut[256];

/* Counters for analyze / encode / decode / total */
unsigned mycounter_analyze;
unsigned mycounter_encode;
unsigned mycounter_total;

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

/* Reset all counters */
void resetcounter(unsigned total);

/* Set counters on analyze */
void setcounter_analyze(unsigned n);

/* Set counters on encode */
void setcounter_encode(unsigned n);

/* Set counters on decode */
void setcounter_decode(unsigned n);

#endif //ECM_UNECM_H
