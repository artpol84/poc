#if defined(__x86_64__)

#include <arch_x86.h>

static inline void ucs_x86_cpuid(uint32_t level,
                                                  uint32_t *a, uint32_t *b,
                                                  uint32_t *c, uint32_t *d)
{
    asm volatile ("cpuid\n\t"
                  : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
                  : "0"(level));
}

#define X86_CPUID_GET_MAX_VALUE   0x80000000u
#define X86_CPUID_INVARIANT_TSC   0x80000007u
static int ucs_x86_invariant_tsc()
{
    uint32_t _eax, _ebx, _ecx, _edx;

    ucs_x86_cpuid(X86_CPUID_GET_MAX_VALUE, &_eax, &_ebx, &_ecx, &_edx);
    if (_eax <= X86_CPUID_INVARIANT_TSC) {
        goto warn;
    }

    ucs_x86_cpuid(X86_CPUID_INVARIANT_TSC, &_eax, &_ebx, &_ecx, &_edx);
    if (!(_edx & (1 << 8))) {
        goto warn;
    }

    return 1;

warn:
    printf("x86: CPU does not support invariant TSC, using fallback timer\n");

    return 0;
}

static double ucs_arch_x86_tsc_freq_from_cpu_model()
{
    char buf[256];
    char model[256];
    char *rate;
    char newline[2];
    double ghz, max_ghz;
    FILE* f;
    int rc;
    int warn;

    f = fopen("/proc/cpuinfo","r");
    if (!f) {
        return -1;
    }

    warn = 0;
    max_ghz = 0.0;
    while (fgets(buf, sizeof(buf), f)) {
        rc = sscanf(buf, "model name : %s", model);
        if (rc != 1) {
            continue;
        }

        rate = strrchr(buf, '@');
        if (rate == NULL) {
            continue;
        }

        rc = sscanf(rate, "@ %lfGHz%[\n]", &ghz, newline);
        if (rc != 2) {
            continue;
        }

        max_ghz = ucs_max(max_ghz, ghz);
        if (max_ghz != ghz) {
            warn = 1;
            break;
        }
    }
    fclose(f);

    if (warn) {
        printf("Conflicting CPU frequencies detected, using fallback timer\n");
        return -1;
    }

    return max_ghz * 1e9;
}

int _x86_enable_rdtsc;
double _x86_tsc_freq = 0.0;

void init_tsc()
{
    double freq;

    if (ucs_x86_invariant_tsc()) {
        freq = ucs_arch_x86_tsc_freq_from_cpu_model();
        if (freq <= 0.0) {
            freq = ucs_arch_x86_tsc_freq_measure();
        }

        _x86_enable_rdtsc = 1;
        _x86_tsc_freq     = freq;
    } else {
        _x86_enable_rdtsc = 0;
    }
}



#endif