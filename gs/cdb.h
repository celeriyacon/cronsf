/*
 * gs/cdb.h
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

#ifndef GIGASANTA_CDB_H
#define GIGASANTA_CDB_H

#include "types.h"

#define CDB_CR(n) (*(volatile uint16*)(0x25890018 + ((n) << 2)))
#define HIRQ (*(volatile uint16*)0x25890008)
#define HIRQ_MASK (*(volatile uint16*)0x2589000C)

enum
{
 HIRQ_CMOK = 0x0001,
 HIRQ_DRDY = 0x0002,
 HIRQ_CSCT = 0x0004,
 HIRQ_BFUL = 0x0008,
 HIRQ_PEND = 0x0010,
 HIRQ_DCHG = 0x0020,
 HIRQ_ESEL = 0x0040,
 HIRQ_EHST = 0x0080,
 HIRQ_ECPY = 0x0100,
 HIRQ_EFLS = 0x0200,
 HIRQ_SCDQ = 0x0400,

 HIRQ_MPED = 0x0800,
 HIRQ_MPCM = 0x1000,
 HIRQ_MPST = 0x2000
};

enum
{
 COMMAND_GET_CDSTATUS	= 0x00,
 COMMAND_GET_HWINFO	= 0x01,
 COMMAND_GET_TOC	= 0x02,
 COMMAND_GET_SESSINFO	= 0x03,
 COMMAND_INIT		= 0x04,
 COMMAND_OPEN		= 0x05,
 COMMAND_END_DATAXFER	= 0x06,

 COMMAND_PLAY		= 0x10,
 COMMAND_SEEK		= 0x11,
 COMMAND_SCAN		= 0x12,

 COMMAND_GET_SUBCODE	= 0x20,

 COMMAND_SET_CDDEVCONN	= 0x30,
 COMMAND_GET_CDDEVCONN	= 0x31,
 COMMAND_GET_LASTBUFDST	= 0x32,

 COMMAND_SET_FILTRANGE	= 0x40,
 COMMAND_GET_FILTRANGE	= 0x41,
 COMMAND_SET_FILTSUBHC	= 0x42,
 COMMAND_GET_FILTSUBHC	= 0x43,
 COMMAND_SET_FILTMODE	= 0x44,
 COMMAND_GET_FILTMODE	= 0x45,
 COMMAND_SET_FILTCONN	= 0x46,
 COMMAND_GET_FILTCONN	= 0x47,
 COMMAND_RESET_SEL	= 0x48,

 COMMAND_GET_BUFSIZE	= 0x50,
 COMMAND_GET_SECNUM	= 0x51,
 COMMAND_CALC_ACTSIZE	= 0x52,
 COMMAND_GET_ACTSIZE	= 0x53,
 COMMAND_GET_SECINFO	= 0x54,
 COMMAND_EXEC_FADSRCH	= 0x55,
 COMMAND_GET_FADSRCH	= 0x56,

 COMMAND_SET_SECLEN	= 0x60,
 COMMAND_GET_SECDATA	= 0x61,
 COMMAND_DEL_SECDATA	= 0x62,
 COMMAND_GETDEL_SECDATA	= 0x63,
 COMMAND_PUT_SECDATA	= 0x64,
 COMMAND_COPY_SECDATA	= 0x65,
 COMMAND_MOVE_SECDATA	= 0x66,
 COMMAND_GET_COPYERR	= 0x67,

 COMMAND_CHANGE_DIR	= 0x70,
 COMMAND_READ_DIR	= 0x71,
 COMMAND_GET_FSSCOPE	= 0x72,
 COMMAND_GET_FINFO	= 0x73,
 COMMAND_READ_FILE	= 0x74,
 COMMAND_ABORT_FILE	= 0x75,

 COMMAND_AUTH_DEVICE	= 0xE0,
 COMMAND_GET_AUTH	= 0xE1
};

enum
{
 STATUS_BUSY	 = 0x00,
 STATUS_PAUSE	 = 0x01,
 STATUS_STANDBY	 = 0x02,
 STATUS_PLAY	 = 0x03,
 STATUS_SEEK	 = 0x04,
 STATUS_SCAN	 = 0x05,
 STATUS_OPEN	 = 0x06,
 STATUS_NODISC	 = 0x07,
 STATUS_RETRY	 = 0x08,
 STATUS_ERROR	 = 0x09,
 STATUS_FATAL	 = 0x0A,

 STATUS_PERIODIC = 0x20,
 STATUS_DTREQ	 = 0x40,
 STATUS_WAIT	 = 0x80,

 STATUS_REJECTED = 0xFF
};

void cdb_cmd(uint16* results, uint16 arg0, uint16 arg1, uint16 arg2, uint16 arg3);

#define CDB_ERROR_REJECTED	-1
#define CDB_ERROR_WAIT 		-2

enum
{
 CDB_INIT_SOFTRESET = 0x01
};
int cdb_cmd_init(uint8 flags, uint16 standby, uint8 ecc, uint8 retry);
int cdb_cmd_get_toc(void);
int cdb_cmd_set_cddevconn(uint8 fnum);
int cdb_cmd_play(uint32 start, uint32 end, uint8 mode);

enum
{
 CDB_SEEK_STOP = 0,
 CDB_SEEK_PAUSE = 0xFFFFFF
};
int cdb_cmd_seek(uint32 target);
int cdb_cmd_set_filtconn(uint8 fnum, uint8 flags, uint8 true_conn, uint8 false_conn);
int cdb_cmd_set_filtmode(uint8 fnum, uint8 mode);
int cdb_cmd_set_filtrange(uint8 fnum, uint32 fad, uint32 count);
int cdb_cmd_set_filtsubhc(uint8 fnum, uint8 channel, uint8 sub_mode, uint8 sub_mode_mask, uint8 coding_info, uint8 coding_info_mask, uint8 file);
int cdb_cmd_reset_sel(uint8 flags, uint8 pnum);

enum
{
 SECLEN_2048 = 0,
 SECLEN_2336 = 1,
 SECLEN_2340 = 2,
 SECLEN_2352 = 3,
};

int cdb_cmd_set_seclen(uint8 get_seclen, uint8 put_seclen);
int cdb_cmd_get_secnum(uint8 pnum);
int cdb_cmd_get_secinfo(uint8 pnum, uint16 offs, uint32* fad);
int cdb_cmd_get_secdata(uint8 pnum, uint16 offs, uint16 count);
int cdb_cmd_del_secdata(uint8 pnum, uint16 offs, uint16 count);
int cdb_cmd_getdel_secdata(uint8 pnum, uint16 offs, uint16 count);
int cdb_cmd_end_dataxfer(void);
//
//
//
int cdb_auth(void);
int32 cdb_get_track_fad(uint8 track);
//int cdb_read_sectors(uint8* dest, uint32 fad, uint16 count);

// Caution: Using many cdb_cmd_*() after cdb_stream_init() will
// necessitate calling cdb_stream_init() again.
int cdb_stream_init(void);
int cdb_stream_seek(int32 offset, int whence);
int32 cdb_stream_tell(void);
int cdb_stream_read(void* dest, uint32 n);

#endif
