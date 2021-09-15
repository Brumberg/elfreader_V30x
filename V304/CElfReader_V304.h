#pragma once
#include <string>
#include <vector>
#include <cstdint>
namespace V304
{
	class CIntelHexConverter
	{
	private:
		struct IntelHexHeader {
			uint8_t RecordMark;
			uint8_t RecordLength;
			uint16_t RecordOffset;
			uint8_t RecordType;
			uint8_t DataType;
		};
	public:
		static bool Convert(size_t length, char* datain, std::vector<uint8_t>& dataout);
	};

	class CIntelHexMerger
	{
	public:
		struct IntelHexLine {
			uint8_t RecordMark;
			uint8_t RecordLength;
			uint32_t RecordOffset;
			uint8_t RecordType;
			uint8_t DataType;
			uint8_t CRC;
			std::string cDataLine;
			std::vector<uint8_t> u8DataLine;
		}HexLine;
	public:
		static bool Convert(size_t length, char* datain, std::vector<CIntelHexMerger>& dataout);
	};

	class CElfReader
	{
	public:
		enum ElfStatus { UNINITIALIZED, INVALIDFILENAME, UNABLEOPENFILE, OUTOFMEMORY, FILEINCOMPLETE, ELF_INVALID, ELF_DESTFILEINVALID, ELF_OK };
		enum BlockType { NORMAL, FILL };
	private:
		static const uint32_t SDRAMSize = 33554432;
		//static const uint32_t MultidimensionalCRC = 12;
		//static const uint32_t LayoutFlashSize = 32;
		//static const uint32_t MultidimensionalCRCArraySize = 4;
		static const uint32_t LdfIdentifier = 0x81ff8000;
		static const uint32_t FlashLayoutLoc = 0x81ff8100;
		static const uint32_t FlashLayoutCRCTable = 0x81ff8400;
		static const uint16_t CRCSeed = 0xFFFF;
		static const uint32_t IgnoreSDRAMLower = 0x81ff8000;
		static const uint32_t IgnoreSDRAMUpper = 0x81ffffff;
		/** Structure of block headers in flash memory */
		struct TFlashHeader
		{
			uint32_t usFlags;
			uint32_t ulRamAddr;   /**< Start address of block in RAM */
			uint32_t ulBlockLen;  /**< Block length in bytes */
			uint32_t Argument;     /**< Block-specific flags */
		};

		struct TMemoryMap {
			size_t					StartAddress;
			size_t					Length;
			size_t					OffsetCompensation;
			bool					ReqDMAAccess;
			bool					Ignore;
			std::vector<uint8_t>* VectorAssignment;
		};

		struct MemoryTable
		{
			uint16_t* startaddress;
			uint16_t* stopaddress;
			uint16_t m_u16CRC;
			struct MemoryTable* m_NextTableEntry;
			bool m_bDMAAccess;
			uint16_t* m_pu16CRCState;
		};
		/*
		struct MemoryDescriptor
		{
			uint32_t startaddress;
			uint32_t stopaddress;
			uint16_t m_u16CRC;
			struct MemoryDescriptor *m_NextTableEntry;
			bool m_bDMAAccess;
			bool *m_pbCRCState;
		};*/

		static const TMemoryMap		m_TemplateMemoryLayout[13];
		TMemoryMap					m_MemoryLayout[13];
		MemoryTable					m_MemoryTable[256];
		char						m_LDRIdentifier[256];
	private:
		CElfReader();
		std::vector<uint8_t> m_FileRawData;
		std::vector<CIntelHexMerger> m_Merger;
		std::vector<uint8_t> m_SDRAM;
		std::vector<uint8_t> m_StaticMemoryBlock1;
		std::vector<uint8_t> m_StaticMemoryBlock0;
		std::vector<uint8_t> m_SPI2Memory;
		std::vector<uint8_t> m_OPTMemory;
		std::vector<uint8_t> m_L1DataBlockC;
		std::vector<uint8_t> m_InstructionCache;
		std::vector<uint8_t> m_InstructionSRAM;
		std::vector<uint8_t> m_L1DataBlockB_Cache;
		std::vector<uint8_t> m_L1DataBlockB;
		std::vector<uint8_t> m_L1DataBlockA_Cache;
		std::vector<uint8_t> m_L1DataBlockA;
		std::vector<uint8_t> m_L2SRAM;

		std::vector<std::vector<uint8_t>*> m_Memory;
		std::vector<MemoryTable> RegeneratedMemTable;
		std::vector<uint8_t> m_PatchedData;
		ElfStatus eElfStatus;
		size_t	m_StreamLength;

		bool	CheckHeader(TFlashHeader* header);
		bool    CreateHeader(TFlashHeader* header, uint32_t flags, uint32_t targetaddress, uint32_t bytecount, uint32_t argument);
		bool	FillMemory(uint32_t address, uint32_t length, uint32_t pattern = 0x00);
		bool	CopyBlock(uint32_t sourceaddress, uint32_t destaddress, uint32_t length);
		bool	RestructureSDRAM();
		bool	GenerateTableEntry(BlockType type, uint32_t startaddress, uint32_t stopaddress);
		const uint8_t* GetMemoryContent(uint32_t start, uint32_t stop)const;
		bool	RequiresDMAAccess(uint32_t start, uint32_t stop)const;
		bool	IgnoreMemorySection(uint32_t start, uint32_t stop)const;
		std::string
			SetExtendedAddress(uint32_t address);

		uint32_t FindBlock(uint32_t address);
		std::vector<CIntelHexMerger*> FindBlockinLdr(uint32_t startaddress, uint32_t stopaddress);

		bool	CorrectApplicationHeaderStructure(bool appendinfoblock, uint32_t appinfoaddress, uint32_t DXEPointer);
		bool	CmpFillBlock(uint32_t address, uint32_t size, uint32_t pattern);
		bool	CmpDataBlock(uint32_t address, uint32_t size, uint8_t* data);
		bool	PatchSection(std::vector<uint8_t>& datavector, TFlashHeader* header);
		bool	SimulateExtraction(std::vector<uint8_t> rawdata);
		void	PrintNextApplicationHeader(TFlashHeader const* pHeader, size_t address) const;
		void	PrintHeader(TFlashHeader const* pHeader, size_t address) const;
		std::string GetFlagAsText(uint32_t flags) const;

		uint8_t CalcCRC(uint8_t size, uint16_t address, uint8_t type, uint8_t* data);
		uint8_t CalcHeaderChecksum(TFlashHeader* header);
	public:
		CElfReader(std::string filename);
		bool PatchFile(bool appendinfoblock, uint32_t appinfoaddress);
		bool Deflate();
		ElfStatus GetState()const { return eElfStatus; }
		const std::string& GetStateMessage() const;
		bool ExtractMemoryLayout(bool usestatevectoraddress, uint32_t statevectoraddress);
		bool OpenLdrFile(std::string existingldr);
		bool PrintFileTree(bool patchedfile = true) const;
		bool Merge(std::string patcheldrfile, uint32_t baseaddress);
		bool CheckIntegrity(std::string filename);
		virtual ~CElfReader() {};
	};
}