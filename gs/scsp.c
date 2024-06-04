#include "types.h"
#include "scsp.h"
#include "smpc.h"

void scsp_init(void)
{
#ifndef __m68k__
 smpc_sound_off();
#endif

 SCSP(0x100400) = (1 << 9);

 for(unsigned slot = 0; slot < 32; slot++)
 {
  SCSP_SREG(slot, 0x00) = 0x0000;
  SCSP_SREG(slot, 0x0A) = 0x001F;
 }
 SCSP_SREG(0, 0x00) = 1 << 12;

 scsp_wait_full_samples(256);

 for(unsigned t = 0; t < 2; t++)
 {
  for(uint32 i = 0x100000; i < 0x100400; i += 2)
   SCSP(i) = 0;

  for(uint32 i = 0x100402; i < 0x101000; i += 2)
   SCSP(i) = 0;

  scsp_wait_full_samples(256);
 }

 SCSP_SCIRE = 0xFFFF;
 SCSP_MCIRE = 0xFFFF;

#ifndef __m68k__
 for(unsigned i = 0x25A00000; i < 0x25A80000; i += 2)
  *(volatile uint16*)i = 0;
#endif
}

