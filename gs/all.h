#include "types.h"
#include "bitmath.h"
#include "endian.h"

#if defined(__sh2__) || defined(__m68k__)
#include "scsp.h"
#endif

#if defined(__sh2__)
#include "smpc.h"
#include "sh2.h"
#include "scu.h"
#include "vdp1.h"
#include "vdp2.h"
#include "cdb.h"
#include "bios.h"
#endif
