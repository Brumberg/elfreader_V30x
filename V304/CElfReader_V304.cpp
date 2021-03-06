
#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <sstream>
#include <stdint.h>
#include "CElfreader_V304.h"
#include "..\Crc16.h"

#define BK_THIS_ID           0xAD
#define BK_THIS_PROJECT      0x01


#define FLASHHEADER_SIZE	16u     /**< Length of block headers in flash memory */


#define BK_ID                0xFF000000
#define BK_PROJECT           0x00FF0000
#define BK_VERSION           0x0000FF00
#define BK_UPDATE            0x000000FF

#define BK_YEAR              0xFFFF0000
#define BK_MONTH             0x0000FF00
#define BK_DAY               0x000000FF

/* ******************************************************************************************* */
/*                                                                                             */
/*   Boot Flags (part of block header's block code field)                                      */
/*                                                                                             */
/* ******************************************************************************************* */

#define BFLAG_DMACODE		0x0000000F	 /* specifies some dma code (see hardware reference manual, not further examined)*/
#define BFLAG_FINAL         0x00008000   /* final block in stream */
#define BFLAG_FIRST         0x00004000   /* first block in stream */
#define BFLAG_INDIRECT      0x00002000   /* load data via intermediate buffer */
#define BFLAG_IGNORE        0x00001000   /* ignore block payload */
#define BFLAG_INIT          0x00000800   /* call initcode routine */
#define BFLAG_CALLBACK      0x00000400   /* call callback routine */
#define BFLAG_QUICKBOOT     0x00000200   /* boot block only when BFLAG_WAKEUP=0 */
#define BFLAG_FILL          0x00000100   /* fill memory with 32-bit argument value */
#define BFLAG_AUX           0x00000020   /* load auxiliary header -- reserved */
#define BFLAG_SAVE          0x00000010   /* save block on power down -- reserved */


/* ******************************************************************************************* */
/*                                                                                             */
/*   Boot Flags (global flags for pFlag word)                                                  */
/*                                                                                             */
/* ******************************************************************************************* */


#define BFLAG_NORESTORE     0x80000000   /* do not restore MMR register when done */
#define BFLAG_RESET         0x40000000   /* issue system reset when done */
#define BFLAG_RETURN        0x20000000   /* issue RTS instead of jumping to EVT1 vector */
#define BFLAG_NEXTDXE       0x10000000   /* parse stream via Next DXE pointer */
#define BFLAG_WAKEUP        0x08000000   /* WURESET bit was a '1', enable quickboot */
#define BFLAG_SLAVE         0x04000000   /* boot mode is a slave mode */
#define BFLAG_PERIPHERAL    0x02000000   /* boot mode is a peripheral mode */
#define BFLAG_NOAUTO        0x01000000   /* skip automatic device detection */
#define BFLAG_ALTERNATE     0x00800000   /* use alternate boot source */
#define BFLAG_FASTREAD      0x00400000   /* use 0xB command in SPI master mode */
#define BFLAG_TYPE          0x00100000   /* device type (number of address bytes) */ 
#define BFLAG_TYPE1      0x00000000   /* device type (1 SPI/TWI address bytes, Small Page NAND Flash) */
#define BFLAG_TYPE2      0x00100000   /* device type (2 SPI/TWI address bytes, Large Page NAND Flash) */
#define BFLAG_TYPE3      0x00200000   /* device type (3 SPI/TWI address bytes, NAND reserved) */
#define BFLAG_TYPE4      0x00300000   /* device type (4 SPI/TWI address bytes, NAND reserved) */
#define BFLAG_HDRINDIRECT   0x00080000   /* boot block headers via intermediate buffer */
#define BFLAG_HOOK          0x00040000   /* call hook routine after initialization */
#define BFLAG_TEST          0x00020000   /* factory testing */

/** \name Block header flags
* @{ */
#define ZERO_FILL       	0x0001  /**< 1=Zero-fill this block */
#define PROCESSOR_TYPE  	0x0002  /**< Processor type, 0=531/532, 1=533-539 */
#define INIT_BLOCK      	0x0008  /**< 1=This is an init block */
#define IGNORE_BLOCK    	0x0010  /**< 1=Ignore (skip) this block */
#define LAST_BLOCK      	0x8000  /**< 1=This is the last block */
/** @} */


/*------------------------------------------------------------------------------------*/
/* TYPEDEFS, CLASSES AND STRUCTURES */
/*------------------------------------------------------------------------------------*/

/**<    Structure used by the bootloader to identify the application
*/
typedef struct _ST_APPINFOS
{
    uint32 u32_magic;                   /**< Magic number */
    uint32 u32_device_FwType;           /**< Initialized with DEVICEFIRMWARETYPE */
    uint32 u32_FW_VersionNumber;        /**< Initialized e.g. with U32_MAINBOARD_FIRMWARE_REVISION */
    uint16 u16_FW_BuildNumber;          /**< Initialized e.g. with BUILD_NUMBER_MB*/
    uint16 u16_ModuleID;                /**< Initialized e.g. with U16_MAINBORAD_MODULE_ID */
    uint16 u16_device_FieldbusType;     /**< Initialized with DEVICE_FIELDBUS_TYPE */
    char   ac8_DeviceName[34];          /**< Initialized with DEVICE_NAME (34 to keep the multiple of 4)*/
    uint8  au8_reserve[32];             /**<  */
}ST_APPINFOS;


namespace V304
{
const CElfReader::TMemoryMap CElfReader::m_TemplateMemoryLayout[13] =
{
	{0x80000000, 0x02000000, 0x80000000, false, false,NULL},//SDRAM
	{0x74000000, 0x00002000, 0x74000000, false, false,NULL },//Static Memory Block 1
	{0x70000000, 0x00002000, 0x70000000, false, false,NULL },//Static Memory Block 0
	{0x40000000, 0x08000000, 0x40000000, false, true,NULL},//SPI2 Memory
	{0x38000000, 0x00000400, 0x38000000, false, true,NULL },//OTP memory
	{0x11B00000, 0x00002000, 0x11B00000, false, false,NULL },//L1 Data Block C
	{0x11A0C000, 0x00004000, 0x11A0C000, true, false,NULL },//L1 Instruction SRAM/Cache
	{0x11A00000, 0x0000C000, 0x11A00000, true, false,NULL },//L1 Instruction SRAM
	{0x11904000, 0x00004000, 0x11904000, false, false,NULL },//L1 Data SRAM/Cache Block B
	{0x11900000, 0x00004000, 0x11900000, false, false,NULL },//L1 Data SRAM Block B
	{0x11804000, 0x00004000, 0x11804000, false, false,NULL },//L1 Data SRAM/Cache Block A
	{0x11800000, 0x00004000, 0x11800000, false, false,NULL },//L1 Data SRAM Block A
	{0x08000000, 0x00100000, 0x08000000, false, false,NULL},//L2 SRAM
};


CElfReader::CElfReader(std::string filename)
:eElfStatus(UNINITIALIZED), m_StreamLength(0)
{
	memset(m_MemoryTable, 0, sizeof(m_MemoryTable));

	m_SDRAM.reserve(m_TemplateMemoryLayout[0].Length);
	m_StaticMemoryBlock1.reserve(m_TemplateMemoryLayout[1].Length);
	m_StaticMemoryBlock0.reserve(m_TemplateMemoryLayout[2].Length);
	m_SPI2Memory.reserve(m_TemplateMemoryLayout[3].Length);
	m_OPTMemory.reserve(m_TemplateMemoryLayout[4].Length);
	m_L1DataBlockC.reserve(m_TemplateMemoryLayout[5].Length);
	m_InstructionCache.reserve(m_TemplateMemoryLayout[6].Length);
	m_InstructionSRAM.reserve(m_TemplateMemoryLayout[7].Length);
	m_L1DataBlockB_Cache.reserve(m_TemplateMemoryLayout[8].Length);
	m_L1DataBlockB.reserve(m_TemplateMemoryLayout[9].Length);
	m_L1DataBlockA_Cache.reserve(m_TemplateMemoryLayout[10].Length);
	m_L1DataBlockA.reserve(m_TemplateMemoryLayout[11].Length);
	m_L2SRAM.reserve(m_TemplateMemoryLayout[12].Length);

	m_SDRAM.resize(m_TemplateMemoryLayout[0].Length,0);
	m_StaticMemoryBlock1.resize(m_TemplateMemoryLayout[1].Length,0);
	m_StaticMemoryBlock0.resize(m_TemplateMemoryLayout[2].Length,0);
	m_SPI2Memory.resize(m_TemplateMemoryLayout[3].Length,0);
	m_OPTMemory.resize(m_TemplateMemoryLayout[4].Length,0);
	m_L1DataBlockC.resize(m_TemplateMemoryLayout[5].Length,0);
	m_InstructionCache.resize(m_TemplateMemoryLayout[6].Length,0);
	m_InstructionSRAM.resize(m_TemplateMemoryLayout[7].Length,0);
	m_L1DataBlockB_Cache.resize(m_TemplateMemoryLayout[8].Length,0);
	m_L1DataBlockB.resize(m_TemplateMemoryLayout[9].Length,0);
	m_L1DataBlockA_Cache.resize(m_TemplateMemoryLayout[10].Length,0);
	m_L1DataBlockA.resize(m_TemplateMemoryLayout[11].Length,0);
	m_L2SRAM.resize(m_TemplateMemoryLayout[12].Length,0);


	m_Memory.push_back(&m_SDRAM);
	m_Memory.push_back(&m_StaticMemoryBlock1);
	m_Memory.push_back(&m_StaticMemoryBlock0);
	m_Memory.push_back(&m_SPI2Memory);
	m_Memory.push_back(&m_OPTMemory);
	m_Memory.push_back(&m_L1DataBlockC);
	m_Memory.push_back(&m_InstructionCache);
	m_Memory.push_back(&m_InstructionSRAM);
	m_Memory.push_back(&m_L1DataBlockB_Cache);
	m_Memory.push_back(&m_L1DataBlockB);
	m_Memory.push_back(&m_L1DataBlockA_Cache);
	m_Memory.push_back(&m_L1DataBlockA);
	m_Memory.push_back(&m_L2SRAM);


	for (size_t i = 0; i < sizeof(m_TemplateMemoryLayout) / sizeof(m_TemplateMemoryLayout[0]); ++i)
	{
		m_MemoryLayout[i].StartAddress = m_TemplateMemoryLayout[i].StartAddress;
		m_MemoryLayout[i].Length = m_TemplateMemoryLayout[i].Length;
		m_MemoryLayout[i].OffsetCompensation = m_TemplateMemoryLayout[i].OffsetCompensation;
		m_MemoryLayout[i].ReqDMAAccess = m_TemplateMemoryLayout[i].ReqDMAAccess;
		m_MemoryLayout[i].Ignore = m_TemplateMemoryLayout[i].Ignore;
		m_MemoryLayout[i].VectorAssignment = m_Memory[i];
	}
	
	m_MemoryLayout[0].VectorAssignment = &m_SDRAM;


	if (filename.length())
	{
		std::fstream elffile;
		std::streampos begin, end;
		elffile.open(filename, std::ios::binary | std::ios_base::in | std::ios_base::out);
		if (elffile.is_open())
		{
			begin = elffile.tellg();
			elffile.seekg(0, std::ios::end);
			end = elffile.tellg();
			elffile.seekg(0, std::ios::beg);
			std::size_t filesize = static_cast<std::size_t>(end - begin);
			m_FileRawData.clear();
			m_FileRawData.reserve(filesize);
			char *dummy = new char [filesize];
			if (dummy!=nullptr)
			{
				elffile.read(dummy, filesize);
				if (elffile)
				{
					if (CIntelHexConverter::Convert(filesize, dummy, m_FileRawData))
					{
						eElfStatus = ELF_OK;
					}
					else
					{
						eElfStatus = ELF_INVALID;
					}
				}
				else
				{
					eElfStatus = FILEINCOMPLETE;
				}
				/*for (size_t i = 0; i < filesize; ++i)
				{
					m_FileRawData.push_back(dummy[i]);
				}*/
				delete[] dummy;
				eElfStatus = ELF_OK;
			}
			else
			{
				eElfStatus = OUTOFMEMORY;
			}
		}
		else
		{
//			throw std::ios::bad;
			eElfStatus = UNABLEOPENFILE;
		}
	}
	else
	{
		eElfStatus = INVALIDFILENAME;
	}
}

const std::string& CElfReader::GetStateMessage() const
{
	static const std::string messages[] =
	{
		"Object not instantiated.",
		"No file name defined.",
		"Unable to open file.",
		"Insufficient memory available.",
		"File is incomplete.",
		"File contains unknown tokens.",
		"Can not write destination file."
		"File is OK.",
		"Unexpected/Unknown error."
	};
	uint32 selector;
	switch (eElfStatus)
	{
		case UNINITIALIZED:
			selector = 0u;
			break;
		case INVALIDFILENAME:
			selector = 1u;
			break;
		case UNABLEOPENFILE:
			selector = 2u;
			break;
		case OUTOFMEMORY:
			selector = 3u;
			break;
		case FILEINCOMPLETE:
			selector = 4u;
			break;
		case ELF_INVALID:
			selector = 5u;
			break;
		case ELF_DESTFILEINVALID:
			selector = 6u;
			break;
		case ELF_OK:
			selector = 7u;
			break;
		default:
			selector = 8u;
			break;
	}
	return messages[selector];
}

/**
* Load application from SPI flash to RAM.
*
* \return None
*/
bool CElfReader::CheckHeader(TFlashHeader *header)
{
	uint32_t i;
	uint8_t chksum = 0;
	for (i = 0; i<sizeof(TFlashHeader); ++i)
	{
		chksum ^= ((uint8_t*)header)[i];
	}
	return !chksum;
}

/**
* Load application from SPI flash to RAM.
*
* \return None
*/
bool CElfReader::CreateHeader(TFlashHeader *header, uint32_t flags, uint32_t targetaddress, uint32_t bytecount, uint32_t argument)
{
    static const uint32 ManId = 0x0AD000000;
    memset(header, 0, sizeof(TFlashHeader));
    flags &= 0x0000FFFF;
    header->usFlags = ManId | flags;
    header->ulRamAddr = targetaddress;
    header->ulBlockLen = bytecount;
    header->Argument = argument;
    uint8_t chksum = 0;
    for (uint32 i = 0; i < sizeof(TFlashHeader); ++i)
    {
        chksum ^= ((uint8_t*)header)[i];
    }
    header->usFlags |= static_cast<uint32>(chksum) << 16;
    chksum = 0;
    for (uint32 i = 0; i < sizeof(TFlashHeader); ++i)
    {
        chksum ^= ((uint8_t*)header)[i];
    }
    return !chksum;
}

bool CElfReader::FillMemory(uint32_t address, uint32_t length, uint32_t pattern)
{
	uint32_t startaddress, endaddress;
	bool retVal=false;
	for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
	{
		startaddress = m_MemoryLayout[i].StartAddress;
		endaddress = m_MemoryLayout[i].StartAddress+ m_MemoryLayout[i].Length;
		if ((address >= startaddress) && (address < endaddress))
		{
			std::vector<uint8_t> &p = *m_MemoryLayout[i].VectorAssignment;
			uint32_t addresscompensation = address-m_MemoryLayout[i].OffsetCompensation;
			for (size_t j = 0; j < length; j+=sizeof(pattern))
			{
				//p[addresscompensation++] = pattern;
				memcpy(&p[addresscompensation],&pattern,sizeof(pattern));
				addresscompensation += sizeof(pattern);
			}
			retVal = true;
		}
	}
	return retVal;
}

bool CElfReader::CopyBlock(uint32_t sourceaddress, uint32_t destaddress, uint32_t length)
{
	uint32_t startaddress, endaddress;
	bool retVal = false;
	for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
	{
		startaddress = m_MemoryLayout[i].StartAddress;
		endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
		if ((destaddress >= startaddress) && (destaddress < endaddress))
		{
			std::vector<uint8_t> &p = *m_MemoryLayout[i].VectorAssignment;
			uint32_t addresscompensation = destaddress - m_MemoryLayout[i].OffsetCompensation;
			for (size_t j = 0; j < length; ++j)
			{
				p[addresscompensation++] = m_FileRawData[sourceaddress++];
			}
			retVal = true;
		}
	}
	return retVal;
}

const uint8_t* CElfReader::GetMemoryContent(uint32_t start, uint32_t stop)const
{
	if (start <= stop)
	{
		for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
		{
			uint32_t startaddress = m_MemoryLayout[i].StartAddress;
			uint32_t endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
			if ((start >= startaddress) && (stop <= endaddress))
			{
				std::vector<uint8_t> &p = *m_MemoryLayout[i].VectorAssignment;
				uint32_t addresscompensation = start - m_MemoryLayout[i].OffsetCompensation;
				uint8_t *dat = p.data();
				dat = &dat[addresscompensation];
				return dat;
			}
		}
	}
	return nullptr;
}

bool  CElfReader::RequiresDMAAccess(uint32_t start, uint32_t stop)const
{
	if (start <= stop)
	{
		for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
		{
			uint32_t startaddress = m_MemoryLayout[i].StartAddress;
			uint32_t endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
			if ((start >= startaddress) && (stop <= endaddress))
			{
				return m_MemoryLayout[i].ReqDMAAccess;
			}
		}
	}
	return false;
}

bool  CElfReader::IgnoreMemorySection(uint32_t start, uint32_t stop)const
{
	if (start <= stop)
	{
		for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
		{
			uint32_t startaddress = m_MemoryLayout[i].StartAddress;
			uint32_t endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
			if ((start >= startaddress) && (stop <= endaddress))
			{
				return m_MemoryLayout[i].Ignore;
			}
		}
	}
	return false;
}

bool CElfReader::GenerateTableEntry(BlockType type, uint32_t startaddress, uint32_t stopaddress)
{
	bool retVal = false;
	MemoryTable entry;
    memset(&entry, 0, sizeof(MemoryTable));
	entry.startaddress = reinterpret_cast<uint16_t*>(startaddress);
	entry.stopaddress = reinterpret_cast<uint16_t*>(stopaddress);
	entry.m_u16CRC = 0;
	entry.m_pu16CRCState = NULL;
	entry.m_NextTableEntry = NULL;
	entry.m_bDMAAccess = false;
	const uint8_t *memorysection = GetMemoryContent(startaddress, stopaddress);
	if (memorysection!=nullptr)//it is a const clock
	{
		if (!((startaddress >= IgnoreSDRAMLower) && (stopaddress <= IgnoreSDRAMUpper)))
		{
			if (!IgnoreMemorySection(startaddress, stopaddress))
			{
				if (type == NORMAL || type == FILL)//don't want to process fill blocks
				{
					entry.m_bDMAAccess = RequiresDMAAccess(startaddress, stopaddress);
					RegeneratedMemTable.push_back(entry);
					retVal = true;
				}
				else
				{
					std::cout << "Block ignored (fill) 0x" << entry.startaddress << " 0x" << entry.stopaddress << std::endl;
				}
			}
			else
			{
				std::cout << "Block ignored (memsection) 0x" << entry.startaddress << " 0x" << entry.stopaddress << std::endl;
			}
		}
		else
		{
			std::cout << "Block ignored 0x" << entry.startaddress << " 0x" << entry.stopaddress << std::endl;
		}
	}
	else
	{
		std::cout << "Block ignored - pointing to zero " << std::endl;
	}
	return retVal;
}

bool CElfReader::Deflate()
{
	bool retVal;
	if (eElfStatus == ELF_OK)
	{
		TFlashHeader *pHdr;
		uint32_t RawPointer = 0;
		retVal = true;
		RegeneratedMemTable.clear();
		m_StreamLength = 0;
		do {
			pHdr = reinterpret_cast<TFlashHeader*>(&m_FileRawData[RawPointer]);
			if ((((pHdr->usFlags&BK_ID) >> 24) == BK_THIS_ID) && CheckHeader(pHdr))
			{
				if (pHdr->usFlags&BFLAG_FILL)
				{
					uint32_t pucAddr = pHdr->ulRamAddr;
					uint32_t ulsize = pHdr->ulBlockLen;
					std::cout << "Processing fill block\tAddress: 0x" << std::hex << pucAddr << "\tSize: 0x" << std::hex << ulsize << std::endl;

					if (!(pHdr->usFlags&BFLAG_IGNORE))
						FillMemory(pucAddr, ulsize, pHdr->Argument);

					GenerateTableEntry(FILL,pucAddr, pucAddr + ulsize);
					RawPointer += FLASHHEADER_SIZE;
					m_StreamLength += ulsize;
				}
				else
				{
					uint32_t pucAddr = pHdr->ulRamAddr;

					uint32_t RawPointerAdd = RawPointer + FLASHHEADER_SIZE;
					uint32_t ulsize = pHdr->ulBlockLen;
					std::cout << "Processing code/data block\tAddress: 0x" << std::hex << pucAddr << "\tSize: 0x" << std::hex << ulsize << std::endl;
					CopyBlock(RawPointerAdd, pucAddr, ulsize);
					GenerateTableEntry(NORMAL,pucAddr, pucAddr + ulsize);
					RawPointer += FLASHHEADER_SIZE + pHdr->ulBlockLen;
					m_StreamLength += ulsize;
				}

				if ((pHdr->usFlags&(BFLAG_INIT | BFLAG_IGNORE)) == BFLAG_INIT)
				{
					std::cout << "Execute Init." << std::endl;
				}

				if ((pHdr->usFlags&(BFLAG_CALLBACK | BFLAG_IGNORE)) == BFLAG_INIT)
				{
					std::cout << "Another callback execution." << std::endl;
				}

			}
			else
			{
				retVal = false;
				std::cerr << "Abnormal header block." << std::endl;
			}
		} while (((pHdr->usFlags&BFLAG_FINAL) == 0) && retVal);
	}
	else
	{
		retVal = false;
	}
	return retVal;
}

bool CElfReader::CmpFillBlock(uint32_t address, uint32_t size, uint32_t pattern)
{
	uint32_t startaddress, endaddress;
	bool retVal = false;
	for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
	{
		startaddress = m_MemoryLayout[i].StartAddress;
		endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
		if ((address >= startaddress) && (address < endaddress))
		{
			retVal = true;
			std::vector<uint8_t> &p = *m_MemoryLayout[i].VectorAssignment;
			uint32_t addresscompensation = address - m_MemoryLayout[i].OffsetCompensation;
			for (size_t j = 0; j < size; j += sizeof(pattern))
			{
				//p[addresscompensation++] = pattern;
				retVal &= memcmp(&p[addresscompensation], &pattern, sizeof(pattern))==0;
				addresscompensation += sizeof(pattern);
			}
		}
	}
	return retVal;
}

bool CElfReader::CmpDataBlock(uint32_t address, uint32_t size, uint8_t *data)
{
	uint32_t startaddress, endaddress;
	bool retVal = false;
	for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
	{
		startaddress = m_MemoryLayout[i].StartAddress;
		endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
		if ((address >= startaddress) && (address+size <= endaddress))
		{
			std::vector<uint8_t> &p = *m_MemoryLayout[i].VectorAssignment;
			uint32_t addresscompensation = address - m_MemoryLayout[i].OffsetCompensation;
			retVal = true;
			for (size_t j = 0; j < size; ++j)
			{
				retVal &= p[addresscompensation++] == data[j];
			}
		}
	}
	return retVal;
}

bool CElfReader::PatchSection(std::vector<uint8_t> &datavector, TFlashHeader *header)
{
	uint32_t startaddress, endaddress;
	bool retVal = false;
	
	uint32_t address = header->ulRamAddr;
	uint32_t size = header->ulBlockLen;
	uint32_t pattern = header->Argument;
	
	for (size_t i = 0; i < sizeof(m_MemoryLayout) / sizeof(m_MemoryLayout[0]); ++i)
	{
		startaddress = m_MemoryLayout[i].StartAddress;
		endaddress = m_MemoryLayout[i].StartAddress + m_MemoryLayout[i].Length;
		if ((address >= startaddress) && (address + size < endaddress))
		{
			std::vector<uint8_t> &p = *m_MemoryLayout[i].VectorAssignment;
			uint32_t addresscompensation = address - m_MemoryLayout[i].OffsetCompensation;
			retVal = true;
			uint32_t j;
			const uint32_t startindex = size / sizeof(pattern);
			uint8_t *data = p.data() + addresscompensation;
			bool nothingtochange = false;
			for (j = startindex-1; j < startindex; --j)
			{
				if (reinterpret_cast<uint32_t*>(data)[j] != pattern)
				{
					nothingtochange = true;
					break;
				}
			}

			if (!nothingtochange)
			{
				for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
					datavector.push_back(reinterpret_cast<uint8_t*>(header)[i]);
			}
			else
			{
				//fill it up
				const uint32_t stopindex = (j * sizeof(pattern)) / sizeof(MemoryTable) + sizeof(MemoryTable) - 1;
				for (uint32_t i = 0; i < stopindex; i += sizeof(MemoryTable))
				{
					for (uint32_t k = 0; k < sizeof(MemoryTable); ++k)
						datavector.push_back(data[i + k]);
				}

				for (uint32_t i = 0; i < stopindex % sizeof(pattern); ++i)
				{
					datavector.push_back(reinterpret_cast<uint8_t*>(&pattern)[i]);
				}
				
				header->ulBlockLen -= (sizeof(pattern)*(stopindex+1)-1)/sizeof(pattern);
				for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
					datavector.push_back(reinterpret_cast<uint8_t*>(header)[i]);
			}
		}
	}
	return retVal;
}

bool CElfReader::CorrectApplicationHeaderStructure(bool appendinfoblock, uint32_t appinfoaddress, uint32_t DXEPointer)
{
	bool retVal = true;
	//what's missing is patching the first argument -> it's pointing to the next dxe.
	//Therefore we have to iterate to the final block and adjust the first argument.
	//iterate to the final dxe$

	TFlashHeader* const pInitialHdr = reinterpret_cast<TFlashHeader*>(&m_PatchedData[DXEPointer]);
	if (DXEPointer + sizeof(TFlashHeader) < m_PatchedData.size())
	{
		//in this case we have to do some additional work
		if (appendinfoblock)
		{
			uint32_t argument = 0;
			TFlashHeader* pHdr = pInitialHdr;
			while (DXEPointer + sizeof(TFlashHeader) < m_PatchedData.size() && !(pHdr->usFlags & BFLAG_FINAL))
			{
				DXEPointer += sizeof(TFlashHeader);
				DXEPointer += (pHdr->usFlags & BFLAG_FILL) ? 0 : pHdr->ulBlockLen;
				argument += sizeof(TFlashHeader);
				argument += (pHdr->usFlags & BFLAG_FILL) ? 0 : pHdr->ulBlockLen;
				pHdr = reinterpret_cast<TFlashHeader*>(&m_PatchedData[DXEPointer]);
			}
			if (pHdr->usFlags & BFLAG_FINAL)
			{
				std::vector<uint8> finalblock;
				//TFlashHeader finalheader;
				//memcpy(&finalheader, pHdr, sizeof(TFlashHeader));
				uint32_t blocksize = pHdr->ulBlockLen + sizeof(TFlashHeader);
				finalblock.reserve(blocksize);
				for (uint32_t i = 0; i < blocksize; ++i)
				{
					finalblock.push_back(m_PatchedData[i + DXEPointer]);
					//m_PatchedData[i + DXEPointer] = 0;
				}

				m_PatchedData.resize(DXEPointer + sizeof(TFlashHeader));
				//now m_PatchedData's last header is erased
				//create a new header - pHdr is still valid -> in place generation. Header does exist
				if (CreateHeader(pHdr, BFLAG_IGNORE, appinfoaddress, sizeof(ST_APPINFOS), 0))
				{
					//getting pointer to dxdata
					const uint8_t* t = GetMemoryContent(appinfoaddress, appinfoaddress + sizeof(ST_APPINFOS));
					if (t!=nullptr)
					{
						const ST_APPINFOS* info = reinterpret_cast<const ST_APPINFOS*>(t);
						std::cout << "Device Name: " << info->ac8_DeviceName << std::endl
							<< "Build No: " << std::dec << info->u16_FW_BuildNumber << std::endl
							<< "Module Id: " << info->u16_ModuleID << std::endl
							<< "Version Info: " << info->u32_FW_VersionNumber << std::endl;

						/*uint32_t i;
						for (i = 0; i < sizeof(ST_APPINFOS)&& i + sizeof(TFlashHeader) + DXEPointer < m_PatchedData.size(); ++i)
						{
							m_PatchedData[i + sizeof(TFlashHeader) + DXEPointer] = t[i];
						}*/
						for (uint32_t i = 0; i < sizeof(ST_APPINFOS); ++i)
						{
							m_PatchedData.push_back(t[i]);
						}
						argument += sizeof(ST_APPINFOS) + sizeof(TFlashHeader);
						//adding final header.
						//not very efficient -> but it is just one final header
						uint32_t i = 0;
						for (std::vector<uint8>::iterator iter = finalblock.begin(); iter != finalblock.end(); ++iter)
						{
							//if (m_PatchedData.size() <= i + sizeof(TFlashHeader) + DXEPointer + sizeof(ST_APPINFOS))
							//{
							m_PatchedData.push_back(*iter);
							//}
							/*else
							{
								m_PatchedData[i + sizeof(TFlashHeader) + DXEPointer + sizeof(ST_APPINFOS)] = *iter;
							}*/
							++i;
						}
						argument += blocksize;
					}
					else
					{
						std::cerr << "Error while creating IGNORE block. Invalid memory section." << std::endl;
						retVal = false;
					}
				}
				else
				{
					std::cerr << "Error while creating IGNORE block." << std::endl;
					retVal = false;
				}

				//pointing to the last block -> save it and append it
				pHdr = pInitialHdr;
				pHdr->Argument = argument - sizeof(TFlashHeader);//the size of initial block has to be subtracted
				//argument += sizeof(TFlashHeader);
				//argument += pHdr->usFlags&BFLAG_FILL ? 0 : pHdr->ulBlockLen;
				//pHdr->Argument = argument;
				//pHdr->usFlags &= 0x0000FFFF;
				//recreate header
				CreateHeader(pHdr, pHdr->usFlags, pHdr->ulRamAddr, pHdr->ulBlockLen, pHdr->Argument);
			}
			else
			{
				std::cerr << "Invalid data stream. Final block missing." << std::endl;
				retVal = false;
			}
		}
		else
		{
			uint32_t argument = 0;
			TFlashHeader* pHdr = pInitialHdr;
			while (DXEPointer + sizeof(TFlashHeader) < m_PatchedData.size() && !(pHdr->usFlags & BFLAG_FINAL))
			{
				DXEPointer += sizeof(TFlashHeader);
				DXEPointer += (pHdr->usFlags & BFLAG_FILL) ? 0 : pHdr->ulBlockLen;
				argument += sizeof(TFlashHeader);
				argument += (pHdr->usFlags & BFLAG_FILL) ? 0 : pHdr->ulBlockLen;
				pHdr = reinterpret_cast<TFlashHeader*>(&m_PatchedData[DXEPointer]);
			}
			if (pHdr->usFlags & BFLAG_FINAL)
			{
				argument += pHdr->ulBlockLen + sizeof(TFlashHeader);
				TFlashHeader* pHdr = pInitialHdr;
				pHdr->Argument = argument - sizeof(TFlashHeader);//subtract size of first header
				pHdr->usFlags &= 0x0000FFFF;
				CreateHeader(pHdr, pHdr->usFlags, pHdr->ulRamAddr, pHdr->ulBlockLen, pHdr->Argument);
			}
			else
			{
				std::cerr << "Invalid data stream. Final block missing." << std::endl;
				retVal = false;
			}
		}
	}
	else
	{
		retVal = false;
	}
	return retVal;
}

bool CElfReader::PatchFile(bool appendinfoblock, uint32_t appinfoaddress)
{
	bool retVal;
	m_PatchedData.reserve(2 * m_FileRawData.size());
	uint32_t PrevHeaderId=-1;
	if (eElfStatus == ELF_OK)
	{
		TFlashHeader *pHdr;
		uint32_t RawPointer = 0;
		uint32_t DXEPointer = 0;
		uint32_t PatchOffset = 0;
		retVal = true;

		//iterate to the final dxe
		while (DXEPointer < m_FileRawData.size())
		{
			pHdr = reinterpret_cast<TFlashHeader*>(&m_FileRawData[DXEPointer]);
			if ((pHdr->usFlags&(BFLAG_IGNORE | BFLAG_FIRST))==(BFLAG_IGNORE | BFLAG_FIRST))
			{
				RawPointer = DXEPointer;
			}
			else
			{
				break;
			}
			DXEPointer += pHdr->Argument+sizeof(TFlashHeader);
		} 

		do {
			pHdr = reinterpret_cast<TFlashHeader*>(&m_FileRawData[RawPointer]);
			if ((((pHdr->usFlags&BK_ID) >> 24) == BK_THIS_ID) && CheckHeader(pHdr))
			{
				uint32_t pucAddr = pHdr->ulRamAddr;
				uint32_t ulsize = pHdr->ulBlockLen;

				if (pHdr->usFlags&BFLAG_FILL)
				{
					bool addblock = true;

					if (!(pHdr->usFlags&BFLAG_IGNORE))
					{
						addblock = CmpFillBlock(pucAddr, ulsize, pHdr->Argument);
						if (!addblock)
						{
							std::cout << "Correcting fill block: " << std::hex << "0x" << pucAddr << std::endl;
						}
					}

					if (addblock)
					{
						for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
							m_PatchedData.push_back(m_FileRawData[RawPointer + i]);
					}
					else if (pHdr->ulRamAddr >= FlashLayoutCRCTable && pHdr->ulRamAddr+ulsize >= FlashLayoutCRCTable + 256 * sizeof(MemoryTable))
					{
						uint8_t *p = const_cast<uint8_t*>(GetMemoryContent(pHdr->ulRamAddr, pHdr->ulRamAddr + pHdr->ulBlockLen));
						if (p!=nullptr)
						{
							uint32_t j = pHdr->ulRamAddr + pHdr->ulBlockLen;
							for (uint32_t i = 0; i < (pHdr->ulBlockLen) / sizeof(pHdr->Argument); ++i)
							{
								bool equ = memcmp(&reinterpret_cast<uint32_t*>(p)[(pHdr->ulBlockLen) / sizeof(pHdr->Argument) - 1 - i], &pHdr->Argument, sizeof(pHdr->Argument)) == 0;
								if (equ)
								{
									j -= sizeof(pHdr->Argument);
								}
								else
								{
									break;
								}
							}

							TFlashHeader newheader = *pHdr;
							newheader.Argument = 0;
							newheader.ulBlockLen = j - pHdr->ulRamAddr;
							newheader.ulRamAddr = pHdr->ulRamAddr;
							newheader.usFlags &= ~BFLAG_FILL;
							CalcHeaderChecksum(&newheader);

							for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
								m_PatchedData.push_back(reinterpret_cast<uint8_t*>(&newheader)[i]);

							uint8_t *pp = const_cast<uint8_t*>(GetMemoryContent(newheader.ulRamAddr, newheader.ulRamAddr + newheader.ulBlockLen));
							for (size_t i = 0; i < newheader.ulBlockLen; ++i) {
								m_PatchedData.push_back(pp[i]);
							}

							TFlashHeader finalfillheader = *pHdr;
							finalfillheader.ulRamAddr = j;
							finalfillheader.ulBlockLen = pHdr->ulRamAddr + pHdr->ulBlockLen - j;
							CalcHeaderChecksum(&finalfillheader);

							for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
								m_PatchedData.push_back(reinterpret_cast<uint8_t*>(&finalfillheader)[i]);

						}
						else
						{
							std::cerr << "Invalid memory section. Unable to correct ldr file." << std::endl;
							retVal = false;
						}

					}
					else if (pHdr->ulRamAddr <= FlashLayoutCRCTable && pHdr->ulRamAddr + pHdr->ulBlockLen >= FlashLayoutCRCTable + 256 * sizeof(MemoryTable))//if the whole table is specified as a fill block we can patch it anyway
					{
						//split section
						uint8_t *p = const_cast<uint8_t*>(GetMemoryContent(pHdr->ulRamAddr, pHdr->ulRamAddr + pHdr->ulBlockLen));
						if (p!=nullptr)
						{
							for (uint32_t i = 0; i < (FlashLayoutCRCTable - pHdr->ulRamAddr) / sizeof(pHdr->Argument); ++i)
								retVal &= memcmp(&reinterpret_cast<uint32_t*>(p)[i], &pHdr->Argument, sizeof(pHdr->Argument))==0;

							if (retVal)
							{
								TFlashHeader header2modify = *pHdr;
								header2modify.ulBlockLen = FlashLayoutCRCTable - pHdr->ulRamAddr;
								uint8_t crcval = CalcHeaderChecksum(&header2modify);

								for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
									m_PatchedData.push_back(reinterpret_cast<uint8_t*>(&header2modify)[i]);

								uint32_t j = pHdr->ulRamAddr + pHdr->ulBlockLen;
								for (uint32_t i = 0; i < (FlashLayoutCRCTable - pHdr->ulRamAddr + pHdr->ulBlockLen) / sizeof(pHdr->Argument); ++i)
								{
									bool equ = memcmp(&reinterpret_cast<uint32_t*>(p)[(FlashLayoutCRCTable - pHdr->ulRamAddr + pHdr->ulBlockLen) / sizeof(pHdr->Argument) - 1 - i], &pHdr->Argument, sizeof(pHdr->Argument)) == 0;
									if (equ)
									{
										j -= sizeof(pHdr->Argument);
									}
									else
									{
										break;
									}
								}

								TFlashHeader newheader = header2modify;
								newheader.Argument = 0;
								newheader.ulBlockLen = j - FlashLayoutCRCTable;
								newheader.ulRamAddr = FlashLayoutCRCTable;
								newheader.usFlags &= ~BFLAG_FILL;
								CalcHeaderChecksum(&newheader);

								for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
									m_PatchedData.push_back(reinterpret_cast<uint8_t*>(&newheader)[i]);

								uint8_t *pp = const_cast<uint8_t*>(GetMemoryContent(newheader.ulRamAddr, newheader.ulRamAddr + newheader.ulBlockLen));
								for (size_t i = 0; i < newheader.ulBlockLen; ++i) {
									m_PatchedData.push_back(pp[i]);
								}
								TFlashHeader finalfillheader = *&header2modify;
								finalfillheader.ulRamAddr = j;
								finalfillheader.ulBlockLen = pHdr->ulRamAddr+pHdr->ulBlockLen - j;
								CalcHeaderChecksum(&finalfillheader);

								for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
									m_PatchedData.push_back(reinterpret_cast<uint8_t*>(&finalfillheader)[i]);
							}
							else
							{
								std::cerr << "Discrepancy in fill section unexpected. Can't patch file." << std::endl;
								retVal = false;
							}
						}
						else
						{
							std::cerr << "Invalid memory section. Unable to correct ldr file." << std::endl;
							retVal = false;
						}
					}
					else
					{
						std::cerr << "Corrupt fill block." << std::endl;
						retVal = false;
					}

					RawPointer += FLASHHEADER_SIZE;
					PrevHeaderId = -1;
				}
				else
				{
					bool addblock = true;

					if (!(pHdr->usFlags&BFLAG_IGNORE))
					{
						if (ulsize > 0)
						{
							addblock = CmpDataBlock(pucAddr, ulsize, &m_FileRawData[RawPointer + FLASHHEADER_SIZE]);
						}
						if (!addblock)
						{
							std::cout << "Correction required (data block)" << std::hex << "0x" << pucAddr << std::endl;
						}
					}
					PrevHeaderId = m_PatchedData.size();
					if (addblock)
					{
						for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
							m_PatchedData.push_back(m_FileRawData[RawPointer + i]);

						for (size_t i = 0; i < ulsize; ++i)
							m_PatchedData.push_back(m_FileRawData[RawPointer + FLASHHEADER_SIZE + i]);
					}
					else
					{
						for (size_t i = 0; i < sizeof(TFlashHeader); ++i)
							m_PatchedData.push_back(m_FileRawData[RawPointer + i]);
						const uint8_t *content = GetMemoryContent(pucAddr, pucAddr + ulsize);
						for (size_t i = 0; i < ulsize; ++i)
							m_PatchedData.push_back(content[i]);
					}
					RawPointer += FLASHHEADER_SIZE + pHdr->ulBlockLen;
				}
			}
			else
			{
				retVal = false;
			}
		} while (((pHdr->usFlags&BFLAG_FINAL) == 0) && retVal);
		if (!retVal)
		{
			m_PatchedData.clear();
		}
        else
        {
			DXEPointer = 0;
			retVal = CorrectApplicationHeaderStructure(appendinfoblock, appinfoaddress, DXEPointer);
        }
	}
	else
	{
		retVal = false;
	}
	return retVal;
}

//#define _DEBUG_
#ifdef _DEBUG_
unsigned char testdata[16777216];
#endif

bool CElfReader::RestructureSDRAM()
{
	/*uint8_t *dummy = m_SDRAM.data();
	dummy = &dummy[FlashLayoutLoc];
	memset(dummy, 0, sizeof(m_MemoryTable));*/
	return true;
}

uint8_t CElfReader::CalcHeaderChecksum(TFlashHeader *header)
{
	header->usFlags &= 0xFF00FFFF;
	uint8_t crc = 0x00;
	for (uint32_t i = 0; i < sizeof(TFlashHeader); ++i)
		crc ^= reinterpret_cast<uint8_t*>(header)[i];
	header->usFlags |= static_cast<uint32_t>(crc) << 16;
	/*uint8_t crcc = reinterpret_cast<uint8_t*>(header)[0];
	for (uint32_t i = 1; i < sizeof(TFlashHeader); ++i)
		crcc ^= reinterpret_cast<uint8_t*>(header)[i];*/

	return crc;
}

bool CElfReader::ExtractMemoryLayout(bool usestatevectoraddress, uint32_t statevectoraddress)
{
	bool retVal;
	//just for debugging
#ifdef _DEBUG_
	std::fstream f;
	f.open("Image.dat", std::ios_base::in);
	size_t count = 0;
	while (f.good()&&count<sizeof(testdata)/sizeof(testdata[0]))
	{
		unsigned char cNum[10];
		f.getline(reinterpret_cast<char*>(cNum), 10, '\n');
		testdata[count] = static_cast<unsigned char>(strtoul(reinterpret_cast<char*>(cNum),NULL, 16));
		count++;
	}
	f.close();
#endif
    std::cout << std::hex;
	if (m_SDRAM.size() >= FlashLayoutLoc + sizeof(m_MemoryTable) / sizeof(m_MemoryTable[0])-m_MemoryLayout[0].OffsetCompensation)
	{
		static const uint32_t LdfIdentifier_rel = LdfIdentifier - m_MemoryLayout[0].OffsetCompensation;
		static const uint32_t FlashLayoutLoc_rel = FlashLayoutLoc - m_MemoryLayout[0].OffsetCompensation;
		std::vector<MemoryTable> layout;
		uint8_t *dummy = m_SDRAM.data();
		std::string identifier = "CRCCheck Version 00.00.01/Build 1 Date:2018/06/06";
		std::string flashid(reinterpret_cast<const char*>(&dummy[LdfIdentifier_rel]), identifier.length());
		if (identifier == flashid)
		{
			MemoryTable *mem = reinterpret_cast<MemoryTable*>(&dummy[FlashLayoutLoc_rel]);
			while (mem != nullptr)
			{
				//just take the one that are really populated
				if (mem->stopaddress-mem->startaddress)
				{
					layout.push_back(*mem);
				}
				
				mem = reinterpret_cast<MemoryTable*>(&dummy[reinterpret_cast<uint32_t>(mem->m_NextTableEntry)-m_MemoryLayout[0].OffsetCompensation]);
				if (mem == reinterpret_cast<MemoryTable*>(&dummy[FlashLayoutLoc_rel]))
				{
					mem = nullptr;
				}
			}


			std::vector<MemoryTable> t;
			for (std::vector<MemoryTable>::iterator it = RegeneratedMemTable.begin(); it != RegeneratedMemTable.end(); ++it) {
				bool remove = false;
				uint32_t cas = 0;
				for (std::vector<MemoryTable>::reverse_iterator rit = RegeneratedMemTable.rbegin(); rit.base() - 1 != it; ++rit)
				{
					if (rit->stopaddress - rit->startaddress)//in this case it is a final block
					{
						if (it->startaddress >= rit->startaddress && it->startaddress < rit->stopaddress)
						{
							remove = true;
							cas = 1;
						}
						else if (it->stopaddress > rit->startaddress && it->stopaddress < rit->stopaddress)
						{
							remove = true;
							cas = 2;
						}
						else if (it->startaddress < rit->startaddress && it->stopaddress > rit->stopaddress)
						{
							remove = true;
							cas = 3;
						}
					}
				}

				if (remove)
				{
					std::cout << "Block: 0x" << it->startaddress << " 0x" << it->stopaddress << " removed (case " << cas << ")" << std::endl;
				}
				else
				{
					if (it->stopaddress - it->startaddress)
					{
						bool blockremoval = true;
						for (auto & value : layout)
						{
							if ((value.startaddress) <= it->startaddress&&it->stopaddress <= value.stopaddress)
							{
								t.push_back(*it);
								blockremoval = false;
								break;
							}
						}

						if (blockremoval)
						{
							uint32_t cas;
							for (auto & value : layout)
							{
								if ((value.startaddress) <= it->startaddress&&it->startaddress < value.stopaddress)
								{
									if (reinterpret_cast<long>(value.stopaddress) & 1)
									{
										std::cout << "Correcting stop address to an even number: 0x" << value.stopaddress << " 0x" << reinterpret_cast<uint16_t*>(reinterpret_cast<long>(value.stopaddress) & (-2)) << std::endl;
										value.stopaddress = reinterpret_cast<uint16_t*>(reinterpret_cast<long>(value.stopaddress) & (-2));
									}
									it->stopaddress = value.stopaddress;

									t.push_back(*it);
									blockremoval = false;
									cas = 1;
									break;
								}
								else if ((value.stopaddress) >= it->stopaddress&&it->stopaddress > value.startaddress)
								{
									if (reinterpret_cast<long>(value.startaddress) & 1)
									{
										std::cout << "Correcting start address to an even number: 0x" << value.startaddress << " 0x" << reinterpret_cast<uint16_t*>((reinterpret_cast<long>(value.startaddress) + 1) & (-2)) << std::endl;
										value.startaddress = reinterpret_cast<uint16_t*>((reinterpret_cast<long>(value.startaddress)+1) & (-2));
									}
									it->startaddress = value.startaddress;
									t.push_back(*it);
									blockremoval = false;
									cas = 2;
									break;
								}
								else if (it->startaddress<value.startaddress&&it->stopaddress>=value.stopaddress)
								{
									if (reinterpret_cast<long>(value.stopaddress) & 1)
									{
										std::cout << "Correcting stop address to an even number: 0x" << value.stopaddress << " 0x" << reinterpret_cast<uint16_t*>(reinterpret_cast<long>(value.stopaddress) & (-2)) << std::endl;
										value.stopaddress = reinterpret_cast<uint16_t*>(reinterpret_cast<long>(value.stopaddress) & (-2));
									}
									if (reinterpret_cast<long>(value.startaddress) & 1)
									{
										std::cout << "Correcting start address to an even number: 0x" << value.startaddress << " 0x" << reinterpret_cast<uint16_t*>((reinterpret_cast<long>(value.startaddress) + 1) & (-2)) << std::endl;
										value.startaddress = reinterpret_cast<uint16_t*>((reinterpret_cast<long>(value.startaddress) + 1) & (-2));
									}
									it->startaddress = value.startaddress;
									it->stopaddress = value.stopaddress;
									t.push_back(*it);
									blockremoval = false;
									cas = 3;
									break;
								}
							}

							if (blockremoval)
							{
								std::cout << "Block: 0x" << it->startaddress << " 0x" << it->stopaddress << " removed (outside boundaries)" << std::endl;
							}
							else
							{
								std::cout << "Block range adjusted: 0x" << it->startaddress << " 0x" << it->stopaddress << " (case " << cas << ")" << std::endl;
							}
						}

					}
					else
					{
						std::cout << "Block: 0x" << it->startaddress << " removed (it is an empty block)" << std::endl;
					}
				}
			}

			std::cout << "Overall number of blocks " << std::dec << RegeneratedMemTable.size() << "(dec)" << std::endl;
			RegeneratedMemTable = t;
			std::cout << "Number of blocks after removal " << std::dec << RegeneratedMemTable.size() << "(dec)" << std::endl;
			std::cout << std::hex;


			if (!usestatevectoraddress)
			{
				statevectoraddress = reinterpret_cast<uint32_t>(reinterpret_cast<MemoryTable*>(&dummy[FlashLayoutCRCTable-m_MemoryLayout[0].OffsetCompensation])->m_pu16CRCState);
				//check range
				if (statevectoraddress >= IgnoreSDRAMLower && statevectoraddress <= IgnoreSDRAMUpper)
				{
					std::cout << "Statevectoraddress set to: 0x" << statevectoraddress << std::endl;
				}
				else
				{
					std::cout << "Invalid statevectoraddress" << std::endl;
				}
			}
			else
			{
                uint32_t statevectoraddress_ = statevectoraddress;
                statevectoraddress = reinterpret_cast<uint32_t>(reinterpret_cast<MemoryTable*>(&dummy[statevectoraddress - m_MemoryLayout[0].OffsetCompensation])->m_pu16CRCState);
                //check range
                if (statevectoraddress >= IgnoreSDRAMLower && statevectoraddress <= IgnoreSDRAMUpper)
                {
                    std::cout << "Statevectoraddress set to: 0x" << statevectoraddress << std::endl;
                }
                else
                {
                    std::cout << "Invalid statevectoraddress" << std::endl;
                }
				std::cout << "Statevectoraddress set to: 0x" << statevectoraddress_ << " virtual_address: 0x" << statevectoraddress << std::endl;
			}

			uint32_t blockno = 1;
			uint32_t crcix = 0;
			uint32_t sizechecked = 0;
			retVal = true;

			for (auto& value : RegeneratedMemTable) {
				//std::cout << "Block Number: " << std::hex << std::setfill('0') << std::setw(2) << blockno++ << " " << "Block start: 0x" << value.startaddress << " " << "Block stop: 0x" << value.stopaddress << std::endl;
				const uint8_t* d = GetMemoryContent(reinterpret_cast<uint32_t>(value.startaddress), reinterpret_cast<uint32_t>(value.stopaddress));
				if (d != nullptr)
				{
					value.m_u16CRC = g_CalcCrcSum(CRCSeed, (value.stopaddress - value.startaddress) * sizeof(*value.startaddress), d);
					std::string bstat = value.m_bDMAAccess == false ? "false" : "true";
					if (blockno)
					{
						std::cout << "{ " << "(uint16*)0x0" << value.startaddress << ", " << "(uint16*)0x0" << value.stopaddress << ", " << "0x0" << value.m_u16CRC << ", &m_astMemDescriptor[0x0" << blockno++ << "], " << bstat << ", " << "&m_au16CRCState[0x0" << crcix++ << "]},\t/* Length=0x" << value.stopaddress - value.startaddress << "*/" << std::endl;
						if (reinterpret_cast<uint32_t>(value.startaddress) % 2)
						{
							std::cerr << "Invalid block start" << std::endl;
							retVal = false;
						}
					}
					else
					{
						std::cout << "{ " << "(uint16*)0x0" << value.startaddress << ", " << "(uint16*)0x0" << value.stopaddress << ", " << "0x0" << value.m_u16CRC << ", &m_astMemDescriptor[0x0" << blockno++ << "], " << bstat << ", " << "&m_au16CRCState[0x0" << crcix++ << "]}\t/* Length=0x" << value.stopaddress - value.startaddress << "*/" << std::endl;
						if (reinterpret_cast<uint32_t>(value.startaddress) % 2)
						{
							std::cerr << "Invalid block start" << std::endl;
							retVal = false;
						}
					}

					//update memory content
					MemoryTable* mem = reinterpret_cast<MemoryTable*>(&dummy[FlashLayoutCRCTable - m_MemoryLayout[0].OffsetCompensation]);
					size_t i = 0;
					for (auto& value : RegeneratedMemTable) {
						value.m_pu16CRCState = &reinterpret_cast<uint16_t*>(statevectoraddress)[i];
						mem[i++] = value;
					}

					if (i > 0)
					{
						const uint32_t tablesize = sizeof(MemoryTable);
						mem[--i].m_NextTableEntry = reinterpret_cast<MemoryTable*>(FlashLayoutCRCTable);
						while (i > 0)
						{
							uint32_t address = FlashLayoutCRCTable + i * sizeof(MemoryTable);
							mem[i - 1].m_NextTableEntry = reinterpret_cast<MemoryTable*>(address);
							--i;
						}

					}
					else
					{
						mem[0].m_bDMAAccess = false;
						mem[0].m_NextTableEntry = reinterpret_cast<MemoryTable*>(FlashLayoutCRCTable);
						mem[0].m_pu16CRCState = 0;
						mem[0].startaddress = 0;
						mem[0].stopaddress = 0;
						mem[0].m_u16CRC = 0xFFFF;
						std::cout << "No section found. Deactivate CRC checking." << std::endl;
					}
#ifdef _DEBUG_
					if (reinterpret_cast<uint32_t>(value.startaddress) >= 0 && reinterpret_cast<uint32_t>(value.stopaddress) <= SDRAMSize)
					{
						volatile uint16_t benchmark = g_CalcCrcSum(CRCSeed, (value.stopaddress - value.startaddress) * sizeof(*value.startaddress), &testdata[reinterpret_cast<uint32_t>(value.startaddress)]);
						int deviation = 0;
						for (int j = 0; j < value.stopaddress - value.startaddress; j++)
						{
							if (d[j] != testdata[j + reinterpret_cast<uint32_t>(value.startaddress)])
							{
								std::cout << "Discrepancy at address: "  "0x0" << std::hex << reinterpret_cast<uint32_t>(value.startaddress) + j << " target: " << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32>(testdata[reinterpret_cast<uint32_t>(value.startaddress) + j]) << " ldr: " << "0x" << std::hex << std::setfill('0') << std::setw(2) << static_cast<uint32>(d[j]) << std::endl;
								deviation++;
							}
						}
						if (deviation > 0) {
							std::cout << "Block: " << "0x0" << std::hex << value.startaddress << " deviates in " << std::hex << deviation << " bytes" << "CRC is: 0x" << benchmark << std::endl;
						}
					}
#endif
					sizechecked += (value.stopaddress - value.startaddress) * sizeof(*value.startaddress);
					if (blockno == RegeneratedMemTable.size())
					{
						blockno = 0;
					}
				}
				else
				{
					retVal = false;
					break;
				}
			}

			if (retVal)
			{
				std::cout << std::dec << "Length of stream: " << m_StreamLength << "(dec) Bytes " << "Code/const data size: " << sizechecked << "(dec) " << "Percentage of stream being checked= " << 100.0 * sizechecked / m_StreamLength << " %" << std::endl;
				if (RegeneratedMemTable.size() <= sizeof(m_MemoryTable) / sizeof(m_MemoryTable[0]))
				{
					std::cout << std::dec << "No. of CRC vector table entries: " << RegeneratedMemTable.size() << " out of " << sizeof(m_MemoryTable) / sizeof(m_MemoryTable[0]) << ". Remaining table size: " << (sizeof(m_MemoryTable) / sizeof(m_MemoryTable[0])) - RegeneratedMemTable.size() << " entries." << std::endl;
				}
				else
				{
					std::cerr << std::dec << "Error. Number of table entries: " << RegeneratedMemTable.size() << " out of " << sizeof(m_MemoryTable) / sizeof(m_MemoryTable[0]) << ". Insufficient table space." << std::endl;
					retVal = false;
				}
			}
			else
			{
				std::cerr << "Invalid flash file. Unable to resolve memory sections." << std::endl;
				retVal = false;
			}
		}
		else
		{
			std::cerr << "Invalid file. ldf file doesn't contain a valid identifier." << std::endl;
			retVal = false;
		}
	}
	else
	{
		std::cerr << "Invalid file. ldf file does not contain valid CRC table section." << std::endl;
		retVal = false;
	}
	return retVal;
}

bool CElfReader::OpenLdrFile(std::string existingldr)
{
	if (existingldr.length())
	{
		std::fstream elffile;
		std::streampos begin, end;
		m_Merger.clear();
		elffile.open(existingldr, std::ios::binary | std::ios_base::in | std::ios_base::out);
		if (elffile.is_open())
		{
			begin = elffile.tellg();
			elffile.seekg(0, std::ios::end);
			end = elffile.tellg();
			elffile.seekg(0, std::ios::beg);
			std::size_t filesize = static_cast<std::size_t>(end - begin);
			char *dummy = new char[filesize];
			if (dummy)
			{
				elffile.read(dummy, filesize);
				if (elffile)
				{
					if (CIntelHexMerger::Convert(filesize, dummy, m_Merger))
					{
						eElfStatus = ELF_OK;
					}
					else
					{
						eElfStatus = ELF_INVALID;
					}
				}
				else
				{
					eElfStatus = FILEINCOMPLETE;
				}
				/*for (size_t i = 0; i < filesize; ++i)
				{
				m_FileRawData.push_back(dummy[i]);
				}*/
				delete[] dummy;
				eElfStatus = ELF_OK;
			}
			else
			{
				eElfStatus = OUTOFMEMORY;
			}
		}
		else
		{
			//			throw std::ios::bad;
			eElfStatus = UNABLEOPENFILE;
		}
	}
	return true;
}

uint8_t CElfReader::CalcCRC(uint8_t size, uint16_t address, uint8_t type, uint8_t *data)
{
	uint8_t crc = size + static_cast<uint8_t>(address & 0xFF) + static_cast<uint8_t>(address >> 8) + type;
	for (uint32_t i = 0; i < size; ++i)
		crc += data[i];
	return (~crc) + 1;
}

std::string CElfReader::SetExtendedAddress(uint32_t address)
{
	std::stringstream stream;
	uint8_t bytecount = 0x2;	//length
	uint8_t datatype = 0x4;		//Extended Linear Address
	uint32_t addresssection = address >> 16;
	uint8_t crc = CalcCRC(bytecount, static_cast<uint16_t>(address & 0xFFFFu), datatype, reinterpret_cast<uint8_t*>(&addresssection));
	stream << ":" 
		<< std::hex 
		<< std::uppercase 
		<< std::setfill('0') 
		<< std::setw(2) 
		<< static_cast<uint32_t>(bytecount) 
		<< std::setw(4) 
		<< static_cast<uint16_t>(address & 0xFFFF)
		<< std::setw(2) 
		<< static_cast<uint32_t>(datatype) 
		<< std::setw(4) 
		<< addresssection 
		<< std::setw(2) 
		<< static_cast<uint32_t>(crc) 
		<< std::endl;
	return stream.str();
}

bool CElfReader::Merge(std::string patcheldrfile, uint32_t baseaddress)
{
	uint32_t i, j;
	uint32_t base = baseaddress;
	j = baseaddress;

	std::fstream elffile;
	std::streampos begin, end;
	elffile.open(patcheldrfile, std::ios_base::out | std::ios_base::trunc);

	uint32_t linecount = 0;
	bool retVal = true;
	if (elffile.is_open())
	{
		std::vector<uint8_t>::iterator iter = m_PatchedData.begin();
		uint32_t addresscounter = base;

		//the end? Then our task is quite simple (we are done) 
		if (iter != m_PatchedData.end())
		{
			uint32 mask = 0xFFFF0000u;
			std::string dummy = SetExtendedAddress(base&mask);
			elffile << dummy;
			linecount++;
		}

		//that is special at the beginning. We start with zero (that is the reason why it is outside the loop)
		while (iter!=m_PatchedData.end()){
			
				uint8_t buffer[32];
				uint32_t offset = 0;
				for (i = 0; i < sizeof(TFlashHeader); ++i)
				{
					buffer[i] = *iter++;
					j++;
					if (!(j & 0xFFFF))
					{
						std::string dummy = SetExtendedAddress(j);

						const uint8_t bytecount = i+1;	//length
						const uint8_t datatype = 0x0;	//Extended Linear Address
						uint8_t crc = CalcCRC(bytecount, static_cast<uint16_t>(addresscounter & 0xFFFF), datatype, reinterpret_cast<uint8_t*>(&buffer));
						elffile << ":" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(bytecount) << std::setw(4) << static_cast<uint16_t>(addresscounter & 0xFFFF) << std::setw(2) << static_cast<uint32_t>(datatype);
						for (uint32_t k = 0; k <= i; k++)
						{
							elffile << std::hex << std::uppercase << std::setw(2) << static_cast<uint32_t>(buffer[k]);
						}
						elffile << std::hex << std::uppercase << std::setw(2) << static_cast<uint32_t>(crc) << std::endl;
						linecount++;

						elffile << dummy;
						linecount++;
						offset = i+1;
						addresscounter += i+1;
					}
				}

				uint32_t blocksize = reinterpret_cast<TFlashHeader*>(buffer)->ulBlockLen;
				uint32_t totalsize = blocksize + sizeof(TFlashHeader);

				if (!(reinterpret_cast<TFlashHeader*>(buffer)->usFlags&BFLAG_FILL))
				{
					//are there additional bytes to write?
					if (totalsize != offset)
					{
						bool bSetExtendedAddress = false;
						do
						{
							bool exitwhile = false;
							while (i < totalsize&&i < 0x20 && !exitwhile)
							{
								reinterpret_cast<uint8_t*>(buffer)[i++] = *iter++;
								j++;
								exitwhile = (j & 0xFFFFu) == 0;
							}

							const uint8_t bytecount = i - offset;	//length
							const uint8_t datatype = 0x0;	//Extended Linear Address
							uint8_t crc = CalcCRC(bytecount, static_cast<uint16_t>(addresscounter & 0xFFFF), datatype, reinterpret_cast<uint8_t*>(&buffer[offset]));
							elffile << ":" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(bytecount) << std::setw(4) << static_cast<uint16_t>(addresscounter & 0xFFFF) << std::setw(2) << static_cast<uint32_t>(datatype);
							for (uint32_t k = offset; k < i; k++)
							{
								elffile << std::hex << std::uppercase << std::setw(2) << static_cast<uint32_t>(buffer[k]);
							}
							elffile << std::hex << std::uppercase << std::setw(2) << static_cast<uint32_t>(crc) << std::endl;
							addresscounter += i - offset;
							totalsize -= i;
							i = 0;
							linecount++;
							if (!(j & 0xFFFF) && i < totalsize)
							{
								std::string dummy = SetExtendedAddress(addresscounter);
								elffile << dummy;
								linecount++;
								bSetExtendedAddress = true;
							}
							else
							{
								bSetExtendedAddress = false;
							}
							offset = 0;
						} while (totalsize > 0);
						
						//the last byte written was a SetExtendedAddress -> clear to continue
						if (bSetExtendedAddress)
							continue;
					}
					else
					{
						//the last byte written was a SetExtendedAddress -> clear to continue
						continue;
					}
				}
				else
				{
					//standard fill block
					static const uint32_t headerblocksize = sizeof(TFlashHeader);
					uint8_t bytecount = headerblocksize - offset;	//length
					if (bytecount)
					{
						uint8_t datatype = 0x00;			//Extended Linear Address

						uint8_t crc = CalcCRC(bytecount, static_cast<uint16_t>(addresscounter & 0xFFFF), datatype, reinterpret_cast<uint8_t*>(&buffer[offset]));
						elffile << ":" << std::hex << std::uppercase << std::setfill('0') << std::setw(2) << static_cast<uint32_t>(bytecount) << std::setw(4) << static_cast<uint16_t>(addresscounter & 0xFFFF) << std::setw(2) << static_cast<uint32_t>(datatype);
						for (uint32_t k = offset; k < i; k++)
						{
							elffile << std::hex << std::uppercase << std::setw(2) << static_cast<uint32_t>(buffer[k]);
						}
						elffile << std::hex << std::uppercase << std::setw(2) << static_cast<uint32_t>(crc) << std::endl;

						addresscounter += i - offset;

						if (offset)
						{
							std::cout << "Fill Block at boundary condition." << std::endl;
						}
					}
					else
					{
						//iterator has already been incremented
						//the last action was flushing the buffer -> we are done
						//as SetExtendedAddress(addresscounter); was already being called
						continue;
					}
				}

				//for all cases - is j zero and are there additional blocks?
				if (!(j & 0xFFFF) && (iter+1)!=m_PatchedData.end())
				{
					std::string dummy = SetExtendedAddress(addresscounter);
					elffile << dummy;
				}
		}
		elffile << ":00000001FF" << std::endl;
	}
	else
	{
		eElfStatus = ELF_DESTFILEINVALID;
		retVal = false;
	}
	return retVal;
}

uint32_t CElfReader::FindBlock(uint32_t address)
{
	uint32_t retVal=-1;
	if (eElfStatus == ELF_OK)
	{
		TFlashHeader *pHdr;
		uint32_t RawPointer = 0;
		do {
			pHdr = reinterpret_cast<TFlashHeader*>(&m_FileRawData[RawPointer]);
			if ((((pHdr->usFlags&BK_ID) >> 24) == BK_THIS_ID) && CheckHeader(pHdr))
			{
				uint32_t pucAddr;
				uint32_t ulsize;
				uint32_t RRawPointer;
				if (pHdr->usFlags&BFLAG_FILL)
				{
					pucAddr = pHdr->ulRamAddr;
					ulsize = pHdr->ulBlockLen;
					RRawPointer = RawPointer;
					RawPointer += FLASHHEADER_SIZE;
				}
				else
				{
					pucAddr = pHdr->ulRamAddr;
					ulsize = pHdr->ulBlockLen;
					RRawPointer = RawPointer+FLASHHEADER_SIZE;
					RawPointer += FLASHHEADER_SIZE + pHdr->ulBlockLen;
				}

				if ((pHdr->usFlags&BFLAG_IGNORE) == 0x00)
				{
					if (address >= pucAddr && address < pucAddr + pHdr->ulBlockLen)
					{
						retVal = RRawPointer+address-pHdr->ulRamAddr;
					}
				}
			}
		} while (((pHdr->usFlags&BFLAG_FINAL) == 0));
	}
	else
	{
		retVal = -1;
	}
	return retVal;
}

std::vector<CIntelHexMerger*> CElfReader::FindBlockinLdr(uint32_t startaddress, uint32_t stopaddress)
{
	std::vector<CIntelHexMerger*> j;
	uint32_t start=0, stop=0;
	uint32_t prevrecordlength;
	for (auto &i : m_Merger)
	{
		if (!i.HexLine.DataType)
		{
			stop += i.HexLine.RecordLength;
			if (start+i.HexLine.RecordLength-1 >= startaddress && stop < stopaddress+i.HexLine.RecordLength-1)
			{
				j.push_back(&i);
			}
			prevrecordlength = i.HexLine.RecordLength;
			start = stop;
		}
	}
	return j;
}

bool CElfReader::SimulateExtraction(std::vector<uint8_t> rawdata)
{
	bool retVal;
	if (eElfStatus == ELF_OK)
	{
		TFlashHeader *pHdr;
		uint32_t RawPointer = 0;
		retVal = true;
		do {
			pHdr = reinterpret_cast<TFlashHeader*>(&rawdata[RawPointer]);
			if ((((pHdr->usFlags&BK_ID) >> 24) == BK_THIS_ID) && CheckHeader(pHdr))
			{
				if (pHdr->usFlags&BFLAG_FILL)
				{
					uint32_t pucAddr = pHdr->ulRamAddr;
					uint32_t ulsize = pHdr->ulBlockLen;
					std::cout << "Verifying fill block\tAddress: 0x" << std::hex << pucAddr << "\tSize: 0x" << std::hex << ulsize << std::endl;

					if (!(pHdr->usFlags&BFLAG_IGNORE))
						retVal &= CmpFillBlock(pucAddr, ulsize, pHdr->Argument);

					RawPointer += FLASHHEADER_SIZE;
				}
				else
				{
					uint32_t pucAddr = pHdr->ulRamAddr;
					uint32_t RawPointerAdd = RawPointer + FLASHHEADER_SIZE;
					uint32_t ulsize = pHdr->ulBlockLen;
					std::cout << "Verifying code/data block\tAddress: 0x" << std::hex << pucAddr << "\tSize: 0x" << std::hex << ulsize << std::endl;

					if (!(pHdr->usFlags&BFLAG_IGNORE)&&ulsize>0)
						retVal &= CmpDataBlock(pucAddr, ulsize, &rawdata[RawPointer + FLASHHEADER_SIZE]);
					
					RawPointer += FLASHHEADER_SIZE + pHdr->ulBlockLen;
				}

				if ((pHdr->usFlags&(BFLAG_INIT | BFLAG_IGNORE)) == BFLAG_INIT)
				{
					std::cout << "Simulate Execute Init" << std::endl;
				}

				if ((pHdr->usFlags&(BFLAG_CALLBACK | BFLAG_IGNORE)) == BFLAG_CALLBACK)
				{
					std::cout << "Simulate callback execution" << std::endl;
				}
			}
			else
			{
				retVal = false;
				std::cerr << "Abnormal header block." << std::endl;
			}
		} while (((pHdr->usFlags&BFLAG_FINAL) == 0) && retVal);
	}
	else
	{
		retVal = false;
	}
	return retVal;
}

std::string CElfReader::GetFlagAsText(uint32_t flags) const
{
	struct Flag2Text
	{
		uint32 flag;
		std::string text;
	};
	static const Flag2Text FlagEntries[] =
	{
		{BFLAG_DMACODE, "DMA config"},
		{BFLAG_FINAL, "Final"},
		{BFLAG_FIRST, "First"},
		{BFLAG_INDIRECT, "Indirect"},
		{BFLAG_IGNORE, "Ignore"},
		{BFLAG_INIT, "Init"},
		{BFLAG_CALLBACK, "Callback"},
		{BFLAG_QUICKBOOT, "FQuickboot"},
		{BFLAG_FILL, "Fill"},
		{BFLAG_AUX, "Aux"},
		{BFLAG_SAVE, "Save"}
	};

	std::string dummy;
	for (auto& i : FlagEntries)
	{
		if (flags & i.flag)
		{
			if (dummy.size())
			{
				dummy += " | " + i.text;
			}
			else
			{
				dummy = i.text;
			}
		}
	}
	if (dummy.size() == 0u)
	{
		dummy = "Data";
	}

	return dummy;
}

void CElfReader::PrintNextApplicationHeader(TFlashHeader const* pHeader, size_t address) const
{
	std::cout << "FIRST APPLICATION BLOCK" << " @ " << std::hex << "0x0" << address << std::hex << "(Next @ 0x0" << address + pHeader->Argument << std::endl <<
		"\t" << "Flags:\t" << GetFlagAsText(pHeader->usFlags) << std::endl <<
		"\t" << "BlockLength:\t" << std::hex << "0x0" << pHeader->ulBlockLen << " / " << std::dec << "(" << pHeader->ulBlockLen << ")" << std::endl <<
		"\t" << "Address:\t" << std::hex << "0x0" << pHeader->ulRamAddr << std::endl <<
		"\t" << "Argument:\t" << std::hex << "0x0" << pHeader->Argument << std::endl;
}

void CElfReader::PrintHeader(TFlashHeader const* pHeader, size_t address) const
{
	size_t next = address + sizeof(TFlashHeader);
	if (!(pHeader->usFlags & BFLAG_FILL))
	{
		next += pHeader->ulBlockLen;
	}

	std::cout << "STANDARDHEADER" << " @ " << std::hex << "0x0" << address << "(Next @ 0x0" << std::hex << next << ")" << std::endl <<
		"\t\t" << "Flags:\t" << GetFlagAsText(pHeader->usFlags) << std::endl <<
		"\t\t" << "BlockLength:\t" << std::hex << "0x0" << pHeader->ulBlockLen << " / " << std::dec << "(" << pHeader->ulBlockLen << ")" << std::endl <<
		"\t\t" << "Address:\t" << std::hex << "0x0" << pHeader->ulRamAddr << std::endl <<
		"\t\t" << "Argument:\t" << std::hex << "0x0" << pHeader->Argument << std::endl;

}

bool CElfReader::PrintFileTree(bool patchedfile) const
{
	const std::vector<uint8_t>& surrogate = patchedfile ? m_PatchedData : m_FileRawData;
	std::vector<uint8_t>::const_iterator pRaw = surrogate.cbegin();

	while (std::distance(surrogate.begin(), pRaw) + sizeof(TFlashHeader) < surrogate.size())
	{
		TFlashHeader const* pHeader = reinterpret_cast<TFlashHeader const*>(&pRaw[0]);
		if (pHeader->usFlags & BFLAG_FIRST)
		{
			PrintNextApplicationHeader(pHeader, std::distance(surrogate.begin(), pRaw));
		}
		else
		{
			PrintHeader(pHeader, std::distance(surrogate.begin(), pRaw));
		}
		//if (pHeader->usFlags & BFLAG_FINAL)
			//break;

		pRaw += sizeof(TFlashHeader);

		if (!(pHeader->usFlags & BFLAG_FILL))
		{
			pRaw += pHeader->ulBlockLen;
		}
	}
	return true;
}

bool CElfReader::CheckIntegrity(std::string filename)
{
	bool retVal = false;
	if (filename.length()&& m_FileRawData.size())
	{
		std::vector<uint8_t> m_RawData;
		m_RawData.reserve(m_FileRawData.size());
		m_RawData.clear();

		std::fstream elffile;
		std::streampos begin, end;
		elffile.open(filename, std::ios::binary | std::ios_base::in | std::ios_base::out);
		if (elffile.is_open())
		{
			begin = elffile.tellg();
			elffile.seekg(0, std::ios::end);
			end = elffile.tellg();
			elffile.seekg(0, std::ios::beg);
			std::size_t filesize = static_cast<std::size_t>(end - begin);
			m_FileRawData.clear();
			m_FileRawData.reserve(filesize);
			char *dummy = new char[filesize];
			if (dummy)
			{
				elffile.read(dummy, filesize);
				if (elffile)
				{
					if (CIntelHexConverter::Convert(filesize, dummy, m_RawData))
					{
						eElfStatus = ELF_OK;
						retVal = SimulateExtraction(m_RawData);
					}
					else
					{
						eElfStatus = ELF_INVALID;
					}
				}
				else
				{
					eElfStatus = FILEINCOMPLETE;
				}
				/*for (size_t i = 0; i < filesize; ++i)
				{
				m_FileRawData.push_back(dummy[i]);
				}*/
				delete[] dummy;
				eElfStatus = ELF_OK;
			}
			else
			{
				eElfStatus = OUTOFMEMORY;
			}
		}
		else
		{
			//			throw std::ios::bad;
			eElfStatus = UNABLEOPENFILE;
		}
	}
	return retVal;
}

bool CIntelHexConverter::Convert(size_t length, char *datain, std::vector<uint8_t> &dataout)
{
	std::string data = datain;
	std::size_t startpos = data.find_first_of(":");
	std::size_t endpos = data.find_first_of("\n\r", startpos);
	int32_t addressoffset = 0;
	int32_t streamlength = 0;
	bool retVal = true;
	while ((startpos!=std::string::npos)&&(endpos!=std::string::npos))
	{
		std::string line = data.substr(startpos, endpos-startpos);
		std::string mark = line.substr(0, 1);
		std::string reclen = line.substr(1, 2);
		size_t recordlength = strtoul(reclen.c_str(),NULL,16);
		std::string address = line.substr(3, 4);
		std::string settype = line.substr(7, 2);
		std::string datarec = line.substr(9,2*recordlength);
		std::string crc = line.substr(9+2*recordlength,2);
		unsigned char checksum = static_cast<unsigned char>(strtoul(crc.c_str(),NULL, 16));
		unsigned char crosscheck=0;
		for (size_t i = 0; i < recordlength+4; ++i)
		{		
			std::string dummy = line.substr(2 * i + 1, 2);
			crosscheck += static_cast<unsigned char>(strtoul(dummy.c_str(), NULL, 16));
		}
		unsigned char test = (~crosscheck) + 1;
		retVal &= !(static_cast<unsigned char>((~crosscheck) + 1) ^ checksum);
		if (!retVal)
		{
			std::cerr << "CRC error" << "(File: " << __FILE__ << " Line: " << __LINE__ << ")." << std::endl;
		}
		unsigned char cdatatype = static_cast<unsigned char>(strtoul(settype.c_str(), NULL, 16));
		switch (cdatatype)
		{
			case 0:
				for (size_t i = 0; i < recordlength; ++i)
				{
					std::string dummy = datarec.substr(2 * i, 2);
					dataout.push_back(static_cast<unsigned char>(strtoul(dummy.c_str(), NULL, 16)));
				}
				streamlength += recordlength;
				break;
			case 1:
				std::cout << "End of file (Streamlength: " << std::dec << streamlength << "(dec))" << std::endl;
				break;
			case 2:
				addressoffset = static_cast<size_t>(strtoul(datarec.c_str(), NULL, 16) * 16);
				std::cout << "Start Segment (16):" << std::hex << "0x" << addressoffset << std::endl;
				break;
			case 3:
				std::cout << "Blocktype 3 not supported" << std::endl;
				break;
			case 4:
				addressoffset = static_cast<size_t>(strtoul(datarec.c_str(), NULL, 16) * 65536);
				std::cout << "Start Segment (32):" << std::hex << "0x" << addressoffset << std::endl;
				break;
			case 5:
				std::cout << "Blocktype 5 not supported" << std::endl;
				break;
			default:
				retVal = false;
				break;
		}

		startpos = data.find_first_of(":", endpos);
		endpos = data.find_first_of("\n\r", startpos);
	}
	return retVal;
}

bool CIntelHexMerger::Convert(size_t length, char *datain, std::vector<CIntelHexMerger> &dataout)
{
	std::string data = datain;
	std::size_t startpos = data.find_first_of(":");
	std::size_t endpos = data.find_first_of("\n\r", startpos);
	int32_t addressoffset = 0;
	bool retVal = true;
	while ((startpos != std::string::npos) && (endpos != std::string::npos))
	{
		std::string line = data.substr(startpos, endpos - startpos);
		std::string mark = line.substr(0, 1);
		std::string reclen = line.substr(1, 2);
		size_t recordlength = strtoul(reclen.c_str(), NULL, 16);
		std::string address = line.substr(3, 4);
		std::string settype = line.substr(7, 2);
		std::string datarec = line.substr(9, 2 * recordlength);
		std::string crc = line.substr(9 + 2 * recordlength, 2);
		unsigned char checksum = static_cast<unsigned char>(strtoul(crc.c_str(), NULL, 16));
		unsigned char crosscheck = 0;
		for (size_t i = 0; i < recordlength + 4; ++i)
		{
			std::string dummy = line.substr(2 * i + 1, 2);
			crosscheck += static_cast<unsigned char>(strtoul(dummy.c_str(), NULL, 16));
		}
		unsigned char test = (~crosscheck) + 1;
		retVal &= !(static_cast<unsigned char>((~crosscheck) + 1) ^ checksum);
		if (!retVal)
		{
			std::cerr << "CRC error" << "(File: " << __FILE__ << " Line: " << __LINE__ << ")." << std::endl;
		}
		unsigned char cdatatype = static_cast<unsigned char>(strtoul(settype.c_str(), NULL, 16));
		CIntelHexMerger merge;
		merge.HexLine.cDataLine = line;
		merge.HexLine.RecordMark = static_cast<uint8_t>(mark[0]);
		merge.HexLine.RecordLength = static_cast<uint8_t>(strtoul(reclen.c_str(), NULL, 16));
		merge.HexLine.RecordOffset = static_cast<uint16_t>(strtoul(address.c_str(), NULL, 16))+addressoffset;
		merge.HexLine.RecordType = static_cast<uint8_t>(strtoul(settype.c_str(), NULL, 16));
		merge.HexLine.CRC = checksum;
		merge.HexLine.DataType = cdatatype;
		for (size_t i = 0; i < recordlength; ++i)
		{
			std::string dummy = datarec.substr(2 * i, 2);
			merge.HexLine.u8DataLine.push_back(static_cast<unsigned char>(strtoul(dummy.c_str(), NULL, 16)));
		}

		dataout.push_back(merge);
		switch (cdatatype)
		{
			case 2:
				addressoffset = static_cast<size_t>(strtoul(datarec.c_str(), NULL, 16) * 16);
				break;
			case 4:
				addressoffset = static_cast<size_t>(strtoul(datarec.c_str(), NULL, 16) * 65536);
				break;
			default:
				break;
		}
		startpos = data.find_first_of(":", endpos);
		endpos = data.find_first_of("\n\r", startpos);
	}
	return retVal;
}
}//end namespace V304