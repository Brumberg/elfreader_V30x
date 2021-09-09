// elfreader_V30x.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <algorithm>
#include "V303/CElfReader_V303.h"
#include "V304/CElfReader_V304.h"


typedef float float32;
typedef int32_t sint32;
typedef uint32_t uint32;
typedef double float64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint64_t uint64;


enum class EN_DATATYPE { NO, FLAGUSAGE, BOOL, INT32, UINT32, FLOAT, DOUBLE, BUFFER, STRING, ENUM, COMPLEXTYPE, FILE };

enum class EN_ProcessorType { EN_PROCESSOR_BF52x, EN_PROCESSOR_BF70x };

class CRangeType
{
protected:
    CRangeType() {}
    virtual ~CRangeType() {}
public:
    virtual bool CheckRange(float32 val) = 0;
};

class CUint32Range : public CRangeType
{
public:
    CUint32Range() : CRangeType() {}
    virtual ~CUint32Range() {}
private:
    bool CheckRange(float32 val) override { static const float32 MINVAL = 0.f;  static const float32 MAXVAL = 4294967295.f;  return (val >= MINVAL) && (val <= MAXVAL); }
}CUint32Range;


struct UnitConverter {
    const char* Name;
    float32 Factor;
    float32 Offset;
};

class CUnitHandler
{
protected:
    bool FindAndExtract(size_t len, std::string& opt, UnitConverter const* list, float32& sivalue)
    {
        bool retVal;
        std::size_t pos = std::string::npos;
        UnitConverter const* pmatch = nullptr;

        for (size_t i = 0u; i < len; ++i)
        {
            UnitConverter const* ite = &list[i];
            std::string_view temp = opt;
            std::size_t findeq = temp.find('=');
            if ((findeq != std::string::npos) && ((findeq + 1u) < temp.size()))
            {
                temp.remove_prefix(findeq + 1u);
                std::size_t found = temp.rfind(ite->Name);
                if (found != std::string::npos)
                {
                    found += findeq + 1u;
                    if (pos != std::string::npos)
                    {
                        if (found < pos)
                        {
                            pos = found;
                            pmatch = ite;
                        }
                    }
                    else
                    {
                        pos = found;
                        pmatch = ite;
                    }
                }
            }
        }

        if (pmatch != nullptr)
        {
            float32 Factor = pmatch->Factor;
            float32 Offset = pmatch->Offset;
            std::size_t found = opt.find('=');
            if (found != std::string::npos)
            {
                ++found;
                std::string subst = opt.substr(found, pos - found);
                sivalue = Factor * std::stof(subst) + Offset;
                retVal = true;
            }
            else
            {
                sivalue = 0.f;
                retVal = false;
            }
        }
        else
        {
            std::size_t found = opt.find('=');
            if (found != std::string::npos)
            {
                ++found;
                std::string subst = opt.substr(found, opt.length() - found);
                sivalue = std::stof(subst);
                retVal = true;
            }
            else
            {
                sivalue = 0.f;
                retVal = false;
            }
        }
        return retVal;
    }
public:
    virtual float32 GetSIUnit(std::string& str) { return std::stof(str); };
};

class CCallback;

struct CommandLineOption
{
    const char* cOption;
    const char* plainLogText;
    const char* plainLogUnits;
    CCallback const* pCallback;
    EN_DATATYPE eDatatype;
    size_t uItemSize;
    void* pData;
    CUnitHandler* pUnitHandler;
    CRangeType* pRangeCheck;
};


class CCallback
{
public:
    CCallback() {}
    virtual ~CCallback() {}
    virtual bool OnCallback() const = 0;
};

class CDefaultCallback : public CCallback
{
public:
    CDefaultCallback() : CCallback() {}
    virtual ~CDefaultCallback() {}
    virtual bool OnCallback() const override { return false; };
};

class IEnumerator : public CCallback
{
protected:
    IEnumerator() {};
public:
    virtual bool ConvertEnumType(std::string const& str, void* pRes) const = 0;
    virtual ~IEnumerator() {};
};
class COnHelp : public CCallback
{
private:
    CommandLineOption const* pCommandLineOption;
    size_t m_CommandLineOptionSize;
public:
    COnHelp() : CCallback(), pCommandLineOption(nullptr), m_CommandLineOptionSize(0u) {}
    void InitHelp(size_t len, CommandLineOption const* popt) { m_CommandLineOptionSize = len; pCommandLineOption = popt; }
    virtual ~COnHelp() {}
    virtual bool OnCallback() const override {
        bool retVal;
        if (pCommandLineOption != nullptr && m_CommandLineOptionSize > 0u)
        {
            std::cout << "Usage: " << std::endl;
            for (size_t i = 0u; i < m_CommandLineOptionSize; ++i)
            {
                std::cout << "\t[" << pCommandLineOption[i].cOption << "]=\t" << pCommandLineOption[i].plainLogText << "\t" << pCommandLineOption[i].plainLogUnits << std::endl;
            }
            std::cout << std::endl;
            retVal = true;
        }
        else
        {
            std::cout << "Help not initialized." << std::endl;
            retVal = false;
        }
        return retVal;
    };
};


class CProcessorType : public IEnumerator
{
public:
    using ENUM = EN_ProcessorType;
    CProcessorType() {}
    ~CProcessorType() {}
    virtual bool OnCallback() const override { return false; }
    virtual bool ConvertEnumType(std::string const& str, void* pdst) const
    {
        struct MAPStuct
        {
            const std::string stringid;
            const ENUM en_enum;
        };

        static const MAPStuct map[] =
        {
            {"BF52x", ENUM::EN_PROCESSOR_BF52x},
            {"BF522", ENUM::EN_PROCESSOR_BF52x},
            {"BF523", ENUM::EN_PROCESSOR_BF52x},
            {"BF70x", ENUM::EN_PROCESSOR_BF70x},
            {"BF700", ENUM::EN_PROCESSOR_BF70x},
            {"BF701", ENUM::EN_PROCESSOR_BF70x},
            {"BF702", ENUM::EN_PROCESSOR_BF70x},
            {"BF703", ENUM::EN_PROCESSOR_BF70x},
            {"BF704", ENUM::EN_PROCESSOR_BF70x},
            {"BF705", ENUM::EN_PROCESSOR_BF70x},
            {"BF706", ENUM::EN_PROCESSOR_BF70x},
            {"BF707", ENUM::EN_PROCESSOR_BF70x}
        };

        std::string upper = str;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

        bool retVal = false;
        ENUM DefVal = ENUM::EN_PROCESSOR_BF70x;
        for (auto i : map)
        {
            if (i.stringid == upper)
            {
                DefVal = i.en_enum;
                retVal = true;
            }
        }
        *reinterpret_cast<ENUM*>(pdst) = DefVal;
        return retVal;
    }
};

static bool SetBoolType(std::string& opt, bool* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        bool t = opt.find("true") != std::string::npos;
        bool f = opt.find("false") != std::string::npos;
        *poptions = t;
        retVal = t || f;
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetIntType(std::string& opt, CommandLineOption const* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            std::string value = opt.substr(pos + 1);
            sint32 val = std::stoi(value);
            if (poptions->pRangeCheck != nullptr)
            {
                if (poptions->pRangeCheck->CheckRange(static_cast<float32>(val)))
                {
                    *reinterpret_cast<sint32*>(poptions->pData) = val;
                    retVal = true;
                }
                else
                {
                    retVal = false;
                }
            }
            else
            {
                *reinterpret_cast<sint32*>(poptions->pData) = val;
                retVal = true;
            }
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetUIntType(std::string& opt, CommandLineOption const* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            std::string value = opt.substr(pos + 1);
            uint32 val = std::strtoul(value.c_str(), nullptr, NULL);
            if (poptions->pRangeCheck != nullptr)
            {
                if (poptions->pRangeCheck->CheckRange(static_cast<float32>(val)))
                {
                    *reinterpret_cast<uint32*>(poptions->pData) = val;
                    retVal = true;
                }
                else
                {
                    retVal = false;
                }
            }
            else
            {
                *reinterpret_cast<uint32*>(poptions->pData) = val;
                retVal = true;
            }
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetBufferType(std::string& opt, size_t maxlength, char* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            std::string value = opt.substr(pos);
            strcpy_s(poptions, maxlength, value.data());
            retVal = true;
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetFloatType(std::string& opt, CommandLineOption const* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            if (poptions->pUnitHandler != nullptr)
            {
                float32 val = poptions->pUnitHandler->GetSIUnit(opt);
                if (poptions->pRangeCheck != nullptr)
                {
                    if (poptions->pRangeCheck->CheckRange(static_cast<float32>(val)))
                    {
                        *reinterpret_cast<float32*>(poptions->pData) = val;
                        retVal = true;
                    }
                    else
                    {
                        retVal = false;
                    }
                }
                else
                {
                    *reinterpret_cast<float32*>(poptions->pData) = val;
                    retVal = true;
                }
            }
            else
            {
                retVal = false;
            }
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetDoubleType(std::string& opt, CommandLineOption const* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            if (poptions->pUnitHandler != nullptr)
            {
                double val = poptions->pUnitHandler->GetSIUnit(opt);
                if (poptions->pRangeCheck != nullptr)
                {
                    if (poptions->pRangeCheck->CheckRange(static_cast<float32>(val)))
                    {
                        *reinterpret_cast<float64*>(poptions->pData) = val;
                        retVal = true;
                    }
                    else
                    {
                        retVal = false;
                    }
                }
                else
                {
                    *reinterpret_cast<float64*>(poptions->pData) = val;
                    retVal = true;
                }
            }
            else
            {
                retVal = false;
            }
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetStringType(std::string& opt, CommandLineOption const* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            std::string dummy = opt.substr(pos + 1);
            *reinterpret_cast<std::string*>(poptions->pData) = dummy;
            retVal = true;
        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool SetEnumType(std::string& opt, CommandLineOption const* poptions)
{
    bool retVal;
    if (poptions != nullptr)
    {
        size_t pos = opt.find("=");
        if (pos != std::string::npos)
        {
            std::string dummy = opt.substr(pos + 1);
            if (poptions->pCallback != nullptr)
            {
                const IEnumerator* p = dynamic_cast<const IEnumerator*>(poptions->pCallback);
                if (p != nullptr)
                {
                    retVal = p->ConvertEnumType(dummy, poptions->pData);
                }
                else
                {
                    retVal = false;
                }
            }
            else
            {
                retVal = false;
            }

        }
        else
        {
            retVal = false;
        }
    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static bool ExtractArgument(size_t const optionlen, CommandLineOption const* poptions, std::string& opt);
bool LoadFileConfig(size_t const optionlen, CommandLineOption const* poptions, const std::string& opt, std::string& filename)
{
    bool retVal;
    size_t opcodelen = opt.find_first_of("=");
    if (opcodelen != std::string::npos)
    {
        size_t start = opt.find_first_of("\"", opcodelen + 1u);
        size_t stop = opt.find_last_of("\"", opcodelen + 1u);
        std::string code = opt.substr(0u, opcodelen);
        std::string extfilename;
        if ((start != std::string::npos) && (stop != std::string::npos))
        {
            extfilename = opt.substr(start, stop - start);
            retVal = true;
        }
        else if ((start == std::string::npos) && (stop == std::string::npos))
        {
            start = opcodelen + 1u;
            extfilename = opt.substr(start);
            retVal = true;
        }
        else
        {
            extfilename = "";
            retVal = false;
        }
        filename = extfilename;

        if (retVal)
        {
            std::stringstream buffer;
            std::ifstream cfgfile(extfilename, std::ifstream::in);
            if (cfgfile.is_open())
            {
                buffer << cfgfile.rdbuf();
                std::string FileContent(buffer.str());
                size_t startoption = FileContent.find_first_of("-");
                while (startoption != std::string::npos)
                {
                    size_t stopoption = FileContent.find_first_of("= ");

                    size_t stop;
                    std::string option;

                    if (stopoption == std::string::npos)
                    {
                        option = FileContent.substr(startoption);
                        stop = std::string::npos;
                    }
                    else
                    {
                        if (FileContent[stopoption] == ' ')
                        {
                            stop = stopoption + 1u;
                            option = FileContent.substr(startoption, stopoption - startoption);
                        }
                        else
                        {
                            //"" is possible
                            if (stopoption + 2u < FileContent.size())
                            {
                                if (FileContent[stopoption + 1u] == '\"')
                                {
                                    stop = FileContent.find_first_of("\"", stopoption + 2u);
                                    if (stop == std::string::npos)
                                    {
                                        std::cerr << "invalid format" << std::endl;
                                    }
                                    else
                                    {
                                        stop = FileContent.find_first_of(" ", stop);
                                    }
                                    if (stop != std::string::npos)
                                    {
                                        option = FileContent.substr(startoption + 1u, stop - startoption - 2u);//+1//-2 remove quotes
                                    }

                                }
                                else
                                {
                                    //this is an equal
                                    stop = FileContent.find_first_of(" ", stopoption);
                                    if (stop == std::string::npos)
                                    {
                                        option = FileContent.substr(startoption);
                                    }
                                    else
                                    {
                                        option = FileContent.substr(startoption, stop - startoption);
                                    }
                                }
                            }
                            else
                            {
                                //this is an equal
                                stop = FileContent.find_first_of(" ", stopoption);
                                if (stop == std::string::npos)
                                {
                                    option = FileContent.substr(startoption);
                                }
                                else
                                {
                                    option = FileContent.substr(startoption, stopoption - startoption);
                                }
                            }
                        }
                    }

                    ExtractArgument(optionlen, poptions, option);
                    startoption = FileContent.find_first_of("-", stop);
                    if (startoption != std::string::npos)
                    {
                        FileContent = FileContent.substr(startoption);
                        startoption = 0;
                    }

                }
            }
            else
            {
                std::cerr << "Unable to open cfg-file";
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


class CParameterResolution : public CCallback
{
protected:
    uint32& Resolver;
public:
    CParameterResolution(uint32& Resolution) : CCallback(), Resolver(Resolution) { Resolution = 0u; }
    virtual ~CParameterResolution() {}
    virtual bool OnCallback() const override { return false; };
};

class CLocationResolver : public CParameterResolution
{
public:
    CLocationResolver(uint32& Resolution):CParameterResolution(Resolution) { }
    virtual ~CLocationResolver() {}
    virtual bool OnCallback() const override { Resolver = 1u;  return true; };
};


static bool ExtractArgument(size_t const optionlen, CommandLineOption const* poptions, std::string& opt)
{
    bool retVal;
    if (poptions != nullptr)
    {
        retVal = false;

        size_t opcodelen = opt.find_first_of("=");
        std::string option = opt.substr(0u, opcodelen);

        for (size_t i = 0u; i < optionlen; ++i)
        {
            std::string opcode(poptions[i].cOption);

            if (opcode == option)
            {
                switch (poptions[i].eDatatype)
                {
                case EN_DATATYPE::BOOL:
                    retVal = SetBoolType(opt, reinterpret_cast<bool*>(poptions[i].pData));
                    break;
                case EN_DATATYPE::INT32:
                    retVal = SetIntType(opt, &poptions[i]);
                    break;
                case EN_DATATYPE::UINT32:
                    retVal = SetUIntType(opt, &poptions[i]);
                    break;
                case EN_DATATYPE::FLOAT:
                    retVal = SetFloatType(opt, &poptions[i]);
                    if (poptions[i].pCallback != nullptr && retVal == true)
                    {
                        retVal = poptions[i].pCallback->OnCallback();
                    }
                    break;
                case EN_DATATYPE::DOUBLE:
                    retVal = SetDoubleType(opt, &poptions[i]);
                    break;
                case EN_DATATYPE::BUFFER:
                    retVal = SetBufferType(opt, 256, reinterpret_cast<char*>(poptions[i].pData));
                    break;
                case EN_DATATYPE::STRING:
                    retVal = SetStringType(opt, &poptions[i]);
                    break;
                case EN_DATATYPE::ENUM:
                    retVal = SetEnumType(opt, &poptions[i]);
                    break;
                case EN_DATATYPE::COMPLEXTYPE:
                    break;
                case EN_DATATYPE::FILE:
                    retVal = LoadFileConfig(optionlen, poptions, opt, *reinterpret_cast<std::string*>(poptions[i].pData));
                    break;
                case EN_DATATYPE::FLAGUSAGE:
                    retVal = poptions[i].pCallback->OnCallback();
                    break;
                case EN_DATATYPE::NO:
                    break;
                default:
                    break;
                }
                break;
            }
        }

    }
    else
    {
        retVal = false;
    }
    return retVal;
}

static void PrintRecordSet(size_t tablelength, CommandLineOption const* const ptable)
{
    std::cout << std::hex;
    for (size_t i = 0u; i < tablelength; ++i)
    {
        if ((ptable[i].eDatatype != EN_DATATYPE::FLAGUSAGE) && (ptable[i].eDatatype != EN_DATATYPE::NO))
        {
            std::cout << "\t" << ptable[i].plainLogText << ": ";
            switch (ptable[i].eDatatype)
            {
            case EN_DATATYPE::FLOAT:
                std::cout << *reinterpret_cast<float32*>(ptable[i].pData);
                break;
            case EN_DATATYPE::DOUBLE:
                std::cout << *reinterpret_cast<float32*>(ptable[i].pData);
                break;
            case EN_DATATYPE::INT32:
                std::cout << *reinterpret_cast<sint32*>(ptable[i].pData);
                break;
            case EN_DATATYPE::UINT32:
                std::cout << "0x0" << *reinterpret_cast<uint32*>(ptable[i].pData);
                break;
            case EN_DATATYPE::ENUM:
                if (ptable[i].uItemSize == 1u)
                {
                    std::cout << *reinterpret_cast<uint8*>(ptable[i].pData);
                }
                else if (ptable[i].uItemSize == 2u)
                {
                    std::cout << *reinterpret_cast<uint16*>(ptable[i].pData);
                }
                else if (ptable[i].uItemSize == 4u)
                {
                    std::cout << *reinterpret_cast<uint32*>(ptable[i].pData);
                }
                else if (ptable[i].uItemSize == 8u)
                {
                    std::cout << *reinterpret_cast<uint64*>(ptable[i].pData);
                }
                else
                {
                    //?
                }
                break;
            case EN_DATATYPE::BUFFER:
                std::cout << *reinterpret_cast<char*>(ptable[i].pData);
                break;
            case EN_DATATYPE::STRING:
                std::cout << *reinterpret_cast<std::string*>(ptable[i].pData);
                break;
            case EN_DATATYPE::FILE:
                if (ptable[i].pData != nullptr)
                {
                    std::cout << "\"" << *reinterpret_cast<std::string*>(ptable[i].pData) << "\"";
                }
                break;
            case EN_DATATYPE::BOOL:
                if (*reinterpret_cast<bool*>(ptable[i].pData) == false)
                {
                    std::cout << "false";
                }
                else
                {
                    std::cout << "true";
                }
                break;
            case EN_DATATYPE::FLAGUSAGE:
                break;
            case EN_DATATYPE::NO:
                break;
            default:
                std::cout << "No conversion available";
                break;

            }
            std::cout << " " << ptable[i].plainLogUnits << std::endl;
        }
    }
    std::cout << std::dec;
    std::cout << std::endl;
}

template <typename T, int sz>
constexpr size_t size(T(&)[sz])
{
    return sz;
}

static void Execute_V303(std::string& src, std::string& dst, uint32 addr_offset, bool usestatevector, uint32 StateVectorAddress, bool appendinfoblock, uint32 appendinfoblocklocation, bool verify)
{
    V303::CElfReader reader(src);
    if (reader.GetState() == V303::CElfReader::ELF_OK)
    {
        std::cout << "File OK" << std::endl;
        if (reader.Deflate())
        {
            std::cout << "Deflating completed..." << std::endl;

            reader.ExtractMemoryLayout(usestatevector, StateVectorAddress);

            if (reader.PatchFile(appendinfoblock, appendinfoblocklocation))
            {
                if (reader.OpenLdrFile(src))
                {
                    if (reader.Merge(dst, addr_offset, appendinfoblock, appendinfoblocklocation))
                    {
                        std::cout << "File successfully merged." << std::endl;
                        if (verify)
                        {
                            std::cout << "Checking integrity..." << std::endl;
                            if (reader.CheckIntegrity(dst))
                            {
                                std::cout << "Verification succeeded." << std::endl;
                            }
                            else
                            {
                                std::cout << "Integrity check failed. " << std::endl;
                            }
                        }
                        else
                        {
                            std::cout << "Unknown attribute" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "Error. Unable to patch file (merge process)." << std::endl;
                    }
                }
                else
                {
                    std::cout << "Error. Unable to patch file (OpenLdrFile)." << std::endl;
                }
            }
            else
            {
                std::cout << "Error. Unable to patch file (patch)." << std::endl;
            }

        }
        else
        {
            std::cout << "Deflating process stopped. File corrupt." << std::endl;
        }
    }
    else
    {
        std::cout << "Error occured. " << "State" << reader.GetState() << std::endl;
    }
}

static void Execute_V304(std::string& src, std::string& dst, uint32 addr_offset, bool usestatevector, uint32 StateVectorAddress, bool appendinfoblock, uint32 appendinfoblocklocation, bool verify)
{
    V304::CElfReader reader(src);
    if (reader.GetState() == V304::CElfReader::ELF_OK)
    {
        std::cout << "File OK" << std::endl;
        if (reader.Deflate())
        {
            std::cout << "Deflating completed..." << std::endl;

            reader.ExtractMemoryLayout(usestatevector, StateVectorAddress);

            if (reader.PatchFile(appendinfoblock, appendinfoblocklocation))
            {
                if (reader.OpenLdrFile(src))
                {
                    if (reader.Merge(dst, addr_offset, appendinfoblock, appendinfoblocklocation))
                    {
                        std::cout << "File successfully merged." << std::endl;
                        if (verify)
                        {
                            std::cout << "Checking integrity..." << std::endl;
                            if (reader.CheckIntegrity(dst))
                            {
                                std::cout << "Verification succeeded." << std::endl;
                            }
                            else
                            {
                                std::cout << "Integrity check failed. " << std::endl;
                            }
                        }
                        else
                        {
                            std::cout << "Unknown attribute" << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "Error. Unable to patch file (merge process)." << std::endl;
                    }
                }
                else
                {
                    std::cout << "Error. Unable to patch file (OpenLdrFile)." << std::endl;
                }
            }
            else
            {
                std::cout << "Error. Unable to patch file (patch)." << std::endl;
            }

        }
        else
        {
            std::cout << "Deflating process stopped. File corrupt." << std::endl;
        }
    }
    else
    {
        std::cout << "Error occured. " << "State" << reader.GetState() << std::endl;
    }
}

int main(int argc, char** argv)
{
    struct DefaultValues
    {
        std::string Help;
        std::string src;
        std::string dst;
        EN_ProcessorType en_ProcessorType;
        bool bVectorStateAddress;
        uint32 u32_VectorStateAddress;
        uint32 u32_BaseAddress;
        bool bAppendInfoBlock;
        uint32 u32_AppendInfoBlockLocation;
        bool b_VerifyOutput;
    }DefEnvironment;
    
    static const EN_ProcessorType en_ProcessorType = EN_ProcessorType::EN_PROCESSOR_BF70x;
    static const std::string emptystring = "";
    static const bool bVectorStateAddress = false;
    static const uint32 VectorStateAddress = 0x00;
    static const uint32 baseaddress = 0x3002C;
    static const bool AppendInfoBlock = false;
    static const uint32 AppendInfoBlockLocation = 0x80b00000;
    static const bool VerifyOutput = false;
    const CDefaultCallback DefCallBack;
    uint32 VectorStateAddressResolvent = 0u;
    CLocationResolver VectorStateAddressResolutor(VectorStateAddressResolvent);
    uint32 InfoBlockLocation;
    CLocationResolver InfoBlockAddressResolutor(InfoBlockLocation);

    DefEnvironment.en_ProcessorType = en_ProcessorType;
    DefEnvironment.src = emptystring;
    DefEnvironment.dst = emptystring;
    DefEnvironment.bVectorStateAddress = bVectorStateAddress; //to be checked
    DefEnvironment.u32_VectorStateAddress = VectorStateAddress; //to be checked
    DefEnvironment.u32_BaseAddress = baseaddress;
    DefEnvironment.bAppendInfoBlock = AppendInfoBlock;
    DefEnvironment.u32_AppendInfoBlockLocation = AppendInfoBlockLocation;
    DefEnvironment.b_VerifyOutput = VerifyOutput;
    bool bPrintRecord = false;

    COnHelp OnHelp;
    const CProcessorType OnProcessor;

    const CommandLineOption CommandLineOptions[] =
    {
        {"-help", "Help", "",&OnHelp, EN_DATATYPE::FLAGUSAGE, 0, nullptr, nullptr, nullptr},
        {"-h", "Help", "", &OnHelp, EN_DATATYPE::FLAGUSAGE, 0, nullptr, nullptr, nullptr},
        {"-proc", "Processor type", "", &OnProcessor, EN_DATATYPE::ENUM, sizeof(EN_ProcessorType), &DefEnvironment.en_ProcessorType, nullptr, nullptr},
        {"-srcfile", "Source file", "", nullptr, EN_DATATYPE::STRING, 0, &DefEnvironment.src, nullptr, nullptr},
        {"-dstfile", "Destination file", "", nullptr, EN_DATATYPE::STRING, 0, &DefEnvironment.dst, nullptr, nullptr},
        {"-uvsa", "specify vector state address", "[false/true]", &DefCallBack, EN_DATATYPE::BOOL, sizeof(bool), &DefEnvironment.bVectorStateAddress, nullptr, nullptr},
        {"-vsa", "define vector state address", "", &VectorStateAddressResolutor, EN_DATATYPE::INT32, sizeof(uint32), &DefEnvironment.u32_VectorStateAddress, nullptr, &CUint32Range},
        {"-offset", "defines address offset ", "", &DefCallBack, EN_DATATYPE::UINT32, sizeof(uint32), &DefEnvironment.u32_BaseAddress, nullptr, &CUint32Range},
        {"-appib", "append info block", "[false/true]", &DefCallBack, EN_DATATYPE::BOOL, sizeof(bool), &DefEnvironment.bAppendInfoBlock, nullptr, nullptr},
        {"-ibloc", "info block location", "", &InfoBlockAddressResolutor, EN_DATATYPE::UINT32, sizeof(uint32), &DefEnvironment.u32_AppendInfoBlockLocation, nullptr, nullptr},
        {"-verify", "Verify file", "[false/true]", &DefCallBack, EN_DATATYPE::BOOL, sizeof(bool), &DefEnvironment.b_VerifyOutput, nullptr, nullptr},
        {"-r", "Print Record", "[false/true]", &DefCallBack, EN_DATATYPE::BOOL, sizeof(bool), &bPrintRecord, nullptr, nullptr},
    };

    OnHelp.InitHelp(size(CommandLineOptions), CommandLineOptions);

    for (size_t i = 1u; i < static_cast<size_t>(argc); ++i)
    {
        std::string string(argv[i]);
        (void)ExtractArgument(sizeof(CommandLineOptions) / sizeof(CommandLineOptions[0]), CommandLineOptions, string);
    }


    if (VectorStateAddressResolvent == 0u)
    {
        if (DefEnvironment.en_ProcessorType == EN_ProcessorType::EN_PROCESSOR_BF70x)
        {
            DefEnvironment.u32_VectorStateAddress = 0x81ff8400u;
        }
        else
        {
            DefEnvironment.u32_VectorStateAddress = 0x00ff8400u;
        }
    }
    
    if (InfoBlockLocation == 0u)
    {
        if (DefEnvironment.en_ProcessorType == EN_ProcessorType::EN_PROCESSOR_BF70x)
        {
            DefEnvironment.u32_AppendInfoBlockLocation = 0x80b00000u;
        }
        else
        {
            DefEnvironment.u32_AppendInfoBlockLocation = 0x00c00000u;
        }
    }

    if (bPrintRecord == true)
    {
        PrintRecordSet(sizeof(CommandLineOptions) / sizeof(CommandLineOptions[0]), &CommandLineOptions[0]);
    }
    
    if (DefEnvironment.en_ProcessorType != EN_ProcessorType::EN_PROCESSOR_BF70x)
    {
        Execute_V303(DefEnvironment.src, DefEnvironment.dst, DefEnvironment.u32_BaseAddress, DefEnvironment.bVectorStateAddress, DefEnvironment.u32_VectorStateAddress, DefEnvironment.bAppendInfoBlock, DefEnvironment.u32_AppendInfoBlockLocation, DefEnvironment.b_VerifyOutput);
    }
    else
    {
        Execute_V304(DefEnvironment.src, DefEnvironment.dst, DefEnvironment.u32_BaseAddress, DefEnvironment.bVectorStateAddress, DefEnvironment.u32_VectorStateAddress, DefEnvironment.bAppendInfoBlock, DefEnvironment.u32_AppendInfoBlockLocation, DefEnvironment.b_VerifyOutput);
    }    
}

// Programm ausführen: STRG+F5 oder Menüeintrag "Debuggen" > "Starten ohne Debuggen starten"
// Programm debuggen: F5 oder "Debuggen" > Menü "Debuggen starten"

// Tipps für den Einstieg: 
//   1. Verwenden Sie das Projektmappen-Explorer-Fenster zum Hinzufügen/Verwalten von Dateien.
//   2. Verwenden Sie das Team Explorer-Fenster zum Herstellen einer Verbindung mit der Quellcodeverwaltung.
//   3. Verwenden Sie das Ausgabefenster, um die Buildausgabe und andere Nachrichten anzuzeigen.
//   4. Verwenden Sie das Fenster "Fehlerliste", um Fehler anzuzeigen.
//   5. Wechseln Sie zu "Projekt" > "Neues Element hinzufügen", um neue Codedateien zu erstellen, bzw. zu "Projekt" > "Vorhandenes Element hinzufügen", um dem Projekt vorhandene Codedateien hinzuzufügen.
//   6. Um dieses Projekt später erneut zu öffnen, wechseln Sie zu "Datei" > "Öffnen" > "Projekt", und wählen Sie die SLN-Datei aus.
