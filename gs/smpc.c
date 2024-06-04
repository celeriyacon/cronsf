/*
 * gs/smpc.c
 *
 * Copyright (C) 2021 celeriyacon - https://github.com/celeriyacon
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

#include "types.h"
#include "smpc.h"

void smpc_sound_on(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SNDON;
 while(SMPC_SF & 0x1);
}

void smpc_sound_off(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SNDOFF;
 while(SMPC_SF & 0x1);
}

void smpc_cd_on(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_CDON;
 while(SMPC_SF & 0x1);
}

void smpc_cd_off(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_CDOFF;
 while(SMPC_SF & 0x1);
}

void smpc_ssh_on(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SSHON;
 while(SMPC_SF & 0x1);
}

void smpc_ssh_off(void)
{
 while(SMPC_SF & 0x1);
 SMPC_SF = 0x1;
 SMPC_COMREG = SMPC_CMD_SSHOFF;
 while(SMPC_SF & 0x1);
}

