/*
  Copyright (C) 2008-2009 FreeIPMI Core Team

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA
*/

#if HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else  /* !TIME_WITH_SYS_TIME */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* !HAVE_SYS_TIME_H */
#include <time.h>
#endif  /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */
#include <assert.h>

#include <freeipmi/freeipmi.h>

#include "ipmi-oem.h"
#include "ipmi-oem-argp.h"
#include "ipmi-oem-common.h"
#include "ipmi-oem-dell.h"

#include "freeipmi-portability.h"
#include "pstdout.h"

/* 256 b/c length is 8 bit field */
#define IPMI_OEM_DELL_MAX_BYTES 256

#define IPMI_OEM_DELL_SYSTEM_INFO_ASSET_TAG          0xC4
#define IPMI_OEM_DELL_SYSTEM_INFO_SERVICE_TAG        0xC5
#define IPMI_OEM_DELL_SYSTEM_INFO_PRODUCT_NAME       0xD1
#define IPMI_OEM_DELL_SYSTEM_INFO_10G_MAC_ADDRESSES  0xCB
#define IPMI_OEM_DELL_SYSTEM_INFO_11G_MAC_ADDRESSES  0xDA
#define IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_VALIDATOR    0xDD

#define IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_TYPE_10G            0x08
#define IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_TYPE_11G_MONOLITHIC 0x0A
#define IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_TYPE_11G_MODULAR    0x0B

#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_TYPE_ETHERNET  0
#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_TYPE_ISCSI     1
#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_TYPE_RESERVED  3

#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_ENABLED      0
#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_DISABLED     1
#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_PLAYING_DEAD 2
#define IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_RESERVED     3

#define IPMI_OEM_DELL_MAC_ADDRESS_LENGTH 6

#define IPMI_OEM_DELL_11G_MAC_ADDRESS_LENGTH 8 

/* Will call ipmi_cmd_get_system_info_parameters only once, b/c field
 * requested is defined by OEM to be < 16 bytes in length
 */
static int
_get_dell_system_info_short_string (ipmi_oem_state_data_t *state_data,
                                    uint8_t parameter_selector,
                                    char *string,
                                    unsigned int string_len)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  int len;
  int rv = -1;

  assert (state_data);
  assert (string);
  assert (string_len);

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_system_info_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_system_info_parameters (state_data->ipmi_ctx,
                                           IPMI_GET_SYSTEM_INFO_PARAMETER,
                                           parameter_selector,
                                           0,
                                           IPMI_SYSTEM_INFO_NO_BLOCK_SELECTOR,
                                           obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if ((len = fiid_obj_get_data (obj_cmd_rs,
                                "configuration_parameter_data",
                                configuration_parameter_data,
                                IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  /* configuration_parameter_data[0] - string length
   * configuration_parameter_data[1-n] - string
   */

  if (len < 1)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: invalid buffer length returned: %d\n",
                       len);
      goto cleanup;
    }

  if (configuration_parameter_data[0] != (len - 1))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: invalid string length returned: %u\n",
                       configuration_parameter_data[0]);
      goto cleanup;
    }

  if (configuration_parameter_data[0])
    {
      if (configuration_parameter_data[0] > string_len)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "internal buffer overflow\n");
          goto cleanup;
        }
      
      memcpy (string,
              &(configuration_parameter_data[1]),
              configuration_parameter_data[0]);
    }

  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
_get_dell_system_info_long_string (ipmi_oem_state_data_t *state_data,
                                   uint8_t parameter_selector,
                                   char *string,
                                   unsigned int string_len)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  uint8_t set_selector = 0;
  uint8_t string_length = 0;
  unsigned int string_count = 0;
  int len;
  int rv = -1;

  assert (state_data);
  assert (string);
  assert (string_len);

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_system_info_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_system_info_parameters (state_data->ipmi_ctx,
                                           IPMI_GET_SYSTEM_INFO_PARAMETER,
                                           parameter_selector,
                                           set_selector,
                                           IPMI_SYSTEM_INFO_NO_BLOCK_SELECTOR,
                                           obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if ((len = fiid_obj_get_data (obj_cmd_rs,
                                "configuration_parameter_data",
                                configuration_parameter_data,
                                IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  if (len < 3)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: invalid buffer length returned: %d\n",
                       len);
      goto cleanup;
    }

  /* configuration_parameter_data[0] is the set selector, we don't care */

  if (configuration_parameter_data[1] != IPMI_SYSTEM_INFO_ENCODING_ASCII_LATIN1)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: invalid string type returned: %Xh\n",
                       configuration_parameter_data[0]);
      goto cleanup;
    }

  string_length = configuration_parameter_data[2];

  if (!string_length)
    goto out;

  if (len - 3)
    {
      if ((len - 3) > (string_len - string_count))
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "internal buffer overflow\n");
          goto cleanup;
        }

      memcpy (string + string_count,
              &(configuration_parameter_data[3]),
              (len - 3));
      string_count += (len - 3);
    }

  /* string_length is 8 bits, so we should not call >= 17 times,
   *
   * ceiling ( (255 - 14) / 16 ) + 1 = 17
   *
   */

  set_selector++;
  while (string_count < string_length && set_selector < 17)
    {
      if (fiid_obj_clear (obj_cmd_rs) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_clear: %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      
      if (ipmi_cmd_get_system_info_parameters (state_data->ipmi_ctx,
                                               IPMI_GET_SYSTEM_INFO_PARAMETER,
                                               parameter_selector,
                                               set_selector,
                                               IPMI_SYSTEM_INFO_NO_BLOCK_SELECTOR,
                                               obj_cmd_rs) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_cmd_get_system_info_parameters: %s\n",
                           ipmi_ctx_errormsg (state_data->ipmi_ctx));
          goto cleanup;
        }
      
      if ((len = fiid_obj_get_data (obj_cmd_rs,
                                    "configuration_parameter_data",
                                    configuration_parameter_data,
                                    IPMI_OEM_MAX_BYTES)) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      
      if (len < 2)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_cmd_get_system_info_parameters: invalid buffer length returned: %d\n",
                           len);
          goto cleanup;
        }
      
      /* configuration_parameter_data[0] is the set selector, we don't care */

      if ((string_count + (len - 1)) > (string_len - string_count))
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "internal buffer overflow\n");
          goto cleanup;
        }
      
      memcpy (string + string_count,
              &(configuration_parameter_data[1]),
              (len - 1));
      
      string_count += (len - 1);
      
      set_selector++;
    }

 out:
  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

/* returns 1 on success, 0 on not supported, -1 on error */
static int
_get_dell_system_info_idrac_info (ipmi_oem_state_data_t *state_data,
                                  uint8_t *idrac_type)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  int len;
  int rv = -1;

  assert (state_data);
  assert (idrac_type);

  /* Dell OEM
   *
   * From Dell Provided Source Code
   *
   * Uses Get System Info command
   *
   * iDRAC Validator Parameter = 0xDD
   * iDRAC Validator Set Selector = 0x02
   *
   * Parameter data response formatted:
   *
   * 1st byte = ??
   * 2nd byte = ??
   * 3rd byte = ??
   * 4th byte = ??
   * 5th byte = ??
   * 6th byte = ??
   * 7th byte = ??
   * 8th byte = ??
   * 9th byte = ??
   * 10th byte = iDRAC type
   * - 0x08 = iDRAC 10g
   * - 0x0A = iDRAC 11g monolithic
   * - 0x0B = iDRAC 11g modular
   */

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_system_info_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_system_info_parameters (state_data->ipmi_ctx,
                                           IPMI_GET_SYSTEM_INFO_PARAMETER,
                                           IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_VALIDATOR,
                                           0x02,
                                           IPMI_SYSTEM_INFO_NO_BLOCK_SELECTOR,
                                           obj_cmd_rs) < 0)
    {
      if (ipmi_ctx_errnum (state_data->ipmi_ctx) == IPMI_ERR_BAD_COMPLETION_CODE
          && (ipmi_check_completion_code (obj_cmd_rs,
                                          IPMI_COMP_CODE_GET_SYSTEM_INFO_PARAMETER_NOT_SUPPORTED) == 1))
	{
	  rv = 0;
	  goto cleanup;
	}

      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if ((len = fiid_obj_get_data (obj_cmd_rs,
                                "configuration_parameter_data",
                                configuration_parameter_data,
                                IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  if (len < 1)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: invalid buffer length returned: %d\n",
                       len);
      goto cleanup;
    }

  (*idrac_type) = configuration_parameter_data[9];

  rv = 1;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
_get_dell_system_info_10g_mac_addresses (ipmi_oem_state_data_t *state_data)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t number_of_nics;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  int len;
  int i;
  int rv = -1;

  assert (state_data);

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_system_info_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_system_info_parameters (state_data->ipmi_ctx,
                                           IPMI_GET_SYSTEM_INFO_PARAMETER,
                                           IPMI_OEM_DELL_SYSTEM_INFO_10G_MAC_ADDRESSES,
                                           0,
                                           IPMI_SYSTEM_INFO_NO_BLOCK_SELECTOR,
                                           obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if ((len = fiid_obj_get_data (obj_cmd_rs,
                                "configuration_parameter_data",
                                configuration_parameter_data,
                                IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  number_of_nics = configuration_parameter_data[0];

  if (!number_of_nics)
    {
      rv = 0;
      goto cleanup;
    }

  if ((number_of_nics * IPMI_OEM_DELL_MAC_ADDRESS_LENGTH) != (len - 1))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_system_info_parameters: invalid buffer length returned: number of nics = %u, bytes = %d\n",
		       number_of_nics,
                       len);
      goto cleanup;
    }

  pstdout_printf (state_data->pstate,
		  "NIC Number\tMAC Address\n");
  for (i = 0; i < number_of_nics; i++)
    pstdout_printf (state_data->pstate,
		    "%u\t\t%02X:%02X:%02X:%02X:%02X:%02X\n",
		    i,
		    configuration_parameter_data[i*6 + 1],
		    configuration_parameter_data[i*6 + 2],
		    configuration_parameter_data[i*6 + 3],
		    configuration_parameter_data[i*6 + 4],
		    configuration_parameter_data[i*6 + 5],
		    configuration_parameter_data[i*6 + 6]);

  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
_get_dell_system_info_11g_mac_addresses (ipmi_oem_state_data_t *state_data)
{
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  uint8_t total_bytes;
  int rs_len;
  int i;
  int rv = -1;

  assert (state_data);

  /* see info below in ipmi_oem_dell_get_system_info() for packet
   * format.  We cannot use normal Get System Info b/c Dell hacked it
   * to include/support extra bytes. 
   */

  bytes_rq[0] = IPMI_CMD_GET_SYSTEM_INFO_PARAMETERS;
  bytes_rq[1] = 0x00;		/* get parameter */
  bytes_rq[2] = IPMI_OEM_DELL_SYSTEM_INFO_11G_MAC_ADDRESSES; /* parameter selector */
  bytes_rq[3] = 0x00;		/* set selector */
  bytes_rq[4] = 0x00;		/* block selector */
  bytes_rq[5] = 0x00;		/* offset */
  bytes_rq[6] = 0x00;		/* length */

  if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                              0, /* lun */
                              IPMI_NET_FN_APP_RQ, /* network function */
                              bytes_rq, /* data */
                              7, /* num bytes */
                              bytes_rs,
                              IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_raw: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_oem_check_response_and_completion_code (state_data,
                                                   bytes_rs,
                                                   rs_len,
                                                   3,
                                                   IPMI_CMD_GET_SYSTEM_INFO_PARAMETERS,
                                                   IPMI_NET_FN_APP_RQ) < 0)
    goto cleanup;
  
  total_bytes = bytes_rs[3];

  if (!total_bytes)
    {
      rv = 0;
      goto cleanup;
    }

  if (total_bytes % 8)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "invalid total bytes of data returned: %u\n",
		       total_bytes);
      goto cleanup;
    }

  /* see record format below in ipmi_oem_dell_get_system_info(), record length = 8 */
  pstdout_printf (state_data->pstate,
		  "NIC Number\tMAC Address\t\tNIC Status\n");
  for (i = 0; i < (total_bytes / IPMI_OEM_DELL_11G_MAC_ADDRESS_LENGTH); i++)
    {
      uint8_t mac_type;
      
      bytes_rq[0] = IPMI_CMD_GET_SYSTEM_INFO_PARAMETERS;
      bytes_rq[1] = 0x00;		/* get parameter */
      bytes_rq[2] = IPMI_OEM_DELL_SYSTEM_INFO_11G_MAC_ADDRESSES; /* parameter selector */
      bytes_rq[3] = 0x00;		/* set selector */
      bytes_rq[4] = 0x00;		/* block selector */
      bytes_rq[5] = i * IPMI_OEM_DELL_11G_MAC_ADDRESS_LENGTH; /* offset */
      bytes_rq[6] = IPMI_OEM_DELL_11G_MAC_ADDRESS_LENGTH; /* length */
      
      if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
				  0, /* lun */
				  IPMI_NET_FN_APP_RQ, /* network function */
				  bytes_rq, /* data */
				  7, /* num bytes */
				  bytes_rs,
				  IPMI_OEM_MAX_BYTES)) < 0)
	{
	  pstdout_fprintf (state_data->pstate,
			   stderr,
			   "ipmi_cmd_raw: %s\n",
			   ipmi_ctx_errormsg (state_data->ipmi_ctx));
	  goto cleanup;
	}
      
      /* 11 = IPMI_OEM_DELL_11G_MAC_ADDRESS_LENGTH + 3 (for cmd, completion code, parameter revision) */
      if (ipmi_oem_check_response_and_completion_code (state_data,
						       bytes_rs,
						       rs_len,
						       11,
						       IPMI_CMD_GET_SYSTEM_INFO_PARAMETERS,
						       IPMI_NET_FN_APP_RQ) < 0)
	goto cleanup;
      
      mac_type = (bytes_rs[3] & 0x30) >> 4;
      
      if (mac_type == IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_TYPE_ETHERNET)
	{
	  uint8_t nic_number;
	  uint8_t nic_status;
	  char *nic_status_str = NULL;

	  nic_status = (bytes_rs[3] & 0xC0) >> 6;
	  nic_number = (bytes_rs[4] & 0x1F);

	  if (nic_status == IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_ENABLED)
	    nic_status_str = "Enabled";
	  else if (nic_status == IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_DISABLED)
	    nic_status_str = "Disabled";
	  else if (nic_status == IPMI_OEM_DELL_SYSTEM_INFO_MAC_ADDRESS_STATUS_PLAYING_DEAD)
	    nic_status_str = "Playing Dead";
	  else
	    nic_status_str = "Unknown";

	  pstdout_printf (state_data->pstate,
			  "%u\t\t%02X:%02X:%02X:%02X:%02X:%02X\t%s\n",
			  nic_number,
			  bytes_rs[5],
			  bytes_rs[6],
			  bytes_rs[7],
			  bytes_rs[8],
			  bytes_rs[9],
			  bytes_rs[10],
			  nic_status_str);
	}
    }

  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_oem_dell_get_system_info (ipmi_oem_state_data_t *state_data)
{
  char string[IPMI_OEM_DELL_MAX_BYTES+1];
  int rv = -1;

  assert (state_data);
  assert (state_data->prog_data->args->oem_options_count == 1);

  /* achu: handle some common typo situations */
  if (strcasecmp (state_data->prog_data->args->oem_options[0], "asset-tag")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "asset_tag")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "assettag")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "service-tag")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "service_tag")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "servicetag")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "product-name")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "product_name")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "productname")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "mac-addresses")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "mac_addresses")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "macaddresses"))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "%s:%s invalid OEM option argument '%s'\n",
                       state_data->prog_data->args->oem_id,
                       state_data->prog_data->args->oem_command,
                       state_data->prog_data->args->oem_options[0]);
      goto cleanup;
    }

  /* Dell OEM
   *
   * Some from http://linux.dell.com/files/openipmi/ipmitool/
   * Some from Dell Provided Source Code
   *
   * Uses Get System Info command
   *
   * For asset-tag and service-tag, the response format is different
   * than product name.
   *
   * Format #1)
   *
   * asset-tag parameter = 0xC4
   * service-tag parameter = 0xC5
   *
   * Parameter data response formatted:
   *
   * 1st byte = length
   * ? bytes = string
   *
   * Format #2)
   *
   * product-name parameter = 0xD1
   *
   * Parameter data response formatted:
   *
   * Set Selector 0:
   *
   * 1st byte = set selector
   * 2nd byte = encoding
   * 3rd byte = string length
   * ? bytes = string
   *
   * Set Selector > 0
   *
   * 1st byte = set selector
   * ? bytes = string
   *
   * Format #3)
   *
   * Dell 10G systems, mac-addresses = 0xCB
   *
   * Parameter data response formatted:
   *
   * 1st byte = number of NICs
   * ? bytes = MAC address of NICS, number of NICS * 6 total bytes
   *
   * Format #4)
   *
   * Dell 11G systems, mac-addresses = 0xDA
   * + 2 extra bytes
   * byte 5 : offset into data to read
   * byte 6 : length of data to read
   *
   *
   * Parameter data response formatted:
   *
   * if byte 5 and byte 6 are 0x00
   *
   * 1st byte - total bytes of MAC address data
   *
   * if byte 5 and byte 6 have real offsets/lengths
   *
   * parameter revision byte = total number of bytes returned
   * ? bytes = record stored in following format
   *   byte 1 - 0:3 - blade slot number
   *   byte 1 - 4:5 - mac address type
   *                - 0 = ethernet
   *                - 1 = iSCSI
   *                - 2 = ???
   *                - 3 = reserved
   *   byte 1 - 6:7 - ethernet status
   *                - 0 = enabled
   *                - 1 = disabled
   *                - 2 = playing dead
   *                - 3 = reserved
   *   byte 2 - 0:4 - NIC number
   *   byte 2 - 5:7 - reserved
   *   bytes 3 - 8 - MAC address
   */

  memset (string, '\0', IPMI_OEM_DELL_MAX_BYTES + 1);

  if (!strcasecmp (state_data->prog_data->args->oem_options[0], "asset-tag")
      || !strcasecmp (state_data->prog_data->args->oem_options[0], "asset_tag")
      || !strcasecmp (state_data->prog_data->args->oem_options[0], "assettag"))
    {
      if (_get_dell_system_info_short_string (state_data,
                                              IPMI_OEM_DELL_SYSTEM_INFO_ASSET_TAG,
                                              string,
                                              IPMI_OEM_DELL_MAX_BYTES) < 0)
        goto cleanup;

      pstdout_printf (state_data->pstate,
		      "%s\n",
		      string);
    }
  else if (!strcasecmp (state_data->prog_data->args->oem_options[0], "service-tag")
           || !strcasecmp (state_data->prog_data->args->oem_options[0], "service_tag")
           || !strcasecmp (state_data->prog_data->args->oem_options[0], "servicetag"))
    {
      if (_get_dell_system_info_short_string (state_data,
                                              IPMI_OEM_DELL_SYSTEM_INFO_SERVICE_TAG,
                                              string,
                                              IPMI_OEM_DELL_MAX_BYTES) < 0)
        goto cleanup;

      pstdout_printf (state_data->pstate,
		      "%s\n",
		      string);
    }
  else if (!strcasecmp (state_data->prog_data->args->oem_options[0], "product-name")
           || !strcasecmp (state_data->prog_data->args->oem_options[0], "product_name")
           || !strcasecmp (state_data->prog_data->args->oem_options[0], "productname"))
    {
      if (_get_dell_system_info_long_string (state_data,
                                             IPMI_OEM_DELL_SYSTEM_INFO_PRODUCT_NAME,
                                             string,
                                             IPMI_OEM_DELL_MAX_BYTES) < 0)
        goto cleanup;

      pstdout_printf (state_data->pstate,
		      "%s\n",
		      string);

    }
  else /* (!strcasecmp (state_data->prog_data->args->oem_options[0], "mac-addresses")
          || !strcasecmp (state_data->prog_data->args->oem_options[0], "mac_addresses")
          || !strcasecmp (state_data->prog_data->args->oem_options[0], "macaddresses")) */
    {
      uint8_t idrac_type = 0;
      int ret;
      
      if ((ret = _get_dell_system_info_idrac_info (state_data, &idrac_type)) < 0)
	goto cleanup;

      if (ret)
	{
	  /* iDRAC 10g */
	  if (idrac_type == IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_TYPE_10G)
	    {
	      if (_get_dell_system_info_10g_mac_addresses (state_data) < 0)
		goto cleanup;
	    }
	  /* iDRAC 11g */
	  else if (idrac_type == IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_TYPE_11G_MONOLITHIC
		   || idrac_type == IPMI_OEM_DELL_SYSTEM_INFO_IDRAC_TYPE_11G_MODULAR)
	    {
	      if (_get_dell_system_info_11g_mac_addresses (state_data) < 0)
		goto cleanup;
	    }
	  else
	    {
	      pstdout_fprintf (state_data->pstate,
			       stderr,
			       "Unrecognized iDRAC system %02Xh\n",
			       idrac_type);
	      goto cleanup;
	    }
	}
      else
	{
	  /* assume iDRAC 10g */
	  if (_get_dell_system_info_10g_mac_addresses (state_data) < 0)
	    goto cleanup;
	}

    }
 
  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_oem_dell_get_nic_selection (ipmi_oem_state_data_t *state_data)
{
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  int rs_len;
  int rv = -1;

  assert (state_data);
  assert (!state_data->prog_data->args->oem_options_count);

  /* Dell OEM
   *
   * Get NIC Selection Request
   *
   * 0x30 - OEM network function
   * 0x25 - OEM cmd
   * 
   * Get NIC Selection Response
   *
   * 0x25 - OEM cmd
   * 0x?? - Completion Code
   * 0x?? - NIC selection
   *      - 0x00 = shared
   *      - 0x01 = shared w/ failover to NIC2
   *      - 0x02 = dedicated
   *      - 0x03 = shared w/ failover to all
   */

  bytes_rq[0] = 0x25;

  if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                              0, /* lun */
                              0x30, /* network function */
                              bytes_rq, /* data */
                              1, /* num bytes */
                              bytes_rs,
                              IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_raw: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_oem_check_response_and_completion_code (state_data,
                                                   bytes_rs,
                                                   rs_len,
                                                   3,
                                                   0x02,
                                                   0x30) < 0)
    goto cleanup;

  switch (bytes_rs[2])
    {
    case 0x00:
      pstdout_printf (state_data->pstate, "shared\n");
      break;
    case 0x01:
      pstdout_printf (state_data->pstate, "shared with failover to NIC2\n");
      break;
    case 0x02:
      pstdout_printf (state_data->pstate, "dedicated\n");
      break;
    case 0x03:
      pstdout_printf (state_data->pstate, "shared with failover to all NICs\n");
      break;
    default:
      pstdout_printf (state_data->pstate, "unknown NIC selection: %Xh\n", bytes_rs[2]);
      break;
    }
  
  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_oem_dell_set_nic_selection (ipmi_oem_state_data_t *state_data)
{
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  int rs_len;
  int rv = -1;

  assert (state_data);
  assert (state_data->prog_data->args->oem_options_count == 1);

  if (strcasecmp (state_data->prog_data->args->oem_options[0], "dedicated")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "shared")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "shared_failover_nic2")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "shared_failover_all"))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "%s:%s invalid OEM option argument '%s'\n",
                       state_data->prog_data->args->oem_id,
                       state_data->prog_data->args->oem_command,
                       state_data->prog_data->args->oem_options[0]);
      goto cleanup;
    }

  /* Dell OEM
   *
   * Set NIC Selection Request
   *
   * 0x30 - OEM network function
   * 0x24 - OEM cmd
   * 0x?? - NIC selection
   *      - 0x00 = shared
   *      - 0x01 = shared w/ failover to NIC2
   *      - 0x02 = dedicated
   *      - 0x03 = shared w/ failover to all
   * 
   * Set NIC Selection Response
   *
   * 0x24 - OEM cmd
   * 0x?? - Completion Code
   */

  bytes_rq[0] = 0x24;

  if (!strcasecmp (state_data->prog_data->args->oem_options[0], "shared"))
    bytes_rq[1] = 0x00;
  else if (!strcasecmp (state_data->prog_data->args->oem_options[0], "shared_failover_nic2"))
    bytes_rq[1] = 0x01;
  else if (!strcasecmp (state_data->prog_data->args->oem_options[0], "dedicated"))
    bytes_rq[1] = 0x02;
  else
    bytes_rq[1] = 0x03;

  if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                              0, /* lun */
                              0x30, /* network function */
                              bytes_rq, /* data */
                              2, /* num bytes */
                              bytes_rs,
                              IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_raw: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_oem_check_response_and_completion_code (state_data,
                                                   bytes_rs,
                                                   rs_len,
                                                   2, /* don't care about the 3rd byte, don't know what it is used for */
                                                   0x03,
                                                   0x30) < 0)
    goto cleanup;

  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_oem_dell_get_power_info (ipmi_oem_state_data_t *state_data)
{
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  uint32_t cumulative_start_time;
  uint32_t cumulative_reading;
  uint32_t peak_start_time;
  uint32_t peak_amp_time;
  uint16_t peak_amp_reading;
  uint32_t peak_watt_time;
  uint16_t peak_watt_reading;
  double cumulative_reading_val;
  double peak_amp_reading_val;
  time_t timetmp;
  struct tm time_tm;
  char time_buf[IPMI_OEM_TIME_BUFLEN + 1];
  int rs_len;
  int rv = -1;

  assert (state_data);
  assert (!state_data->prog_data->args->oem_options_count);

  /* Dell OEM
   *
   * From http://linux.dell.com/files/openipmi/ipmitool/
   *
   * Request
   *
   * 0x30 - OEM network function
   * 0x9c - OEM cmd
   * 0x07 - ??
   * 0x01 - ??
   * 
   * Response
   *
   * 0x9c - OEM cmd
   * 0x?? - Completion Code
   * bytes 2-5 - cumulative start time
   * bytes 6-9 - cumulative reading
   * bytes 10-13 - peak start time
   * bytes 14-17 - peak amp time
   * bytes 18-21 - peak amp reading
   * bytes 22-25 - peak watt time
   * bytes 26-29 - peak watt reading
   */

  bytes_rq[0] = 0x9c;
  bytes_rq[1] = 0x07;
  bytes_rq[2] = 0x01;

  if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                              0, /* lun */
                              0x30, /* network function */
                              bytes_rq, /* data */
                              3, /* num bytes */
                              bytes_rs,
                              IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_raw: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_oem_check_response_and_completion_code (state_data,
                                                   bytes_rs,
                                                   rs_len,
                                                   26,
                                                   0x9c,
                                                   0x30) < 0)
    goto cleanup;

  cumulative_start_time = bytes_rs[2];
  cumulative_start_time |= (bytes_rs[3] << 8);
  cumulative_start_time |= (bytes_rs[4] << 16);
  cumulative_start_time |= (bytes_rs[5] << 24);

  cumulative_reading = bytes_rs[6];
  cumulative_reading |= (bytes_rs[7] << 8);
  cumulative_reading |= (bytes_rs[8] << 16);
  cumulative_reading |= (bytes_rs[9] << 24);

  peak_start_time = bytes_rs[10];
  peak_start_time |= (bytes_rs[11] << 8);
  peak_start_time |= (bytes_rs[12] << 16);
  peak_start_time |= (bytes_rs[13] << 24);

  peak_amp_time = bytes_rs[14];
  peak_amp_time |= (bytes_rs[15] << 8);
  peak_amp_time |= (bytes_rs[16] << 16);
  peak_amp_time |= (bytes_rs[17] << 24);

  peak_amp_reading = bytes_rs[18];
  peak_amp_reading |= (bytes_rs[19] << 8);

  peak_watt_time = bytes_rs[20];
  peak_watt_time |= (bytes_rs[21] << 8);
  peak_watt_time |= (bytes_rs[22] << 16);
  peak_watt_time |= (bytes_rs[23] << 24);

  peak_watt_reading = bytes_rs[24];
  peak_watt_reading |= (bytes_rs[25] << 8);

  cumulative_reading_val = ((double)cumulative_reading) / 1000.0;

  timetmp = cumulative_start_time;
  localtime_r (&timetmp, &time_tm);
  memset (time_buf, '\0', IPMI_OEM_TIME_BUFLEN + 1);
  strftime (time_buf, IPMI_OEM_TIME_BUFLEN, "%D - %T", &time_tm);

  pstdout_printf (state_data->pstate,
                  "Cumulative Energy Start Time : %s\n",
                  time_buf);

  pstdout_printf (state_data->pstate,
                  "Cumulative Energy            : %.2f kWh\n",
                  cumulative_reading_val);

  peak_amp_reading_val = ((double)peak_amp_reading) / 10.0;

  timetmp = peak_amp_time;
  localtime_r (&timetmp, &time_tm);
  memset (time_buf, '\0', IPMI_OEM_TIME_BUFLEN + 1);
  strftime (time_buf, IPMI_OEM_TIME_BUFLEN, "%D - %T", &time_tm);

  pstdout_printf (state_data->pstate,
                  "Peak Amp Time                : %s\n",
                  time_buf);

  pstdout_printf (state_data->pstate,
                  "Peak Amp                     : %.2f A\n",
                  peak_amp_reading_val);

  timetmp = peak_watt_time;
  localtime_r (&timetmp, &time_tm);
  memset (time_buf, '\0', IPMI_OEM_TIME_BUFLEN + 1);
  strftime (time_buf, IPMI_OEM_TIME_BUFLEN, "%D - %T", &time_tm);

  pstdout_printf (state_data->pstate,
                  "Peak Watt Time               : %s\n",
                  time_buf);

  pstdout_printf (state_data->pstate,
                  "Peak Watt                    : %u W\n",
                  peak_watt_reading);

  rv = 0;
 cleanup:
  return (rv);
}

int
ipmi_oem_dell_reset_power_info (ipmi_oem_state_data_t *state_data)
{
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  int rs_len;
  int rv = -1;

  assert (state_data);
  assert (state_data->prog_data->args->oem_options_count == 1);

  if (strcasecmp (state_data->prog_data->args->oem_options[0], "cumulative")
      && strcasecmp (state_data->prog_data->args->oem_options[0], "peak"))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "%s:%s invalid OEM option argument '%s'\n",
                       state_data->prog_data->args->oem_id,
                       state_data->prog_data->args->oem_command,
                       state_data->prog_data->args->oem_options[0]);
      goto cleanup;
    }

  /* Dell OEM
   *
   * From http://linux.dell.com/files/openipmi/ipmitool/
   *
   * Request
   *
   * 0x30 - OEM network function
   * 0x9d - OEM cmd
   * 0x07 - ??
   * 0x01 - ??
   * 0x?? - field to clear (0x1 = cumulative, 0x2 = peak)
   * 
   * Response
   *
   * 0x9d - OEM cmd
   * 0x?? - Completion Code
   */

  bytes_rq[0] = 0x9d;
  bytes_rq[1] = 0x07;
  bytes_rq[2] = 0x01;

  if (!strcasecmp (state_data->prog_data->args->oem_options[0], "cumulative"))
    bytes_rq[3] = 1;
  else
    bytes_rq[3] = 2;
  
  if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                              0, /* lun */
                              0x30, /* network function */
                              bytes_rq, /* data */
                              4, /* num bytes */
                              bytes_rs,
                              IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_raw: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }
  
  if (ipmi_oem_check_response_and_completion_code (state_data,
                                                   bytes_rs,
                                                   rs_len,
                                                   2,
                                                   0x9d,
                                                   0x30) < 0)
    goto cleanup;
  
  rv = 0;
 cleanup:
  return (rv);
 
}

int
ipmi_oem_dell_get_fcb_version (ipmi_oem_state_data_t *state_data)
{
  uint8_t bytes_rq[IPMI_OEM_MAX_BYTES];
  uint8_t bytes_rs[IPMI_OEM_MAX_BYTES];
  int32_t rs_len;
  int rv = -1;

  assert (state_data);
  assert (!state_data->prog_data->args->oem_options_count);

  /* llnlxanadu2 OEM
   *
   * Get FCB Version Request
   *
   * 0x34 - OEM network function
   * 0x16 - OEM cmd
   *
   * Get FCB Version Response
   *
   * 0x16 - OEM cmd
   * 0x?? - Completion Code
   * 0x?? - major version (in hex)
   * 0x?? - minor version (in hex)
   */

  bytes_rq[0] = 0x16;

  if ((rs_len = ipmi_cmd_raw (state_data->ipmi_ctx,
                              0, /* lun */
                              0x34, /* network function */
                              bytes_rq, /* data */
                              1, /* num bytes */
                              bytes_rs,
                              IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_raw: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_oem_check_response_and_completion_code (state_data,
                                                   bytes_rs,
                                                   rs_len,
                                                   4,
                                                   0x16,
                                                   0x34) < 0)
    goto cleanup;

  pstdout_printf (state_data->pstate,
                  "%X.%02X\n",
                  bytes_rs[2],
                  bytes_rs[3]);

  rv = 0;
 cleanup:
  return (rv);
}

#if 0
/* cannot verify */

int
ipmi_oem_dell_get_dhcp_retry (ipmi_oem_state_data_t *state_data)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  uint8_t lan_channel_number;
  int len;
  int rv = -1;

  assert (state_data);
  assert (!state_data->prog_data->args->oem_options_count);

  /* Dell OEM
   *
   * Uses Get/Set Lan Configuration
   *
   * parameter = 192
   *
   * Data format
   *
   * 1st byte = retry count, 1 based, 0h = no retries, ffh = infinite
   * 2nd byte = retry interval, 1 based, 10 second increments
   * 3rd byte = retry timeout, 1 based, 1 minute increments
   *
   */

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_lan_configuration_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_get_channel_number (state_data->ipmi_ctx,
                               IPMI_CHANNEL_MEDIUM_TYPE_LAN_802_3,
                               &lan_channel_number) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_get_channel_number: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_cmd_get_lan_configuration_parameters (state_data->ipmi_ctx,
                                                 lan_channel_number,
                                                 IPMI_GET_LAN_PARAMETER,
                                                 192,
                                                 0,
                                                 0,
                                                 obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_lan_configuration_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if ((len = fiid_obj_get_data (obj_cmd_rs,
                                "configuration_parameter_data",
                                configuration_parameter_data,
                                IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  if (len < 3)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_lan_configuration_parameters: invalid buffer length returned: %d\n",
                       len);
      goto cleanup;
    }

  if (!configuration_parameter_data[0])
    pstdout_printf (state_data->pstate, "Retry Count    : no retries\n");
  else if (configuration_parameter_data[0] == 0xFF)
    pstdout_printf (state_data->pstate, "Retry Count    : infinite retries\n");
  else
    pstdout_printf (state_data->pstate, "Retry Count    : %u\n", configuration_parameter_data[0]);
  pstdout_printf (state_data->pstate, "Retry Interval : %u seconds\n", configuration_parameter_data[1] * 10);
  pstdout_printf (state_data->pstate, "Retry Timeout  : %u minutes\n", configuration_parameter_data[2]);
                  
  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

int
ipmi_oem_dell_set_dhcp_retry (ipmi_oem_state_data_t *state_data)
{
  assert (state_data);
  assert (state_data->prog_data->args->oem_options_count == 3);

  return (0);
}
#endif

int
ipmi_oem_dell_get_sol_inactivity_timeout (ipmi_oem_state_data_t *state_data)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  uint8_t lan_channel_number;
  uint16_t sol_inactivity_timeout;
  int len;
  int rv = -1;

  assert (state_data);
  assert (!state_data->prog_data->args->oem_options_count);

  /* Dell OEM
   *
   * Uses Get/Set SOL Configuration
   *
   * parameter = 192
   *
   * Data format
   *
   * 1st & 2nd byte = inactivity timeout, 1 based, 1 minute
   * increments, LSbyte first
   *
   */

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_sol_configuration_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_get_channel_number (state_data->ipmi_ctx,
                               IPMI_CHANNEL_MEDIUM_TYPE_LAN_802_3,
                               &lan_channel_number) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_get_channel_number: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (ipmi_cmd_get_sol_configuration_parameters (state_data->ipmi_ctx,
                                                 lan_channel_number,
                                                 IPMI_GET_SOL_PARAMETER,
                                                 192,
                                                 0,
                                                 0,
                                                 obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_sol_configuration_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if ((len = fiid_obj_get_data (obj_cmd_rs,
                                "configuration_parameter_data",
                                configuration_parameter_data,
                                IPMI_OEM_MAX_BYTES)) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get_data: 'configuration_parameter_data': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  if (len < 2)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_sol_configuration_parameters: invalid buffer length returned: %d\n",
                       len);
      goto cleanup;
    }

  sol_inactivity_timeout = 0;
  sol_inactivity_timeout |= configuration_parameter_data[0];
  sol_inactivity_timeout |= (configuration_parameter_data[1] << 8);

  if (sol_inactivity_timeout)
    pstdout_printf (state_data->pstate, "SOL Inactivity Timeout : %u minutes\n", sol_inactivity_timeout);
  else
    pstdout_printf (state_data->pstate, "SOL Inactivity Timeout : no timeout\n", sol_inactivity_timeout);
                  
  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

int
ipmi_oem_dell_set_sol_inactivity_timeout (ipmi_oem_state_data_t *state_data)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t configuration_parameter_data[IPMI_OEM_MAX_BYTES];
  uint8_t lan_channel_number;
  uint16_t sol_inactivity_timeout = 0;
  int rv = -1;

  assert (state_data);
  assert (state_data->prog_data->args->oem_options_count == 1);

  if (strcasecmp (state_data->prog_data->args->oem_options[0], "none"))
    {
      char *ptr = NULL;

      errno = 0;
      
      sol_inactivity_timeout = strtoul (state_data->prog_data->args->oem_options[0], &ptr, 10);
      if (errno || ptr[0] != '\0')
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "%s:%s invalid OEM option argument '%s'\n",
                           state_data->prog_data->args->oem_id,
                           state_data->prog_data->args->oem_command,
                           state_data->prog_data->args->oem_options[0]);
          goto cleanup;
        }
    }
  else
    sol_inactivity_timeout = 0;
  
  /* Dell OEM
   *
   * Uses Get/Set SOL Configuration
   *
   * parameter = 192
   *
   * Data format
   *
   * 1st & 2nd byte = inactivity timeout, 1 based, 1 minute
   * increments, LSbyte first
   *
   */

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_set_sol_configuration_parameters_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_get_channel_number (state_data->ipmi_ctx,
                               IPMI_CHANNEL_MEDIUM_TYPE_LAN_802_3,
                               &lan_channel_number) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_get_channel_number: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  configuration_parameter_data[0] = sol_inactivity_timeout & 0x00FF;
  configuration_parameter_data[1] = (sol_inactivity_timeout & 0xFF00) >> 8;

  if (ipmi_cmd_set_sol_configuration_parameters (state_data->ipmi_ctx,
                                                 lan_channel_number,
                                                 192,
                                                 configuration_parameter_data,
                                                 2,
                                                 obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_sol_configuration_parameters: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}
