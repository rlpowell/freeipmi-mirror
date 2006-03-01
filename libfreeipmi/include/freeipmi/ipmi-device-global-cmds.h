/*
   ipmi-device-global-cmds.h - IPMI Device Global Commands

   Copyright (C) 2003, 2004, 2005 FreeIPMI Core Team

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#ifndef _IPMI_DEVICE_GLOBAL_CMDS_H
#define	_IPMI_DEVICE_GLOBAL_CMDS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <freeipmi/fiid.h>

#define IPMI_DEVICE_ID_UNSPECIFIED        0x00
#define IPMI_MANUFACTURER_ID_UNSPECIFIED  0x000000
#define IPMI_MANUFACTURER_ID_RESERVED     0x0FFFFF
#define IPMI_MANUFACTURER_ID_INTEL        0x157

#define IPMI_PRODUCT_ID_SR870BN4          0x100
#define IPMI_PRODUCT_ID_CDC6440           0x100

#define IPMI_PRODUCT_ID_SE7501WV2         0x1B
#define IPMI_PRODUCT_ID_CDC1620           0x1B

extern fiid_template_t tmpl_cmd_get_device_id_rq;
extern fiid_template_t tmpl_cmd_get_device_id_rs;
extern fiid_template_t tmpl_cmd_get_device_id_sr870bn4_rs;

int8_t fill_cmd_get_device_id (fiid_obj_t obj_data_rq);

#ifdef __cplusplus
}
#endif

#endif /* ipmi-device-global-cmds.h */
