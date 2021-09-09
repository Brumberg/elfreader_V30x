/*                            =======================
================================ C/C++ SOURCE FILE =================================
                              =======================                          *//**
\file Crc16.c
\brief Implementation from the crc16 functions.
       Polynomial:    0x1021 x^16 + x^12 + x^5 +1
       Bit order:     MSB of each byte is shifted in the first
       Initial value: 0xffff
\n\n
Copyright (c) Endress+Hauser AG \n
All rights reserved.
*//*==================================================================================*/
/*
CHANGE HISTORY:
---------------
Date       Author Description
01.11.2007 HM    v0.0 Released
*/

/*------------------------------------------------------------------------------------*/
/* INCLUDES */
/*------------------------------------------------------------------------------------*/
#include "Crc16.h"


/*------------------------------------------------------------------------------------*/
/* DEFINITIONS AND MACROS */
/*------------------------------------------------------------------------------------*/
// #define FFGENPOLY  0x1021 /* CCITT-16 */


/*------------------------------------------------------------------------------------*/
/* TYPEDEFS AND STRUCTURES */
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
/* PROTOTYPES */
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*/
/* GLOBAL VARIABLES */
/*------------------------------------------------------------------------------------*/
const uint16 CRC16_DEFAULT_INITIAL_VALUE =  0xFFFFu;


/*------------------------------------------------------------------------------------*/
/* LOCAL VARIABLES */
/*------------------------------------------------------------------------------------*/
static const uint16 mau16_CrcTable[256]=
{
    0u,      4129u,  8258u, 12387u, 16516u, 20645u, 24774u, 28903u, 33032u, 37161u, 41290u, 45419u, 49548u, 53677u, 57806u, 61935u,
    4657u,    528u, 12915u,  8786u, 21173u, 17044u, 29431u, 25302u, 37689u, 33560u, 45947u, 41818u, 54205u, 50076u, 62463u, 58334u, 
    9314u,  13379u,  1056u,  5121u, 25830u, 29895u, 17572u, 21637u, 42346u, 46411u, 34088u, 38153u, 58862u, 62927u, 50604u, 54669u,
    13907u,  9842u,  5649u,  1584u, 30423u, 26358u, 22165u, 18100u, 46939u, 42874u, 38681u, 34616u, 63455u, 59390u, 55197u, 51132u,
    18628u, 22757u, 26758u, 30887u,  2112u,  6241u, 10242u, 14371u, 51660u, 55789u, 59790u, 63919u, 35144u, 39273u, 43274u, 47403u,
    23285u, 19156u, 31415u, 27286u,  6769u,  2640u, 14899u, 10770u, 56317u, 52188u, 64447u, 60318u, 39801u, 35672u, 47931u, 43802u,
    27814u, 31879u, 19684u, 23749u, 11298u, 15363u,  3168u,  7233u, 60846u, 64911u, 52716u, 56781u, 44330u, 48395u, 36200u, 40265u,
    32407u, 28342u, 24277u, 20212u, 15891u, 11826u,  7761u,  3696u, 65439u, 61374u, 57309u, 53244u, 48923u, 44858u, 40793u, 36728u,
    37256u, 33193u, 45514u, 41451u, 53516u, 49453u, 61774u, 57711u,  4224u,   161u, 12482u,  8419u, 20484u, 16421u, 28742u, 24679u,
    33721u, 37784u, 41979u, 46042u, 49981u, 54044u, 58239u, 62302u,   689u,  4752u,  8947u, 13010u, 16949u, 21012u, 25207u, 29270u,
    46570u, 42443u, 38312u, 34185u, 62830u, 58703u, 54572u, 50445u, 13538u,  9411u,  5280u,  1153u, 29798u, 25671u, 21540u, 17413u,
    42971u, 47098u, 34713u, 38840u, 59231u, 63358u, 50973u, 55100u,  9939u, 14066u,  1681u,  5808u, 26199u, 30326u, 17941u, 22068u,
    55628u, 51565u, 63758u, 59695u, 39368u, 35305u, 47498u, 43435u, 22596u, 18533u, 30726u, 26663u,  6336u,  2273u, 14466u, 10403u,
    52093u, 56156u, 60223u, 64286u, 35833u, 39896u, 43963u, 48026u, 19061u, 23124u, 27191u, 31254u,  2801u,  6864u, 10931u, 14994u,
    64814u, 60687u, 56684u, 52557u, 48554u, 44427u, 40424u, 36297u, 31782u, 27655u, 23652u, 19525u, 15522u, 11395u,  7392u,  3265u,
    61215u, 65342u, 53085u, 57212u, 44955u, 49082u, 36825u, 40952u, 28183u, 32310u, 20053u, 24180u, 11923u, 16050u,  3793u,  7920u, 
};



/*------------------------------------------------------------------------------------*/
/* FUNCTION IMPLEMENTATION */
/*------------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------------*//**
\brief  Set proper initial value for the crc calculation.

\param[in] u16_start  Crc start value 

\return  u16_init   the new crc start value 
*//*----------------------------------------------------------------------------------*/

uint16 g_SetCRCInitValue(uint16 u16_start)
{
    uint16 u16_init = 0u;
    uint16 u16_byte;
    uint16 u16_val;

    if(u16_start)
    {
        u16_val = 16u;
        while( u16_val )
        {
            u16_val  = (uint16)(u16_val - (uint16)8);
            u16_byte = (u16_start >> u16_val) & (uint16)0x00FF;
            u16_init = mau16_CrcTable[((u16_init >> 8u) ^ u16_byte) & (uint16)0x00FF] ^ (uint16)(u16_init << (uint16)8);
        }
    }
    return u16_init;
}


/*------------------------------------------------------------------------------------*//**
\brief  Update the Crc sum with one data byte.

\param[in] u16_Crc     the last calculated u16_Crc number 
\param[in] u8_Databyte next data byte
\return  calculated the new crcv sum 
*//*----------------------------------------------------------------------------------*/
uint16 g_UpdateCrc(uint16 u16_Crc, uint8 u8_Databyte)
{
    uint16 u16_tmp;
    u16_tmp = (u16_Crc>>(uint16)8) ^ u8_Databyte;  
    return ((uint16)(u16_Crc<<(uint16)8) ^ mau16_CrcTable[u16_tmp]);  
}


/*------------------------------------------------------------------------------------*//**
\brief  Calculate the Crc sum.

\param[in] u16_Crc     the first crc value (initial value) 
\param[in] u32_Len     numbers of data bytes for calculate crc16 sum 
\param[in] pv_Buf      data buffer 

\return  calculated    the crc16 sum
*//*----------------------------------------------------------------------------------*/
uint16 g_CalcCrcSum(uint16 u16_Crc, uint32 u32_Len, const void* pv_Buf)
{
    uint32 u32_i; 
    uint16 u16_tmp;
    const uint8 * const pu8_Buf = (const uint8 *)(pv_Buf); //lint !e925 cast from pointer to pointer. SST: Cast is required due to generic interface

    if ( pu8_Buf != (uint8*)0 )
    {
        for(u32_i=(uint32)0; u32_i<u32_Len; u32_i++)
        {
            u16_tmp = (u16_Crc>>(uint16)8) ^ pu8_Buf[u32_i];  //lint !e960 LK: Issue 960: (Elective Note -- Violates MISRA 2004 Required Rule 17.4, pointer arithmetic other than array indexing used) Rule is broken for have an high efficient code.

            u16_Crc = (uint16)(u16_Crc<<(uint16)8) ^ mau16_CrcTable[u16_tmp];  
        }
    }
    return u16_Crc;
}



/*------------------------------------------------------------------------------------*//**
\brief  update of an existing crc16 check sum

\param[in] u16_oldCrc           the old crc 16 sum  
\param[in] pv_newData           data buffer with the new data 
\param[in] pv_oldData           data buffer with the old data
\param[in] u16_size             buffer size (old and new data)
\param[in] u16_remainingBytes   numbers of data bytes from end of the modified data until the data end

\return  
*//*----------------------------------------------------------------------------------*/
uint16 g_UpdateCrcSum(const uint16 u16_oldCrc, const void * const pv_newData, const void * const pv_oldData, 
                           const uint16 u16_size, const uint16 u16_remainingBytes)
{
    uint16 u16_crc;
    uint16 u16_cnt;
    uint8 u8_idx;
    const uint8 * const pu8_tmpNewData = (const uint8 *)(pv_newData); //lint !e925 cast from pointer to pointer. SST: Cast is required due to generic interface
    const uint8 * const pu8_tmpOldData = (const uint8 *)(pv_oldData); //lint !e925 cast from pointer to pointer. SST: Cast is required due to generic interface

    u16_crc = 0u;

    if((pu8_tmpNewData !=  (void *)0) && (pu8_tmpOldData != (void *)0))
    {
        for(u16_cnt = 0u; u16_cnt < u16_size; u16_cnt++)
        {
            u8_idx = (uint8)(u16_crc >> 8) ^ (pu8_tmpNewData[u16_cnt] ^ pu8_tmpOldData[u16_cnt]); //lint !e960 LK: Issue 960: (Elective Note -- Violates MISRA 2004 Required Rule 17.4, pointer arithmetic other than array indexing used) Rule is broken for have an high efficient code.
            u16_crc = (uint16)(u16_crc << 8) ^ mau16_CrcTable[u8_idx];
        }

        for(u16_cnt = 0u; u16_cnt < u16_remainingBytes; u16_cnt++)
        {
            u8_idx = (uint8)(u16_crc >> 8);
            u16_crc = (uint16)(u16_crc << 8) ^ mau16_CrcTable[u8_idx];
        }
    }
    return (u16_crc ^ u16_oldCrc);
}

/* Example code: With the next two fuctions you can generate the crc table */
#if 0

/*------------------------------------------------------------------------------------*//**
\brief  Generate Crc table 

\return  none
*//*----------------------------------------------------------------------------------*/
static void Gentable(void)
{
    uint16 u16_pos;
 
    for (u16_pos=0; u16_pos<256; u16_pos++)
    {
        crcTable[u16_pos] = gencrc(0, u16_pos, FFGENPOLY);
    }
}


/*------------------------------------------------------------------------------------*//**
\brief  Generate Crc values for table 

\param[in] u16_Crc      
\param[in] u8_Databyte 
\param[in] u16_mask 

\return  crc value 
*//*----------------------------------------------------------------------------------*/
static uint16 gencrc(uint16 u16_Crc, uint16 u16_Databyte, uint16 u16_mask)
{
    uint8 u8_uc;
    u16_Databyte <<= 8;
    for(u8_uc=0; u8_uc<8; u8_uc++)
    {
        if( (u16_Crc^u16_Databyte) & 0x8000)
        {
          u16_Crc = (u16_Crc<<1) ^ u16_mask;
        }
        else
        {
            u16_Crc <<= 1;
        }

        u16_Databyte <<= 1;
    }
    return u16_Crc;
}

#endif 



/*------------------------------------------------------------------------------------*/
/* EOF */
/*------------------------------------------------------------------------------------*/
