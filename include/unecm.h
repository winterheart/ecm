#ifndef ECM_UNECM_H
#define ECM_UNECM_H

#if defined(WIN32) || defined(WIN64)
#define strcasecmp _stricmp
#endif

/* Data types */
#define ecc_uint8 unsigned char
#define ecc_uint16 unsigned short
#define ecc_uint32 unsigned

/* LUTs used for computing ECC/EDC */
ecc_uint8 ecc_f_lut[256];
ecc_uint8 ecc_b_lut[256];
ecc_uint32 edc_lut[256];

/* Init routine */
void eccedc_init(void);

#endif //ECM_UNECM_H
