/*  
    
    This file is a part of mfkeys

    Copyright (c) 2010 Christian Panton <christian@panton.org>

    mfkeys is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    mfkeys is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with mfkeys.  If not, see <http://www.gnu.org/licenses/>.
    
*/

#include <nfc/nfc.h>
#include <string.h>

#include "mifare.h"
#include "nfc-helper.h"

bool mf_configure(nfc_device_t *reader)
{

    bool nfc_status;
    nfc_status  = nfc_initiator_init(reader);
    nfc_status &= nfc_configure(reader, NDO_ACTIVATE_FIELD, false);
    nfc_status &= nfc_configure(reader, NDO_INFINITE_SELECT, false);
    nfc_status &= nfc_configure(reader, NDO_HANDLE_CRC, true);
    nfc_status &= nfc_configure(reader, NDO_HANDLE_PARITY, true);
    nfc_status &= nfc_configure(reader, NDO_ACTIVATE_FIELD, true);
    nfc_status &= nfc_configure(reader, NDO_AUTO_ISO14443_4, false);
    
    return nfc_status;
}

bool mf_anticol(nfc_device_t *reader, nfc_target_info_t *target)
{
    if(nfc_initiator_select_passive_target(reader, NM_ISO14443A_106, NULL, 0, target))
    {
        if(target == NULL)
            return true;
            
        if ((target->nai.btSak & 0x08) == 0) {
            fprintf (stderr, "Error: tag is not a MIFARE Classic card\n");
            return false;
        }
        else
        {
            return true;
        }
    }
    
    fprintf(stderr, "Error: No tag was found\n");
    return false;
}

bool mf_checkkey(nfc_device_t *reader, byte_t *uid, uint8_t sector, uint8_t keytype, byte_t *key)
{
    mifare_param param;
    mifare_cmd mc;
    uint8_t block;
     
    memcpy(param.mpa.abtUid, uid, sizeof(param.mpa.abtUid));
    memcpy(param.mpa.abtKey, key, sizeof(param.mpa.abtKey));
   
    mc = keytype == 0 ? MC_AUTH_A : MC_AUTH_B;
    
    block = sector * 4;
    
    if(sector > 15)
        block = 64 + (sector-16)*16;    

    if(nfc_initiator_mifare_cmd(reader, mc, block, &param))
    {
        return true;
    }
    else{
        mf_anticol(reader, NULL);
        return false;
    }
    
}

int mf_check_card(nfc_device_t *reader, byte_t *uid, uint8_t numsector, byte_t *key, byte_t *amap, byte_t *bmap, byte_t *akeys, byte_t *bkeys)
{
    int i;
    int c = 0;
    
    for(i = 0; i < numsector; i++)
    {
        if(!amap[i])
        {
            if(mf_checkkey(reader, uid, i, 0, key))
            {
                amap[i] = true;
                memcpy(&akeys[i], key, 6);
                c++;
            }
        }
        if(!bmap[i])
        {
            if(mf_checkkey(reader, uid, i, 1, key))
            {
                bmap[i] = true;
                memcpy(&bkeys[i], key, 6);
                c++;
            }
        }
    }
    
    return c;
}

long long unsigned int bytes_to_num(byte_t* src, uint32_t len) {
	uint64_t num = 0;
	while (len--)
	{
		num = (num << 8) | (*src);
		src++;
	}
	return num;
}