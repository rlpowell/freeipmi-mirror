/*
   ipmi-rmcpplus-interface.h - IPMI LAN Commands

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

#ifndef _IPMI_RMCPPLUS_DEBUG_H
#define _IPMI_RMCPPLUS_DEBUG_H
 
#ifdef __cplusplus
extern "C" {
#endif

int32_t fiid_obj_dump_rmcpplus (int fd, char *prefix, char *hdr, uint8_t authentication_algorithm, uint8_t integrity_algorithm, uint8_t confidentiality_algorithm, uint8_t *integrity_key, uint32_t integrity_key_len, uint8_t *confidentiality_key, uint32_t confidentiality_key_len, uint8_t *pkt, uint32_t pkt_len, fiid_template_t tmpl_msg_hdr, fiid_template_t tmpl_cmd, fiid_template_t tmpl_trlr_session);

#ifdef __cplusplus
}
#endif

#endif
