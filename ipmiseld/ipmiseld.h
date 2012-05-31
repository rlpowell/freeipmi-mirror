/*****************************************************************************\
 *  $Id: ipmiseld.h,v 1.11 2010-02-08 22:02:30 chu11 Exp $
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

#ifndef IPMISELD_H
#define IPMISELD_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <freeipmi/freeipmi.h>

#include "tool-cmdline-common.h"
#include "tool-oem-common.h"
#include "tool-sensor-common.h"

enum ipmiseld_argp_option_keys
  {
    IPMISELD_VERBOSE_KEY = 'v',
    IPMISELD_SENSOR_TYPES_KEY = 't',
    IPMISELD_EXCLUDE_SENSOR_TYPES_KEY = 'T',
    IPMISELD_SYSTEM_EVENT_ONLY_KEY = 160,
    IPMISELD_OEM_EVENT_ONLY_KEY = 161,
    IPMISELD_EVENT_STATE_CONFIG_FILE_KEY = 162,
    IPMISELD_INTERPRET_OEM_DATA_KEY = 163,
    IPMISELD_OUTPUT_OEM_EVENT_STRINGS_KEY = 164,
    IPMISELD_ENTITY_SENSOR_NAMES_KEY = 165,
    IPMISELD_NON_ABBREVIATED_UNITS_KEY = 166,
    IPMISELD_EVENT_STATE_FILTER_KEY = 167,
    IPMISELD_SYSTEM_EVENT_FORMAT_KEY = 168,
    IPMISELD_OEM_TIMESTAMPED_EVENT_FORMAT_KEY = 169,
    IPMISELD_OEM_NON_TIMESTAMPED_EVENT_FORMAT_KEY = 170,
    IPMISELD_LOG_FACILITY_KEY = 171,
    IPMISELD_LOG_PRIORITY_KEY = 172,
    IPMISELD_CACHE_DIRECTORY_KEY = 173,
    IPMISELD_TEST_RUN_KEY = 174,
    IPMISELD_FOREGROUND_KEY = 175,
  };

struct ipmiseld_arguments
{
  struct common_cmd_args common_args;
  int verbose_count;
  char sensor_types[MAX_SENSOR_TYPES][MAX_SENSOR_TYPES_STRING_LENGTH+1];
  unsigned int sensor_types_length;
  char exclude_sensor_types[MAX_SENSOR_TYPES][MAX_SENSOR_TYPES_STRING_LENGTH+1];
  unsigned int exclude_sensor_types_length;
  int system_event_only;
  int oem_event_only;
  char *event_state_config_file;
  int interpret_oem_data;
  int output_oem_event_strings;
  int entity_sensor_names;
  int non_abbreviated_units;
  char *event_state_filter_str;
  char *system_event_format_str;
  char *oem_timestamped_event_format_str;
  char *oem_non_timestamped_event_format_str;
  char *log_facility_str;
  char *log_priority_str;
  char *cache_directory;
  int test_run;
  int foreground;
};

typedef struct ipmiseld_prog_data
{
  char *progname;
  int event_state_filter_mask;
  int log_facility;
  int log_priority;
  struct ipmiseld_arguments *args;
} ipmiseld_prog_data_t;

typedef struct ipmiseld_state_data
{
  ipmiseld_prog_data_t *prog_data;
  ipmi_ctx_t ipmi_ctx;
  char *hostname;
  ipmi_sdr_ctx_t sdr_ctx;
  ipmi_sel_ctx_t sel_ctx;
  ipmi_interpret_ctx_t interpret_ctx;
  struct ipmi_oem_data oem_data;
} ipmiseld_state_data_t;

#endif /* IPMISELD_H */
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

#ifndef IPMI_SEL_H
#define IPMI_SEL_H

#include <freeipmi/freeipmi.h>

#include "tool-cmdline-common.h"
#include "tool-oem-common.h"
#include "tool-sensor-common.h"
#include "pstdout.h"

#define IPMI_SEL_MAX_RECORD 4096

enum ipmi_sel_argp_option_keys
  {
    VERBOSE_KEY = 'v',
    INFO_KEY = 'i',
    DISPLAY_KEY = 160,
    EXCLUDE_DISPLAY_KEY = 161,
    DISPLAY_RANGE_KEY = 162,
    EXCLUDE_DISPLAY_RANGE_KEY = 163,
    DATE_RANGE_KEY = 164,
    EXCLUDE_DATE_RANGE_KEY = 165,
    SENSOR_TYPES_KEY = 't',
    EXCLUDE_SENSOR_TYPES_KEY = 'T',
    LIST_SENSOR_TYPES_KEY = 'L',
    TAIL_KEY = 166,
    CLEAR_KEY = 167,
    POST_CLEAR_KEY = 168,
    DELETE_ALL_KEY = 169,       /* legacy */
    DELETE_KEY = 170,
    DELETE_RANGE_KEY = 171,
    SYSTEM_EVENT_ONLY_KEY = 172,
    OEM_EVENT_ONLY_KEY = 173,
    OUTPUT_MANUFACTURER_ID_KEY = 174,
    OUTPUT_EVENT_STATE_KEY = 175,
    EVENT_STATE_CONFIG_FILE_KEY = 176,
    HEX_DUMP_KEY = 177,
    ASSUME_SYSTEM_EVENT_RECORDS_KEY = 178,
    INTERPRET_OEM_DATA_KEY = 179,
    OUTPUT_OEM_EVENT_STRINGS_KEY = 180,
    ENTITY_SENSOR_NAMES_KEY = 181,
    NO_SENSOR_TYPE_OUTPUT_KEY = 182,
    COMMA_SEPARATED_OUTPUT_KEY = 183,
    NO_HEADER_OUTPUT_KEY = 184,
    NON_ABBREVIATED_UNITS_KEY = 185,
    LEGACY_OUTPUT_KEY = 186,
  };

struct ipmi_sel_arguments
{
  struct common_cmd_args common;
  struct sdr_cmd_args sdr;
  struct hostrange_cmd_args hostrange;
  int verbose_count;
  int info;
  int display;
  uint16_t display_record_list[IPMI_SEL_MAX_RECORD];
  unsigned int display_record_list_length;
  int exclude_display;
  uint16_t exclude_display_record_list[IPMI_SEL_MAX_RECORD];
  unsigned int exclude_display_record_list_length;
  int display_range;
  uint16_t display_range1;
  uint16_t display_range2;
  int exclude_display_range;
  uint16_t exclude_display_range1;
  uint16_t exclude_display_range2;
  int date_range;
  uint32_t date_range1;
  uint32_t date_range2;
  int exclude_date_range;
  uint32_t exclude_date_range1;
  uint32_t exclude_date_range2;
  char sensor_types[MAX_SENSOR_TYPES][MAX_SENSOR_TYPES_STRING_LENGTH+1];
  unsigned int sensor_types_length;
  char exclude_sensor_types[MAX_SENSOR_TYPES][MAX_SENSOR_TYPES_STRING_LENGTH+1];
  unsigned int exclude_sensor_types_length;
  int list_sensor_types;
  int tail;
  uint16_t tail_count;
  int clear;
  int post_clear;
  int delete;
  uint16_t delete_record_list[IPMI_SEL_MAX_RECORD];
  unsigned int delete_record_list_length;
  int delete_range;
  uint16_t delete_range1;
  uint16_t delete_range2;
  int system_event_only;
  int oem_event_only;
  int output_manufacturer_id;
  int output_event_state;
  char *event_state_config_file;
  int hex_dump;
  int assume_system_event_records;
  int interpret_oem_data;
  int output_oem_event_strings;
  int entity_sensor_names;
  int no_sensor_type_output;
  int comma_separated_output;
  int no_header_output;
  int non_abbreviated_units;
  int legacy_output;
};

typedef struct ipmi_sel_prog_data
{
  char *progname;
  struct ipmi_sel_arguments *args;
} ipmi_sel_prog_data_t;

typedef struct ipmi_sel_state_data
{
  ipmi_sel_prog_data_t *prog_data;
  ipmi_ctx_t ipmi_ctx;
  pstdout_state_t pstate;
  char *hostname;
  ipmi_sdr_cache_ctx_t sdr_cache_ctx;
  ipmi_sdr_parse_ctx_t sdr_parse_ctx;
  ipmi_sel_parse_ctx_t sel_parse_ctx;
  ipmi_interpret_ctx_t interpret_ctx;
  int output_headers;
  struct sensor_entity_id_counts entity_id_counts;
  struct sensor_column_width column_width;
  struct ipmi_oem_data oem_data;
  /* for tail usage */
  uint16_t first_record_id;
  uint16_t last_record_id;
} ipmi_sel_state_data_t;

#endif /* IPMI_SEL_H */