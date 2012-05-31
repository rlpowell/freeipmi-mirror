/*****************************************************************************\
 *  $Id: ipmiseld.c,v 1.17 2010-02-08 22:02:30 chu11 Exp $
 *****************************************************************************
 *  Copyright (C) 2012 Lawrence Livermore National Security, LLC.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  LLNL-CODE-559172
 *
 *  This file is part of Ipmiseld, an IPMI SEL syslog logging daemon.
 *  For details, see http://www.llnl.gov/linux/.
 *
 *  Ipmiseld is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmiseld is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmiseld.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <syslog.h>
#include <assert.h>
#include <errno.h>

#include <freeipmi/freeipmi.h>

#include "ipmiseld.h"
#include "ipmiseld-argp.h"
#include "ipmiseld-common.h"
#include "ipmiseld-sdr.h"

#include "freeipmi-portability.h"
#include "error.h"
#include "tool-common.h"
#include "tool-daemon-common.h"
#include "tool-event-common.h"
#include "tool-util-common.h"

#define IPMISELD_PIDFILE             IPMISELD_LOCALSTATEDIR "/run/ipmiseld.pid"

#define IPMISELD_EVENT_OUTPUT_BUFLEN 4096

#define IPMISELD_SYSTEM_EVENT_FORMAT_STR_DEFAULT              "SEL System Event: %s, %I, %E"

#define IPMISELD_OEM_TIMESTAMPED_EVENT_FORMAT_STR_DEFAULT     "SEL OEM Event: %I, %o"

#define IPMISELD_OEM_NON_TIMESTAMPED_EVENT_FORMAT_STR_DEFAULT "SEL OEM Event: %I, %o"

/* return (-1), real error */
static int
_sel_parse_err_handle (ipmiseld_state_data_t *state_data, char *func)
{
  assert (state_data);
  assert (func);

  if (ipmi_sel_ctx_errnum (state_data->sel_ctx) == IPMI_SEL_ERR_INVALID_SEL_ENTRY)
    {
      /* maybe a bad SEL entry returned from remote system, don't error out */
      if (state_data->prog_data->args->common_args.debug)
        err_debug ("Invalid SEL entry read");
      return (0);
    }

  err_output ("%s: %s",
	      func,
	      ipmi_sel_ctx_errormsg (state_data->sel_ctx));

  return (-1);
}

static int
_normal_output (ipmiseld_state_data_t *state_data, uint8_t record_type)
{
  char outbuf[IPMISELD_EVENT_OUTPUT_BUFLEN+1];
  int outbuf_len;
  unsigned int flags;
  int record_type_class;
  char *format_str;

  assert (state_data);

  memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
   
  flags = IPMI_SEL_STRING_FLAGS_IGNORE_UNAVAILABLE_FIELD;
  flags |= IPMI_SEL_STRING_FLAGS_OUTPUT_NOT_AVAILABLE;
  flags |= IPMI_SEL_STRING_FLAGS_DATE_MONTH_STRING;
  if (state_data->prog_data->args->verbose_count)
    flags |= IPMI_SEL_STRING_FLAGS_VERBOSE;
  if (state_data->prog_data->args->entity_sensor_names)
    flags |= IPMI_SEL_STRING_FLAGS_ENTITY_SENSOR_NAMES;
  if (state_data->prog_data->args->non_abbreviated_units)
    flags |= IPMI_SEL_STRING_FLAGS_NON_ABBREVIATED_UNITS;
  if (state_data->prog_data->args->interpret_oem_data)
    flags |= IPMI_SEL_STRING_FLAGS_INTERPRET_OEM_DATA;

  record_type_class = ipmi_sel_record_type_class (record_type);
  if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_SYSTEM_EVENT_RECORD)
    {
      if (state_data->prog_data->args->system_event_format_str)
	format_str = state_data->prog_data->args->system_event_format_str;
      else
	format_str = IPMISELD_SYSTEM_EVENT_FORMAT_STR_DEFAULT;
    }
  else if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_TIMESTAMPED_OEM_RECORD)
    {
      if (state_data->prog_data->args->oem_timestamped_event_format_str)
	format_str = state_data->prog_data->args->oem_timestamped_event_format_str;
      else
	format_str = IPMISELD_OEM_TIMESTAMPED_EVENT_FORMAT_STR_DEFAULT;
    }
  else if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_NON_TIMESTAMPED_OEM_RECORD)
    {
      if (state_data->prog_data->args->oem_non_timestamped_event_format_str)
	format_str = state_data->prog_data->args->oem_non_timestamped_event_format_str;
      else
	format_str = IPMISELD_OEM_NON_TIMESTAMPED_EVENT_FORMAT_STR_DEFAULT;
    }
  else
    {
      if (state_data->prog_data->args->verbose_count)
	ipmiseld_syslog (state_data,
			 "SEL Event: Unknown SEL Record Type: %Xh",
			 record_type);
      return (0);
    }
  
  if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_ctx,
						       format_str,
						       NULL,
						       0,
						       outbuf,
						       IPMISELD_EVENT_OUTPUT_BUFLEN,
						       flags)) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
	return (-1);
      return (0);
    }
  
  if (outbuf_len)
    ipmiseld_syslog (state_data, "%s", outbuf);
  
  return (0);
}

static int
_sel_parse_callback (ipmi_sel_ctx_t ctx, void *callback_data)
{
  ipmiseld_state_data_t *state_data;
  uint8_t record_type;
  int record_type_class;
  int rv = -1;

  assert (ctx);
  assert (callback_data);

  state_data = (ipmiseld_state_data_t *)callback_data;

  if (state_data->prog_data->args->sensor_types_length
      || state_data->prog_data->args->exclude_sensor_types_length)
    {
      uint8_t sensor_type;
      int flag;

      if (ipmi_sel_parse_read_sensor_type (state_data->sel_ctx,
                                           NULL,
                                           0,
                                           &sensor_type) < 0)
        {
          if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_type") < 0)
            goto cleanup;
          goto out;
        }

      if (state_data->prog_data->args->sensor_types_length)
        {
          if ((flag = sensor_type_listed (NULL,
                                          sensor_type,
                                          state_data->prog_data->args->sensor_types,
                                          state_data->prog_data->args->sensor_types_length)) < 0)
            goto cleanup;
          
          if (!flag)
            goto out;
        }

      if (state_data->prog_data->args->exclude_sensor_types_length)
        {
          if ((flag = sensor_type_listed (NULL,
                                          sensor_type,
                                          state_data->prog_data->args->exclude_sensor_types,
                                          state_data->prog_data->args->exclude_sensor_types_length)) < 0)
            goto cleanup;

          if (flag)
            goto out;
        }
    }

  if (ipmi_sel_parse_read_record_type (state_data->sel_ctx,
                                       NULL,
                                       0,
                                       &record_type) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_type") < 0)
        goto cleanup;
      goto out;
    }

  /* IPMI Workaround
   *
   * HP DL 380 G5
   * Intel S2600JF/Appro 512X
   *
   * Motherboard is reporting invalid SEL Records types (0x00 on HP DL
   * 380 G5, 0x03 on Intel S2600JF/Appro 512X)
   */
  if (state_data->prog_data->args->common_args.section_specific_workaround_flags & IPMI_PARSE_SECTION_SPECIFIC_WORKAROUND_FLAGS_ASSUME_SYSTEM_EVENT
      && (!IPMI_SEL_RECORD_TYPE_VALID (record_type)))
    record_type = IPMI_SEL_RECORD_TYPE_SYSTEM_EVENT_RECORD;

  record_type_class = ipmi_sel_record_type_class (record_type);

  if (state_data->prog_data->args->system_event_only
      && record_type_class != IPMI_SEL_RECORD_TYPE_CLASS_SYSTEM_EVENT_RECORD)
    goto out;

  if (state_data->prog_data->args->oem_event_only
      && record_type_class != IPMI_SEL_RECORD_TYPE_CLASS_TIMESTAMPED_OEM_RECORD
      && record_type_class != IPMI_SEL_RECORD_TYPE_CLASS_NON_TIMESTAMPED_OEM_RECORD)
    goto out;

  if (state_data->prog_data->event_state_filter_mask)
    {
      char sel_record[IPMI_SEL_RECORD_MAX_RECORD_LENGTH];
      int sel_record_len;
      unsigned int event_state = 0;

      if ((sel_record_len = ipmi_sel_parse_read_record (state_data->sel_ctx,
							sel_record,
							IPMI_SEL_RECORD_MAX_RECORD_LENGTH)) < 0)
	{
	  if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_type") < 0)
	    goto cleanup;
	  goto out;
	}
      
      if (ipmi_interpret_sel (state_data->interpret_ctx,
			      sel_record,
			      sel_record_len,
			      &event_state) < 0)
	{
	  err_output ("ipmi_interpret_sel: %s",
		      ipmi_interpret_ctx_errormsg (state_data->interpret_ctx));
	  goto cleanup;
	}

      if ((state_data->prog_data->event_state_filter_mask & IPMISELD_NOMINAL_FILTER)
	  && event_state == IPMI_INTERPRET_STATE_NOMINAL)
	goto out;

      if ((state_data->prog_data->event_state_filter_mask & IPMISELD_WARNING_FILTER)
	  && event_state == IPMI_INTERPRET_STATE_WARNING)
	goto out;

      if ((state_data->prog_data->event_state_filter_mask & IPMISELD_CRITICAL_FILTER)
	  && event_state == IPMI_INTERPRET_STATE_CRITICAL)
	goto out;

      if ((state_data->prog_data->event_state_filter_mask & IPMISELD_NA_FILTER)
	  && event_state == IPMI_INTERPRET_STATE_UNKNOWN)
	goto out;
    }

  if (_normal_output (state_data, record_type) < 0)
    goto cleanup;

 out:
  rv = 0;
 cleanup:
  return (rv);
}

static int
run_cmd_args (ipmiseld_state_data_t *state_data)
{
  struct ipmiseld_arguments *args;

  assert (state_data);

  args = state_data->prog_data->args;

  if (ipmi_sel_ctx_set_separator (state_data->sel_ctx, EVENT_OUTPUT_SEPARATOR) < 0)
    {
      err_output ("ipmi_sel_parse: %s",
		  ipmi_sel_ctx_errormsg (state_data->sel_ctx));
      return (-1);
    }

  if (args->interpret_oem_data || args->output_oem_event_strings)
    {
      if (ipmi_get_oem_data (NULL,
                             state_data->ipmi_ctx,
                             &state_data->oem_data) < 0)
        return (-1);

      if (ipmi_sel_ctx_set_manufacturer_id (state_data->sel_ctx,
                                            state_data->oem_data.manufacturer_id) < 0)
        {
          err_output ("ipmi_sel_ctx_set_manufacturer_id: %s",
		      ipmi_sel_ctx_errormsg (state_data->sel_ctx));
          return (-1);
        }
      
      if (ipmi_sel_ctx_set_product_id (state_data->sel_ctx,
                                       state_data->oem_data.product_id) < 0)
        {
          err_output ("ipmi_sel_ctx_set_product_id: %s",
		      ipmi_sel_ctx_errormsg (state_data->sel_ctx));
          return (-1);
        }

      if (ipmi_sel_ctx_set_ipmi_version (state_data->sel_ctx,
                                         state_data->oem_data.ipmi_version_major,
                                         state_data->oem_data.ipmi_version_minor) < 0)
        {
          err_output ("ipmi_sel_ctx_set_ipmi_version: %s",
		      ipmi_sel_ctx_errormsg (state_data->sel_ctx));
          return (-1);
        }
      
      if (args->interpret_oem_data)
        {
          if (ipmi_interpret_ctx_set_manufacturer_id (state_data->interpret_ctx,
                                                      state_data->oem_data.manufacturer_id) < 0)
            {
              err_output ("ipmi_interpret_ctx_set_manufacturer_id: %s",
			  ipmi_interpret_ctx_errormsg (state_data->interpret_ctx));
              return (-1);
            }
	  
          if (ipmi_interpret_ctx_set_product_id (state_data->interpret_ctx,
                                                 state_data->oem_data.product_id) < 0)
            {
              err_output ("ipmi_interpret_ctx_set_product_id: %s",
			  ipmi_interpret_ctx_errormsg (state_data->interpret_ctx));
              return (-1);
            }
        }
    }

  if (ipmi_sel_parse (state_data->sel_ctx,
		      IPMI_SEL_RECORD_ID_FIRST,
		      IPMI_SEL_RECORD_ID_LAST,
		      _sel_parse_callback,
		      state_data) < 0)
    {
      err_output ("ipmi_sel_parse: %s",
		  ipmi_sel_ctx_errormsg (state_data->sel_ctx));
      return (-1);
    }

  return (0);
}

static int
_ipmi_setup (ipmiseld_state_data_t *state_data)
{
  struct common_cmd_args *common_args;
  unsigned int workaround_flags = 0;
  int rv = -1;

  assert (state_data);

  common_args = &(state_data->prog_data->args->common_args);

  if (!(state_data->ipmi_ctx = ipmi_ctx_create ()))
    {
      err_output ("ipmi_ctx_create: %s", strerror (errno));
      goto cleanup;
    }

  if (common_args->hostname
      && strcasecmp (common_args->hostname, "localhost") != 0
      && strcmp (common_args->hostname, "127.0.0.1") != 0)
    {
      if (common_args->driver_type == IPMI_DEVICE_LAN_2_0)
        {
          parse_get_freeipmi_outofband_2_0_flags (common_args->workaround_flags_outofband_2_0,
                                                  &workaround_flags);
          
          if (ipmi_ctx_open_outofband_2_0 (state_data->ipmi_ctx,
                                           common_args->hostname,
                                           common_args->username,
                                           common_args->password,
                                           (common_args->k_g_len) ? common_args->k_g : NULL,
                                           (common_args->k_g_len) ? common_args->k_g_len : 0,
                                           common_args->privilege_level,
                                           common_args->cipher_suite_id,
                                           common_args->session_timeout,
                                           common_args->retransmission_timeout,
                                           workaround_flags,
                                           (common_args->debug) ? IPMI_FLAGS_DEBUG_DUMP : IPMI_FLAGS_DEFAULT) < 0)
            {
	      /* XXX deal w/ specific errors */
	      err_output ("ipmi_ctx_open_outofband_2_0: %s", ipmi_ctx_errormsg (state_data->ipmi_ctx));
              goto cleanup;
            }
        }
      else
        {
          if (ipmi_ctx_open_outofband (state_data->ipmi_ctx,
                                       common_args->hostname,
                                       common_args->username,
                                       common_args->password,
                                       common_args->authentication_type,
                                       common_args->privilege_level,
                                       common_args->session_timeout,
                                       common_args->retransmission_timeout,
                                       workaround_flags,
                                       (common_args->debug) ? IPMI_FLAGS_DEBUG_DUMP : IPMI_FLAGS_DEFAULT) < 0)
            {
	      /* XXX deal w/ specific errors */
	      err_output ("ipmi_ctx_open_outofband: %s", ipmi_ctx_errormsg (state_data->ipmi_ctx));
              goto cleanup;
            }
        }
    }
  else
    {
      if (!ipmi_is_root ())
        {
	  err_output ("%s", ipmi_ctx_strerror (IPMI_ERR_PERMISSION));
          goto cleanup;
        }

      parse_get_freeipmi_inband_flags (common_args->workaround_flags_inband,
                                       &workaround_flags);

      if (common_args->driver_type == IPMI_DEVICE_UNKNOWN)
        {
          int ret;

          if ((ret = ipmi_ctx_find_inband (state_data->ipmi_ctx,
                                           NULL,
                                           common_args->disable_auto_probe,
                                           common_args->driver_address,
                                           common_args->register_spacing,
                                           common_args->driver_device,
                                           workaround_flags,
                                           (common_args->debug) ? IPMI_FLAGS_DEBUG_DUMP : IPMI_FLAGS_DEFAULT)) < 0)
            {
	      /* XXX deal w/ specific errors */
	      err_output ("ipmi_ctx_find_inband: %s", ipmi_ctx_errormsg (state_data->ipmi_ctx));
              goto cleanup;
            }

          if (!ret)
            {
	      /* XXX deal w/ specific errors */
              err_output ("could not find inband device");
              goto cleanup;
            }
        }
      else
        {
	  if (ipmi_ctx_open_inband (state_data->ipmi_ctx,
                                    common_args->driver_type,
                                    common_args->disable_auto_probe,
                                    common_args->driver_address,
                                    common_args->register_spacing,
                                    common_args->driver_device,
                                    workaround_flags,
                                    (common_args->debug) ? IPMI_FLAGS_DEBUG_DUMP : IPMI_FLAGS_DEFAULT) < 0)
            {
	      /* XXX deal w/ specific errors */
	      err_output ("ipmi_ctx_open_inband: %s", ipmi_ctx_errormsg (state_data->ipmi_ctx));
              goto cleanup;
            }
        }
    }

  if (common_args->target_channel_number_is_set
      || common_args->target_slave_address_is_set)
    {
      if (ipmi_ctx_set_target (state_data->ipmi_ctx,
                               common_args->target_channel_number_is_set ? &common_args->target_channel_number : NULL,
                               common_args->target_slave_address_is_set ? &common_args->target_slave_address : NULL) < 0)
        {
	  err_output ("ipmi_ctx_set_target: %s", ipmi_ctx_errormsg (state_data->ipmi_ctx));
          goto cleanup;
        } 
    }
  
  rv = 0;
 cleanup:
  if (rv < 0)
    {
      ipmi_ctx_close (state_data->ipmi_ctx);
      ipmi_ctx_destroy (state_data->ipmi_ctx);
    }
  return (rv);
}

static int
_ipmiseld (ipmiseld_prog_data_t *prog_data)
{
  ipmiseld_state_data_t state_data;
  unsigned int sel_flags = 0;
  unsigned int interpret_flags = 0;
  int exit_code = EXIT_FAILURE;

  assert (prog_data);

  memset (&state_data, '\0', sizeof (ipmiseld_state_data_t));
  state_data.prog_data = prog_data;
  /* XXX deal w/ hostrange later */
  state_data.hostname = prog_data->args->common_args.hostname;

  if (_ipmi_setup (&state_data) < 0)
    goto cleanup;

  if (ipmiseld_sdr_cache_create_and_load (&state_data, state_data.hostname) < 0)
    goto cleanup;
  
  if (!(state_data.sel_ctx = ipmi_sel_ctx_create (state_data.ipmi_ctx, state_data.sdr_ctx)))
    {
      err_output ("ipmi_sel_ctx_create: %s", strerror (errno));
      goto cleanup;
    }
  
  if (state_data.prog_data->args->foreground
      && state_data.prog_data->args->common_args.debug)
    sel_flags |= IPMI_SEL_FLAGS_DEBUG_DUMP;
  
  if (state_data.prog_data->args->common_args.section_specific_workaround_flags & IPMI_PARSE_SECTION_SPECIFIC_WORKAROUND_FLAGS_ASSUME_SYSTEM_EVENT)
    sel_flags |= IPMI_SEL_FLAGS_ASSUME_SYTEM_EVENT_RECORDS;
  
  if (sel_flags)
    {
      /* Don't error out, if this fails we can still continue */
      if (ipmi_sel_ctx_set_flags (state_data.sel_ctx, sel_flags) < 0)
	err_output ("ipmi_sel_ctx_set_flags: %s",
		    ipmi_sel_ctx_errormsg (state_data.sel_ctx));
    }

  if (state_data.prog_data->args->foreground
      && state_data.prog_data->args->common_args.debug
      && prog_data->args->common_args.hostname)
    {
      if (ipmi_sel_ctx_set_debug_prefix (state_data.sel_ctx,
                                         prog_data->args->common_args.hostname) < 0)
        err_output ("ipmi_sel_ctx_set_debug_prefix: %s",
		    ipmi_sel_ctx_errormsg (state_data.sel_ctx));
    }
  
  if (!(state_data.interpret_ctx = ipmi_interpret_ctx_create ()))
    {
      err_output ("ipmi_interpret_ctx_create: %s", strerror (errno));
      goto cleanup;
    }


  if (ipmi_interpret_load_sel_config (state_data.interpret_ctx,
				      prog_data->args->event_state_config_file) < 0)
    {
      /* if default file is missing its ok */
      if (!(!prog_data->args->event_state_config_file
	    && ipmi_interpret_ctx_errnum (state_data.interpret_ctx) == IPMI_INTERPRET_ERR_SEL_CONFIG_FILE_DOES_NOT_EXIST))
	{
	  err_output ("ipmi_interpret_load_sel_config: %s", ipmi_interpret_ctx_errormsg (state_data.interpret_ctx));
	  goto cleanup;
        }
    }

  if (prog_data->args->interpret_oem_data)
    interpret_flags |= IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA;
  
  if (interpret_flags)
    {
      if (ipmi_interpret_ctx_set_flags (state_data.interpret_ctx, interpret_flags) < 0)
	{
	  err_output ("ipmi_interpret_ctx_set_flags: %s",
		      ipmi_interpret_ctx_errormsg (state_data.interpret_ctx));
	  goto cleanup;
	}
    }

  if (ipmi_sel_ctx_set_parameter (state_data.sel_ctx,
				  IPMI_SEL_PARAMETER_INTERPRET_CONTEXT,
				  &(state_data.interpret_ctx)) < 0)
    {
      err_output("ipmi_sel_ctx_set_interpret: %s",
		 ipmi_sel_ctx_errormsg (state_data.sel_ctx));
      goto cleanup;
    }
  
  if (run_cmd_args (&state_data) < 0)
    goto cleanup;

  exit_code = EXIT_SUCCESS;
 cleanup:
  ipmi_interpret_ctx_destroy (state_data.interpret_ctx);
  ipmi_sel_ctx_destroy (state_data.sel_ctx);
  ipmi_sdr_ctx_destroy (state_data.sdr_ctx);
  ipmi_ctx_close (state_data.ipmi_ctx);
  ipmi_ctx_destroy (state_data.ipmi_ctx);
  return (exit_code);
}

int
main (int argc, char **argv)
{
  ipmiseld_prog_data_t prog_data;
  struct ipmiseld_arguments cmd_args;

  err_init (argv[0]);
  err_set_flags (ERROR_STDERR);

  ipmi_disable_coredump ();

  memset (&prog_data, '\0', sizeof (ipmiseld_prog_data_t));
  prog_data.progname = argv[0];
  ipmiseld_argp_parse (argc, argv, &cmd_args);
  prog_data.args = &cmd_args;

  if (prog_data.args->event_state_filter_str)
    prog_data.event_state_filter_mask = ipmiseld_event_state_filter_parse (prog_data.args->event_state_filter_str);
  else
    prog_data.event_state_filter_mask = 0;

  if (prog_data.args->log_facility_str)
    prog_data.log_facility = ipmiseld_log_facility_parse (prog_data.args->log_facility_str);
  else
    prog_data.log_facility = LOG_DAEMON;

  if (prog_data.args->log_priority_str)
    prog_data.log_priority = ipmiseld_log_priority_parse (prog_data.args->log_priority_str);
  else
    prog_data.log_priority = LOG_ERR;

  if (!cmd_args.test_run)
    {
      if (!cmd_args.foreground)
	{
	  daemonize_common (IPMISELD_PIDFILE);
	  err_set_flags (ERROR_SYSLOG);
	}
      else
	err_set_flags (ERROR_STDERR);
      
      daemon_signal_handler_setup (NULL);

      /* Call after daemonization, since daemonization closes currently
       * open fds
       */
      openlog (argv[0], LOG_ODELAY | LOG_PID, prog_data.log_facility);
    }
  
  return (_ipmiseld (&prog_data));
}
/*
 * Copyright (C) 2003-2012 FreeIPMI Core Team
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
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
#else /* !TIME_WITH_SYS_TIME */
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else /* !HAVE_SYS_TIME_H */
#include <time.h>
#endif /* !HAVE_SYS_TIME_H */
#endif /* !TIME_WITH_SYS_TIME */
#include <assert.h>

#include <freeipmi/freeipmi.h>

#include "ipmi-sel.h"
#include "ipmi-sel-argp.h"

#include "freeipmi-portability.h"
#include "pstdout.h"
#include "tool-common.h"
#include "tool-cmdline-common.h"
#include "tool-event-common.h"
#include "tool-hostrange-common.h"
#include "tool-oem-common.h"
#include "tool-sdr-cache-common.h"
#include "tool-sensor-common.h"

static int
_display_sel_info (ipmi_sel_state_data_t *state_data)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint8_t major, minor;
  uint16_t entries, free_space;
  uint64_t val;
  char timestr[512];
  int rv = -1;
  time_t t;
  struct tm tm;
  uint8_t allocation_supported = 0;

  assert (state_data);

  if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_sel_info_rs)))
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_create: %s\n",
                       strerror (errno));
      goto cleanup;
    }

  if (ipmi_cmd_get_sel_info (state_data->ipmi_ctx, obj_cmd_rs) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_cmd_get_sel_info: %s\n",
                       ipmi_ctx_errormsg (state_data->ipmi_ctx));
      goto cleanup;
    }

  if (FIID_OBJ_GET (obj_cmd_rs, "sel_version_major", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'sel_version_major': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  major = val;

  if (FIID_OBJ_GET (obj_cmd_rs, "sel_version_minor", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'sel_version_minor': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  minor = val;

  /* achu: ipmi version is BCD encoded, but major/minor are only 4 bits */
  pstdout_printf (state_data->pstate,
                  "SEL version                            : %u.%u\n",
                  major,
                  minor);

  if (FIID_OBJ_GET (obj_cmd_rs, "entries", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'entries': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  entries = val;

  pstdout_printf (state_data->pstate,
                  "Number of log entries                  : %u\n",
                  entries);

  if (FIID_OBJ_GET (obj_cmd_rs, "free_space", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'free_space': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }
  free_space = val;

  pstdout_printf (state_data->pstate,
                  "Free space remaining                   : %u bytes\n",
                  free_space);

  if (FIID_OBJ_GET (obj_cmd_rs, "most_recent_addition_timestamp", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'most_recent_addition_timestamp': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  /* Posix says individual calls need not clear/set all portions of
   * 'struct tm', thus passing 'struct tm' between functions could
   * have issues.  So we need to memset.
   */
  memset (&tm, '\0', sizeof(struct tm));

  t = val;
  localtime_r (&t, &tm);
  strftime (timestr, sizeof (timestr), "%m/%d/%Y - %H:%M:%S", &tm);
  pstdout_printf (state_data->pstate,
                  "Recent addition timestamp              : %s\n",
                  timestr);

  if (FIID_OBJ_GET (obj_cmd_rs, "most_recent_erase_timestamp", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'most_recent_erase_timestamp': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  /* Posix says individual calls need not clear/set all portions of
   * 'struct tm', thus passing 'struct tm' between functions could
   * have issues.  So we need to memset.
   */
  memset (&tm, '\0', sizeof(struct tm));

  t = val;
  localtime_r (&t, &tm);
  strftime (timestr, sizeof (timestr), "%m/%d/%Y - %H:%M:%S", &tm);
  pstdout_printf (state_data->pstate,
                  "Recent erase timestamp                 : %s\n",
                  timestr);

  if (FIID_OBJ_GET (obj_cmd_rs, "get_sel_allocation_info_command_supported", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'get_sel_allocation_info_command_supported': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  pstdout_printf (state_data->pstate,
                  "Get SEL Allocation Information Command : %s\n",
                  (val ? "supported" : "unsupported"));

  allocation_supported = val;

  if (FIID_OBJ_GET (obj_cmd_rs, "reserve_sel_command_supported", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'reserve_sel_command_supported': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  pstdout_printf (state_data->pstate,
                  "Reserve SEL Command                    : %s\n",
                  (val ? "supported" : "unsupported"));

  if (FIID_OBJ_GET (obj_cmd_rs, "partial_add_sel_entry_command_supported", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'partial_add_sel_entry_command_supported': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  pstdout_printf (state_data->pstate,
                  "Partial Add SEL Entry Command          : %s\n",
                  (val ? "supported" : "unsupported"));

  if (FIID_OBJ_GET (obj_cmd_rs, "delete_sel_command_supported", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'delete_sel_command_supported': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  pstdout_printf (state_data->pstate,
                  "Delete SEL Command                     : %s\n",
                  (val ? "supported" : "unsupported"));

  if (FIID_OBJ_GET (obj_cmd_rs, "overflow_flag", &val) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "fiid_obj_get: 'overflow_flag': %s\n",
                       fiid_obj_errormsg (obj_cmd_rs));
      goto cleanup;
    }

  /* "Events have been dropped due to lack of space in the SEL" */
  pstdout_printf (state_data->pstate,
                  "Events dropped due to lack of space    : %s\n",
                  (val ? "Yes" : "No"));

  if (allocation_supported)
    {
      uint16_t number_of_possible_allocation_units;
      uint16_t allocation_unit_size;
      uint16_t number_of_free_allocation_units;
      uint16_t largest_free_block;
      uint8_t maximum_record_size;

      fiid_obj_destroy (obj_cmd_rs);
      
      if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_sel_allocation_info_rs)))
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_create: %s\n",
                           strerror (errno));
          goto cleanup;
        }

      if (ipmi_cmd_get_sel_allocation_info (state_data->ipmi_ctx, obj_cmd_rs) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_cmd_get_sel_allocation_info: %s\n",
                           ipmi_ctx_errormsg (state_data->ipmi_ctx));
          goto cleanup;
        }
      
      if (FIID_OBJ_GET (obj_cmd_rs, "number_of_possible_allocation_units", &val) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get: 'number_of_possible_allocation_units': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      number_of_possible_allocation_units = val;
       
      if (FIID_OBJ_GET (obj_cmd_rs, "allocation_unit_size", &val) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get: 'allocation_unit_size': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      allocation_unit_size = val;

      if (FIID_OBJ_GET (obj_cmd_rs, "number_of_free_allocation_units", &val) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get: 'number_of_free_allocation_units': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      number_of_free_allocation_units = val;

      if (FIID_OBJ_GET (obj_cmd_rs, "largest_free_block", &val) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get: 'largest_free_block': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      largest_free_block = val;

      if (FIID_OBJ_GET (obj_cmd_rs, "maximum_record_size", &val) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get: 'maximum_record_size': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      maximum_record_size = val;

      if (!number_of_possible_allocation_units)
        pstdout_printf (state_data->pstate,
                        "Number of possible allocation units    : unspecified\n");
      else
        pstdout_printf (state_data->pstate,
                        "Number of possible allocation units    : %u\n",
                        number_of_possible_allocation_units);
      
      if (!allocation_unit_size)
        pstdout_printf (state_data->pstate,
                        "Allocation unit size                   : unspecified\n");
      else
        pstdout_printf (state_data->pstate,
                        "Allocation unit size                   : %u bytes\n",
                        allocation_unit_size);
      
      pstdout_printf (state_data->pstate,
                      "Number of free allocation units        : %u\n",
                      number_of_free_allocation_units);
      
      pstdout_printf (state_data->pstate,
                      "Largest free block                     : %u allocation units\n",
                      largest_free_block);

      pstdout_printf (state_data->pstate,
                      "Maximum record size                    : %u allocation units\n",
                      maximum_record_size);
    }

  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
_flush_cache (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);

  if (sdr_cache_flush_cache (state_data->sdr_cache_ctx,
                             state_data->pstate,
                             state_data->prog_data->args->sdr.quiet_cache,
                             state_data->hostname,
			     state_data->prog_data->args->sdr.sdr_cache_directory,
			     state_data->prog_data->args->sdr.sdr_cache_file) < 0)
    return (-1);

  return (0);
}

static int
_list_sensor_types (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);

  if (list_sensor_types (state_data->pstate) < 0)
    return (-1);

  return (0);
}

static int
_clear_entries (ipmi_sel_state_data_t *state_data)
{
  int rv = -1;

  assert (state_data);

  if (ipmi_sel_parse_clear_sel (state_data->sel_parse_ctx) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sel_parse_clear_sel: %s\n",
                       ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
      goto cleanup;
    }

  rv = 0;
 cleanup:
  return (rv);
}

static int
_delete_entry (ipmi_sel_state_data_t *state_data,
               uint16_t record_id,
               int ignore_missing_sel_entries)
{
  int rv = -1;

  assert (state_data);
  assert (record_id);

  if (ipmi_sel_parse_delete_sel_entry (state_data->sel_parse_ctx,
                                       record_id) < 0)
    {
      if (!(ignore_missing_sel_entries
            && (ipmi_sel_parse_ctx_errnum (state_data->sel_parse_ctx) == IPMI_SEL_PARSE_ERR_NOT_FOUND)))
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse_delete_sel_entry: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }
    }

  rv = 0;
 cleanup:
  return (rv);
}

static int
_delete_records (ipmi_sel_state_data_t *state_data)
{
  struct ipmi_sel_arguments *args;
  unsigned int i;

  assert (state_data);

  args = state_data->prog_data->args;

  for (i = 0; i < args->delete_record_list_length; i++)
    {
      if (_delete_entry (state_data,
                         args->delete_record_list[i],
                         0) < 0)
        return (-1);
    }

  return (0);
}

static int
_delete_range (ipmi_sel_state_data_t *state_data)
{
  struct ipmi_sel_arguments *args;
  unsigned int i;

  assert (state_data);

  args = state_data->prog_data->args;

  for (i = args->delete_range1; i <= args->delete_range2; i++)
    {
      if (_delete_entry (state_data, i, 1) < 0)
        return (-1);
    }

  return (0);
}

/* return (-1), real error */
static int
_sel_parse_err_handle (ipmi_sel_state_data_t *state_data, char *func)
{
  assert (state_data);
  assert (func);

  if (ipmi_sel_parse_ctx_errnum (state_data->sel_parse_ctx) == IPMI_SEL_PARSE_ERR_INVALID_SEL_ENTRY)
    {
      /* maybe a bad SEL entry returned from remote system, don't error out */
      if (state_data->prog_data->args->common.debug)
        pstdout_fprintf (state_data->pstate,
                         stderr,
                         "Invalid SEL entry read\n");
      return (0);
    }

  pstdout_fprintf (state_data->pstate,
                   stderr,
                   "%s: %s\n",
                   func,
                   ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
  return (-1);
}

static int
_hex_output (ipmi_sel_state_data_t *state_data)
{
  uint8_t record_data[IPMI_SEL_RECORD_MAX_RECORD_LENGTH];
  int record_data_len;
  int rv = -1;

  assert (state_data);

  if ((record_data_len = ipmi_sel_parse_read_record (state_data->sel_parse_ctx,
                                                     record_data,
                                                     IPMI_SEL_RECORD_MAX_RECORD_LENGTH)) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record") < 0)
        goto cleanup;
      goto out;
    }

  if (state_data->prog_data->args->common.debug
      && record_data_len < IPMI_SEL_RECORD_MAX_RECORD_LENGTH)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "Invalid length SEL entry read: %d\n",
                       record_data_len);
      goto out;
    }

  pstdout_printf (state_data->pstate,
                  "RID:[%02X][%02X] "
                  "RT:[%02X] "
                  "TS:[%02X][%02X][%02X][%02X] "
                  "GID:[%02X][%02X] "
                  "ER:[%02X] "
                  "ST:[%02X] "
                  "SN:[%02X] "
                  "EDIR:[%02X] "
                  "ED1: [%02X] "
                  "ED2: [%02X] "
                  "ED3: [%02X]\n",
                  record_data[0], record_data[1],
                  record_data[2],
                  record_data[3], record_data[4], record_data[5], record_data[6],
                  record_data[7], record_data[8],
                  record_data[9],
                  record_data[10],
                  record_data[11],
                  record_data[12],
                  record_data[13],
                  record_data[14],
                  record_data[15]);

 out:
  rv = 0;
 cleanup:
  return (rv);
}

static int
_legacy_normal_output (ipmi_sel_state_data_t *state_data, uint8_t record_type)
{
  char fmtbuf[EVENT_FMT_BUFLEN+1];
  char outbuf[EVENT_OUTPUT_BUFLEN+1];
  char *fmt;
  int outbuf_len;
  unsigned int flags;
  int record_type_class;
  int rv = -1;
  int ret;

  assert (state_data);
  assert (state_data->prog_data->args->legacy_output);

  flags = IPMI_SEL_PARSE_STRING_FLAGS_IGNORE_UNAVAILABLE_FIELD;
  flags |= IPMI_SEL_PARSE_STRING_FLAGS_OUTPUT_NOT_AVAILABLE;
  flags |= IPMI_SEL_PARSE_STRING_FLAGS_DATE_MONTH_STRING;
  flags |= IPMI_SEL_PARSE_STRING_FLAGS_LEGACY;

  /* IPMI Workaround
   *
   * HP DL 380 G5
   * Intel S2600JF/Appro 512X
   *
   * Motherboard is reporting invalid SEL Records types (0x00 on HP DL
   * 380 G5, 0x03 on Intel S2600JF/Appro 512X)
   */
  if (state_data->prog_data->args->assume_system_event_records
      && (!IPMI_SEL_RECORD_TYPE_VALID (record_type)))
    record_type = IPMI_SEL_RECORD_TYPE_SYSTEM_EVENT_RECORD;

  record_type_class = ipmi_sel_record_type_class (record_type);
  if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_SYSTEM_EVENT_RECORD)
    {
      uint8_t event_type_code;
      uint8_t event_data2_flag;
      uint8_t event_data3_flag;
      uint8_t event_data2;
      uint8_t event_data3;

      if ((ret = event_data_info (state_data->pstate,
				  state_data->sel_parse_ctx,
				  NULL,
				  0,
				  state_data->prog_data->args->common.debug,
				  NULL,
				  NULL,
				  NULL,
				  &event_type_code,
				  &event_data2_flag,
				  &event_data3_flag,
				  &event_data2,
				  &event_data3)) < 0)
        goto cleanup;

      if (!ret)
        goto out;

      strcpy (fmtbuf, "%i:%d %t:%T %s:%e");

      /* achu: special case, legacy output didn't support
         previous/severity output and would not output 0xFF for
         discrete events.
      */
      if (!(((ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_GENERIC_DISCRETE
              || ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_SENSOR_SPECIFIC_DISCRETE)
             && event_data2_flag == IPMI_SEL_EVENT_DATA_PREVIOUS_STATE_OR_SEVERITY)
            || ((ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD
                 || ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_GENERIC_DISCRETE
                 || ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_SENSOR_SPECIFIC_DISCRETE)
                && event_data2_flag == IPMI_SEL_EVENT_DATA_SENSOR_SPECIFIC_EVENT_EXTENSION_CODE
                && event_data2 == IPMI_SEL_RECORD_UNSPECIFIED_EVENT)))
        {
          if (event_data2_flag != IPMI_SEL_EVENT_DATA_UNSPECIFIED_BYTE)
            strcat (fmtbuf, ":%f");
        }

      if (!((ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_THRESHOLD
             || ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_GENERIC_DISCRETE
             || ipmi_event_reading_type_code_class (event_type_code) == IPMI_EVENT_READING_TYPE_CODE_CLASS_SENSOR_SPECIFIC_DISCRETE)
            && event_data3_flag == IPMI_SEL_EVENT_DATA_SENSOR_SPECIFIC_EVENT_EXTENSION_CODE
            && event_data3 == IPMI_SEL_RECORD_UNSPECIFIED_EVENT))
        {
          if (event_data3_flag != IPMI_SEL_EVENT_DATA_UNSPECIFIED_BYTE)
            strcat (fmtbuf, ":%h");
        }

      fmt = fmtbuf;
    }
  else if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_TIMESTAMPED_OEM_RECORD)
    fmt = "%i:%d %t:%m:%o";
  else if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_NON_TIMESTAMPED_OEM_RECORD)
    fmt = "%i:o";
  else
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "Unknown SEL Record Type: %Xh\n",
                       record_type);
      goto out;
    }

  memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
  if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                       fmt,
                                                       outbuf,
                                                       EVENT_OUTPUT_BUFLEN,
                                                       flags)) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
        goto cleanup;
      goto out;
    }

  if (outbuf_len)
    pstdout_printf (state_data->pstate, "%s\n", outbuf);

 out:
  rv = 0;
 cleanup:
  return (rv);
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_record_id (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  char fmt[EVENT_FMT_BUFLEN + 1];
  char outbuf[EVENT_OUTPUT_BUFLEN+1];
  int outbuf_len;

  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
  if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                       "%i",
                                                       outbuf,
                                                       EVENT_OUTPUT_BUFLEN,
                                                       flags)) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
        return (-1);
      return (0);
    }

  memset (fmt, '\0', EVENT_FMT_BUFLEN + 1);
  if (state_data->prog_data->args->comma_separated_output)
    snprintf (fmt,
              EVENT_FMT_BUFLEN,
              "%%s");
  else
    snprintf (fmt,
              EVENT_FMT_BUFLEN,
              "%%-%ds",
              state_data->column_width.record_id);

  if (outbuf_len)
    pstdout_printf (state_data->pstate, fmt, outbuf);
  else
    pstdout_printf (state_data->pstate, fmt, EVENT_NA_STRING);

  return (1);
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_date (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  char outbuf[EVENT_OUTPUT_BUFLEN+1];
  int outbuf_len;

  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
  if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                       "%d",
                                                       outbuf,
                                                       EVENT_OUTPUT_BUFLEN,
                                                       flags)) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
        return (-1);
      return (0);
    }

  if (state_data->prog_data->args->comma_separated_output)
    {
      if (outbuf_len)
        pstdout_printf (state_data->pstate, ",%s", outbuf);
      else
        pstdout_printf (state_data->pstate, ",%s", EVENT_NA_STRING);
    }
  else
    {
      if (outbuf_len)
        pstdout_printf (state_data->pstate, " | %-11s", outbuf);
      else
        pstdout_printf (state_data->pstate, " | %-11s", EVENT_NA_STRING);
    }

  return (1);
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_not_available_date (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  if (state_data->prog_data->args->comma_separated_output)
    pstdout_printf (state_data->pstate, ",%s", EVENT_NA_STRING);
  else
    pstdout_printf (state_data->pstate, " | %-11s", EVENT_NA_STRING);

  return (1);
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_time (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  return (event_output_time (state_data->pstate,
                             state_data->sel_parse_ctx,
			     NULL,
			     0,
                             state_data->prog_data->args->comma_separated_output,
                             state_data->prog_data->args->common.debug,
                             flags));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_not_available_time (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);
  
  return (event_output_not_available_time (state_data->pstate,
                                           state_data->prog_data->args->comma_separated_output));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_sensor_name (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  return (event_output_sensor_name (state_data->pstate,
                                    state_data->sel_parse_ctx,
                                    state_data->sdr_cache_ctx,
                                    state_data->sdr_parse_ctx,
				    NULL,
				    0,
                                    &state_data->entity_id_counts,
                                    &state_data->column_width,
				    &state_data->prog_data->args->sdr,
				    state_data->prog_data->args->entity_sensor_names,
                                    state_data->prog_data->args->comma_separated_output,
                                    state_data->prog_data->args->common.debug,
                                    flags));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_not_available_sensor_name (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  return (event_output_not_available_sensor_name (state_data->pstate,
						  &state_data->column_width,
						  state_data->prog_data->args->comma_separated_output));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_sensor_type (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  assert (state_data);
  assert (!state_data->prog_data->args->no_sensor_type_output);
  assert (!state_data->prog_data->args->legacy_output);

  return (event_output_sensor_type (state_data->pstate,
				    state_data->sel_parse_ctx,
				    NULL,
				    0,
				    &state_data->column_width,
				    state_data->prog_data->args->comma_separated_output,
				    state_data->prog_data->args->common.debug,
				    flags));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_not_available_sensor_type (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);
  assert (!state_data->prog_data->args->no_sensor_type_output);
  assert (!state_data->prog_data->args->legacy_output);

  return (event_output_not_available_sensor_type (state_data->pstate,
						  &state_data->column_width,
						  state_data->prog_data->args->comma_separated_output));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_event_state (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);
  assert (state_data->prog_data->args->output_event_state);
  
  return (event_output_event_state (state_data->pstate,
				    state_data->sel_parse_ctx,
				    state_data->interpret_ctx,
				    NULL,
				    0,
				    state_data->prog_data->args->comma_separated_output,
				    state_data->prog_data->args->common.debug,
				    flags));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_event_direction (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);
  assert (state_data->prog_data->args->verbose_count >= 1);

  return (event_output_event_direction (state_data->pstate,
					state_data->sel_parse_ctx,
					NULL,
					0,
					state_data->prog_data->args->comma_separated_output,
					state_data->prog_data->args->common.debug,
					flags));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_not_available_event_direction (ipmi_sel_state_data_t *state_data)
{
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);
  assert (state_data->prog_data->args->verbose_count >= 1);

  return (event_output_not_available_event_direction (state_data->pstate,
						      state_data->prog_data->args->comma_separated_output));
}

/* return length written into buffer on success (may be zero)
 * return (-1) on error
 */
static int
_output_oem_event_strings (ipmi_sel_state_data_t *state_data,
                           char *outbuf,
                           unsigned int outbuflen,
                           unsigned int flags)
{
  unsigned int len;
  int ret;

  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  if ((ret = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                "%O",
                                                outbuf,
                                                outbuflen,
                                                flags)) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
        return (-1);
      return (0);
    }

  len = strlen (outbuf);
  
  /* we don't count N/A as a valid string to be returned */
  
  if (len && strcmp (outbuf, EVENT_NA_STRING))
    return (ret);
  
  return (0);
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_event (ipmi_sel_state_data_t *state_data, unsigned int flags)
{
  char outbuf[EVENT_OUTPUT_BUFLEN+1];
  int outbuf_len = 0;

  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  if (state_data->prog_data->args->output_oem_event_strings)
    {
      memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
      if ((outbuf_len = _output_oem_event_strings (state_data,
						   outbuf,
						   EVENT_OUTPUT_BUFLEN,
						   flags)) < 0)
	return (-1);
      
      if (outbuf_len)
	{
	  if (state_data->prog_data->args->comma_separated_output)
	    pstdout_printf (state_data->pstate, ",%s", outbuf);
	  else
	    pstdout_printf (state_data->pstate, " | %s", outbuf);
	  return (1);
	}
    }
  
  return (event_output_event (state_data->pstate,
			      state_data->sel_parse_ctx,
			      NULL,
			      0,
			      &state_data->oem_data,
			      state_data->prog_data->args->interpret_oem_data,
			      state_data->prog_data->args->comma_separated_output,
			      state_data->prog_data->args->common.debug,
			      flags));
}

/* return 1 on success
 * return (0) on non-success, but don't fail
 * return (-1) on error
 */
static int
_normal_output_oem_data (ipmi_sel_state_data_t *state_data,
                         int record_has_manufacturer_id,
                         unsigned int flags)
{
  char outbuf[EVENT_OUTPUT_BUFLEN+1];
  int outbuf_len = 0;
  
  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);
  
  if (state_data->prog_data->args->output_oem_event_strings)
    {
      memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
      if ((outbuf_len = _output_oem_event_strings (state_data,
                                                   outbuf,
                                                   EVENT_OUTPUT_BUFLEN,
                                                   flags)) < 0)
        return (-1);
    }

  if (!outbuf_len)
    {
      memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
      if (state_data->prog_data->args->output_manufacturer_id
          && record_has_manufacturer_id)
        {
          if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                               "%m ; %o",
                                                               outbuf,
                                                               EVENT_OUTPUT_BUFLEN,
                                                               flags)) < 0)
            {
              if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
                return (-1);
              return (0);
            }
        }
      else
        {
          if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                               "%o",
                                                               outbuf,
                                                               EVENT_OUTPUT_BUFLEN,
                                                               flags)) < 0)
            {
              if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_string") < 0)
                return (-1);
              return (0);
            }
        }
    }

  if (state_data->prog_data->args->comma_separated_output)
    {
      if (outbuf_len)
        pstdout_printf (state_data->pstate, ",%s", outbuf);
      else
        pstdout_printf (state_data->pstate, ",%s", EVENT_NA_STRING);
    }
  else
    {
      if (outbuf_len)
        pstdout_printf (state_data->pstate, " | %s", outbuf);
      else
        pstdout_printf (state_data->pstate, " | %s", EVENT_NA_STRING);
    }

  return (1);
}

static int
_normal_output (ipmi_sel_state_data_t *state_data, uint8_t record_type)
{
  char fmt[EVENT_FMT_BUFLEN + 1];
  unsigned int flags;
  int record_type_class;
  int rv = -1;
  int ret;

  assert (state_data);
  assert (!state_data->prog_data->args->legacy_output);

  if (!state_data->prog_data->args->no_header_output
      && !state_data->output_headers)
    {
      if (state_data->prog_data->args->comma_separated_output)
        {
          if (state_data->prog_data->args->no_sensor_type_output)
            pstdout_printf (state_data->pstate,
                            "%s,Date,Time,%s",
                            SENSORS_HEADER_RECORD_ID_STR,
                            SENSORS_HEADER_NAME_STR);
          else
            pstdout_printf (state_data->pstate,
                            "%s,Date,Time,%s,%s",
                            SENSORS_HEADER_RECORD_ID_STR,
                            SENSORS_HEADER_NAME_STR,
                            SENSORS_HEADER_TYPE_STR);

          if (state_data->prog_data->args->output_event_state)
            pstdout_printf (state_data->pstate,
                            ",%s",
                            SENSORS_HEADER_STATE_STR);

          if (state_data->prog_data->args->verbose_count >= 1)
            pstdout_printf (state_data->pstate, ",Event Direction");
          
          pstdout_printf (state_data->pstate, ",Event");
          
          pstdout_printf (state_data->pstate, "\n");
        }
      else
        {          
          memset (fmt, '\0', EVENT_FMT_BUFLEN + 1);
          if (state_data->prog_data->args->no_sensor_type_output)
            {
              snprintf (fmt,
                        EVENT_FMT_BUFLEN,
                        "%%-%ds | Date        | Time     | %%-%ds",
                        state_data->column_width.record_id,
                        state_data->column_width.sensor_name);
              
              pstdout_printf (state_data->pstate,
                              fmt,
                              SENSORS_HEADER_RECORD_ID_STR,
                              SENSORS_HEADER_NAME_STR);
            }
          else
            {
              snprintf (fmt,
                        EVENT_FMT_BUFLEN,
                        "%%-%ds | Date        | Time     | %%-%ds | %%-%ds",
                        state_data->column_width.record_id,
                        state_data->column_width.sensor_name,
                        state_data->column_width.sensor_type);
              
              pstdout_printf (state_data->pstate,
                              fmt,
                              SENSORS_HEADER_RECORD_ID_STR,
                              SENSORS_HEADER_NAME_STR,
                              SENSORS_HEADER_TYPE_STR);
            }
          
          if (state_data->prog_data->args->output_event_state)
            pstdout_printf (state_data->pstate,
                            " | %s   ",
                            SENSORS_HEADER_STATE_STR);

          if (state_data->prog_data->args->verbose_count >= 1)
            pstdout_printf (state_data->pstate, " | Event Direction  ");
          
          pstdout_printf (state_data->pstate, " | Event");
          
          pstdout_printf (state_data->pstate, "\n");
        }

      state_data->output_headers++;
    }

  flags = IPMI_SEL_PARSE_STRING_FLAGS_IGNORE_UNAVAILABLE_FIELD;
  flags |= IPMI_SEL_PARSE_STRING_FLAGS_OUTPUT_NOT_AVAILABLE;
  flags |= IPMI_SEL_PARSE_STRING_FLAGS_DATE_MONTH_STRING;
  if (state_data->prog_data->args->verbose_count >= 2)
    flags |= IPMI_SEL_PARSE_STRING_FLAGS_VERBOSE;
  if (state_data->prog_data->args->non_abbreviated_units)
    flags |= IPMI_SEL_PARSE_STRING_FLAGS_NON_ABBREVIATED_UNITS;
  if (state_data->prog_data->args->interpret_oem_data)
    flags |= IPMI_SEL_PARSE_STRING_FLAGS_INTERPRET_OEM_DATA;

  /* IPMI Workaround
   *
   * HP DL 380 G5
   * Intel S2600JF/Appro 512X
   *
   * Motherboard is reporting invalid SEL Records types (0x00 on HP DL
   * 380 G5, 0x03 on Intel S2600JF/Appro 512X)
   */
  if (state_data->prog_data->args->assume_system_event_records
      && (!IPMI_SEL_RECORD_TYPE_VALID (record_type)))
    record_type = IPMI_SEL_RECORD_TYPE_SYSTEM_EVENT_RECORD;

  record_type_class = ipmi_sel_record_type_class (record_type);
  if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_SYSTEM_EVENT_RECORD)
    {
      if ((ret = _normal_output_record_id (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto out;

      if ((ret = _normal_output_date (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if ((ret = _normal_output_time (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if ((ret = _normal_output_sensor_name (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if (!state_data->prog_data->args->no_sensor_type_output)
	{
	  if ((ret = _normal_output_sensor_type (state_data, flags)) < 0)
	    goto cleanup;

	  if (!ret)
	    goto newline_out;
	}

      if (state_data->prog_data->args->output_event_state)
        {
          if ((ret = _normal_output_event_state (state_data, flags)) < 0)
            goto cleanup;

          if (!ret)
            goto newline_out;
        }

      if (state_data->prog_data->args->verbose_count >= 1)
        {
          if ((ret = _normal_output_event_direction (state_data, flags)) < 0)
            goto cleanup;

          if (!ret)
            goto newline_out;
        }

      if ((ret = _normal_output_event (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;
    }
  else if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_TIMESTAMPED_OEM_RECORD)
    {
      if ((ret = _normal_output_record_id (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto out;

      if ((ret = _normal_output_date (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if ((ret = _normal_output_time (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if ((ret = _normal_output_not_available_sensor_name (state_data)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if (!state_data->prog_data->args->no_sensor_type_output)
	{
	  if ((ret = _normal_output_not_available_sensor_type (state_data)) < 0)
	    goto cleanup;

	  if (!ret)
	    goto newline_out;
	}

      if (state_data->prog_data->args->output_event_state)
        {
          if ((ret = _normal_output_event_state (state_data, flags)) < 0)
            goto cleanup;

          if (!ret)
            goto newline_out;
        }

      if (state_data->prog_data->args->verbose_count >= 1)
        {
          if ((ret = _normal_output_not_available_event_direction (state_data)) < 0)
            goto cleanup;

          if (!ret)
            goto newline_out;
        }

      if ((ret = _normal_output_oem_data (state_data, 1, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;
    }
  else if (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_NON_TIMESTAMPED_OEM_RECORD)
    {
      if ((ret = _normal_output_record_id (state_data, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto out;

      if ((ret = _normal_output_not_available_date (state_data)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if ((ret = _normal_output_not_available_time (state_data)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if ((ret = _normal_output_not_available_sensor_name (state_data)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;

      if (!state_data->prog_data->args->no_sensor_type_output)
	{
	  if ((ret = _normal_output_not_available_sensor_type (state_data)) < 0)
	    goto cleanup;

	  if (!ret)
	    goto newline_out;
	}

      if (state_data->prog_data->args->output_event_state)
        {
          if ((ret = _normal_output_event_state (state_data, flags)) < 0)
            goto cleanup;

          if (!ret)
            goto newline_out;
        }

      if (state_data->prog_data->args->verbose_count >= 1)
        {
          if ((ret = _normal_output_not_available_event_direction (state_data)) < 0)
            goto cleanup;

          if (!ret)
            goto newline_out;
        }

      if ((ret = _normal_output_oem_data (state_data, 0, flags)) < 0)
        goto cleanup;

      if (!ret)
        goto newline_out;
    }
  else
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "Unknown SEL Record Type: %Xh\n",
                       record_type);
      goto out;
    }

 newline_out:
  pstdout_printf (state_data->pstate, "\n");
 out:
  rv = 0;
 cleanup:
  return (rv);
}

static int
_sel_parse_callback (ipmi_sel_parse_ctx_t ctx, void *callback_data)
{
  ipmi_sel_state_data_t *state_data;
  uint8_t record_type;
  int record_type_class;
  int rv = -1;

  assert (ctx);
  assert (callback_data);

  state_data = (ipmi_sel_state_data_t *)callback_data;

  if (state_data->prog_data->args->exclude_display
      || state_data->prog_data->args->exclude_display_range)
    {
      uint16_t record_id;

      if (ipmi_sel_parse_read_record_id (state_data->sel_parse_ctx,
                                         &record_id) < 0)
        {
          if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_id") < 0)
            goto cleanup;
          goto out;
        }

      if (state_data->prog_data->args->exclude_display)
        {
          unsigned int i;

          /* achu: I know it's slow, shouldn't be that big of a deal in the grand scheme */
          for (i = 0; i < state_data->prog_data->args->exclude_display_record_list_length; i++)
            {
              if (state_data->prog_data->args->exclude_display_record_list[i] == record_id)
                goto out;
            }
        }
      else
        {
          if (record_id >= state_data->prog_data->args->exclude_display_range1
              && record_id <= state_data->prog_data->args->exclude_display_range2)
            goto out;
        }
    }

  if (state_data->prog_data->args->sensor_types_length
      || state_data->prog_data->args->exclude_sensor_types_length)
    {
      uint8_t sensor_type;
      int flag;

      if (ipmi_sel_parse_read_sensor_type (state_data->sel_parse_ctx,
                                           &sensor_type) < 0)
        {
          if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_type") < 0)
            goto cleanup;
          goto out;
        }

      if (state_data->prog_data->args->sensor_types_length)
        {
          if ((flag = sensor_type_listed (state_data->pstate,
                                          sensor_type,
                                          state_data->prog_data->args->sensor_types,
                                          state_data->prog_data->args->sensor_types_length)) < 0)
            goto cleanup;
          
          if (!flag)
            goto out;
        }

      if (state_data->prog_data->args->exclude_sensor_types_length)
        {
          if ((flag = sensor_type_listed (state_data->pstate,
                                          sensor_type,
                                          state_data->prog_data->args->exclude_sensor_types,
                                          state_data->prog_data->args->exclude_sensor_types_length)) < 0)
            goto cleanup;

          if (flag)
            goto out;
        }
    }

  if (ipmi_sel_parse_read_record_type (state_data->sel_parse_ctx,
                                       &record_type) < 0)
    {
      if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_record_type") < 0)
        goto cleanup;
      goto out;
    }

  /* IPMI Workaround
   *
   * HP DL 380 G5
   * Intel S2600JF/Appro 512X
   *
   * Motherboard is reporting invalid SEL Records types (0x00 on HP DL
   * 380 G5, 0x03 on Intel S2600JF/Appro 512X)
   */
  if (state_data->prog_data->args->assume_system_event_records
      && (!IPMI_SEL_RECORD_TYPE_VALID (record_type)))
    record_type = IPMI_SEL_RECORD_TYPE_SYSTEM_EVENT_RECORD;

  record_type_class = ipmi_sel_record_type_class (record_type);

  if (state_data->prog_data->args->system_event_only
      && record_type_class != IPMI_SEL_RECORD_TYPE_CLASS_SYSTEM_EVENT_RECORD)
    goto out;

  if (state_data->prog_data->args->oem_event_only
      && record_type_class != IPMI_SEL_RECORD_TYPE_CLASS_TIMESTAMPED_OEM_RECORD
      && record_type_class != IPMI_SEL_RECORD_TYPE_CLASS_NON_TIMESTAMPED_OEM_RECORD)
    goto out;

  if ((state_data->prog_data->args->date_range
       || state_data->prog_data->args->exclude_date_range)
      && (record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_SYSTEM_EVENT_RECORD
          || record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_TIMESTAMPED_OEM_RECORD))
    {
      uint32_t timestamp;
      
      if (ipmi_sel_parse_read_timestamp (state_data->sel_parse_ctx,
                                         &timestamp) < 0)
        {
          if (_sel_parse_err_handle (state_data, "ipmi_sel_parse_read_timestamp") < 0)
            goto cleanup;
          goto out;
        }
      
      if (state_data->prog_data->args->date_range)
        {
          if (timestamp < state_data->prog_data->args->date_range1
              || timestamp > state_data->prog_data->args->date_range2)
            goto out;
        }

      if (state_data->prog_data->args->exclude_date_range)
        {
          if (timestamp >= state_data->prog_data->args->exclude_date_range1
              && timestamp <= state_data->prog_data->args->exclude_date_range2)
            goto out;
        }
    }

  /* Special case, if there is no timestamp, filter it out */
  if (state_data->prog_data->args->date_range
      && record_type_class == IPMI_SEL_RECORD_TYPE_CLASS_NON_TIMESTAMPED_OEM_RECORD)
    {
      if (state_data->prog_data->args->date_range)
        goto out;
    }
  

  if (state_data->prog_data->args->hex_dump)
    {
      if (_hex_output (state_data) < 0)
        goto cleanup;
    }
  else
    {
      if (state_data->prog_data->args->legacy_output)
        {
          if (_legacy_normal_output (state_data, record_type) < 0)
            goto cleanup;
        }
      else
        {
          if (_normal_output (state_data, record_type) < 0)
            goto cleanup;
        }
    }

 out:
  rv = 0;
 cleanup:
  return (rv);
}

static int
_sel_record_id_callback (ipmi_sel_parse_ctx_t ctx, void *callback_data)
{
  ipmi_sel_state_data_t *state_data;
  char outbuf[EVENT_OUTPUT_BUFLEN+1];
  int outbuf_len;
  int rv = -1;

  assert (ctx);
  assert (callback_data);

  state_data = (ipmi_sel_state_data_t *)callback_data;

  /* won't bother with exclude or record-type only options */

  memset (outbuf, '\0', EVENT_OUTPUT_BUFLEN+1);
  if ((outbuf_len = ipmi_sel_parse_read_record_string (state_data->sel_parse_ctx,
                                                       "%i",
                                                       outbuf,
                                                       EVENT_OUTPUT_BUFLEN,
                                                       0)) < 0)
    {
      if (ipmi_sel_parse_ctx_errnum (state_data->sel_parse_ctx) == IPMI_SEL_PARSE_ERR_INVALID_SEL_ENTRY)
        goto out;
      goto cleanup;
    }

  if (outbuf_len > state_data->column_width.record_id)
    state_data->column_width.record_id = outbuf_len;

 out:
  rv = 0;
 cleanup:
  return (rv);
}

static int
_sel_record_id_first_callback (ipmi_sel_parse_ctx_t ctx, void *callback_data)
{
  ipmi_sel_state_data_t *state_data;
  int rv = -1;

  assert (ctx);
  assert (callback_data);

  state_data = (ipmi_sel_state_data_t *)callback_data;

  if (ipmi_sel_parse_read_record_id (ctx, &state_data->first_record_id) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sel_parse_read_record_id: %s\n",
                       ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
      goto cleanup;
    }
  
  rv = 0;
 cleanup:
  return (rv);
}

static int
_sel_record_id_last_callback (ipmi_sel_parse_ctx_t ctx, void *callback_data)
{
  ipmi_sel_state_data_t *state_data;
  int rv = -1;

  assert (ctx);
  assert (callback_data);

  state_data = (ipmi_sel_state_data_t *)callback_data;

  if (ipmi_sel_parse_read_record_id (ctx, &state_data->last_record_id) < 0)
    {
      pstdout_fprintf (state_data->pstate,
                       stderr,
                       "ipmi_sel_parse_read_record_id: %s\n",
                       ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
      goto cleanup;
    }
  
  rv = 0;
 cleanup:
  return (rv);
}

static int
_display_sel_records (ipmi_sel_state_data_t *state_data)
{
  struct ipmi_sel_arguments *args;
  fiid_obj_t obj_cmd_rs = NULL;
  int rv = -1;
  uint64_t val;

  assert (state_data);

  args = state_data->prog_data->args;

  if (!args->sdr.ignore_sdr_cache)
    {
      if (sdr_cache_create_and_load (state_data->sdr_cache_ctx,
                                     state_data->pstate,
                                     state_data->ipmi_ctx,
                                     args->sdr.quiet_cache,
                                     args->sdr.sdr_cache_recreate,
                                     state_data->hostname,
                                     args->sdr.sdr_cache_directory,
                                     args->sdr.sdr_cache_file) < 0)
        goto cleanup;
    }

  if (!args->legacy_output)
    {
      if (ipmi_sel_parse_ctx_set_separator (state_data->sel_parse_ctx, EVENT_OUTPUT_SEPARATOR) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }

    }

  if (args->post_clear)
    {
      if (ipmi_sel_parse_ctx_register_reservation_id (state_data->sel_parse_ctx, NULL) < 0)
	{
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse_ctx_register_reservation_id: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
	  goto cleanup;
	}
    }

  if (!args->legacy_output)
    {
      if (!args->sdr.ignore_sdr_cache)
        {
          struct sensor_entity_id_counts *entity_ptr = NULL;
          
          if (args->entity_sensor_names)
            {
              if (calculate_entity_id_counts (state_data->pstate,
                                              state_data->sdr_cache_ctx,
                                              state_data->sdr_parse_ctx,
                                              &(state_data->entity_id_counts)) < 0)
                goto cleanup;
              
              entity_ptr = &(state_data->entity_id_counts);
            }
          
          if (calculate_column_widths (state_data->pstate,
                                       state_data->sdr_cache_ctx,
                                       state_data->sdr_parse_ctx,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       state_data->prog_data->args->non_abbreviated_units,
                                       (entity_ptr) ? 1 : 0, /* shared_sensors */
                                       1, /* count_event_only_records */
                                       0, /* count_device_locator_records */
                                       0, /* count_oem_records */
                                       entity_ptr,
                                       &(state_data->column_width)) < 0)
            goto cleanup;
          
          /* Unlike sensors output, SEL entries are not predictable,
           * events can happen w/ sensor numbers and sensor types that are
           * not listed in the SDR.  So I can't perfectly predict the
           * largest column size (w/o going through the SEL atleast once).
           *
           * Ultimately, there is some balance that must be done to:
           *
           * A) make sure the output looks good
           *
           * B) not have a ridiculously sized sensor type column that
           * makes the output look bad.
           *
           * The following is the fudging I have elected to do
           */
          
          /* Fudging #1 - "System Firmware Progress" is a relatively
           * common sensor event that isn't mentioned in the SDR.
           *
           * However, it's a pretty big string and can lead to a big
           * column size.  So I will only assume it can happen if there
           * are sensor types in the SDR that are atleast 2 chars less
           * than this string.
           *
           * I think this is a pretty good guess.  "System Firmware
           * Progress" is a sensor that seems to exist more on the
           * major tier 1 vendor motherboards that also include
           * additional "fancy" sensors such as "Event Logging
           * Disabled" (2 shorter in string length) or "System ACPI
           * Power State" (1 shorter in string length).
           *
           * The non-tier 1 vendors tend not to include such fancy
           * sensors on their motherboards, limiting themselves to the
           * standard temp, voltage, fan, etc. so the probability of
           * hitting "System Firmware Progress" is lower.
           */
          if (state_data->column_width.sensor_type >= (strlen (ipmi_sensor_types[IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS]) - 2))
            {
              if (state_data->column_width.sensor_type < strlen (ipmi_sensor_types[IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS]))
                state_data->column_width.sensor_type = strlen (ipmi_sensor_types[IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS]);
            }
        }
      else
        {
	  if (calculate_column_widths_ignored_sdr_cache (state_data->prog_data->args->non_abbreviated_units,
							 &(state_data->column_width)) < 0)
	    goto cleanup;
        }

      /* Record IDs for SEL entries are calculated a bit differently */
      
      if (state_data->prog_data->args->display)
        {
          uint16_t max_record_id = 0;
          int i;
          
          for (i = 0; i < state_data->prog_data->args->display_record_list_length; i++)
            {
              if (state_data->prog_data->args->display_record_list[i] > max_record_id)
                max_record_id = state_data->prog_data->args->display_record_list[i];
            }

          if (ipmi_sel_parse_record_ids (state_data->sel_parse_ctx,
                                         &max_record_id,
                                         1,
                                         _sel_record_id_callback,
                                         state_data) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sel_parse_record_ids: %s\n",
                               ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
              goto cleanup;
            }
        }
      else if (state_data->prog_data->args->display_range)
        {
          /* assume biggest record is is the last specified */
          if (ipmi_sel_parse (state_data->sel_parse_ctx,
                              state_data->prog_data->args->display_range2,
                              state_data->prog_data->args->display_range2,
                              _sel_record_id_callback,
                              state_data) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sel_parse: %s\n",
                               ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
              goto cleanup;
            }
        }
      else
        {
          /* assume biggest record id is the last one */
          if (ipmi_sel_parse (state_data->sel_parse_ctx,
                              IPMI_SEL_RECORD_ID_LAST,
                              IPMI_SEL_RECORD_ID_LAST,
                              _sel_record_id_callback,
                              state_data) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sel_parse: %s\n",
                               ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
              goto cleanup;
            }
        }
    }

  if (args->interpret_oem_data || args->output_oem_event_strings)
    {
      if (ipmi_get_oem_data (state_data->pstate,
                             state_data->ipmi_ctx,
                             &state_data->oem_data) < 0)
        goto cleanup;

      if (ipmi_sel_parse_ctx_set_manufacturer_id (state_data->sel_parse_ctx,
                                                  state_data->oem_data.manufacturer_id) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse_ctx_set_manufacturer_id: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }
      
      if (ipmi_sel_parse_ctx_set_product_id (state_data->sel_parse_ctx,
                                             state_data->oem_data.product_id) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse_ctx_set_product_id: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }

      if (ipmi_sel_parse_ctx_set_ipmi_version (state_data->sel_parse_ctx,
                                               state_data->oem_data.ipmi_version_major,
                                               state_data->oem_data.ipmi_version_minor) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse_ctx_set_ipmi_version: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }

      if (args->output_event_state)
        {
          if (ipmi_interpret_ctx_set_manufacturer_id (state_data->interpret_ctx,
                                                      state_data->oem_data.manufacturer_id) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_interpret_ctx_set_manufacturer_id: %s\n",
                               ipmi_interpret_ctx_errormsg (state_data->interpret_ctx));
              goto cleanup;
            }

          if (ipmi_interpret_ctx_set_product_id (state_data->interpret_ctx,
                                                 state_data->oem_data.product_id) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_interpret_ctx_set_product_id: %s\n",
                               ipmi_interpret_ctx_errormsg (state_data->interpret_ctx));
              goto cleanup;
            }
        }
    }

  if (state_data->prog_data->args->display)
    {
      if (ipmi_sel_parse_record_ids (state_data->sel_parse_ctx,
                                     state_data->prog_data->args->display_record_list,
                                     state_data->prog_data->args->display_record_list_length,
                                     _sel_parse_callback,
                                     state_data) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse_record_ids: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }
    }
  else if (state_data->prog_data->args->display_range)
    {
      if (ipmi_sel_parse (state_data->sel_parse_ctx,
                          state_data->prog_data->args->display_range1,
                          state_data->prog_data->args->display_range2,
                          _sel_parse_callback,
                          state_data) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }
    }
  else if (state_data->prog_data->args->tail)
    {
      uint16_t entries;
      uint16_t range_begin;

      if (!(obj_cmd_rs = fiid_obj_create (tmpl_cmd_get_sel_info_rs)))
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_create: %s\n",
                           strerror (errno));
          goto cleanup;
        }

      if (ipmi_cmd_get_sel_info (state_data->ipmi_ctx, obj_cmd_rs) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_cmd_get_sel_info: %s\n",
                           ipmi_ctx_errormsg (state_data->ipmi_ctx));
          goto cleanup;
        }

      if (FIID_OBJ_GET (obj_cmd_rs, "entries", &val) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "fiid_obj_get: 'entries': %s\n",
                           fiid_obj_errormsg (obj_cmd_rs));
          goto cleanup;
        }
      entries = val;
      
      /* Special case, display all records */
      if (entries <= state_data->prog_data->args->tail_count)
        {
          if (ipmi_sel_parse (state_data->sel_parse_ctx,
                              IPMI_SEL_RECORD_ID_FIRST,
                              IPMI_SEL_RECORD_ID_LAST,
                              _sel_parse_callback,
                              state_data) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sel_parse: %s\n",
                               ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
              goto cleanup;
            }
          goto out;
        }

      if (ipmi_sel_parse (state_data->sel_parse_ctx,
                          IPMI_SEL_RECORD_ID_FIRST,
                          IPMI_SEL_RECORD_ID_FIRST,
                          _sel_record_id_first_callback,
                          state_data) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }

      if (ipmi_sel_parse (state_data->sel_parse_ctx,
                          IPMI_SEL_RECORD_ID_LAST,
                          IPMI_SEL_RECORD_ID_LAST,
                          _sel_record_id_last_callback,
                          state_data) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }

      /* Assume entries distributed evenly throughout SEL */

      if ((state_data->last_record_id - state_data->first_record_id + 1) < entries)
        {
          /* Assume SEL record IDs separated by 1 */
          
          if ((state_data->prog_data->args->tail_count + 1) >= state_data->last_record_id)
            range_begin = IPMI_SEL_RECORD_ID_FIRST;
          else
            range_begin = state_data->last_record_id - state_data->prog_data->args->tail_count + 1;
        }
      else
        {
          uint16_t spacing;

          spacing = (state_data->last_record_id - state_data->first_record_id + 1) / entries;

          if ((state_data->prog_data->args->tail_count * spacing + 1) >= state_data->last_record_id)
            range_begin = IPMI_SEL_RECORD_ID_FIRST;
          else
            range_begin = state_data->last_record_id - (state_data->prog_data->args->tail_count * spacing) + 1;
        }
        
      if (ipmi_sel_parse (state_data->sel_parse_ctx,
                          range_begin,
                          IPMI_SEL_RECORD_ID_LAST,
                          _sel_parse_callback,
                          state_data) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }

      fiid_obj_destroy (obj_cmd_rs);
      obj_cmd_rs = NULL;
    }
  else
    {
      if (ipmi_sel_parse (state_data->sel_parse_ctx,
                          IPMI_SEL_RECORD_ID_FIRST,
                          IPMI_SEL_RECORD_ID_LAST,
                          _sel_parse_callback,
                          state_data) < 0)
        {
          pstdout_fprintf (state_data->pstate,
                           stderr,
                           "ipmi_sel_parse: %s\n",
                           ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
          goto cleanup;
        }
    }

  if (args->post_clear)
    {
      if (ipmi_sel_parse_clear_sel (state_data->sel_parse_ctx) < 0)
	{
	  pstdout_fprintf (state_data->pstate,
			   stderr,
			   "ipmi_sel_parse_clear_sel: %s\n",
			   ipmi_sel_parse_ctx_errormsg (state_data->sel_parse_ctx));
	  goto cleanup;
	}
    }

 out:
  rv = 0;
 cleanup:
  fiid_obj_destroy (obj_cmd_rs);
  return (rv);
}

static int
run_cmd_args (ipmi_sel_state_data_t *state_data)
{
  struct ipmi_sel_arguments *args;

  assert (state_data);

  args = state_data->prog_data->args;

  if (args->info)
    return (_display_sel_info (state_data));

  if (args->sdr.flush_cache)
    return (_flush_cache (state_data));

  if (args->list_sensor_types)
    return (_list_sensor_types (state_data));

  if (args->clear)
    return (_clear_entries (state_data));

  if (args->delete)
    return (_delete_records (state_data));

  if (args->delete_range)
    return (_delete_range (state_data));

  /* else default to displaying records */

  if (_display_sel_records (state_data) < 0)
    return (-1);

  return (0);
}

static int
_ipmi_sel (pstdout_state_t pstate,
           const char *hostname,
           void *arg)
{
  ipmi_sel_state_data_t state_data;
  ipmi_sel_prog_data_t *prog_data;
  char errmsg[IPMI_OPEN_ERRMSGLEN];
  int exit_code = -1;

  assert (pstate);
  assert (arg);

  prog_data = (ipmi_sel_prog_data_t *)arg;
  memset (&state_data, '\0', sizeof (ipmi_sel_state_data_t));

  state_data.prog_data = prog_data;
  state_data.pstate = pstate;
  state_data.hostname = (char *)hostname;

  /* Special case, just flush, don't do an IPMI connection */
  if (!prog_data->args->sdr.flush_cache)
    {
      if (!(state_data.ipmi_ctx = ipmi_open (prog_data->progname,
                                             hostname,
                                             &(prog_data->args->common),
                                             errmsg,
                                             IPMI_OPEN_ERRMSGLEN)))
        {
          pstdout_fprintf (pstate,
                           stderr,
                           "%s\n",
                           errmsg);
          exit_code = EXIT_FAILURE;
          goto cleanup;
        }
    }

  if (!(state_data.sdr_cache_ctx = ipmi_sdr_cache_ctx_create ()))
    {
      pstdout_perror (pstate, "ipmi_sdr_cache_ctx_create()");
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  if (state_data.prog_data->args->common.debug)
    {
      /* Don't error out, if this fails we can still continue */
      if (ipmi_sdr_cache_ctx_set_flags (state_data.sdr_cache_ctx,
                                        IPMI_SDR_CACHE_FLAGS_DEBUG_DUMP) < 0)
        pstdout_fprintf (pstate,
                         stderr,
                         "ipmi_sdr_cache_ctx_set_flags: %s\n",
                         ipmi_sdr_cache_ctx_errormsg (state_data.sdr_cache_ctx));

      if (hostname)
        {
          if (ipmi_sdr_cache_ctx_set_debug_prefix (state_data.sdr_cache_ctx,
                                                   hostname) < 0)
            pstdout_fprintf (pstate,
                             stderr,
                             "ipmi_sdr_cache_ctx_set_debug_prefix: %s\n",
                             ipmi_sdr_cache_ctx_errormsg (state_data.sdr_cache_ctx));
        }
    }

  if (!(state_data.sdr_parse_ctx = ipmi_sdr_parse_ctx_create ()))
    {
      pstdout_perror (pstate, "ipmi_sdr_parse_ctx_create()");
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  /* Special case, just flush, don't do SEL stuff */
  if (!prog_data->args->sdr.flush_cache)
    {
      unsigned int flags = 0;

      if (!(state_data.sel_parse_ctx = ipmi_sel_parse_ctx_create (state_data.ipmi_ctx,
                                                                  prog_data->args->sdr.ignore_sdr_cache ? NULL : state_data.sdr_cache_ctx)))
        {
          pstdout_perror (pstate, "ipmi_sel_parse_ctx_create()");
          goto cleanup;
        }

      if (state_data.prog_data->args->common.debug)
	flags |= IPMI_SEL_PARSE_FLAGS_DEBUG_DUMP;

      if (state_data.prog_data->args->assume_system_event_records)
	flags |= IPMI_SEL_PARSE_FLAGS_ASSUME_SYTEM_EVENT_RECORDS;

      if (flags)
        {
          /* Don't error out, if this fails we can still continue */
          if (ipmi_sel_parse_ctx_set_flags (state_data.sel_parse_ctx, flags) < 0)
            pstdout_fprintf (pstate,
                             stderr,
                             "ipmi_sel_parse_ctx_set_flags: %s\n",
                             ipmi_sel_parse_ctx_errormsg (state_data.sel_parse_ctx));
	}
      
      if (state_data.prog_data->args->common.debug && hostname)
	{
	  if (ipmi_sel_parse_ctx_set_debug_prefix (state_data.sel_parse_ctx,
						   hostname) < 0)
	    pstdout_fprintf (pstate,
			     stderr,
			     "ipmi_sel_parse_ctx_set_debug_prefix: %s\n",
			     ipmi_sel_parse_ctx_errormsg (state_data.sel_parse_ctx));
        }
    }

  if (prog_data->args->output_event_state)
    {
      unsigned int flags = 0;

      if (!(state_data.interpret_ctx = ipmi_interpret_ctx_create ()))
        {
          pstdout_perror (pstate, "ipmi_interpret_ctx_create()");
          exit_code = EXIT_FAILURE;
          goto cleanup;
        }

      if (prog_data->args->event_state_config_file)
        {
          if (ipmi_interpret_load_sel_config (state_data.interpret_ctx,
                                              prog_data->args->event_state_config_file) < 0)
            {
              if (ipmi_interpret_ctx_errnum (state_data.interpret_ctx) == IPMI_INTERPRET_ERR_SEL_CONFIG_FILE_DOES_NOT_EXIST)
                pstdout_fprintf (pstate,
                                 stderr,
                                 "event state config file '%s' does not exist\n",
                                 prog_data->args->event_state_config_file);
              else if (ipmi_interpret_ctx_errnum (state_data.interpret_ctx) == IPMI_INTERPRET_ERR_SEL_CONFIG_FILE_PARSE)
                pstdout_fprintf (pstate,
                                 stderr,
                                 "event state config file '%s' parse error\n",
                                 prog_data->args->event_state_config_file);
              else
                pstdout_fprintf (pstate,
                                 stderr,
                                 "ipmi_interpret_load_sel_config: %s\n",
                                 ipmi_interpret_ctx_errormsg (state_data.interpret_ctx));
              exit_code = EXIT_FAILURE;
              goto cleanup;
            }
        }
      else
        {
          if (ipmi_interpret_load_sel_config (state_data.interpret_ctx, NULL) < 0)
            {
              if (ipmi_interpret_ctx_errnum (state_data.interpret_ctx) == IPMI_INTERPRET_ERR_SEL_CONFIG_FILE_PARSE)
                pstdout_fprintf (pstate,
                                 stderr,
                                 "event state config file parse error\n");
              else
                pstdout_fprintf (pstate,
                                 stderr,
                                 "ipmi_interpret_load_sel_config: %s\n",
                                 ipmi_interpret_ctx_errormsg (state_data.interpret_ctx));
              exit_code = EXIT_FAILURE;
              goto cleanup;
            }
        }

      if (prog_data->args->assume_system_event_records)
        flags |= IPMI_INTERPRET_FLAGS_SEL_ASSUME_SYSTEM_EVENT_RECORDS;

      if (prog_data->args->interpret_oem_data)
        flags |= IPMI_INTERPRET_FLAGS_INTERPRET_OEM_DATA;

      if (flags)
        {
          if (ipmi_interpret_ctx_set_flags (state_data.interpret_ctx, flags) < 0)
            {
              pstdout_fprintf (pstate,
                               stderr,
                               "ipmi_interpret_ctx_set_flags: %s\n",
                               ipmi_interpret_ctx_errormsg (state_data.interpret_ctx));
              exit_code = EXIT_FAILURE;
              goto cleanup;
            }
        }
    }

  if (run_cmd_args (&state_data) < 0)
    {
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  exit_code = 0;
 cleanup:
  ipmi_sdr_cache_ctx_destroy (state_data.sdr_cache_ctx);
  ipmi_sdr_parse_ctx_destroy (state_data.sdr_parse_ctx);
  ipmi_sel_parse_ctx_destroy (state_data.sel_parse_ctx);
  ipmi_ctx_close (state_data.ipmi_ctx);
  ipmi_ctx_destroy (state_data.ipmi_ctx);
  return (exit_code);
}

int
main (int argc, char **argv)
{
  ipmi_sel_prog_data_t prog_data;
  struct ipmi_sel_arguments cmd_args;
  int exit_code;
  int hosts_count;
  int rv;

  ipmi_disable_coredump ();

  memset (&prog_data, '\0', sizeof (ipmi_sel_prog_data_t));
  prog_data.progname = argv[0];
  ipmi_sel_argp_parse (argc, argv, &cmd_args);
  prog_data.args = &cmd_args;

  /* Special case, if user specified workaround via flags instead of option */
  if (prog_data.args->common.section_specific_workaround_flags & IPMI_PARSE_SECTION_SPECIFIC_WORKAROUND_FLAGS_ASSUME_SYSTEM_EVENT)
    prog_data.args->assume_system_event_records = 1;
  
  if ((hosts_count = pstdout_setup (&(prog_data.args->common.hostname),
                                    prog_data.args->hostrange.buffer_output,
                                    prog_data.args->hostrange.consolidate_output,
                                    prog_data.args->hostrange.fanout,
                                    prog_data.args->hostrange.eliminate,
                                    prog_data.args->hostrange.always_prefix)) < 0)
    {
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  if (!hosts_count)
    {
      exit_code = EXIT_SUCCESS;
      goto cleanup;
    }

  /* We don't want caching info to output when are doing ranged output */
  if (hosts_count > 1)
    prog_data.args->sdr.quiet_cache = 1;

  if ((rv = pstdout_launch (prog_data.args->common.hostname,
                            _ipmi_sel,
                            &prog_data)) < 0)
    {
      fprintf (stderr,
               "pstdout_launch: %s\n",
               pstdout_strerror (pstdout_errnum));
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  exit_code = rv;
 cleanup:
  return (exit_code);
}