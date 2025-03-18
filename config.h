//
// Define to the maximum supported NSF ROM data size in memory, in bytes.
// The size should be a multiple of 4096.
//
#define NSFCORE_MAX_ROM_SIZE 0xA0000

//
// Define to (CPU Frequency / 44100 / 3) when running on the slave CPU so the APU<->SCSP time
// synchronization won't hog the bus.
//
// Interrupts *MUST* be disabled before calling nsfcore_run_frame() when
// this option is enabled.
//
// #define APU_BUSYWAIT_NO_HOG_BUS ((1746818182 / 65) / 44100 / 3) // NTSC 26.87MHz
// #define APU_BUSYWAIT_NO_HOG_BUS ((1746818182 / 61) / 44100 / 3) // NTSC 28.64MHz
//
// #define APU_BUSYWAIT_NO_HOG_BUS ((1734687500 / 65) / 44100 / 3) // PAL 26.69MHz
// #define APU_BUSYWAIT_NO_HOG_BUS ((1734687500 / 61) / 44100 / 3) // PAL 28.44MHz

//
// Define to 1, 2, or 3 to enable some APU debugging checks and printf()s
//
// Don't enable this unless you know what you're doing, as it has timing effects which
// can break the APU emulation.
//
// #define APU_DEBUG 1


//
// Define to 1 to enable rough benchmarking of expansion chip code.
//
// Don't enable this unless you know what you're doing, as it has timing effects which
// can break the APU emulation.
//
// #define EXCHIP_BENCH 1


//
// Define to 1 to enable a pre-resampler sine sweep in the Namco 163,
// Sunsoft 5B, and FDS code, and silence base APU output.
//
// #define EXCHIP_SINESWEEP 1

//
// Define to 1 to enable consistency checks for master CPU sound chip
// emulation vs slave CPU sound chip emulation.  Currently only affects
// FDS and MMC5 emulation.
//
// Don't enable this unless you know what you're doing,
//
// #define EXCHIP_CONSISTENCY_CHECK 1

