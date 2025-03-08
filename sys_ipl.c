/*
 * sys_ipl.c
 *
 * Copyright (C) 2025 celeriyacon - https://github.com/celeriyacon
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

typedef unsigned char uint8;
typedef unsigned int uint32;

static uint32 read32_le(void* p)
{
 uint8* d = (uint8*)p;

 return (d[0] << 0) | (d[1] << 8) | ((uint32)d[2] << 16) | ((uint32)d[3] << 24);
}

static int loadff_directory(void* const dst, uint32 lba, uint32 dir_size)
{
 const uint8* p = (const uint8*)0x02000000 + lba * 2048;
 const uint8* p_bound = p + dir_size;
 uint8 dr[256];

 while(p != p_bound)
 {
  dr[0] = *(p++);

  if(!dr[0])
   continue;
  //
  for(unsigned i = 1; i < dr[0]; i++)
   dr[i] = *(p++);

  p += dr[1];
  //
  const uint32 file_lba = read32_le(dr + 0x02);
  const uint32 file_size = read32_le(dr + 0x0A);
  const uint8 file_flags = dr[0x19];
  const uint8 file_name_len = (dr[0x20] > (0x100 - 0x21)) ? (0x100 - 0x21) : dr[0x20];

  if(file_name_len && !(file_name_len == 1 && dr[0x21] == 0x01) && !(file_flags & 0x2))
  {
   p = (const uint8*)0x02000000 + file_lba * 2048;

   for(uint32 i = 0; i < file_size; i += 4)
   {
    uint32 v = *(uint32*)p;

    if((i + 4) > file_size)
    {
     for(; i < file_size; i++)
     {
      *(uint8*)((uint8*)dst + i) = v >> 24;
      v <<= 8;
     }
    }
    else
     *(uint32*)((uint8*)dst + i) = v;

    p += 4;
   }

   return 1;
  }
 }

 return 0;
}

static __attribute__((noinline,noclone)) int loadff(void* const dst)
{
 const uint8* p;
 uint8 vdheader[8];
 uint32 pvd_offset = 0;
 uint32 evd_offset = 0;

 for(unsigned i = 0; i < 256; i++)
 {
  const uint32 offs = 0x8000 + (i * 0x800);
  p = (const uint8*)0x02000000 + offs;

  for(uint32 j = 0; j < sizeof(vdheader); j++)
   vdheader[j] = *(p++);

  int match = 1;
  static const uint8 sig[5] = { 'C', 'D', '0', '0', '1' };

  for(uint32 j = 0; j < sizeof(sig); j++)
   match &= (sig[j] == vdheader[1 + j]);

  if(match)
  {
   const uint8 t = vdheader[0];
   const uint8 ver = vdheader[6];

   if(t == 0xFF)
    break;
   else if(t == 0x01)
    pvd_offset = offs;
   else if(t == 0x02)
   {
    if(ver == 0x02)
     evd_offset = offs;
   }
  }
 }
 //
 if(!pvd_offset && !evd_offset)
  return 0;
 else
 {
  const uint32 Xvd_offset = (evd_offset ? evd_offset : pvd_offset);
  uint8 dr[34];

  p = (const uint8*)0x02000000 + Xvd_offset;
  p += 156;

  for(uint32 i = 0; i < sizeof(dr); i++)
   dr[i] = *(p++);

  const uint32 root_dir_lba = read32_le(dr + 0x02);
  const uint32 root_dir_len = read32_le(dr + 0x0A);

  return loadff_directory(dst, root_dir_lba, root_dir_len);
 }
}

void start(void) asm("start") __attribute__((section(".init")));
void start(void)
{
 void* const dst = (void*)*(uint32*)0x060020F0;

 if(!*(uint32*)0x06000290)
 {
  if(!loadff(dst))
  {
   for(;;) ;
  }
 }
 //
 void (*fnp)(void) = (void(*)(void))dst;

 return fnp();
}
