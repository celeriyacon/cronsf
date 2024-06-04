/*
 * gentables.cpp
 *
 * Copyright (C) 2024 celeriyacon - https://github.com/celeriyacon
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#undef NDEBUG
#include <assert.h>

#include <algorithm>
#include <memory>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

static const unsigned noise_period_table[2][0x10] =
{
 // NTSC
 {
  0x004, 0x008, 0x010, 0x020, 0x040, 0x060, 0x080, 0x0A0,
  0x0CA, 0x0FE, 0x17C, 0x1FC, 0x2FA, 0x3F8, 0x7F2, 0xFE4
 },

 // PAL
 {
  0x004, 0x008, 0x00E, 0x01E, 0x03C, 0x058, 0x076, 0x094,
  0x0BC, 0x0EC, 0x162, 0x1D8, 0x2C4, 0x3B0, 0x762, 0xEC2
 }
};

#define SAMPLE_RATE 44100

#define LN_FBIN_COUNT 6	// 16 max
#define SN_FBIN_COUNT 12 // 16 max
#define SN_VARIANT_COUNT 4

#define SAW_FBIN_COUNT (16U + 14 * 2)
#define TRI_FBIN_COUNT (16U + 14 * 2)

static bool pal;
static double apu_freq;
static uint32 time_scale_ntsc;
static uint32 time_scale_pal;

static int8 saw_waveform[SAW_FBIN_COUNT][1024];
static int8 tri_waveform[TRI_FBIN_COUNT][2048];

static int8 sn_waveform[16 * SN_VARIANT_COUNT][1024];
static int8 ln_waveform[LN_FBIN_COUNT][65536];


template<unsigned n, unsigned num_samples>
static inline void synthpt(double* input, double* output, unsigned max_harm)
{
 double siney[num_samples];
 double p[num_samples];

 assert((num_samples % n) == 0);

 for(unsigned i = 0; i < num_samples; i++)
 {
  siney[i] = sin(M_PI * 2 * i / num_samples);
 }

 for(unsigned i = 0; i < num_samples; i++)
 {
  double v = 0;

  for(unsigned harm = 1; harm <= max_harm; harm++)
  {
   unsigned ph0 = (i * harm) % num_samples;
   unsigned ph1 = ((i + num_samples - num_samples / n) * harm) % num_samples;

   v += (siney[ph0] - siney[ph1]) / harm;
  }

  v *= (1.0 / M_PI);
  v += 1.0 / n;
  p[i] = v;
 }

 for(unsigned j = 0; j < num_samples; j++)
 {
  double v = 0;

  for(unsigned i = 0; i < n; i++)
   v += input[i] * p[(j + num_samples - (num_samples * i / n)) % num_samples];

  output[j] = v;
 }
}


template<int n, int num_samples>
static inline void synthptex(double* input, double* output, unsigned max_harm)
{
 const unsigned ns_n = (num_samples * n);
 double samples[num_samples] = { 0 };
 double siney[num_samples * n];

 for(unsigned i = 0; i < num_samples * n; i++)
  siney[i] = sin(M_PI * 2 * i / (num_samples * n)) * (1.0 / M_PI);

 for(int i = 0; i < n; i++)
 {
  for(int j = 0; j < num_samples; j++)
  {
   double v = 0;
   unsigned iph = j * n + (n - i) * num_samples;

   for(unsigned harm = 1; harm <= max_harm; harm++)
   {
    v += (siney[(iph * harm) % ns_n] - siney[((iph - num_samples) * harm) % ns_n]) / harm;
   }

   v += 1.0 / n;

   samples[j] += input[i] * v;
  }
 }

 for(unsigned i = 0; i < num_samples; i++)
  output[i] = samples[i];
}


static inline double nuttall4(double x)
{
 x += 0.5;

 return 0.355768 - 0.487396 * cos(2 * M_PI * x) + 0.144232 * cos(4 * M_PI * x) - 0.012604 * cos(6 * M_PI * x);
}

template<unsigned num_input_samples, unsigned num_output_samples>
static void resample(double* input, double* output)
{
 const double cutoff = 0.495;
 const int hn = 256;
 const int wn = (hn * 2) + 1;

 for(unsigned i = 0; i < num_output_samples; i++)
 {
  const double ifrp = (double)i * num_input_samples / num_output_samples;
  const unsigned input_pos = floor(0.5 + ifrp);
  const double input_ph = input_pos - ifrp;
  double acc = 0;

  for(int c = hn - 1; c >= 0; c--)
  {
   double k0 =  1 + c + input_ph;
   double k1 = -1 - c + input_ph;
   double w0 = nuttall4(k0 / wn);
   double w1 = nuttall4(k1 / wn);
   double t;

   t  = w0 * input[(input_pos +  1 + c) % num_input_samples] * sin(M_PI * 2 * k0 * cutoff) / (M_PI * k0);
   t += w1 * input[(input_pos + num_input_samples - 1 - c) % num_input_samples] * sin(M_PI * 2 * k1 * cutoff) / (M_PI * k1);
   acc += t;
  }

  {
   double k = 0 + input_ph;

   if(!k)
    acc += input[input_pos % num_input_samples] * (2 * cutoff);
   else
    acc += nuttall4(k / wn) * input[input_pos % num_input_samples] * sin(M_PI * 2 * k * cutoff) / (M_PI * k);
  }

  output[i] = acc;
 }
}


template<unsigned num_samples>
static void reduce(double* input, int8* output)
{
 for(unsigned i = 0; i < num_samples; i++)
 {
  int temp;

  temp = ((int)(input[i] * 256) + ((rand() >> 12) & 0xFF)) >> 8;
  //temp = (int)floor(0.5 + input[i]);
  assert(temp >= -127 && temp <= 127);
  output[i] = temp;
 }
}

template<unsigned num_samples>
static void reduce(double* input, int16* output)
{
 for(unsigned i = 0; i < num_samples; i++)
 {
  int temp;

  temp = floor(0.5 + input[i] * 256);

  assert(temp >= -127 * 256 && temp <= 127 * 256);
  output[i] = temp;
 }
}

static void noise_long(void)
{
 static double ln_cont_wf[32767];
 static double ln_disc_wf[32767 * 2];
 static double ln_output_wf[65536];
 uint32 lfsr = 1;

 for(unsigned i = 0; i < 32767; i++)
 {
  ln_cont_wf[i] = ((int32)(((lfsr & 1) ^ 1) * 130) - 65);
  //
  const bool fb = ((lfsr >> 1) ^ lfsr) & 1;

  lfsr = (lfsr >> 1) | (fb << 14);
 }

 for(unsigned nf = 0; nf < LN_FBIN_COUNT; nf++)
 {
  double freq = apu_freq / noise_period_table[pal][nf & 0xF] / 32767.0;
  unsigned max_harm = (unsigned)floor(std::min<double>((32767 * 2) / 2.0, SAMPLE_RATE / 2.0 / freq));

  //printf("Long %u max harm: %u\n", nf, max_harm);

  if(nf >= 0x5)
  {
   for(unsigned i = 0; i < 65536; i++)
    ln_output_wf[i] = ln_cont_wf[(i * 32767 + 32768) / 65536];
   //resample<32767 * 2, 65536>(ln_disc_wf, ln_output_wf);
   reduce<65536>(ln_output_wf, ln_waveform[nf]);
  }
  else
  {
   synthpt<32767, 32767 * 2>(ln_cont_wf, ln_disc_wf, max_harm);
   resample<32767 * 2, 65536>(ln_disc_wf, ln_output_wf);
   reduce<65536>(ln_output_wf, ln_waveform[nf]);
  }
 }
}


static void noise_short(void)
{
 double sn_input_wf[93];
 double sn_output_wf[1024];

 for(unsigned variant = 0; variant < SN_VARIANT_COUNT; variant++)
 {
  static const uint16 initial[4] =
  {
   0x03B3, 0x0472, 0x02FA, 0x02B1
  }; 
  uint32 lfsr = initial[variant];

  for(unsigned i = 0; i < 93; i++)
  {
   sn_input_wf[i] = ((int32)(((lfsr & 1) ^ 1) * 130) - 65);
   //
   const bool fb = ((lfsr >> 6) ^ lfsr) & 1;

   lfsr = (lfsr >> 1) | (fb << 14);
  }

  for(unsigned nf = 0; nf < SN_FBIN_COUNT; nf++)
  {
   double freq = apu_freq / noise_period_table[pal][nf & 0xF] / 93;
   unsigned max_harm = (unsigned)floor(std::min<double>(1024 / 2.0, SAMPLE_RATE / 2.0 / freq));

   //printf("Short %u max harm: %u\n", nf, max_harm);

   synthptex<93, 1024>(sn_input_wf, sn_output_wf, max_harm);
   reduce<1024>(sn_output_wf, sn_waveform[variant * 16 + nf]);
  }
 }
}

/*
static unsigned tri_period_to_fbin(const unsigned freq)
{
 unsigned fbi;

 if(freq & 0x400)
  fbi = (0x10 + (0x6 << 1)) | ((freq >> 9) & 0x1);
 else if(freq & 0x200)
  fbi = (0x10 + (0x5 << 1)) | ((freq >> 8) & 0x1);
 else if(freq & 0x100)
  fbi = (0x10 + (0x4 << 1)) | ((freq >> 7) & 0x1);
 else if(freq & 0x080)
  fbi = (0x10 + (0x3 << 1)) | ((freq >> 6) & 0x1);
 else if(freq & 0x040)
  fbi = (0x10 + (0x2 << 1)) | ((freq >> 5) & 0x1);
 else if(freq & 0x020)
  fbi = (0x10 + (0x1 << 1)) | ((freq >> 4) & 0x1);
 else if(freq & 0x010)
  fbi = (0x10 + (0x0 << 1)) | ((freq >> 3) & 0x1);
 else
  fbi = (freq & 0xF);

 return fbi;
}
*/

static unsigned pulse_period_to_fbin(const unsigned freq)
{
 unsigned fbi;

 if(freq & 0x400)
  fbi = (0x10 + (0x6 << 2)) | ((freq >> 8) & 0x3);
 else if(freq & 0x200)
  fbi = (0x10 + (0x5 << 2)) | ((freq >> 7) & 0x3);
 else if(freq & 0x100)
  fbi = (0x10 + (0x4 << 2)) | ((freq >> 6) & 0x3);
 else if(freq & 0x080)
  fbi = (0x10 + (0x3 << 2)) | ((freq >> 5) & 0x3);
 else if(freq & 0x040)
  fbi = (0x10 + (0x2 << 2)) | ((freq >> 4) & 0x3);
 else if(freq & 0x020)
  fbi = (0x10 + (0x1 << 2)) | ((freq >> 3) & 0x3);
 else if(freq & 0x010)
  fbi = (0x10 + (0x0 << 2)) | ((freq >> 2) & 0x3);
 else
  fbi = (freq & 0xF);

 return fbi;
}

static double get_pulse_output_level(uint8 pulse0, uint8 pulse1)
{
 double ret;

 if(!pulse0 && !pulse1)
  return 0;

 ret = 95.88 / (8128.0 / (pulse0 + pulse1) + 100);

 return ret;
}

static double get_tnp_output_level(uint8 triangle, uint8 noise, uint8 pcm)
{
 double ret;

 if(!triangle && !noise && !pcm)
  return 0;

 ret = 159.79 / (1.0 / (triangle / 8227.0 + noise / 12241.0 + pcm / 22638.0) + 100);

 return ret;
}


static void lookup_table(void)
{
 uint16 pulse_period_to_base[2048] = { 0 };
 uint16 pulse_period_to_freq_lo[2048] = { 0 };
 uint16 pulse_period_to_freq_hi[2048] = { 0 };
 uint16 pulse0_phase_reset[16] = { 0 };
 uint16 pulse1_phase_reset[16] = { 0 };
 uint16 pulse0_duty_to_phoffs_a[16] = { 0 };
 uint16 pulse0_duty_to_phoffs_b[16] = { 0 };
 uint16 pulse1_duty_to_phoffs_a[16] = { 0 };
 uint16 pulse1_duty_to_phoffs_b[16] = { 0 };

 uint16 tri_period_to_base[2048] = { 0 };
 uint16 tri_period_to_freq_lo[2048] = { 0 };
 uint16 tri_period_to_freq_hi[2048] = { 0 };

 uint16 noise_period_to_base_lo[256] = { 0 };
 uint16 noise_period_to_base_hi[256] = { 0 };
 uint16 noise_period_to_modemul[256] = { 0 };

 uint16 noise_period_to_freq_lo[256] = { 0 };
 uint16 noise_period_to_freq_hi[256] = { 0 };

 uint16 pulse_volume_lut[256] = { 0 };
 uint16 noise_volume_lut[256] = { 0 };
 //
 //
 static uint8 lump[524288];
 int32 offs = 0;
 FILE* fp;
 //
 //
 const double volume_scale = 5424; //256 * 15;

 memset(lump, 0, sizeof(lump));

 fp = fopen(pal ? "madrs-pal.h" : "madrs.h", "wb");
 assert(fp);

 // Long noise samples need to be aligned to 65536-byte boundaries.
 for(int i = LN_FBIN_COUNT - 1; i >= 0; i--)
 {
  const size_t s = sizeof(ln_waveform[i]);
  assert(s == 65536);

  if(i == 0x0)
  {
   for(int j = 0; j < 65536; j += 2)
   {
    lump[offs] = ln_waveform[i][j];
    offs++;
   }
  }
  else
  {
   memcpy(lump + offs, ln_waveform[i], s);
   offs += s;
  }
  assert(!(offs & 1));
 }

 const uint8 saw_bank64k = offs >> 16;
 {
  uint16 base[SAW_FBIN_COUNT] = { 0 };

  for(unsigned i = 0; i < SAW_FBIN_COUNT; i++)
  {
   const size_t s = sizeof(saw_waveform[i]);

   //assert(s == 1024);
   //assert(s == 2048);

   base[i] = offs + (s / 2);	// + (s / 2) for FM signed displacement.
   if(i >= 8 && i != 0x8 && i != 0xA && i != 0xB && i != 0xC && i != 0xD && i != 0x0F && i != 0x11 && i != 0x2A)
   {
    memcpy(lump + offs + (s / 2), saw_waveform[i],         s / 2);
    memcpy(lump + offs,           saw_waveform[i] + s / 2, s / 2);
    lump[offs + s] = lump[offs]; // Duplicate sample for linear interpolation.
    offs += s + 2;

    assert(!(offs & 1));
   }
  }

  for(unsigned i = 0; i < 2048; i++)
  {
   unsigned period = i;
   uint64 freq;

   freq = ((uint64)(((uint64)1 << 31) * apu_freq / SAMPLE_RATE / 16) * 2 + (period + 1)) / ((period + 1) * 2);
   //
   if(period < 8)
   {
    period = 0;
    freq = 0;
   }
   //
   assert(freq <= 0x7FFFFFFFULL);

   pulse_period_to_base[i] = -base[pulse_period_to_fbin(period)];
   pulse_period_to_freq_lo[i] = freq & 0x7FFF;
   pulse_period_to_freq_hi[i] = freq >> 15;
  }

  for(unsigned i = 0; i < 16; i++)
  {
   const unsigned offs = i & 0xF;

   pulse0_phase_reset[offs] = ((i & 0x2) ? 0x8000 : 0) | (offs & 1);
   pulse1_phase_reset[offs] = ((i & 0x4) ? 0x8000 : 0) | (offs & 1);
  }

  for(unsigned i = 0; i < 16; i++)
  {
   int pulse_i, noise_i;

   if(!i)
   {
    pulse_i = 0;
    noise_i = 0;
   }
   else
   {
    double tri_base = 0;
    double noise_cur = 0;
    double noise_max = 0;
    double pulse, noise;

    for(unsigned tri = 0; tri < 16; tri++)
    {
     tri_base += (1.0 / 16) * get_tnp_output_level(tri, 0, 0);
     noise_cur += (1.0 / 16) * get_tnp_output_level(tri, i, 0);
     noise_max += (1.0 / 16) * get_tnp_output_level(tri, 15, 0);
    }

    pulse = get_pulse_output_level(i, 0) / get_pulse_output_level(15, 0);
    noise = (noise_cur - tri_base) / get_pulse_output_level(15, 0) * 106.0 / 65.0;

    pulse_i = (int)-floor(0.5 + volume_scale * pulse);
    noise_i = (int)-floor(0.5 + volume_scale * noise);
   }

   //printf("0x%01x: pulse=%d noise=%d --- noise_to_pulse_ratio=%f\n", i, pulse_i, noise_i, (double)noise_i / pulse_i);

   for(unsigned j = 0; j < 16; j++)
   {
    pulse_volume_lut[j * 16 + i] = pulse_i;
    noise_volume_lut[j * 16 + i] = noise_i;
   }
  }

  for(unsigned i = 0; i < 16; i++)
  {
   static const uint16 duty_tab[2][4] =
   {
    { 0x0000, 0x0000, 0x0000, 0xC000 },
    { 0xE000, 0xC000, 0x8000, 0x0000 }
   };
   const unsigned ds[2] = { ((i >> 0) & 0x3), ((i >> 2) & 0x3) };
   const unsigned go = 0xE000;

   pulse0_duty_to_phoffs_a[i] = duty_tab[0][ds[0]] + go;
   pulse0_duty_to_phoffs_b[i] = duty_tab[1][ds[0]] + go;
   pulse1_duty_to_phoffs_a[i] = duty_tab[0][ds[1]] + go;
   pulse1_duty_to_phoffs_b[i] = duty_tab[1][ds[1]] + go;
  }
 }
 assert(((offs - 1) >> 16) == saw_bank64k);

 // Triangle map
 {
  int16 tri_map[2048] = { 0 };

  fprintf(fp, "static const uint32 tri_map_addr = 0x%06X;\n\n", (unsigned)(offs + sizeof(tri_map) / 2)); //(unsigned)(offs + s / 2));

  // 0x000 -> 0x000
  // 0x1FF -> 0x1FF
  // 0x200 -> 0x200
  // 0x201 -> 0x1FF
  // 0x3FF -> 0x001
  for(unsigned i = 0; i < 2048; i++)
  {
   uint16 v;

   if(i >= 0x401)
    v = 0x800 - i;
   else
    v = i;

   tri_map[i] = (-v) << 5;
  }

  for(unsigned i = 0; i < (2048 + 1); i++)
  {
   uint16 v = tri_map[(i + 0x400) & 0x7FF];

   lump[offs + 0] = v >> 8;
   lump[offs + 1] = v >> 0;
   offs += 2;
  }
  assert(!(offs & 1));

#if 0
  for(unsigned i = 0; i < TRI_FBIN_COUNT; i++)
  {
   for(unsigned j = 0; j < (2048 + 1); j++)
   {
    printf("%4u: %d - %d\n", j, tri_waveform[i][j & 2047], tri_waveform[i][-(tri_map[j & 2047] >> 5)]);
    assert(tri_waveform[i][j & 2047] == tri_waveform[i][-(tri_map[j & 2047] >> 5)]);
   }
  }
#endif
 }


 // Triangle:
 //printf("%u\n", 65536 - (offs & 0xFFFF));
 //offs = (offs + 0xFFFF) &~ 0xFFFF;
 const uint8 tri_bank64k = offs >> 16;
 {
  uint16 base[TRI_FBIN_COUNT] = { 0 };

  for(unsigned i = 0; i < TRI_FBIN_COUNT; i++)
  {
   base[i] = offs + (sizeof(tri_waveform[i]) / 2 + 2 - 1); // + (s / 4);	// + (s / 4) for FM signed displacement.
   //base[i] = offs + (s / 2);	// + (s / 2) for FM signed displacement.
   if(i < 0x2)
    continue;
   //
   //
   if(i != 0x3 && i != 0x4 && i != 0x5 && i != 0x6 && i != 0x8 && i != 0x9 && i != 0xA && i != 0xB && i != 0xD && i != 0xE && i != 0xF && i != 0x10)
   {
    for(unsigned j = 0; j < (sizeof(tri_waveform[i]) / 2 + 1 + 1); j++)
    {
     uint8 v = tri_waveform[i][j];
     unsigned so = offs + sizeof(tri_waveform[i]) / 2 + 2 - 1 - (int)j;

     lump[so] = v;
    }
    offs += (sizeof(tri_waveform[i]) / 2 + 1 + 1);
   }
   else
    base[i] = base[i - 1];
  }

  for(unsigned i = 0; i < 2048; i++)
  {
   unsigned period = i;
   uint32 freq;

   freq = ((uint64)(((uint64)1 << 31) * apu_freq / SAMPLE_RATE / 32) * 2 + (period + 1)) / ((period + 1) * 2);
   //
   if(period < 2)
   {
    period = 2;
    freq = 0;
   }
   //
   //printf("%4d: 0x%08x\n", i, (unsigned)freq);

   tri_period_to_base[i] = -base[pulse_period_to_fbin(period)];
   tri_period_to_freq_lo[i] = freq & 0x7FFF;
   tri_period_to_freq_hi[i] = freq >> 15;
  }
 }
 assert(((offs - 1) >> 16) == tri_bank64k);

 // Short mode noise
 {
  uint32 base[64] = { 0 };

  //printf("%u\n", 0x400 - (offs & 0x3FF));
  offs = (offs + 0x3FF) &~ 0x3FF;
  //assert(!(offs & 0x3FF));

  for(unsigned i = 0; i < 64; i++)
  {
   //const unsigned variant = i >> 4;
   const unsigned nf = i & 0xF;
   const size_t s = sizeof(sn_waveform[i]);

   assert(s == 1024);
   assert((offs >> 16) == ((offs + 0x3FF) >> 16));

   if(nf >= SN_FBIN_COUNT)
    base[i] = base[(i & ~0xF) + SN_FBIN_COUNT - 1];
   else
   {
    // Offset of 512 to handle sign extension in DSP when translating 16-bit position
    // to 10-bit position with multiplication for short noise mode.
    base[i] = offs + (1024 / 2);
    memcpy(lump + offs, sn_waveform[i], s);
    offs += s;
    assert(!(offs & 1));
   }
  }

  for(unsigned i = 0; i < 256; i++)
  {
   const bool mode = (bool)(i & 0x80);
   const uint8 ps = i & 0x0F;
   const unsigned period = noise_period_table[pal][i & 0x0F];
   uint32 freq;

   if(mode)
    freq = (uint64)(((uint64)1 << (31 - 10)) * apu_freq / SAMPLE_RATE * 1024 / 1023 * 11 / period);
   else
    freq = (uint64)(((uint64)1 << (31 - 16)) * apu_freq / SAMPLE_RATE * 65536 / (32767 * 2) * 2 / period);

   noise_period_to_freq_lo[i] = freq & 0x7FFF;
   noise_period_to_freq_hi[i] = freq >> 15;

   if(mode)
    noise_period_to_modemul[i] = (int16)0x8000 >> (16 - 10);
   else if(ps == 0x0)
    noise_period_to_modemul[i] = 0x8000 >> 1;
   else
    noise_period_to_modemul[i] = 0x8000;

   assert(!(noise_period_to_modemul[i] & 0x7));

   {
    uint16 base_lo;
    uint16 base_hi;

    if(mode)
    {
     const uint32 b = base[i & 0x3F];
     base_lo = b;
     base_hi = b >> 16;
    }
    else
    {
     if(ps == 0x0)
      base_lo = 0 + (32768 / 2);
     else
      base_lo = 0;

     base_hi = LN_FBIN_COUNT - 1 - std::min<int>(LN_FBIN_COUNT - 1, ps);
    }

    assert(base_hi < 0x10);
    noise_period_to_base_lo[i] = -base_lo;
    noise_period_to_base_hi[i] = -(base_hi | (1U << 4) | (1U << 5) | (0 << 7) | (0 << 9) | (1 << 11));
   }
  }
 }
 //
 //
 const uint32 dsp_base = offs &~ 0x1FFF;
 uint16 madrs[32] = { 0 };
 unsigned madr_counter = 0;

 #define APPEND(v,name)				\
 {						\
  const size_t s = sizeof(v);			\
  assert(!(s & 1));				\
  assert((offs - dsp_base) < 65536 * 2);	\
  madrs[madr_counter] = (offs - dsp_base) >> 1;	\
  fprintf(fp, "#define MADR_%s 0x%02X // %4zu\n", name, madr_counter, s >> 1);	\
  for(unsigned appi = 0; appi < s; appi++)	\
   lump[offs + appi] = ((uint8*)(&v))[appi ^ 1];		\
  offs += s;					\
  madr_counter++;				\
 }

 #define APPEND_SO(v,name)							\
 {										\
  const size_t s = sizeof(v);							\
  assert(!(s & 1));								\
  assert((offs - dsp_base) < 65536 * 2);					\
  madrs[madr_counter] = (((offs + (s >> 1)) - dsp_base) >> 1);			\
  fprintf(fp, "#define MADR_%s 0x%02X // %4zu\n", name, madr_counter, s >> 1);	\
  for(unsigned appi = 0; appi < s; appi++)					\
   lump[offs + (((s >> 1) + appi) % s)] = ((uint8*)(&v))[appi ^ 1];		\
  offs += s;									\
  madr_counter++;								\
 }


 APPEND(pulse_period_to_base, "PULSE_BASE")
 APPEND(pulse_period_to_freq_lo, "PULSE_FREQLO")
 APPEND(pulse_period_to_freq_hi, "PULSE_FREQHI")
 APPEND(pulse0_phase_reset, "PULSE0_PHRST")
 APPEND(pulse1_phase_reset, "PULSE1_PHRST")
 APPEND(pulse0_duty_to_phoffs_a, "PULSE0_PHOFFSA")
 APPEND(pulse0_duty_to_phoffs_b, "PULSE0_PHOFFSB")
 APPEND(pulse1_duty_to_phoffs_a, "PULSE1_PHOFFSA")
 APPEND(pulse1_duty_to_phoffs_b, "PULSE1_PHOFFSB")

 APPEND(tri_period_to_base, "TRI_BASE")
 APPEND(tri_period_to_freq_lo, "TRI_FREQLO")
 APPEND(tri_period_to_freq_hi, "TRI_FREQHI")

 APPEND_SO(noise_period_to_base_lo, "NOISE_BASELO")
 APPEND_SO(noise_period_to_base_hi, "NOISE_BASEHI")
 APPEND_SO(noise_period_to_modemul, "NOISE_MODEMUL")

 APPEND_SO(noise_period_to_freq_lo, "NOISE_FREQLO")
 APPEND_SO(noise_period_to_freq_hi, "NOISE_FREQHI")

 APPEND(pulse_volume_lut, "PULSE_VOLUME")
 APPEND(noise_volume_lut, "NOISE_VOLUME")

 {
  uint16 position[5] = { 0 };

  fprintf(fp, "static const uint32 position_addr = 0x%06X;\n\n", offs);

  APPEND(position[0], "TRI_POSITION")
  APPEND(position[1], "PULSE0A_POSITION")
  APPEND(position[2], "PULSE0B_POSITION")
  APPEND(position[3], "PULSE1A_POSITION")
  APPEND(position[4], "PULSE1B_POSITION")
 }

 {
  uint16 pcm_qnlscale[128];

  for(unsigned i = 0; i < 128; i++)
  {
   // 1 / 0.66311739897910929222 ? 
   //double d = 127.0 / (1 / 0.707106781 - 1);
   double d = 127.0 / (1 / 0.66311739897910929222 - 1);
   double tmp = 1.0 / (1 + i / d);

   pcm_qnlscale[i] = std::min<unsigned>(4095, (unsigned)floor(0.5 + (tmp * 4095))) << 3;
  }

  APPEND(pcm_qnlscale, "PCM_QNLSCALE")
  //
  //
  double tri_volume = volume_scale * 106.0 / 121.0 * get_tnp_output_level(15, 0, 0) / get_pulse_output_level(15, 0);

  fprintf(fp, "static const int16 tri_coef_volume = %d;\n", (int)floor(0.5 + tri_volume / 8));
  //
  double pcm_volume = 0;

  for(unsigned i = 0; i < 16; i++)
   pcm_volume += (1.0 / 16) * volume_scale * 106.0 / (127.0 * 0.66311739897910929222) * (get_tnp_output_level(i, 0, 127) - get_tnp_output_level(i, 0, 0)) / get_pulse_output_level(15, 0);

  //printf("%f\n", pcm_volume);

  fprintf(fp, "static const int16 pcm_coef_volume = %d;\n", (int)floor(0.5 + pcm_volume / 8));
 }

 fprintf(fp, "static const uint32 time_scale_ntsc = %u;\n", time_scale_ntsc);
 fprintf(fp, "static const uint32 time_scale_pal = %u;\n", time_scale_pal);

 #undef APPEND
 #undef APPEND_SO


 for(unsigned i = 0; i < 2; i++)
 {
  const unsigned ptnfifo_size = 4096 * sizeof(uint16);
  const unsigned ptnfifo_base = offs;

  fprintf(fp, "#define MADR_PTNFIFO%c 0x%02X\n", 'A' + i, madr_counter);

  madrs[madr_counter] = (ptnfifo_base - dsp_base) / 2 + ptnfifo_size / sizeof(uint16) / 2;
  madr_counter++;

  offs += ptnfifo_size;
 }

 {
  const unsigned cfifo_size = 2048 * sizeof(uint16);
  const unsigned cfifo_base = offs;

  fprintf(fp, "#define MADR_CFIFO 0x%02X\n", madr_counter);
  madrs[madr_counter] = (cfifo_base - dsp_base) / 2 + cfifo_size / sizeof(uint16) / 2;
  madr_counter++;

  offs += cfifo_size;
 }

 {
  static const int8 cfifo_counter_waveform[32 + 1] =
  {
   -0x00, -0x04, -0x08, -0x0C,
   -0x10, -0x14, -0x18, -0x1C,
   -0x20, -0x24, -0x28, -0x2C,
   -0x30, -0x34, -0x38, -0x3C,
   -0x40, -0x44, -0x48, -0x4C,
   -0x50, -0x54, -0x58, -0x5C,
   -0x60, -0x64, -0x68, -0x6C,
   -0x70, -0x74, -0x78, -0x7C,
   -0x80
  };

  fprintf(fp, "static const uint32 cfifo_counter_waveform_addr = 0x%06X;\n\n", (unsigned)offs);

  const size_t s = sizeof(cfifo_counter_waveform);

  memcpy(lump + offs, cfifo_counter_waveform, s);
  offs += (s + 1) &~ 1;
 }

 {
  fprintf(fp, "#define MADR_CFIFO_POSITION 0x%02X\n", madr_counter);
  fprintf(fp, "static const uint32 cfifo_position_addr = 0x%06X;\n\n", offs);

  madrs[madr_counter] = (offs - dsp_base) / 2;
  madr_counter++;

  offs += 2;
 }

 fprintf(fp, "\n\n");

 fprintf(fp, "static const uint8 rbp_init = 0x%02X;\n\n", (unsigned)(dsp_base / sizeof(uint16)) >> 12);

 fprintf(fp, "static const uint8 saw_bank64k = 0x%02X;\n\n", saw_bank64k);
 fprintf(fp, "static const uint8 tri_bank64k = 0x%02X;\n\n", tri_bank64k);

 {
  fprintf(fp, "static const uint16 madrs_init[0x20] = \n");
  fprintf(fp, "{\n");
  for(unsigned i = 0x00; i < 0x20; i++)
  {
   fprintf(fp, " 0x%04X, // 0x%02X ;;; Abs byte offs: 0x%05X\n", madrs[i], i, dsp_base + (madrs[i] << 1));
  }
  fprintf(fp, "};\n");
 }

 fclose(fp);
 //
 //
 {
  fp = fopen(pal ? "scsppal.bin" : "scspntsc.bin", "wb");
  fwrite(&lump, 1, sizeof(lump), fp);
  fclose(fp);
 }
 //
 assert(offs <= 524288);
 printf("SCSP RAM Usage: %u bytes\n", offs);
}

static void saw(void)
{
 const int samples_per_saw_fbin = 1024;

 for(unsigned w = 0; w < SAW_FBIN_COUNT; w++)
 {
  int max_period;

  if(w < 0x10)
   max_period = 1 + w;
  else
   max_period = 1 + ((0x10 + ((w & 0x3) << 2) ) << ((w - 0x10) >> 2));

  //printf("0x%04x %d\n", max_period - 1, w);
  assert(max_period <= 0x800);

  double max_freq = apu_freq / 16 / max_period;
  unsigned max_harm = std::min<double>((samples_per_saw_fbin / 4) - 1, SAMPLE_RATE / 2.0 / max_freq);

  //printf("Saw %u max harm: %u\n", w, max_harm);

  for(unsigned i = 0; i < samples_per_saw_fbin; i++)
  {
   double v = 0;

   for(unsigned harm = 1; harm <= max_harm; harm++)
    v += sin(i * M_PI * 2 * harm / samples_per_saw_fbin) / harm;

   v = (v * 2 / M_PI) * 106;
   int temp = ((int)(v * 256) + ((rand() >> 8) & 0xFF)) >> 8;
   assert(temp >= -127 && temp <= 127);
   
   saw_waveform[w][i] = temp;
  }
 }
}

static void triangle(void)
{
 double tri_input_wf[32];
 double tri_output_wf[2048];

 for(unsigned i = 0x00; i < 0x20; i++)
 {
  int temp = i ^ ((i & 0x10) ? 0x10 : 0x0F);

  tri_input_wf[i] = (double)temp * 121 * 2 / 15 - 121;
 }

 for(unsigned w = 0; w < TRI_FBIN_COUNT; w++)
 {
  int max_period;

  if(w < 0x10)
   max_period = 1 + w;
  else
   max_period = 1 + ((0x10 + ((w & 0x3) << 2) ) << ((w - 0x10) >> 2));

  assert(max_period <= 0x800);

  double max_freq = apu_freq / 32 / max_period;
  unsigned max_harm = (unsigned)floor(std::min<double>(1024 / 2, SAMPLE_RATE / 2.0 / max_freq));

  //printf("Triangle %u max harm: %u\n", w, max_harm);

  synthpt<32, 2048>(tri_input_wf, tri_output_wf, max_harm);

  reduce<2048>(tri_output_wf, tri_waveform[w]);
 }
}

int main(int argc, char* argv[])
{
 const double ideal_ntsc_cpu_freq = 315.0 / 88 * 1000 * 1000 / 2;
 const double ideal_pal_cpu_freq = 625 * (50.0 / 2) * (1135.0 / 4 + 1.0 / 625) * 6.0 / 16;

 time_scale_ntsc = (unsigned)floor(0.5 + (double)SAMPLE_RATE * (1 << 21) / ideal_ntsc_cpu_freq);
 time_scale_pal = (unsigned)floor(0.5 + (double)SAMPLE_RATE * (1 << 21) / ideal_pal_cpu_freq);

 for(unsigned i = 0; i < 2; i++)
 {
  pal = i;
  //
  //
  const double ideal_cpu_freq = (pal ? ideal_pal_cpu_freq : ideal_ntsc_cpu_freq);
  uint32 time_scale = (pal ? time_scale_pal : time_scale_ntsc);

  printf("%f %f\n", ideal_ntsc_cpu_freq, ideal_pal_cpu_freq);

  apu_freq = (double)SAMPLE_RATE * (1 << 21) / time_scale;

  printf("%f %u -- %f\n", apu_freq, time_scale, ideal_cpu_freq * time_scale / (1 << 21));

  printf("Generating %s APU data...\n", (pal ? "PAL" : "NTSC"));

  saw();
  triangle();
  noise_long();
  noise_short();

  lookup_table();
 }
}
