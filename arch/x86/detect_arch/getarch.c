//                       PMCTestA.cpp                2017-04-28 Agner Fog
//
//          Multithread PMC Test program for Windows and Linux
//
//
// This program is intended for testing the performance of a little piece of 
// code written in C, C++ or assembly. 
// The code to test is inserted at the place marked "Test code start" in
// PMCTestB.cpp, PMCTestB32.asm or PMCTestB64.asm.
// 
// In 64-bit Windows: Run as administrator, with driver signature enforcement
// off.
//
// See PMCTest.txt for further instructions.
//
// To turn on counters for use in another program, run with command line option
//     startcounters
// To turn counters off again, use command line option 
//     stopcounters
//
// © 2000-2017 GNU General Public License v. 3. www.gnu.org/licenses
//////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdint.h>

// codes for processor vendor
enum EProcVendor {VENDOR_UNKNOWN = 0, INTEL, AMD, VIA};

// codes for processor family
enum EProcFamily {
    PRUNKNOWN    = 0,                       // unknown. cannot do performance monitoring
    PRALL        ,              // All processors with the specified scheme (Renamed. Previous name P_ALL clashes with <sys/wait.h>)
    INTEL_P1MMX  ,                       // Intel Pentium 1 or Pentium MMX
    INTEL_P23    ,                       // Pentium Pro, Pentium 2, Pentium 3
    INTEL_PM     ,                       // Pentium M, Core, Core 2
    INTEL_P4     ,                       // Pentium 4 and Pentium 4 with EM64T
    INTEL_CORE   ,                    // Intel Core Solo/Duo
    INTEL_P23M   ,                    // Pentium Pro, Pentium 2, Pentium 3, Pentium M, Core1
    INTEL_CORE2_65NM  ,               // Intel Core 2, 65 nm
    INTEL_CORE2_45NM  ,               // Intel Core 2, 45 nm
    INTEL_i7     ,                    // Intel Core i7, Nehalem, Sandy Bridge
    INTEL_NEHALEM,                    // Nehalem
    INTEL_WESTMERE,                    // Westmere
    INTEL_SANDY,                        // Sandy Bridge
    INTEL_IVY    ,                    // Intel Ivy Bridge
    INTEL_7I     ,                    // Nehalem, Sandy Bridge, Ivy bridge
    INTEL_HASW   ,                   // Intel Haswell 
    INTEL_BROADW   ,                   // Intel Broadwell
    INTEL_SKYL   ,                   // Intel Skylake and later
    INTEL_ATOM   ,                  // Intel Atom
    INTEL_SILV   ,                  // Intel Silvermont
    INTEL_KNIGHT ,                  // Intel Knights Landing (Xeon Phi 7200/5200/3200)
    AMD_ATHLON   ,                 // AMD Athlon
    AMD_ATHLON64 ,                 // AMD Athlon 64 or Opteron
    AMD_BULLD    ,                 // AMD Family 15h (Bulldozer) and 16h?
    AMD_ZEN      ,                 // AMD Family 17h (Zen)
    AMD_ALL      ,                 // AMD any processor
    VIA_NANO     ,                // VIA Nano (Centaur)
};

enum EProcVendor MVendor;
enum EProcFamily MFamily;

static void Cpuid (int Output[4], int aa) {	
    int a, b, c, d;
    __asm("cpuid" : "=a"(a),"=b"(b),"=c"(c),"=d"(d) : "a"(aa),"c"(0) : );
    Output[0] = a;
    Output[1] = b;
    Output[2] = c;
    Output[3] = d;
}


void GetProcessorVendor() {
    // get microprocessor vendor
    int CpuIdOutput[4];

    // Call cpuid function 0
    Cpuid(CpuIdOutput, 0);

    // Interpret vendor string
    MVendor = VENDOR_UNKNOWN;
    if (CpuIdOutput[2] == 0x6C65746E) MVendor = INTEL;  // Intel "GenuineIntel"
    if (CpuIdOutput[2] == 0x444D4163) MVendor = AMD;    // AMD   "AuthenticAMD"
    if (CpuIdOutput[1] == 0x746E6543) MVendor = VIA;    // VIA   "CentaurHauls"
}

void GetProcessorFamily() {
    // get microprocessor family
    int CpuIdOutput[4];
    int Family, Model;

    MFamily = PRUNKNOWN;                // default = unknown

    // Call cpuid function 0
    Cpuid(CpuIdOutput, 0);
    if (CpuIdOutput[0] == 0) return;     // cpuid function 1 not supported

    // call cpuid function 1 to get family and model number
    Cpuid(CpuIdOutput, 1);
    Family = ((CpuIdOutput[0] >> 8) & 0x0F) + ((CpuIdOutput[0] >> 20) & 0xFF);   // family code
    Model  = ((CpuIdOutput[0] >> 4) & 0x0F) | ((CpuIdOutput[0] >> 12) & 0xF0);   // model code
    // printf("\nCPU family 0x%X, model 0x%X\n", Family, Model);

    if (MVendor == INTEL)  {
        // Intel processor
        if (Family <  5)    MFamily = PRUNKNOWN;           // less than Pentium
        if (Family == 5)    MFamily = INTEL_P1MMX;         // pentium 1 or mmx
        if (Family == 0x0F) MFamily = INTEL_P4;            // pentium 4 or other netburst
        if (Family == 6) {
            switch(Model) {  // list of known Intel families with different performance monitoring event tables
            case 0x09:  case 0x0D:
                MFamily = INTEL_PM;  break;                // Pentium M
            case 0x0E: 
                MFamily = INTEL_CORE;  break;              // Core 1
            case 0x0F: case 0x16: 
                MFamily = INTEL_CORE2_65NM;  break;             // Core 2, 65 nm
            case 0x17: case 0x1D:
                MFamily = INTEL_CORE2_45NM;  break;             // Core 2, 45 nm
            case 0x1A: case 0x1E: case 0x1F: case 0x2E:
                MFamily = INTEL_NEHALEM;  break;                 // Nehalem
            case 0x25: case 0x2C: case 0x2F:
                MFamily = INTEL_WESTMERE;  break;                 // Westmere
            case 0x2A: case 0x2D:
                MFamily = INTEL_SANDY;  break;               // Sandy Bridge
            case 0x3A: case 0x3E:
                MFamily = INTEL_IVY;  break;               // Ivy Bridge
            case 0x3C: case 0x3F: case 0x45: case 0x46:
                MFamily = INTEL_HASW;  break;              // Haswell
            case 0x3D: case 0x47: case 0x4F: case 0x56:
                MFamily = INTEL_BROADW;  break;              // Broadwell
            case 0x5E: 
                MFamily = INTEL_SKYL;  break;              // Skylake
            // low power processors:
            case 0x1C: case 0x26: case 0x27: case 0x35: case 0x36:
                MFamily = INTEL_ATOM;  break;              // Atom
            case 0x37: case 0x4A: case 0x4D:
                MFamily = INTEL_SILV;  break;              // Silvermont
            case 0x57:
                MFamily = INTEL_KNIGHT; break;             // Knights Landing
            // unknown and future
            default:
                MFamily = INTEL_P23;                       // Pentium 2 or 3
                if (Model >= 0x3C) MFamily = INTEL_HASW;   // Haswell
                if (Model >= 0x5E) MFamily = INTEL_SKYL;   // Skylake
            }
        }
    }

    if (MVendor == AMD)  {
        // AMD processor
        MFamily = PRUNKNOWN;                               // old or unknown AMD
        if (Family == 6)    MFamily = AMD_ATHLON;          // AMD Athlon
        if (Family >= 0x0F && Family <= 0x14) {
            MFamily = AMD_ATHLON64;                        // Athlon 64, Opteron, etc
        }
        if (Family >= 0x15) MFamily = AMD_BULLD;           // Family 15h
        if (Family >= 0x17) MFamily = AMD_ZEN;             // Family 17h
    }

    if (MVendor == VIA)  {
        // VIA processor
        if (Family == 6 && Model >= 0x0F) MFamily = VIA_NANO; // VIA Nano
    }
}



void print_muAch() 
{
    GetProcessorVendor();
    GetProcessorFamily();
    printf("uArch: ");
    switch(MFamily) {
    case INTEL_P1MMX:
        printf("Intel Pentium 1 or Pentium MMX\n");
        break;
    case INTEL_P23:
        printf("Intel Pentium Pro, Pentium 2, Pentium 3\n");
        break;
    case INTEL_PM: 
        printf("Intel Pentium M\n");
        break;
    case INTEL_P4:
        printf("Intel Pentium 4 and Pentium 4 with EM64T\n");
        break;
    case INTEL_CORE:
        printf("Intel Core Solo/Duo\n");
        break;
    case INTEL_P23M:
        printf("Intel Pentium Pro, Pentium 2, Pentium 3, Pentium M, Core1\n");
        break;
    case INTEL_CORE2_65NM:
        printf("Intel Core 2, 65 nm\n");
        break;
    case INTEL_CORE2_45NM:
        printf("Intel Core 2, 45 nm\n");
        break;
    case INTEL_i7:
        printf("Intel Core i7, Nehalem, Sandy Bridge\n");
        break;
    case INTEL_NEHALEM:
        printf("Intel Nehalem\n");
        break;
    case INTEL_WESTMERE:
        printf("Intel Westmere\n");
        break;
    case INTEL_SANDY:
        printf("Intel Sandy Bridge\n");
        break;
    case INTEL_IVY:
        printf("Intel Ivy Bridge\n");
        break;
    case INTEL_HASW:
        printf("Intel Haswell\n");
        break;
    case INTEL_BROADW:
        printf("Intel Broadwell\n");
        break;
    case INTEL_SKYL:
        printf("Intel Skylake and later\n");
        break;
    case INTEL_ATOM:
        printf("Intel Atom\n");
        break;
    case INTEL_SILV:
        printf("Intel Silvermont\n");
        break;
/*
    INTEL_KNIGHT ,                  // Intel Knights Landing (Xeon Phi 7200/5200/3200)
    AMD_ATHLON   ,                 // AMD Athlon
    AMD_ATHLON64 ,                 // AMD Athlon 64 or Opteron
    AMD_BULLD    ,                 // AMD Family 15h (Bulldozer) and 16h?
    AMD_ZEN      ,                 // AMD Family 17h (Zen)
    AMD_ALL      ,                 // AMD any processor
    VIA_NANO     ,                // VIA Nano (Centaur)

*/

    }
}

void print_vendor()
{
    char str[13] = { 0 };
    int regs[4];
    Cpuid(regs, 0);
    memcpy(str, (char*)(regs + 1), 4);
    memcpy(str + 4, (char*)(regs + 3), 4);
    memcpy(str + 8, (char*)(regs + 2), 4);
    printf("Vendor ID: %s\n", str);
    printf("max leaf = %ld\n", regs[0]);
}

void print_PMC()
{
    int regs[4];
    int ver;
    
    /* Get PMU */
    Cpuid(regs, 0xa);
    printf("PMU info\n");
    printf("\tBased on: Intel 64 & IA32 Architectures Software Development manual vol 3\n");
    printf("\t\tChapter 18. Performance monitoring\n");
    printf("\tEAX data:\n");
    ver = (int)(regs[0] & 0xff);
    printf("\t\tRaw value: 0x%x\n", (uint32_t)regs[0]);
    printf("\t\tPM arch v: %d\n", ver);
    printf("\t\tPMC #    : %d\n", (uint32_t)(regs[0]>>8) & 0xff);
    printf("\t\tPMC width: %d\n", (uint32_t)(regs[0]>>16) & 0xff);
    if( ver > 1 ){
        printf("\tEDX data:\n");
        printf("\t\tRaw value           : 0x%x\n", (uint32_t)regs[3]);
        printf("\t\tfixd-func (ff) PMC #: %d\n", (uint32_t)(regs[3] & 0x1f));
        printf("\t\tff PMC width        : %d\n", (uint32_t)((regs[3] >> 5) & 0xff));
    }

}

int main(int argc, char* argv[]) 
{
    print_vendor();
    print_muAch();
    print_PMC();
    return 0;
}