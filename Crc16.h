/*                            =======================
================================ C/C++ HEADER FILE =================================
                              =======================                          *//**
\file Crc16.h
\description Crc16 functions
\copyright (c) Endress+Hauser AG
*//*==================================================================================*/
/*
CHANGE HISTORY:
---------------
Date       Author Description
28.08.2007 HM    v1.0 Released
*/
#ifndef CRC16H
#define CRC16H
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus
/*------------------------------------------------------------------------------------*/
/* INCLUDES */
/*------------------------------------------------------------------------------------*/
//lint_comment:
//OBE: The following include files are protected against multiple inclusion
//lint -save -e537
//#include "GlobalTypes.h"   
//lint -restore
	/*
	typedef uint16 uint16_t;
	typedef uint32 uint32_t;
	typedef uint8  uint8_t;*/

	typedef uint16_t uint16;
	typedef uint32_t uint32;
	typedef uint8_t  uint8;
/*------------------------------------------------------------------------------------*/
/* DEFINITIONS AND MACROS */
/*------------------------------------------------------------------------------------*/
extern const uint16 CRC16_DEFAULT_INITIAL_VALUE;

/*------------------------------------------------------------------------------------*/
/* TYPEDEFS, CLASSES AND STRUCTURES */
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
/* GLOBAL FUNCTIONS */
/*------------------------------------------------------------------------------------*/
uint16 g_UpdateCrc(uint16 u16_Crc, uint8 u8_Databyte);
uint16 g_CalcCrcSum(uint16 u16_Crc, uint32 u32_Len, const void * pv_Buf);
uint16 g_UpdateCrcSum(const uint16 u16_oldCrc, const void * const pv_newData, const void * const pv_oldData, 
                      const uint16 u16_size, const uint16 u16_remainingBytes);
uint16 g_SetCRCInitValue(uint16 u16_start);

#ifdef __cplusplus
}
#endif //__cplusplus
#endif //CRC16H
/*------------------------------------------------------------------------------------*/
/* EOF */
/*------------------------------------------------------------------------------------*/




