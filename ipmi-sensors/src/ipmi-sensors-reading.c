/* 
   Copyright (C) 2005 FreeIPMI Core Team

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.  

*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <errno.h>

#include "freeipmi/api/ipmi-sensor-cmds-api.h"
#include "freeipmi/cmds/ipmi-sensor-cmds.h"
#include "freeipmi/fiid/fiid.h"
#include "freeipmi/record-format/ipmi-sdr-record-format.h"
#include "freeipmi/spec/ipmi-sensor-types-spec.h"
#include "freeipmi/util/ipmi-sensor-and-event-code-tables-util.h"
#include "freeipmi/util/ipmi-sensor-util.h"

#include "ipmi-sensors.h"

#include "tool-fiid-wrappers.h"
#include "tool-sdr-cache-common.h"
#include "tool-sensor-common.h"

enum system_software_type
  {
    IPMI_BIOS,
    IPMI_SMI_HANDLER,
    IPMI_SYSTEM_MANAGEMENT_SOFTWARE,
    IPMI_OEM,
    IPMI_REMOTE_CONSOLE_SOFTWARE,
    IPMI_TERMINAL_MODE_REMOTE_CONSOLE_SOFTWARE,
    IPMI_SYS_SOFT_ID_RESERVED
  };

static int
_get_system_software_type (uint8_t system_software_id)
{
  /* To avoid "warning: comparison is always true due to limited range
   * of data type" 
   */
  if ((system_software_id + 1) >= 1 && system_software_id <= 0x0F)
    return IPMI_BIOS;
  if (system_software_id >= 0x10 && system_software_id <= 0x1F)
    return IPMI_SMI_HANDLER;
  if (system_software_id >= 0x20 && system_software_id <= 0x2F)
    return IPMI_SYSTEM_MANAGEMENT_SOFTWARE;
  if (system_software_id >= 0x30 && system_software_id <= 0x3F)
    return IPMI_OEM;
  if (system_software_id >= 0x40 && system_software_id <= 0x46)
    return IPMI_REMOTE_CONSOLE_SOFTWARE;
  if (system_software_id == 0x47)
    return IPMI_TERMINAL_MODE_REMOTE_CONSOLE_SOFTWARE;

  return IPMI_SYS_SOFT_ID_RESERVED;
}

static char **
_get_threshold_message_list (int debug, uint8_t sensor_state)
{
  char **event_message_list = NULL;
  char *message_list[16];
  char buf[1024];
  int indx = 0;
  int16_t offset;
  uint16_t bit; 
  int i;
  
  /* achu: multiple threshold flags can be crossed but we only want to
   * output one message at the max.  Luckily for us (and due to smarts
   * by the IPMI specification authors) if we go from high bits to low
   * bits, we will read the flags in the correct order for output.
   */

  for (offset = 5; offset >= 0; offset--)
    {
      bit = pow (2, offset);
      if (sensor_state & bit)
	{
	  if (ipmi_get_threshold_message (offset,
                                          buf,
                                          1024) < 0)
            continue;
	  
	  if (!(message_list[indx] = strdup(buf)))
            {
              if (debug)
                perror("strdup");
              goto cleanup;
            }
	  else
	    {
	      indx++;
	      break;
	    }
	}
    }
  
  if (indx)
    {
      if (!(event_message_list = (char **) malloc (sizeof (char *) * (indx + 1))))
        {
          if (debug)
            perror("malloc");
          goto cleanup;
        }
      for (offset = 0; offset < indx; offset++)
	event_message_list[offset] = message_list[offset];
      event_message_list[indx] = NULL;
    }
  
  return event_message_list;

 cleanup:
  for (i = 0; i < indx; i++)
    free(message_list[indx]);
  return NULL;
}

static char **
_get_generic_event_message_list (int debug,
                                 uint8_t event_reading_type_code, 
                                 uint16_t sensor_state)
{
  char **event_message_list = NULL;
  char *message_list[16];
  char buf[1024];
  int indx = 0;
  uint16_t offset;
  uint16_t bit; 
  int i;
  
  for (offset = 0; offset < 16; offset++)
    {
      bit = pow (2, offset);
      if (sensor_state & bit)
	{
	  if (ipmi_get_generic_event_message (event_reading_type_code,
					      offset,
					      buf,
					      1024) < 0)
            continue;

	  if (!(message_list[indx] = strdup(buf)))
            {
              if (debug)
                perror("strdup");
              goto cleanup;
            }
	  else
	    indx++; 
	}
    }
  
  if (indx)
    {
      if (!(event_message_list = (char **) malloc (sizeof (char *) * (indx + 1))))
        {
          if (debug)
            perror("malloc");
          goto cleanup;
        }
      for (offset = 0; offset < indx; offset++)
	event_message_list[offset] = message_list[offset];
      event_message_list[indx] = NULL;
    }
  
  return event_message_list;

 cleanup:
  for (i = 0; i < indx; i++)
    free(message_list[indx]);
  return NULL;
}

static char **
_get_event_message_list (int debug,
                         int sensor_type_code, 
                         uint16_t sensor_state)
{
  char **event_message_list = NULL;
  char *message_list[16];
  char buf[1024];
  int indx = 0;
  uint16_t offset;
  uint16_t bit; 
  int i;
  
  for (offset = 0; offset < 16; offset++)
    {
      bit = pow (2, offset);
      if (sensor_state & bit)
	{
	  if (ipmi_get_sensor_type_code_message (sensor_type_code,
						 offset,
						 buf,
						 1024) < 0)
            continue;

	  if (!(message_list[indx] = strdup(buf)))
            {
              if (debug)
                perror("strdup");
              goto cleanup;
            }
	  else
	    indx++;
	}
    }
  
  if (indx)
    {
      if (!(event_message_list = (char **) malloc (sizeof (char *) * (indx + 1))))
        {
          if (debug)
            perror("malloc");
          goto cleanup;
        }
      for (offset = 0; offset < indx; offset++)
	event_message_list[offset] = message_list[offset];
      event_message_list[indx] = NULL;
    }
  
  return event_message_list;

 cleanup:
  for (i = 0; i < indx; i++)
    free(message_list[indx]);
  return NULL;
}

int
sensor_reading (struct ipmi_sensors_state_data *state_data,
                uint8_t *sdr_record,
                unsigned int sdr_record_len,
                double *reading,
                char *event_message_list[],
                unsigned int event_message_list_len)
{
  fiid_template_t l_tmpl_cmd_get_sensor_reading_threshold_rs =
    {
      {8, "cmd", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {8, "comp_code", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {8, "sensor_reading", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {5, "reserved1", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "reading_state", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "sensor_scanning", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "all_event_messages", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {6, "sensor_state", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {2, "reserved2", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      /* optional byte */
      {8, "ignore", FIID_FIELD_OPTIONAL | FIID_FIELD_LENGTH_FIXED}, 
      
      {0,  "", 0}
    };
  
  fiid_template_t l_tmpl_cmd_get_sensor_reading_discrete_rs =
    {
      {8, "cmd", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {8, "comp_code", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {8, "sensor_reading", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {5, "reserved1", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "reading_state", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "sensor_scanning", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "all_event_messages", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {15, "sensor_state", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      {1, "reserved2", FIID_FIELD_REQUIRED | FIID_FIELD_LENGTH_FIXED}, 
      
      {0,  "", 0}
    };
  
  uint8_t slave_sys_soft_id;
  uint8_t event_reading_type_code;
  uint8_t sensor_number;
  uint8_t sensor_type;
  int16_t b = 0;
  int16_t m = 0;
  int8_t r_exponent = 0;
  int8_t b_exponent = 0;
  uint8_t linearization = 0;
  uint8_t analog_data_format = 0;
  int rv = -1;
  fiid_obj_t obj_cmd_rs = NULL;  
  fiid_obj_t l_obj_cmd_rs = NULL;
  uint8_t buf[1024];
  int32_t len;
  uint64_t val;

  assert(state_data);
  assert(sdr_record);
  assert(sdr_record_len);
  assert(reading);
  assert(event_message_list);
  assert(event_message_list_len);

  switch (sdr_record->record_type)
    {
    case IPMI_SDR_FORMAT_FULL_RECORD:
      slave_sys_soft_id = sdr_record->record.sdr_full_record.sensor_owner_id;
      if (_get_system_software_type (slave_sys_soft_id) == IPMI_SYS_SOFT_ID_RESERVED)
	return -1;
      
      event_reading_type_code = sdr_record->record.sdr_full_record.event_reading_type_code;
      sensor_number = sdr_record->record.sdr_full_record.sensor_number;
      sensor_type = sdr_record->record.sdr_full_record.sensor_type;
      b = sdr_record->record.sdr_full_record.b;
      m = sdr_record->record.sdr_full_record.m;
      r_exponent = sdr_record->record.sdr_full_record.r_exponent;
      b_exponent = sdr_record->record.sdr_full_record.b_exponent;
      linearization = sdr_record->record.sdr_full_record.linearization;
      analog_data_format = sdr_record->record.sdr_full_record.analog_data_format;
      break;
    case IPMI_SDR_FORMAT_COMPACT_RECORD:
      slave_sys_soft_id = sdr_record->record.sdr_compact_record.sensor_owner_id;
      if (_get_system_software_type (slave_sys_soft_id) == IPMI_SYS_SOFT_ID_RESERVED)
	return -1;
      
      event_reading_type_code = sdr_record->record.sdr_compact_record.event_reading_type_code;
      sensor_number = sdr_record->record.sdr_compact_record.sensor_number;
      sensor_type = sdr_record->record.sdr_compact_record.sensor_type;
      break;
    default:
      return -1;
    }
  
  switch (sensor_classify (event_reading_type_code))
    {
    case SENSOR_CLASS_THRESHOLD:
      _FIID_OBJ_CREATE(obj_cmd_rs, tmpl_cmd_get_sensor_reading_threshold_rs);
      _FIID_OBJ_CREATE(l_obj_cmd_rs, l_tmpl_cmd_get_sensor_reading_threshold_rs);

      if (ipmi_cmd_get_sensor_reading_threshold (ctx, 
                                                 sensor_number, 
                                                 obj_cmd_rs) < 0)
        {
          
          if (debug)
            fprintf(stderr, 
                    "ipmi_cmd_get_sensor_reading_discrete: %s\n",
                    ipmi_ctx_strerror(ipmi_ctx_errnum(ctx)));
          goto cleanup;
        }

      _FIID_OBJ_GET_ALL_LEN(len,
                            obj_cmd_rs,
                            buf,
                            1024);

      _FIID_OBJ_SET_ALL (l_obj_cmd_rs, buf, len);
      
      _FIID_OBJ_GET (l_obj_cmd_rs, "sensor_reading", &val);

      if (sdr_record->record_type == IPMI_SDR_FORMAT_FULL_RECORD
	  && analog_data_format != IPMI_SDR_ANALOG_DATA_FORMAT_NOT_ANALOG)
	{
	  if (ipmi_sensor_decode_value (r_exponent, 
                                        b_exponent, 
                                        m, 
                                        b, 
                                        linearization, 
                                        analog_data_format, 
                                        (uint8_t) val,
                                        &(sensor_reading->current_reading)) < 0)
            {
              if (debug)
                perror("ipmi_sensor_decode_value");
              goto cleanup;
            }
	}
      else 
        sensor_reading->current_reading = val;
           
      _FIID_OBJ_GET (l_obj_cmd_rs, 
                     "sensor_state", 
                     &val);
      sensor_reading->event_message_list = 
	_get_threshold_message_list (debug, val);
      
      rv = 0;
      break;
    case SENSOR_CLASS_GENERIC_DISCRETE:
      _FIID_OBJ_CREATE(obj_cmd_rs, tmpl_cmd_get_sensor_reading_discrete_rs);

      _FIID_OBJ_CREATE(l_obj_cmd_rs, l_tmpl_cmd_get_sensor_reading_discrete_rs);

      if (ipmi_cmd_get_sensor_reading_discrete (ctx, 
                                                sensor_number, 
                                                obj_cmd_rs) < 0)
        {
          if (debug)
            fprintf(stderr, 
                    "ipmi_cmd_get_sensor_reading_discrete: %s\n",
                    ipmi_ctx_strerror(ipmi_ctx_errnum(ctx)));
          goto cleanup;
        }
      
      _FIID_OBJ_GET_ALL_LEN(len,
                            obj_cmd_rs,
                            buf,
                            1024);

      _FIID_OBJ_SET_ALL (l_obj_cmd_rs, buf, len);

      _FIID_OBJ_GET (l_obj_cmd_rs, "sensor_reading", &val);

      sensor_reading->current_reading = val;
            
      _FIID_OBJ_GET (l_obj_cmd_rs, 
                     "sensor_state", 
                     &val);
      sensor_reading->event_message_list = 
	_get_generic_event_message_list (debug, 
                                         event_reading_type_code, 
                                         val);
      
      rv = 0;
      break;
    case SENSOR_CLASS_SENSOR_SPECIFIC_DISCRETE:
      _FIID_OBJ_CREATE(obj_cmd_rs, tmpl_cmd_get_sensor_reading_discrete_rs);

      _FIID_OBJ_CREATE(l_obj_cmd_rs, l_tmpl_cmd_get_sensor_reading_discrete_rs);

      if (ipmi_cmd_get_sensor_reading_discrete (ctx, 
                                                sensor_number, 
                                                obj_cmd_rs) < 0)
        {
          if (debug)
            fprintf(stderr, 
                    "ipmi_cmd_get_sensor_reading_discrete: %s\n",
                    ipmi_ctx_strerror(ipmi_ctx_errnum(ctx)));
          goto cleanup;
        }
      
      _FIID_OBJ_GET_ALL_LEN(len,
                            obj_cmd_rs,
                            buf,
                            1024);

      _FIID_OBJ_SET_ALL (l_obj_cmd_rs, buf, len);

      _FIID_OBJ_GET (l_obj_cmd_rs, "sensor_reading", &val);

      sensor_reading->current_reading = val;
            
      _FIID_OBJ_GET (l_obj_cmd_rs, 
                     "sensor_state", 
                     &val);
      sensor_reading->event_message_list = 
	_get_event_message_list (debug,
                                 sensor_type, 
                                 val);
      
      rv = 0;
      break;
    case SENSOR_CLASS_OEM:
      _FIID_OBJ_CREATE(obj_cmd_rs, tmpl_cmd_get_sensor_reading_discrete_rs);

      _FIID_OBJ_CREATE(l_obj_cmd_rs, l_tmpl_cmd_get_sensor_reading_discrete_rs);

      if (ipmi_cmd_get_sensor_reading_discrete (ctx, 
                                                sensor_number, 
                                                obj_cmd_rs) < 0)
        {
          if (debug)
            fprintf(stderr, 
                    "ipmi_cmd_get_sensor_reading_discrete: %s\n",
                    ipmi_ctx_strerror(ipmi_ctx_errnum(ctx)));
          goto cleanup;
        }
      
      _FIID_OBJ_GET_ALL_LEN(len,
                            obj_cmd_rs,
                            buf,
                            1024);

      _FIID_OBJ_SET_ALL (l_obj_cmd_rs, buf, len);

      _FIID_OBJ_GET (l_obj_cmd_rs, 
                     "sensor_reading",
                     &val);
      sensor_reading->current_reading = val;
     
      _FIID_OBJ_GET (l_obj_cmd_rs, 
                     "sensor_state", 
                     &val);

      {
	char *event_message = NULL;
	if (asprintf (&event_message, 
                      "OEM State = %04Xh", 
                      (uint16_t) val) < 0)
          {
            perror("asprintf");
            goto cleanup;
          }
	if (!(sensor_reading->event_message_list = (char **) malloc (sizeof (char *) * 2)))
          {
            perror("malloc");
            goto cleanup;
          }
	sensor_reading->event_message_list[0] = event_message;
	sensor_reading->event_message_list[1] = NULL;
      }
      
      rv = 0;
      break;
    }
  
 cleanup:
  _FIID_OBJ_DESTROY(obj_cmd_rs);
  _FIID_OBJ_DESTROY(l_obj_cmd_rs);
  return (rv);
}
