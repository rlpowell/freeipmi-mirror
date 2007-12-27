/*
  Copyright (C) 2006 FreeIPMI Core Team

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
#endif

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
#include <argp.h>
#include <assert.h>

#include "tool-common.h"
#include "tool-cmdline-common.h"
#include "tool-fiid-wrappers.h"
#include "tool-sensor-common.h"
#include "tool-sdr-cache-common.h"
#include "ipmi-sensors.h"
#include "ipmi-sensors-argp.h"
#include "ipmi-sensors-simple-display.h"
#include "ipmi-sensors-verbose-display.h"
#include "ipmi-sensors-very-verbose-display.h"

#include "pstdout.h"
#include "hostrange.h"

#include "freeipmi-portability.h"

static int 
_sdr_repository_info (ipmi_sensors_state_data_t *state_data)
{
  fiid_obj_t obj_cmd_rs = NULL;
  uint64_t val, val1, val2;
  char str[512];
  time_t t;
  struct tm tmp;
  int rv = -1;

  assert(state_data);
  
  _FIID_OBJ_CREATE(obj_cmd_rs, tmpl_cmd_get_sdr_repository_info_rs);
  
  if (ipmi_cmd_get_sdr_repository_info (state_data->ipmi_ctx, obj_cmd_rs) != 0)
    {
      pstdout_fprintf(state_data->pstate,
                      stderr,
                      "ipmi_cmd_get_sdr_repository_info: %s\n",
                      ipmi_ctx_strerror(ipmi_ctx_errnum(state_data->ipmi_ctx)));
      goto cleanup;
    }
 
  _FIID_OBJ_GET (obj_cmd_rs, "sdr_version_major", &val1);
  _FIID_OBJ_GET (obj_cmd_rs, "sdr_version_minor", &val2);
  pstdout_printf (state_data->pstate, 
                  "SDR version:                     %u.%u\n", 
                  val1, 
                  val2);

  _FIID_OBJ_GET (obj_cmd_rs, "record_count", &val);
  pstdout_printf (state_data->pstate, 
                  "SDR record count:                %u\n", 
                  val);

  _FIID_OBJ_GET (obj_cmd_rs, "free_space", &val); 
  pstdout_printf (state_data->pstate, 
                  "Free space remaining:            %u bytes\n", 
                  val);

  _FIID_OBJ_GET (obj_cmd_rs, "most_recent_addition_timestamp", &val);
  t = val;
  localtime_r (&t, &tmp);
  strftime (str, sizeof (str), "%m/%d/%Y - %H:%M:%S", &tmp);
  pstdout_printf (state_data->pstate, 
                  "Most recent addition timestamp:  %s\n", 
                  str);
  
  _FIID_OBJ_GET (obj_cmd_rs, "most_recent_erase_timestamp", &val);
  t = val;
  localtime_r (&t, &tmp);
  strftime (str, sizeof (str), "%m/%d/%Y - %H:%M:%S", &tmp);
  pstdout_printf (state_data->pstate, 
                  "Most recent erase timestamp:     %s\n", 
                  str);
  
  _FIID_OBJ_GET (obj_cmd_rs, "get_sdr_repository_allocation_info_command_supported", &val);
  pstdout_printf (state_data->pstate, 
                  "Get SDR Repository Allocation Information Command supported:         %s\n", 
                  (val ? "Yes" : "No"));

  _FIID_OBJ_GET (obj_cmd_rs, "reserve_sdr_repository_command_supported", &val);
  pstdout_printf (state_data->pstate, 
                  "Reserve SDR Repository Command supported:                            %s\n", 
                  (val ? "Yes" : "No"));

  _FIID_OBJ_GET (obj_cmd_rs, "partial_add_sdr_command_supported", &val);
  pstdout_printf (state_data->pstate, 
                  "Partial Add SDR Command supported:                                   %s\n", 
                  (val ? "Yes" : "No"));

  _FIID_OBJ_GET (obj_cmd_rs, "delete_sdr_command_supported", &val);
  pstdout_printf (state_data->pstate, 
                  "Delete SDR Command supported:                                        %s\n", 
                  (val ? "Yes" : "No"));

  _FIID_OBJ_GET (obj_cmd_rs, "modal_non_modal_sdr_repository_update_operation_supported", &val);
  pstdout_printf (state_data->pstate, 
                  "Modal/non-modal SDR Repository Update operation supported:           ");
  switch (val)
    {
    case IPMI_SDR_MODAL_NON_MODAL_REPOSITORY_UPDATE_OP_UNSPECIFIED:
      pstdout_printf (state_data->pstate, "Unspecified\n");
      break;
    case IPMI_SDR_NON_MODAL_REPOSITORY_UPDATE_OP_SUPPORTED:
      pstdout_printf (state_data->pstate, "Non-Modal\n");
      break;
    case IPMI_SDR_MODAL_REPOSITORY_UPDATE_OP_SUPPORTED:
      pstdout_printf (state_data->pstate, "Modal\n");
      break;
    case IPMI_SDR_MODAL_NON_MODAL_REPOSITORY_UPDATE_OP_SUPPORTED:
      pstdout_printf (state_data->pstate, "Both\n");
      break;
    default:
      pstdout_printf (state_data->pstate, "Unknown\n");
    }

  _FIID_OBJ_GET (obj_cmd_rs, "overflow_flag", &val);
  pstdout_printf (state_data->pstate, 
                  "SDR could not be written due to lack of space in the SDR Repository: %s\n", 
                  (val ? "Yes" : "No"));
  
  rv = 0;
cleanup:
  _FIID_OBJ_DESTROY(obj_cmd_rs);
  return (rv);
}

static int
_flush_cache (ipmi_sensors_state_data_t *state_data)
{
  assert(state_data);

  if (sdr_cache_flush_cache(state_data->ipmi_sdr_cache_ctx,
                            state_data->pstate,
                            state_data->hostname,
                            state_data->prog_data->args->sdr.sdr_cache_dir_wanted ? state_data->prog_data->args->sdr.sdr_cache_dir : NULL) < 0)
    return -1;
  
  return 0;
}

static void
_str_replace_chr (char *str, char chr, char with)
{
  char *p = NULL;
  char *s = NULL;

  assert(str);

  for (s = str;
       (p = strchr (s, chr));
       s = p + 1)
    *p = with;
}

static int 
_display_group_list (ipmi_sensors_state_data_t *state_data)
{
  int i = 0;
  char *group = NULL;
  
  assert(state_data);

  for (i = 0; ipmi_sensor_types[i]; i++)
    {
      if (!(group = strdupa (ipmi_sensor_types[i])))
        {
          pstdout_fprintf (state_data->pstate, 
                           stderr, 
                           "strdupa: %s\n", 
                           strerror(errno));
          return (-1);
        }
      _str_replace_char (group, ' ', '_');
      pstdout_printf (state_data->pstate, "%s\n", group);
    }
  if (!(group = strdupa (ipmi_oem_sensor_type)))
    {
      pstdout_fprintf (state_data->pstate, 
                       stderr, 
                       "strdupa: %s\n", 
                       strerror(errno));
      return (-1);
    }
  _str_replace_char (group, ' ', '_');
  pstdout_printf (state_data->pstate, "%s\n", group);
  
  return 0;
}

static int
_sensors_group_cmp (ipmi_sensors_state_data_t *state_data,
                    sdr_record_t *sdr_record)
{
  char *sdr_group_name = NULL;
  int i;

  assert(sdr_record);
  assert(state_data->prog_data->args->groups_list_wanted);

  switch (sdr_record->record_type)
    {
    case IPMI_SDR_FORMAT_FULL_RECORD:
      sdr_group_name = (char *) ipmi_get_sensor_group (sdr_record->record.sdr_full_record.sensor_type);
      break;
    case IPMI_SDR_FORMAT_COMPACT_RECORD:
      sdr_group_name = (char *) ipmi_get_sensor_group (sdr_record->record.sdr_compact_record.sensor_type);
      break;
    case IPMI_SDR_FORMAT_EVENT_ONLY_RECORD:
      sdr_group_name = (char *) ipmi_get_sensor_group (sdr_record->record.sdr_event_only_record.sensor_type);
      break;
    }
  
  if (sdr_group_name)
    {
      char sdr_group_name_subst[IPMI_SENSORS_MAX_GROUPS_STRING_LENGTH];

      strcpy(sdr_group_name_subst, sdr_group_name);
      _str_replace_char (sdr_group_name_subst, ' ', '_');
      
      for (i = 0; i < state_data->prog_data->args->groups_list_length; i++)
        {
          if ((strcasecmp (sdr_group_name, 
                           state_data->prog_data->args->groups_list[i]) == 0)
              || (strcasecmp (sdr_group_name_subst, 
                              state_data->prog_data->args->groups_list[i]) == 0))
            return 0;
        }
    }

  return (-1);
}

static int 
_display_group_sensors (ipmi_sensors_state_data_t *state_data)
{
  int i;
  sdr_record_t *sdr_record;
  sensor_reading_t _sensor_reading;
  sensor_reading_t *sensor_reading;
  int verbose_count;

  assert(state_data);
  assert(state_data->prog_data->args->groups_list_wanted);

  verbose_count = state_data->prog_data->args->verbose_count;

  for (i = 0; i < state_data->sdr_record_count; i++)
    {
      sdr_record = state_data->sdr_record_list + i;
      
      if (_sensors_group_cmp (state_data, sdr_record) == 0)
	{
          memset (&_sensor_reading, 0, sizeof (sensor_reading_t));
          
          if (get_sensor_reading(state_data->ipmi_ctx,
                                 (state_data->prog_data->args->common.flags & IPMI_FLAGS_DEBUG_DUMP) ? 1 : 0,
                                 sdr_record,
                                 &_sensor_reading) < 0)
            sensor_reading = NULL;
          else
            sensor_reading = &_sensor_reading;

	  switch (verbose_count)
	    {
	    case 0:
	      if (sensors_display_simple (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	      break;
	    case 1:
	      if (sensors_display_verbose (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	      break;
	    case 2:
	    default:
	      if (sensors_display_very_verbose (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	    }

          sensor_reading_cleanup(sensor_reading);
	}
    }
  
  return 0;
}

static int
_sensors_list_cmp (sdr_record_t *sdr_record,
                   unsigned int *sensors_list,
                   unsigned int sensors_list_length)
{
  int i;

  assert(sdr_record);
  assert(sensors_list);

  for (i = 0; i < sensors_list_length; i++)
    {
      if (sdr_record->record_id == sensors_list[i])
        return 0;
    }

  return (-1);
}

int 
_display_sensor_list (ipmi_sensors_state_data_t *state_data)
{
  int i;
  sdr_record_t *sdr_record;
  sensor_reading_t _sensor_reading;
  sensor_reading_t *sensor_reading;
  unsigned int *sensors_list;
  unsigned int sensors_list_length;
  int verbose_count;

  assert(state_data);
  assert(state_data->prog_data->args->sensors_list_wanted);
  assert(state_data->prog_data->args->sensors_list);
  assert(state_data->prog_data->args->sensors_list_length);

  sensors_list = state_data->prog_data->args->sensors_list;
  sensors_list_length = state_data->prog_data->args->sensors_list_length;
  verbose_count = state_data->prog_data->args->verbose_count;

  for (i = 0; i < state_data->sdr_record_count; i++)
    {
      sdr_record = state_data->sdr_record_list + i;
      
      if (_sensors_list_cmp (sdr_record, sensors_list, sensors_list_length) == 0)
	{
          memset (&_sensor_reading, 0, sizeof (sensor_reading_t));

          if (get_sensor_reading(state_data->ipmi_ctx,
                                 (state_data->prog_data->args->common.flags & IPMI_FLAGS_DEBUG_DUMP) ? 1 : 0,
                                 sdr_record,
                                 &_sensor_reading) < 0)
            sensor_reading = NULL;
          else
            sensor_reading = &_sensor_reading;

	  switch (verbose_count)
	    {
	    case 0:
	      if (sensors_display_simple (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	      break;
	    case 1:
	      if (sensors_display_verbose (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	      break;
	    case 2:
	    default:
	      if (sensors_display_very_verbose (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	    }

          sensor_reading_cleanup(sensor_reading);
	}
    }
  
  return 0;
}

static int 
_display_sensors (ipmi_sensors_state_data_t *state_data)
{
  struct ipmi_sensors_arguments *args = NULL;
  
  assert(state_data);

  args = state_data->prog_data->args;

  if (args->groups_list_wanted)
    {
      if (_display_group_sensors (state_data) < 0)
        return (-1);
    }

  if (args->sensors_list_wanted)
    {
      if (_display_sensor_list (state_data) < 0)
        return (-1);
    }
  
  if (!args->groups_list_wanted && !args->sensors_list_wanted)
    {
      int i;
      sdr_record_t *sdr_record;
      sensor_reading_t _sensor_reading;
      sensor_reading_t *sensor_reading;
      
      for (i = 0; i < state_data->sdr_record_count; i++)
	{
	  sdr_record = state_data->sdr_record_list + i;

	  memset (&_sensor_reading, 0, sizeof (sensor_reading_t));

          if (get_sensor_reading(state_data->ipmi_ctx,
                                 (state_data->prog_data->args->common.flags & IPMI_FLAGS_DEBUG_DUMP) ? 1 : 0,
                                 sdr_record,
                                 &_sensor_reading) < 0)
            sensor_reading = NULL;
          else
            sensor_reading = &_sensor_reading;

	  switch (args->verbose_count)
	    {
	    case 0:
	      if (sensors_display_simple (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	      break;
	    case 1:
	      if (sensors_display_verbose (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	      break;
	    case 2:
	    default:
	      if (sensors_display_very_verbose (state_data, sdr_record, sensor_reading) < 0)
                {
                  sensor_reading_cleanup(sensor_reading);
                  return (-1);
                }
	    }
          
          sensor_reading_cleanup(sensor_reading);
	}
    }
  
  return 0;
}

int 
run_cmd_args (ipmi_sensors_state_data_t *state_data)
{
  struct ipmi_sensors_arguments *args;

  assert(state_data);

  args = state_data->prog_data->args;
  
  if (args->sdr_info_wanted)
    return _sdr_repository_info (state_data);
  
  if (args->sdr.flush_cache_wanted)
    return _flush_cache (state_data);
  
  if (args->list_groups_wanted)
    return _display_group_list (state_data);
  
  if (sdr_cache_create_and_load (state_data->ipmi_sdr_cache_ctx,
                                 state_data->pstate,
                                 state_data->ipmi_ctx,
                                 args->sdr.quiet_cache_wanted,
                                 state_data->hostname,
                                 args->sdr.sdr_cache_dir_wanted ? args->sdr.sdr_cache_dir : NULL) < 0)
    return -1;
  
  if (_display_sensors (state_data) < 0)
    goto cleanup;

  return 0;
}

static int
_ipmi_sensors (pstdout_state_t pstate,
               const char *hostname,
               void *arg)
{
  ipmi_sensors_state_data_t state_data;
  ipmi_sensors_prog_data_t *prog_data;
  char errmsg[IPMI_OPEN_ERRMSGLEN];
  int exit_code = -1;

  prog_data = (ipmi_sensors_prog_data_t *)arg;
  memset(&state_data, '\0', sizeof(ipmi_sensors_state_data_t));
  
  state_data.prog_data = prog_data;
  state_data.pstate = pstate;
  state_data.hostname = (char *)hostname;

  /* Special case, just flush, don't do an IPMI connection */
  /* Special case, just list groups, don't do an IPMI connection */
  if (!prog_data->args->sdr.flush_cache_wanted
      && !prog_data->args->list_groups_wanted)
    {
      if (!(state_data.ipmi_ctx = ipmi_open(prog_data->progname,
                                            hostname,
                                            &(prog_data->args->common),
                                            errmsg,
                                            IPMI_OPEN_ERRMSGLEN)))
        {
          pstdout_fprintf(pstate,
                          stderr,
                          "%s\n",
                          errmsg);
          exit_code = EXIT_FAILURE;
          goto cleanup;
        }
    }      
 
  if (!(state_data.ipmi_sdr_cache_ctx = ipmi_sdr_cache_ctx_create()))
    {
      pstdout_perror (pstate, "ipmi_sdr_cache_ctx_create()");
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  if (run_cmd_args (&state_data) < 0)
    {
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }
  
  exit_code = 0;
 cleanup:
  if (state_data.ipmi_sdr_cache_ctx)
    ipmi_sdr_cache_ctx_destroy(state_data.ipmi_sdr_cache_ctx);
  if (state_data.ipmi_ctx)
    {
      ipmi_ctx_close (state_data.ipmi_ctx);
      ipmi_ctx_destroy (state_data.ipmi_ctx);
    }
  return exit_code;
}

int 
main (int argc, char **argv)
{
  ipmi_sensors_prog_data_t prog_data;
  struct ipmi_sensors_arguments cmd_args;
  int exit_code;
  int hosts_count;
  int rv;
  
  ipmi_disable_coredump();
  
  prog_data.progname = argv[0];
  ipmi_sensors_argp_parse (argc, argv, &cmd_args);
  prog_data.args = &cmd_args;

  if ((hosts_count = pstdout_setup(&(prog_data.args->common.hostname),
                                   prog_data.args->hostrange.buffer_hostrange_output,
                                   prog_data.args->hostrange.consolidate_hostrange_output,
                                   prog_data.args->hostrange.fanout,
                                   prog_data.args->hostrange.eliminate)) < 0)
    {
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  /* We don't want caching info to output when are doing ranged output */
  if (hosts_count > 1)
    prog_data.args->sdr.quiet_cache_wanted = 1;

  if ((rv = pstdout_launch(prog_data.args->common.hostname,
                           _ipmi_sensors,
                           &prog_data)) < 0)
    {
      fprintf(stderr,
              "pstdout_launch: %s\n",
              pstdout_strerror(pstdout_errnum));
      exit_code = EXIT_FAILURE;
      goto cleanup;
    }

  exit_code = rv;
 cleanup:
  return (exit_code);
}
