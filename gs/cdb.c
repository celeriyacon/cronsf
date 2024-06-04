/*
 * gs/cdb.c
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

#include "cdb.h"
#include "sh2.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

void cdb_command(uint16* results, uint16 arg0, uint16 arg1, uint16 arg2, uint16 arg3)
{
 HIRQ = 0;
 CDB_CR(0) = arg0;
 CDB_CR(1) = arg1;
 CDB_CR(2) = arg2;
 CDB_CR(3) = arg3;

 while(!(HIRQ & 1));

 sh2_wait_approx(1000);

 results[0] = CDB_CR(0);
 results[1] = CDB_CR(1);
 results[2] = CDB_CR(2);
 results[3] = CDB_CR(3);
}

static int simple_command(uint16 arg0, uint16 arg1, uint16 arg2, uint16 arg3)
{
 uint16 results[4];

 cdb_command(results, arg0, arg1, arg2, arg3);
 //
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return -1;

 return 0;
}

int cdb_cmd_init(uint8 flags, uint16 standby, uint8 ecc, uint8 retry)
{
 int ret = 0;

 if((ret = simple_command((COMMAND_INIT << 8) | flags, standby, 0, (ecc << 8) | retry)) < 0)
  return ret;

 if(flags & 0x01)
 {
  const uint32 m = (HIRQ_EFLS | HIRQ_ECPY | HIRQ_EHST | HIRQ_ESEL | HIRQ_CMOK);

  while((HIRQ & m) != m)
  {
   //
  }
 }

 return ret;
}

/*
int cdb_cmd_get_toc(void)
{
 return -1;
}
*/


int cdb_auth(void)
{
 uint16 results[4];
 uint8 auth;

 do
 {
  cdb_command(results, COMMAND_GET_AUTH << 8, 0, 0, 0);
 } while((results[0] >> 8) == STATUS_REJECTED || ((results[0] >> 8) & STATUS_WAIT));

 auth = results[1] & 0xFF;
 printf("Auth0: 0x%02x\n", auth);

 if(auth == 0xFF || auth == 0x00)
 {
  do
  {
   cdb_command(results, (COMMAND_AUTH_DEVICE << 8), 0, 0x00 << 8, 0);
  } while((results[0] >> 8) & STATUS_WAIT);

  if((results[0] >> 8) == STATUS_REJECTED)
   return -1;

  do
  {
   cdb_command(results, COMMAND_GET_AUTH << 8, 0, 0, 0);
  } while((results[0] >> 8) == STATUS_REJECTED || ((results[0] >> 8) & STATUS_WAIT));

  auth = results[1] & 0xFF;
  printf("Auth1: 0x%02x\n", auth);
  if(auth == 0xFF || auth == 0x00)
   return -1;

  return 1;
 }

 return 0;
}


int cdb_cmd_set_cddevconn(uint8 fnum)
{
 return simple_command((COMMAND_SET_CDDEVCONN << 8), 0, fnum << 8, 0);
}

int cdb_cmd_play(uint32 start, uint32 end, uint8 mode)
{
 uint16 results[4];

 cdb_command(results,/**/ (COMMAND_PLAY << 8) | ((start >> 16) & 0xFF), start, ((end >> 16) & 0xFF) | (mode << 8), end);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return -1;

 if((status & 0x0F) == STATUS_FATAL || (status & 0x0F) == STATUS_NODISC || (status & 0x0F) == STATUS_OPEN)
  return -1;

 return 0;
}

int cdb_cmd_seek(uint32 target)
{
 uint16 results[4];

 cdb_command(results,/**/ (COMMAND_SEEK << 8) | ((target >> 16) & 0xFF), target, 0, 0);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return CDB_ERROR_REJECTED;

 if((status & 0x0F) == STATUS_FATAL || (status & 0x0F) == STATUS_NODISC || (status & 0x0F) == STATUS_OPEN)
  return CDB_ERROR_REJECTED;

 return 0;
}

int cdb_cmd_set_filtconn(uint8 fnum, uint8 flags, uint8 true_conn, uint8 false_conn)
{
 return simple_command((COMMAND_SET_FILTCONN << 8) | flags, (true_conn << 8) | false_conn, fnum << 8, 0);
}

int cdb_cmd_set_filtmode(uint8 fnum, uint8 mode)
{
 return simple_command((COMMAND_SET_FILTMODE << 8) | mode, 0, fnum << 8, 0);
}

int cdb_cmd_set_filtrange(uint8 fnum, uint32 fad, uint32 count)
{
 return simple_command((COMMAND_SET_FILTRANGE << 8) | ((fad >> 16) & 0xFF), fad, (fnum << 8) | ((count >> 16) & 0xFF), count);
}

int cdb_cmd_set_filtsubhc(uint8 fnum, uint8 channel, uint8 sub_mode, uint8 sub_mode_mask, uint8 coding_info, uint8 coding_info_mask, uint8 file)
{
 return simple_command((COMMAND_SET_FILTSUBHC << 8) | channel, (sub_mode_mask << 8) | coding_info_mask, (fnum << 8) | file, (sub_mode << 8) | coding_info);
}

int cdb_cmd_set_seclen(uint8 get_seclen, uint8 put_seclen)
{
 return simple_command((COMMAND_SET_SECLEN << 8) | get_seclen, put_seclen << 8, 0, 0);
}

int cdb_cmd_reset_sel(uint8 flags, uint8 pnum)
{
 uint16 results[4];

 cdb_command(results, /**/(COMMAND_RESET_SEL << 8) | flags, 0, (pnum << 8), 0);
 //
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return -1;

 while(!(HIRQ & HIRQ_ESEL))
 {
 //
 }
 return 0;
}

int cdb_cmd_get_secnum(uint8 pnum)
{
 uint16 results[4];

 cdb_command(results, (COMMAND_GET_SECNUM << 8), 0, pnum << 8, 0);
 //
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return -1;

 return results[3] & 0xFF;
}

int cdb_cmd_get_secinfo(uint8 pnum, uint16 offs, uint32* fad)
{
 uint16 results[4];

 cdb_command(results, (COMMAND_GET_SECINFO << 8), offs, pnum << 8, 0);
 //
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return -1;

 if(fad)
  *fad = ((results[0] & 0xFF) << 16) | results[1];

 return 0;
}

int cdb_cmd_get_secdata(uint8 pnum, uint16 offs, uint16 count)
{
 uint16 results[4];

 cdb_command(results, /**/(COMMAND_GET_SECDATA << 8), offs, pnum << 8, count);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return CDB_ERROR_REJECTED;

 if(status & STATUS_WAIT)
  return CDB_ERROR_WAIT;

 while(!(HIRQ & HIRQ_DRDY))
 {
  //
 }

 return 0;
}

int cdb_cmd_del_secdata(uint8 pnum, uint16 offs, uint16 count)
{
 uint16 results[4];

 cdb_command(results, /**/(COMMAND_DEL_SECDATA << 8), offs, pnum << 8, count);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return CDB_ERROR_REJECTED;

 if(status & STATUS_WAIT)
  return CDB_ERROR_WAIT;

 while(!(HIRQ & HIRQ_EHST))
 {
  //
 }

 return 0;
}

int cdb_cmd_getdel_secdata(uint8 pnum, uint16 offs, uint16 count)
{
 uint16 results[4];

 cdb_command(results, /**/(COMMAND_GETDEL_SECDATA << 8), offs, pnum << 8, count);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return CDB_ERROR_REJECTED;

 if(status & STATUS_WAIT)
  return CDB_ERROR_WAIT;

 while(!(HIRQ & HIRQ_DRDY))
 {
  //
 }

 return 0;
}

int cdb_cmd_end_dataxfer(void)
{
 uint16 results[4];

 cdb_command(results, /**/(COMMAND_END_DATAXFER << 8), 0, 0, 0);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return -1;

 return ((results[0] & 0xFF) << 16) | results[1];
}


//
//
//
//
static uint8 stream_buffer[2048];
static uint32 stream_buffer_offs;
static int32 stream_pos;
static int32 stream_target_pos;

static uint32 stream_pb_offs;
static uint32 stream_pb_count;
static uint32 stream_pb_fad[0xC8];

static int init_stream(bool internal_reinit)
{
 int ret = 0;

 if(internal_reinit)
 {
  stream_buffer_offs = 2048 + (stream_buffer_offs % 2048);
 }
 else
 {
  stream_target_pos = 96 * 2048;
  stream_pos = 0x7FFFFFFF;
  stream_buffer_offs = 0;
 }

 stream_pb_offs = 0;
 stream_pb_count = 0;
 for(unsigned i = 0; i < 0xC8; i++)
  stream_pb_fad[i] = (uint32)-1;
 //
 //
 if((ret = cdb_cmd_set_cddevconn(0xFF)) < 0)
  return ret;

 if((ret = cdb_cmd_seek(CDB_SEEK_PAUSE)) < 0)
  return ret;

 for(;;)
 {
  uint16 results[4];
 
  cdb_command(results, COMMAND_GET_CDSTATUS << 8, 0, 0, 0);
  const uint8 status = results[0] >> 8;

  if((status & 0x0F) == STATUS_PAUSE || (status & 0x0F) == STATUS_STANDBY)
   break;
 }

 if((ret = cdb_cmd_set_filtmode(0x00, 0x00)) < 0)
  return ret;

 if((ret = cdb_cmd_set_filtconn(0x00, 0x03, 0x00, 0xFF)) < 0)
  return ret;

 if((ret = cdb_cmd_reset_sel(0x00, 0x00)) < 0)
  return ret;

 if((ret = cdb_cmd_set_cddevconn(0x00)) < 0)
  return ret;

 if(cdb_cmd_set_seclen(SECLEN_2048, SECLEN_2048))
  return -1;

 return ret;
}


int cdb_stream_init(void)
{
 return init_stream(false);
}

int32 cdb_stream_tell(void)
{
 return stream_target_pos;
}

static int update_pb(void)
{
 int ns;
 int ret = 0;

 if((ns = cdb_cmd_get_secnum(0x00)) < 0)
  return ns;

 assert(ns >= stream_pb_count);
 assert(ns <= 0xC8);

 if(ns > stream_pb_count)
 {
  for(unsigned i = 0; i < (ns - stream_pb_count); i++)
  {
   uint32 fad;

   if((ret = cdb_cmd_get_secinfo(0x00, stream_pb_count, &fad)) < 0)
    return ret;
   //
#if 1
   //printf("CDB buffered fad=0x%06x\n", fad);

   {
    uint32 actual_fad;

    if((ret = cdb_cmd_get_secinfo(0x00, stream_pb_count, &actual_fad)) < 0)
     return ret;

    //printf("%06x %06x\n", fad, actual_fad);
    assert(fad == actual_fad);
   }
#endif
   //
   const unsigned pbo = (stream_pb_offs + stream_pb_count) % 0xC8;

   stream_pb_fad[pbo] = fad;
   stream_pb_count++;
  }
 }

 return ret;
}

static int prune_pb(void)
{
 int ret = 0;

 while(stream_pb_count > 0x80)
 {
  const uint32 fad = stream_pb_fad[stream_pb_offs];

  //printf("Pruning fad=0x%06x...\n", fad);

#if 1
   {
    uint32 actual_fad;

    if((ret = cdb_cmd_get_secinfo(0x00, 0x0000, &actual_fad)) < 0)
     return ret;

    //printf("%06x %06x\n", fad, actual_fad);
    assert(fad == actual_fad);
   }
#endif

  if((ret = cdb_cmd_del_secdata(0x00, 0x00, 1)) < 0)
   return ret;
  //
  stream_pb_fad[stream_pb_offs] = (uint32)-1;
  stream_pb_offs = (stream_pb_offs + 1) % 0xC8;
  stream_pb_count--;
 }

 return ret;
}

int find_cdb_buffer(const uint32 fad)
{
 for(uint32 i = 0; i < 0xC8; i++)
 {
  if(stream_pb_fad[i] == fad)
   return (0xC8 + i - stream_pb_offs) % 0xC8;
 }

 return -1;
}

static int stream_read_real(uint8* dest, uint32 n)
{
 int dr = 0;

 retry:;
 while(n)
 {
  if(stream_buffer_offs >= 2048)
  {
   const uint32 fad = stream_pos / 2048;
   int cdbn;
   bool allow_play = true;

   //printf("Reading fad=0x%06x...\n", fad);

   update_pb();

   while((cdbn = find_cdb_buffer(fad)) < 0)
   {
    if(prune_pb() < 0)
     return -1;

    if(update_pb() < 0)
     return -1;
    //
    //
    {
     uint16 results[4];
 
     cdb_command(results, COMMAND_GET_CDSTATUS << 8, 0, 0, 0);
     const uint8 status = results[0] >> 8;
     bool need_play = false;

     switch(status & 0x0F)
     {
      case STATUS_STANDBY:
	printf("Recovering from STANDBY...\n");
	need_play = true;
	break;

      case STATUS_PAUSE:
	printf("Recovering from PAUSE...\n");
	need_play = true;
	break;

      case STATUS_ERROR:
	printf("Recovering from ERROR...\n");
	need_play = true;
	break;

      case STATUS_PLAY:
	if(allow_play)
	{
	 const int32 latest_fad = stream_pb_fad[(stream_pb_offs + (stream_pb_count - 1)) % 0xC8];

	 if(fad < latest_fad || (fad - latest_fad) >= 16)
         {
          printf("fad=0x%06x, latest_fad=0x%06x\n", fad, latest_fad);
	  need_play = true;
         }
	}
	break;

      case STATUS_SEEK:
      case STATUS_BUSY:
      case STATUS_OPEN:
      case STATUS_NODISC:
	continue;
     }

     if(need_play)
     {
      // Maybe pointless to handle lid opening here
      // unless we also handle it elsewhere.
      const int car = cdb_auth();

      if(car < 0)
       return -1;
      else if(car > 0)
      {
       init_stream(true);
       goto retry;
      }

      printf("Play fad=0x%06x...\n", fad);

      if(cdb_cmd_play(0x800000 | fad, 0x87FFFF, 0x00) >= 0)
       allow_play = false;
     }
    }
   }
   //
   //
#if 1
   {
    uint32 actual_fad;

    if(cdb_cmd_get_secinfo(0x00, cdbn, &actual_fad) < 0)
     return -1;

    //printf("%06x %06x\n", fad, actual_fad);
    assert(fad == actual_fad);
   }
#endif
   //
   //
   int r;

   while((r = cdb_cmd_get_secdata(0x00, cdbn, 1)) < 0)
   {
    if(r != CDB_ERROR_WAIT)
     return -1;
   }

   for(unsigned i = 0; i < 2048; i += 2)
   {
    uint16 tmp = *(volatile uint16*)0x25890000;

    __builtin_memcpy(stream_buffer + i, &tmp, sizeof(tmp));
   }

   cdb_cmd_end_dataxfer();

   while(!(HIRQ & HIRQ_EHST))
   {
    //
   }
   //
   stream_buffer_offs %= 2048;
  }
  //
  //
  size_t l = n;

  if(l > (2048 - stream_buffer_offs))
   l = 2048 - stream_buffer_offs;

  if(dest)
   memcpy(dest, stream_buffer + stream_buffer_offs, l);

  dr += l;
  stream_pos += l;
  stream_buffer_offs += l;
  if(dest)
   dest += l;
  n -= l;
 }

 return dr;
}

int cdb_stream_read(void* dest, uint32 n)
{
#if 0
 // Testing

 if((rand() & 0xFF00) == 0)
 {
  cdb_cmd_seek(CDB_SEEK_STOP);
 }

#endif

 if(stream_pos != stream_target_pos)
 {
  const uint32 old_fad = stream_pos / 2048;
  const uint32 new_fad = stream_target_pos / 2048;

  if(new_fad == old_fad)
  {
   stream_buffer_offs = stream_target_pos % 2048;
   stream_pos = stream_target_pos;
  }
  else
  {
   stream_buffer_offs = 2048 + (stream_target_pos % 2048);
   stream_pos = stream_target_pos;
  }
 }
 int r = stream_read_real((uint8*)dest, n);

 if(r >= 0)
  stream_target_pos += r;
 else
 {
  printf("FAIL: %d\n", r);
 }

 return r;
}

int cdb_stream_seek(int32 offset, int whence)
{
 int64 new_pos = stream_target_pos;

 switch(whence)
 {
  case SEEK_SET:
	new_pos = offset;
	break;

  case SEEK_CUR:
	new_pos += offset;
	break;
  //case SEEK_END:
 }

 if(new_pos < 0)
  return -1;

 stream_target_pos = new_pos;

 return 0;
}

int32 cdb_get_track_fad(uint8 track)
{
 uint16 results[4];

 cdb_command(results, /**/(COMMAND_GET_TOC << 8), 0, 0, 0);
 //
 const uint8 status = results[0] >> 8;

 if(status == STATUS_REJECTED)
  return CDB_ERROR_REJECTED;

 if(status & STATUS_WAIT)
  return CDB_ERROR_WAIT;

 while(!(HIRQ & HIRQ_DRDY))
 {
  //
 }
 //
 //
 uint32 tmp = 0;

 for(unsigned i = 0; i < track; i++)
 {
  tmp = (uint32)(*(volatile uint16*)0x25890000) << 16;
  tmp |= *(volatile uint16*)0x25890000;

  //printf("%08x\n", tmp);
 }

 cdb_cmd_end_dataxfer();

 return tmp & 0xFFFFFF;
}
