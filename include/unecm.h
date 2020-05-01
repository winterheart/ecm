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

/* Compute ECC for a block (can do either P or Q) */
void ecc_computeblock_decode(ecc_uint8 *src, ecc_uint32 major_count,
                             ecc_uint32 minor_count, ecc_uint32 major_mult,
                             ecc_uint32 minor_inc, ecc_uint8 *dest);


#endif //ECM_UNECM_H
