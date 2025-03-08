/*
 * fsys.h
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

#ifndef FSYS_H
#define FSYS_H

enum
{
 FILE_TYPE_DIRECTORY = 0,
 FILE_TYPE_UNKNOWN = 1,
 FILE_TYPE__USER,
};

typedef struct fsys_dir_entry_t_
{
 char name[80 + 1];
 uint16 file_type;
 uint32 lba;
 uint32 size;
} fsys_dir_entry_t;

// Return false to discard.
typedef bool (*fsys_filter_t)(const char* file_name, uint16* file_type);

bool fsys_init(const unsigned track, uint8* mem_base, uint32 mem_size, fsys_filter_t filter);
bool fsys_change_dir_de(const fsys_dir_entry_t* de, fsys_filter_t filter);
bool fsys_change_dir(unsigned index, fsys_filter_t filter);
bool fsys_restore_cur_dir(uint32 lba, uint32 size, fsys_filter_t filter);
bool fsys_save_cur_dir(uint32* lba, uint32* size);

const char* fsys_get_error(void);

unsigned fsys_num_dir_entries(void);
fsys_dir_entry_t* fsys_get_dir_entry(unsigned index);
fsys_dir_entry_t* fsys_find_file(const char* name);

bool fsys_open(const char* name);
bool fsys_open_de(fsys_dir_entry_t* de);
ssize_t fsys_read(void* ptr, size_t count);
int32 fsys_tell(void);
int fsys_seek(int32 new_position);
int32 fsys_size(void);

#endif
