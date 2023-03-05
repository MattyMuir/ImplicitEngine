#pragma once
#include <bitset>

enum Vendor { UNKNOWN = 0, INTEL = 1, AMD = 2 };
enum Register { EAX = 0, EBX = 1, ECX = 2, EDX = 3 };

#define ECX1			0b00'000001'00000
#define EDX1			0b00'000010'00000
#define EBX7			0b00'000100'00000
#define ECX7			0b00'001000'00000
#define ECX81			0b00'010000'00000
#define EDX81			0b00'100000'00000

#define NEEDS_INTEL		0b01'000000'00000
#define NEEDS_AMD		0b10'000000'00000

#define FLAG_INDEX_MASK 0b00'000000'11111

// Bottom 5 bits give flag index, next 6 give register, next 2 give vendor specificity info
enum InstructionSet : unsigned short
{
	// ECX1 flagged instruction sets
	SSE3 =			0 | ECX1,
	PCLMULQDQ =		1 | ECX1,
	MONITOR =		3 | ECX1,
	SSSE3 =			9 | ECX1,
	FMA =			12 | ECX1,
	CMPXCHG16B =	13 | ECX1,
	SSE41 =			19 | ECX1,
	SSE42 =			20 | ECX1,
	MOVBE =			22 | ECX1,
	POPCNT =		23 | ECX1,
	AES =			25 | ECX1,
	XSAVE =			26 | ECX1,
	OSXSAVE =		27 | ECX1,
	AVX =			28 | ECX1,
	F16C =			29 | ECX1,
	RDRAND =		30 | ECX1,
	
	// EDX1 flagged instruction sets
	MSR =			5 | EDX1,
	CX8 =			8 | EDX1,
	SEP =			11 | EDX1,
	CMOV =			15 | EDX1,
	CLFSH =			19 | EDX1,
	MMX =			23 | EDX1,
	FXSR =			24 | EDX1,
	SSE =			25 | EDX1,
	SSE2 =			26 | EDX1,

	// EBX7 flagged instruction sets
	FSGSBASE =		0 | EBX7,
	BMI1 =			3 | EBX7,
	HLE =			4 | EBX7 | NEEDS_INTEL,
	AVX2 =			5 | EBX7,
	BMI2 =			8 | EBX7,
	ERMS =			9 | EBX7,
	INVPCID =		10 | EBX7,
	RTM =			11 | EBX7 | NEEDS_INTEL,
	AVX512F =		16 | EBX7,
	RDSEED =		18 | EBX7,
	ADX =			19 | EBX7,
	AVX512PF =		26 | EBX7,
	AVX512ER =		27 | EBX7,
	AVX512CD =		28 | EBX7,
	SHA =			29 | EBX7,

	// ECX7 flagged instruction sets
	PREFETCHWT1 =	0 | ECX7,

	// ECX81 flagged instruction sets
	LAHF =			0 | ECX81,
	LZCNT =			5 | ECX81 | NEEDS_INTEL,
	ABM =			5 | ECX81 | NEEDS_AMD,
	SSE4a =			6 | ECX81 | NEEDS_AMD,
	XOP =			11 | ECX81 | NEEDS_AMD,
	FMA4 =			16 | ECX81 | NEEDS_AMD,
	TBM =			21 | ECX81 | NEEDS_AMD,

	// EDX81 flagged instruction sets
	SYSCALL =		11 | EDX81 | NEEDS_INTEL,
	MMXEXT =		22 | EDX81 | NEEDS_AMD,
	RDTSCP =		27 | EDX81 | NEEDS_INTEL,
	_3DNOWEXT =		30 | EDX81 | NEEDS_AMD,
	_3DNOW =		31 | EDX81 | NEEDS_AMD
};

struct InstructionFlags
{
	std::bitset<32> flags_ECX1{ 0 };
	std::bitset<32> flags_EDX1{ 0 };
	std::bitset<32> flags_EBX7{ 0 };
	std::bitset<32> flags_ECX7{ 0 };
	std::bitset<32> flags_ECX81{ 0 };
	std::bitset<32> flags_EDX81{ 0 };
};

struct CacheInfo
{
	int L1Size, L2Size;
};

class Arch
{
public:
	// Class cannot be constructed
	Arch() = delete;

	static Vendor GetVendor();
	static std::string CPUBrand();

	template <InstructionSet set>
	static bool HasInstructions()
	{
		if constexpr (set & NEEDS_INTEL)
			if (vendor != INTEL) return false;

		if constexpr (set & NEEDS_AMD)
			if (vendor != AMD) return false;

		constexpr int flagIndex = set & FLAG_INDEX_MASK;

		if constexpr (set & ECX1)
			return flags.flags_ECX1[flagIndex];

		if constexpr (set & EDX1)
			return flags.flags_EDX1[flagIndex];

		if constexpr (set & EBX7)
			return flags.flags_EBX7[flagIndex];

		if constexpr (set & ECX7)
			return flags.flags_ECX7[flagIndex];

		if constexpr (set & ECX81)
			return flags.flags_ECX81[flagIndex];

		if constexpr (set & EDX81)
			return flags.flags_EDX81[flagIndex];
	}

protected:
	static Vendor InitializeVendor();
	static std::string InitializeBrand();
	static InstructionFlags InitializeInstructionFlags();

	static Vendor vendor;
	static std::string brand;
	static InstructionFlags flags;
};