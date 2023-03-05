#include "Arch.h"

#include <array>

#include <intrin.h>

Vendor Arch::vendor = InitializeVendor();
std::string Arch::brand = InitializeBrand();
InstructionFlags Arch::flags = InitializeInstructionFlags();

Vendor Arch::GetVendor()
{
    return vendor;
}

std::string Arch::CPUBrand()
{
    return brand;
}

Vendor Arch::InitializeVendor()
{
    // Retrieve vendor string
    int cpui[4];
    __cpuidex(cpui, 0, 0);

    std::array<char, 12> vendorStr;
    ((int*)vendorStr.data())[0] = cpui[EBX];
    ((int*)vendorStr.data())[1] = cpui[EDX];
    ((int*)vendorStr.data())[2] = cpui[ECX];

    // Compare vendor string with known vendors
    static constexpr std::array<char, 12> IntelSig  { 'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l' };
    static constexpr std::array<char, 12> AmdSig    { 'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D' };

    if (vendorStr == IntelSig) return INTEL;
    if (vendorStr == AmdSig) return AMD;

    return UNKNOWN;
}

std::string Arch::InitializeBrand()
{
    // Get largest valid extended ID
    int cpui[4];
    __cpuid(cpui, INT32_MIN);
    int numExtendedIDs = cpui[EAX];

    std::array<char, 49> brandStr;
    brandStr[48] = '\0'; // Null terminator
    if (numExtendedIDs > INT32_MIN + 4)
    {
        __cpuidex(cpui, INT32_MIN + 2, 0);
        memcpy(&brandStr[0], cpui, 16);
        __cpuidex(cpui, INT32_MIN + 3, 0);
        memcpy(&brandStr[16], cpui, 16);
        __cpuidex(cpui, INT32_MIN + 4, 0);
        memcpy(&brandStr[32], cpui, 16);

        return std::string{ brandStr.data() };
    }
    else
    {
        return "Unknown CPU";
    }
}


InstructionFlags Arch::InitializeInstructionFlags()
{
    InstructionFlags ret;

    // Get largest valid ID
    int cpui[4];
    __cpuid(cpui, 0);
    int numIDs = cpui[EAX];

    // Load flags for IDs 0x00000001
    if (numIDs >= 1)
    {
        __cpuidex(cpui, 1, 0);
        ret.flags_ECX1 = cpui[ECX];
        ret.flags_EDX1 = cpui[EDX];
    }

    // Load flags for IDs 0x00000007
    if (numIDs >= 7)
    {
        __cpuidex(cpui, 7, 0);
        ret.flags_EBX7 = cpui[EBX];
        ret.flags_ECX7 = cpui[ECX];
    }

    // Get largest valid extended ID
    __cpuid(cpui, INT32_MIN);
    int numExtendedIDs = cpui[EAX];

    // Load flags for IDs 0x80000001
    if (numExtendedIDs >= 0x80000001)
    {
        __cpuidex(cpui, INT32_MIN + 1, 0);
        ret.flags_ECX81 = cpui[ECX];
        ret.flags_EDX81 = cpui[EDX];
    }

    return ret;
}