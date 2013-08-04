#include "pch.h"
#include "cpudetect.h"
#include <xmmintrin.h>
#include <intrin.h>

LMPLATFORM lmPlatform;

#define HASBIT(v, bit) (((v) & (1 << bit)) != 0)

void GetCPUInfo( CPUINFO &inf )
{
	int d[4];
	__cpuid( d, 1 );

	inf.MMX = true;
	inf.SSE1 = HASBIT(d[3], 25);
	inf.SSE2 = HASBIT(d[3], 26);
	inf.SSE3 = HASBIT(d[2], 0);
	inf.SSSE3 = HASBIT(d[2], 9);
	inf.SSE4_1 = HASBIT(d[2], 19);
	inf.SSE4_2 = HASBIT(d[2], 20);

	__cpuid( d, 0x80000001 );
	inf.AMD_3DNOW = HASBIT(d[3], 31);
}

/*
// Returns false if really dumb processor

bool GetCPUInfo( CPUINFO &inf )
{
#ifdef _WIN64
	inf.MMX = false;
	inf.SSE = false;
	inf.SSE2 = false;
	inf.AMD_3DNOW = false;

	inf.Intel = false;
	inf.P3 = false;
	inf.P4 = false;

	inf.AMD = true;
	inf.K7 = false;
	inf.K8 = true;
	return true;
#else
	DWORD Features       = 0;
	DWORD ExtFeatures	 = 0;
	DWORD Processor      = 0;
	char Vendor[20];
	int Init = 0;

	__asm {
			pushfd                  // save EFLAGS to stack.
			pop     eax             // store EFLAGS in EAX.
			mov     edx, eax        // save in EBX for testing later.
			xor     eax, 0x200000   // switch bit 21.
			push    eax             // copy "changed" value to stack.
			popfd                   // save "changed" EAX to EFLAGS.
			pushfd
			pop     eax
			xor     eax, edx        // See if bit changeable.
			jnz     short foundit   // if so, mark 
			mov     eax,-1
			jmp     short around

			ALIGN   4
	foundit:
			// Load up the features and (where appropriate) extended features flags
			mov     eax,1               // Check for processor features
			CPUID
			mov     [Features],edx      // Store features bits
			mov     eax,0x80000000      // Check for support of extended functions.
			CPUID
			cmp     eax,0x80000001      // Make sure function 0x80000001 supported.
			jb      short around
			mov     eax,0x80000001      // Select function 0x80000001
			CPUID
			mov     [Processor],eax     // Store processor family/model/step
			mov     [ExtFeatures],edx  // Store extende features bits
			mov     eax,1               // Set "Has CPUID" flag to true

	around:
			mov     [Init],eax
	}

	if (Init == -1)
	{
		// No CPUID, so no CPUID functions are supported
		return false;
	}

	__asm 
	{

		mov     eax,0           // function 0 = manufacturer string
		CPUID
		mov		dword ptr [Vendor], ebx
		mov		dword ptr [Vendor+4], edx
		mov		dword ptr [Vendor+8], ecx
	}
	Vendor[12] = 0;

	if ( strcmp(Vendor, "AuthenticAMD") == 0 )
		inf.AMD = true;
	else if ( strcmp(Vendor, "GenuineIntel") == 0 )
		inf.Intel = true;

	if ((Features >> 23) & 1) 
		inf.MMX = true;

	// AMD Specific features.
	if ( inf.AMD )
	{
		// just check for extended 3dnow. sod the k6
		if ((ExtFeatures >> 30) & 1)
			inf.AMD_3DNOW = true;
	}

	// Intel specific features.
	if ( inf.Intel )
	{
		if ((Features >> 25) & 1)
			inf.SSE = true;

	}

	// Check OS support
	TestFeatures( inf );

	return true;
#endif
}

// will clear flags if the OS doesn't support them
void TestFeatures( CPUINFO &inf )
{
#ifndef _WIN64
	if (inf.AMD_3DNOW)
	{
		try 
		{
			__asm
			{
				femms
				pfacc mm0, mm1
				femms
			}
		}
		catch (...)
		{
			inf.AMD_3DNOW = false;
		}
	}

	if (inf.SSE)
	{
		try 
		{
			__asm
			{
				xorps xmm0, xmm0
			}
		}
		catch (...)
		{
			inf.SSE = false;
		}
	}
#endif
}
*/

/*

bool StreamingSIMDExtensionOSSupport();
bool StreamingSIMDExtensionsHWSupport();


bool StreamingSIMDExtensionOSSupport()
{
	__try
	{
		_asm xorps xmm0, xmm0 //Execute an instruction using Streaming
			//SIMD Extensions to see if support exists.
	}
	//
	// Catch any exception. If an Invalid Opcode exception (ILLEGAL_INSTRUCTION)
	// occurs, and you have already checked the XMM bit in CPUID, Streaming SIMD
	// Extensions are not supported by the OS.
	//
	__except(EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION) {
			return (false);
		}
		// If we get here, an unknown exception occurred; investigation needed.
		return (false);
	}
	return (true);
}






bool StreamingSIMDExtensionsHWSupport() 
{
	bool HWSupport = false;
	char brand[20];
	unsigned *str = (unsigned *) brand;
	// Does the processor have CPUID, and is it "GenuineIntel"?
	__try 
	{
		_asm
		{
			mov eax, 0		//First, check to make sure this is an Intel processor
			cpuid			//by getting the processor information string with CPUID
			mov str, ebx	// ebx contains "Genu"
			mov str+4, edx	// edx contains "ineI"
			mov str+8, ecx	// ecx contains "ntel" -- "GenuineIntel"
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER) {
		if (_exception_code() == STATUS_ILLEGAL_INSTRUCTION) {
			cout << endl << "****CPUID is not enabled****" << endl;
			return (false);
		}
		return (false); // If we get here, an unexpected exception occurred.
	}
	// Now make sure the processor is "GenuineIntel".
	if (!strncmp(brand, "GenuineIntel", 12)) {
		//cout << endl << "****This is not an Intel processor!****" << endl;
		//return (false);
	}
	// And finally, check the CPUID for Streaming SIMD Extensions support.
	_asm
	{
		mov eax, 1				// Put a "1" in eax to tell CPUID to get the feature bits
			cpuid					// Perform CPUID (puts processor feature info into EDX)
			test edx, 02000000h		// Test bit 25, for Streaming SIMD Extensions existence.
			jz NotFound				// If not set, jump over the next instruction (No Streaming SIMD
			mov [HWSupport], 1		// Extensions). Set return value to 1 to indicate,
														// that the processor does support Streaming SIMD Extensions.
			NotFound:
	}
	return (HWSupport);
}*/
