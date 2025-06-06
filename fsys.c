/*
 * fsys.c
 *
 * Copyright (C) 2024-2025 celeriyacon - https://github.com/celeriyacon
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

#include "config.h"

#include "gs/types.h"
#include "gs/endian.h"
#include "gs/cdb.h"

#include "fsys.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

enum { max_dir_entries_lo = 1691 };
enum { max_dir_entries_hi = 3725 };
enum { max_dir_entries = max_dir_entries_lo + max_dir_entries_hi };

LORAM_BSS static fsys_dir_entry_t dir_entries_lo[max_dir_entries_lo];
VDP2_BSS static fsys_dir_entry_t dir_entries_hi[max_dir_entries_hi];

static uint32 current_dir_lba;
static uint32 current_dir_size;

static uint32 file_list_count;
static uint32 dir_list_count;
static int32 iso_base_fad;

static const char* fsys_error;

const char* fsys_get_error(void)
{
 return fsys_error;
}

static const uint8* ms_base;
static uint32 ms_size;
static int32 ms_pos;

static int mem_stream_seek(int32 offset, int whence)
{
 int64 new_pos = ms_pos;

 switch(whence)
 {
  case SEEK_CUR:
	new_pos = (int64)ms_pos + offset;
	break;

  case SEEK_SET:
	new_pos = offset;
	break;

  case SEEK_END:
	new_pos = (int64)ms_size + offset;
	break;
 }

 if(new_pos < 0 || (int32)new_pos != new_pos)
  return -1;

 ms_pos = new_pos;

 return 0;
}

static int32 mem_stream_tell(void)
{
 return ms_pos;
}

static int mem_stream_read(void* dest, uint32 n)
{
 if(ms_pos >= ms_size)
  n = 0;
 else if(n > (ms_size - ms_pos))
  n = ms_size - ms_pos;

 memcpy(dest, ms_base + ms_pos, n);
 ms_pos += n;

 return n;
}

static int (*stream_seek)(int32 offset, int whence);
static int32 (*stream_tell)(void);
static int (*stream_read)(void* dest, uint32 n);



static fsys_dir_entry_t* get_dir_entry_(unsigned index)
{
 if(index < max_dir_entries_lo)
  return &dir_entries_lo[index];

 return &dir_entries_hi[index - max_dir_entries_lo];
}

fsys_dir_entry_t* fsys_get_dir_entry(unsigned index)
{
 return get_dir_entry_((index < dir_list_count) ? index : (max_dir_entries - 1 - (index - dir_list_count)));
}

static bool read_iso_directory(uint32 lba, uint32 dir_size, fsys_filter_t filter)
{
 bool ret = true;
 const uint32 rdoffs = (iso_base_fad + lba) * 2048;
 uint8 dr[256];

 file_list_count = 0;
 dir_list_count = 0;

 current_dir_lba = lba;
 current_dir_size = dir_size;

 stream_seek(rdoffs, SEEK_SET);

 while(stream_tell() < (rdoffs + dir_size))
 {
  stream_read(dr, 1);

  if(!dr[0])
   continue;
  //
  stream_read(dr + 1, dr[0] - 1);
  stream_seek(dr[1], SEEK_CUR);
  //
  const uint32 file_lba = read32_le(dr + 0x02);
  const uint32 file_size = read32_le(dr + 0x0A);
  const uint8 file_flags = dr[0x19];
  const uint8 file_name_len = (dr[0x20] > (0x100 - 0x21)) ? (0x100 - 0x21) : dr[0x20];
  char file_name[256 + 1];

  if(!file_name_len)
  {
  }
  else
  {
   memcpy(file_name, dr + 0x21, file_name_len);
   file_name[file_name_len] = 0;

   //printf("%s --- 0x%08x %u\n", file_name, file_lba, file_size);

   for(size_t i = file_name_len - 1; i > 0; i--)
   {
    if(file_name[i] == ';')
    {
     file_name[i] = 0;
     break;
    }
    if(!isdigit(file_name[i]))
     break;
   }

   if(strlen(file_name) > 0 && !(file_name_len == 1 && file_name[0] == 0x01))
   {
    const bool is_dir = (file_flags & 0x2);
    const uint32 total_list_count = file_list_count + dir_list_count;

    if(total_list_count >= max_dir_entries)
    {
     fsys_error = "Too many directory entries.";
     ret = false;
     break;
    }
    else
    {
     uint16 ft = (is_dir ? FILE_TYPE_DIRECTORY : FILE_TYPE_UNKNOWN);

     if(!filter || filter(file_name, &ft))
     {
      fsys_dir_entry_t* d = get_dir_entry_(is_dir ? dir_list_count : (max_dir_entries - 1 - file_list_count));
      const size_t fll = ((file_name_len >= sizeof(d->name)) ? sizeof(d->name) - 1 : file_name_len);

      memcpy(d->name, file_name, fll);
      d->name[fll] = 0;
      d->lba = file_lba;
      d->size = file_size;
      d->file_type = ft;

      if(is_dir)
       dir_list_count++;
      else
       file_list_count++;
     }
    }
   }
  }
 }

 return ret;
}

bool fsys_change_dir_de(const fsys_dir_entry_t* de, fsys_filter_t filter)
{
 return read_iso_directory(de->lba, de->size, filter);
}

bool fsys_change_dir(unsigned index, fsys_filter_t filter)
{
 fsys_dir_entry_t* de = fsys_get_dir_entry(index);

 return read_iso_directory(de->lba, de->size, filter);
}

bool fsys_save_cur_dir(uint32* lba, uint32* size)
{
 *lba = current_dir_lba;
 *size = current_dir_size;

 return true;
}

bool fsys_restore_cur_dir(uint32 lba, uint32 size, fsys_filter_t filter)
{
 return read_iso_directory(lba, size, filter);
}


bool fsys_init(const unsigned track, uint8* mem_base, uint32 mem_size, fsys_filter_t filter)
{
 uint8 vdheader[8];
 uint32 pvd_offset = 0;
 uint32 evd_offset = 0;

 fsys_error = NULL;

 if(mem_base)
 {
  stream_seek = mem_stream_seek;
  stream_tell = mem_stream_tell;
  stream_read = mem_stream_read;

  ms_base = mem_base;
  ms_size = mem_size;
  ms_pos = 0;

  iso_base_fad = 0;
 }
 else
 {
  stream_seek = cdb_stream_seek;
  stream_tell = cdb_stream_tell;
  stream_read = cdb_stream_read;

  iso_base_fad = cdb_get_track_fad(track);
  printf("Track %u fad=0x%06x\n", track, iso_base_fad);
 }
 //
 //
 for(unsigned i = 0; i < 256; i++)
 {
  const uint32 offs = iso_base_fad * 2048 + 0x8000 + (i * 0x800);

  stream_seek(offs, SEEK_SET);
  stream_read(vdheader, sizeof(vdheader));

  if(!memcmp(vdheader + 1, "CD001", 5))
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
 {
  fsys_error = "Missing PVD/EVD.";
  return false;
 }
 else
 {
  const uint32 Xvd_offset = (evd_offset ? evd_offset : pvd_offset);
  uint8 dr[34];

  printf("%08x\n", Xvd_offset);

  stream_seek(Xvd_offset, SEEK_SET);

  stream_seek(156, SEEK_CUR);
  stream_read(dr, 34);

  const uint32 root_dir_lba = read32_le(dr + 0x02);
  const uint32 root_dir_len = read32_le(dr + 0x0A);

  printf("root_dir_lba=%u\n", root_dir_lba);
  printf("root_dir_len=%u\n", root_dir_len);

  return read_iso_directory(root_dir_lba, root_dir_len, filter);
 }
}

unsigned fsys_num_dir_entries(void)
{
 return dir_list_count + file_list_count;
}

fsys_dir_entry_t* fsys_find_file(const char* name)
{
 const uint32 tc = fsys_num_dir_entries();

 for(uint32 i = 0; i < tc; i++)
 {
  fsys_dir_entry_t* d = fsys_get_dir_entry(i);

  if(!strcasecmp(d->name, name))
   return d;
 }

 return NULL;
}

static fsys_dir_entry_t* opf;

bool fsys_open_de(fsys_dir_entry_t* de)
{
 opf = de;

 if(!opf)
 {
  fsys_error = "NULL pointer.";
  return false;
 }

 stream_seek((iso_base_fad + opf->lba) * 2048, SEEK_SET);

 return true;
}

bool fsys_open(const char* name)
{
 return fsys_open_de(fsys_find_file(name));
}

ssize_t fsys_read(void* ptr, size_t count)
{
 const int32 csfstart = (iso_base_fad + opf->lba) * 2048;
 const int32 csfbound = csfstart + opf->size;
 const int32 cspos = stream_tell();

 assert(cspos >= csfstart);

 if(cspos >= csfbound)
  return 0;

 if(count > (csfbound - cspos))
  count = csfbound - cspos;

 int32 rv = stream_read(ptr, count);

 if(rv < 0)
  fsys_error = "CDB error.";

 return rv;
}

int32 fsys_tell(void)
{
 const int32 csfstart = (iso_base_fad + opf->lba) * 2048;
 const int32 cspos = stream_tell();

 assert(cspos >= csfstart);

 return cspos - csfstart;
}

int fsys_seek(int32 new_position)
{
 const int32 csfstart = (iso_base_fad + opf->lba) * 2048;

 if(new_position < 0)
  return -1;

 return stream_seek(csfstart + new_position, SEEK_SET);
}

int32 fsys_size(void)
{
 return opf->size;
}


