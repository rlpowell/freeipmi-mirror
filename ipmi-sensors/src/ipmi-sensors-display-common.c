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

#include <stdio.h>

#include "freeipmi/record-format/ipmi-sdr-record-format.h"
#include "freeipmi/spec/ipmi-sensor-units-spec.h"

#include "ipmi-sensors.h"
#include "ipmi-sensors-display-common.h"

#include "pstdout.h"
#include "tool-fiid-wrappers.h"
#include "tool-sdr-cache-common.h"

int
ipmi_sensors_output_event_message_list (ipmi_sensors_state_data_t *state_data,
                                        char **event_message_list,
                                        unsigned int event_message_list_len)
{
  assert(state_data);
  
  if (event_message_list)
    {
      int i;
      
      for (i = 0; i < event_message_list_len; i++)
        pstdout_printf (state_data->pstate,
                        "[%s]",
                        event_message_list[i]);
      pstdout_printf (state_data->pstate,
                      "\n");
    }
  else 
    pstdout_printf (state_data->pstate,
                    "[%s]\n", 
                    "OK");
  
  return 0;
}

int
ipmi_sensors_output_verbose_event_message_list (ipmi_sensors_state_data_t *state_data,
                                                char **event_message_list,
                                                unsigned int event_message_list_len)
{
  assert (state_data);

  pstdout_printf (state_data->pstate, 
                  "Sensor Status: ");
  
  if (ipmi_sensors_output_event_message_list (state_data,
                                              event_message_list,
                                              event_message_list_len) < 0)
    return -1;
  
  /* Extra \n in verbose output */
  pstdout_printf (state_data->pstate, "\n");
  
  return 0;
}

int
ipmi_sensors_get_thresholds (ipmi_sensors_state_data_t *state_data,
                             uint8_t *sdr_record,
                             unsigned int sdr_record_len,
                             double **lower_non_critical_threshold,
                             double **lower_critical_threshold,
                             double **lower_non_recoverable_threshold,
                             double **upper_non_critical_threshold,
                             double **upper_critical_threshold,
                             double **upper_non_recoverable_threshold)
{
  int8_t r_exponent, b_exponent;
  int16_t m, b;
  uint8_t linearization, analog_data_format;
  uint8_t sensor_number;
  fiid_obj_t obj_cmd_rs = NULL;
  double *tmp_lower_non_critical_threshold = NULL;
  double *tmp_lower_critical_threshold = NULL;
  double *tmp_lower_non_recoverable_threshold = NULL;
  double *tmp_upper_non_critical_threshold = NULL;
  double *tmp_upper_critical_threshold = NULL;
  double *tmp_upper_non_recoverable_threshold = NULL;
  uint64_t threshold;
  uint64_t val;
  int rv = -1;

  assert(state_data);
  assert(sdr_record);
  assert(sdr_record_len);

  if (lower_non_critical_threshold)
    *lower_non_critical_threshold = NULL;
  if (lower_critical_threshold)
    *lower_critical_threshold = NULL;
  if (lower_non_recoverable_threshold)
    *lower_non_recoverable_threshold = NULL;
  if (upper_non_critical_threshold)
    *upper_non_critical_threshold = NULL;
  if (upper_critical_threshold)
    *upper_critical_threshold = NULL;
  if (upper_non_recoverable_threshold)
    *upper_non_recoverable_threshold = NULL;

  /* achu:
   *
   * I will admit I'm not entirely sure what the best way is
   * to get thresholds.  It seems the information is
   * stored/retrievable in the SDR and through an IPMI command.
   *
   * Since the readable_threshold_mask in the SDR record indicates the
   * mask is for the "Get Sensor Thresholds" command, it suggests the
   * best/right way is to get the values via that command.  Sounds
   * good to me.
   */

  if (sdr_cache_get_sensor_number (state_data->pstate,
                                   sdr_record,
                                   sdr_record_len,
                                   &sensor_number) < 0)
    goto cleanup;

  if (sdr_cache_get_sensor_decoding_data(state_data->pstate,
                                         sdr_record,
                                         sdr_record_len,
                                         &r_exponent,
                                         &b_exponent,
                                         &m,
                                         &b,
                                         &linearization,
                                         &analog_data_format) < 0)
    goto cleanup;

  /* if the sensor is not analog, this is most likely a bug in the
   * SDR, since we shouldn't be decoding a non-threshold sensor.
   *
   * Don't return an error.  Allow code to output "NA" or something.
   */
  if (!IPMI_SDR_ANALOG_DATA_FORMAT_VALID(analog_data_format))
    {
      if (state_data->prog_data->args->common.flags & IPMI_FLAGS_DEBUG_DUMP)
        pstdout_fprintf(state_data->pstate,
                        stderr,
                        "Attempting to decode non-analog sensor\n");
      rv = 0;
      goto cleanup;
    }

  /* if the sensor is non-linear, I just don't know what to do 
   *
   * Don't return an error.  Allow code to output "NA" or something.
   */
  if (!IPMI_SDR_LINEARIZATION_IS_NON_LINEAR(linearization))
    {
      if (state_data->prog_data->args->common.flags & IPMI_FLAGS_DEBUG_DUMP)
        pstdout_fprintf(state_data->pstate,
                        stderr,
                        "Cannot decode non-linear sensor\n");
      rv = 0;
      goto cleanup;
    }
  
  _FIID_OBJ_CREATE(obj_cmd_rs, tmpl_cmd_get_sensor_thresholds_rs);

  if (ipmi_cmd_get_sensor_thresholds (state_data->ipmi_ctx,
                                      sensor_number,
                                      obj_cmd_rs) < 0)
    {
      pstdout_fprintf(state_data->pstate,
                      stderr,
                      "ipmi_cmd_get_sensor_thresholds: %s\n",
                      ipmi_ctx_strerror(ipmi_ctx_errnum(state_data->ipmi_ctx)));
      goto cleanup;
    } 

  if (lower_non_critical_threshold)
    {
      _FIID_OBJ_GET (obj_cmd_rs,
                     "readable_thresholds.lower_non_critical_threshold",
                     &val);
      if (val)
        {
          _FIID_OBJ_GET(obj_cmd_rs,
                        "lower_non_critical_threshold",
                        &val);

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        val,
                                        &threshold) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror(errno));
              goto cleanup;
            }

          if (!(tmp_lower_non_critical_threshold = (double *)malloc(sizeof(double))))
            {
              pstdout_perror(state_data->pstate, "malloc");
              goto cleanup;
            }
          *tmp_lower_non_critical_threshold = threshold;
        }
    }
  if (lower_critical_threshold)
    {
      _FIID_OBJ_GET (obj_cmd_rs,
                     "readable_thresholds.lower_critical_threshold",
                     &val);
      if (val)
        {
          _FIID_OBJ_GET(obj_cmd_rs,
                        "lower_critical_threshold",
                        &val);

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        val,
                                        &threshold) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror(errno));
              goto cleanup;
            }

          if (!(tmp_lower_critical_threshold = (double *)malloc(sizeof(double))))
            {
              pstdout_perror(state_data->pstate, "malloc");
              goto cleanup;
            }
          *tmp_lower_critical_threshold = threshold;
        }
    }
  if (lower_non_recoverable_threshold)
    {
      _FIID_OBJ_GET (obj_cmd_rs,
                     "readable_thresholds.lower_critical_threshold",
                     &val);
      if (val)
        {
          _FIID_OBJ_GET(obj_cmd_rs,
                        "lower_non_recoverable_threshold",
                        &val);

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        val,
                                        &threshold) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror(errno));
              goto cleanup;
            }

          if (!(tmp_lower_non_recoverable_threshold = (double *)malloc(sizeof(double))))
            {
              pstdout_perror(state_data->pstate, "malloc");
              goto cleanup;
            }
          *tmp_lower_non_recoverable_threshold = threshold;
        }
    }
  if (upper_non_critical_threshold)
    {
      _FIID_OBJ_GET (obj_cmd_rs,
                     "readable_thresholds.upper_non_critical_threshold",
                     &val);
      if (val)
        {
          _FIID_OBJ_GET(obj_cmd_rs,
                        "upper_non_critical_threshold",
                        &val);

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        val,
                                        &threshold) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror(errno));
              goto cleanup;
            }

          if (!(tmp_upper_non_critical_threshold = (double *)malloc(sizeof(double))))
            {
              pstdout_perror(state_data->pstate, "malloc");
              goto cleanup;
            }
          *tmp_upper_non_critical_threshold = threshold;
        }
    }
  if (upper_critical_threshold)
    {
      _FIID_OBJ_GET (obj_cmd_rs,
                     "readable_thresholds.upper_critical_threshold",
                     &val);
      if (val)
        {
          _FIID_OBJ_GET(obj_cmd_rs,
                        "upper_critical_threshold",
                        &val);

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        val,
                                        &threshold) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror(errno));
              goto cleanup;
            }

          if (!(tmp_upper_critical_threshold = (double *)malloc(sizeof(double))))
            {
              pstdout_perror(state_data->pstate, "malloc");
              goto cleanup;
            }
          *tmp_upper_critical_threshold = threshold;
        }
    }
  if (upper_non_recoverable_threshold)
    {
      _FIID_OBJ_GET (obj_cmd_rs,
                     "readable_thresholds.upper_critical_threshold",
                     &val);
      if (val)
        {
          _FIID_OBJ_GET(obj_cmd_rs,
                        "upper_non_recoverable_threshold",
                        &val);

          if (ipmi_sensor_decode_value (r_exponent,
                                        b_exponent,
                                        m,
                                        b,
                                        linearization,
                                        analog_data_format,
                                        val,
                                        &threshold) < 0)
            {
              pstdout_fprintf (state_data->pstate,
                               stderr,
                               "ipmi_sensor_decode_value: %s\n",
                               strerror(errno));
              goto cleanup;
            }

          if (!(tmp_upper_non_recoverable_threshold = (double *)malloc(sizeof(double))))
            {
              pstdout_perror(state_data->pstate, "malloc");
              goto cleanup;
            }
          *tmp_upper_non_recoverable_threshold = threshold;
        }
    }

  if (lower_non_critical_threshold)
    *lower_non_critical_threshold = tmp_lower_non_critical_threshold;
  if (lower_critical_threshold)
    *lower_critical_threshold = tmp_lower_critical_threshold;
  if (lower_non_recoverable_threshold)
    *lower_non_recoverable_threshold = tmp_lower_non_recoverable_threshold;
  if (upper_non_critical_threshold)
    *upper_non_critical_threshold = tmp_upper_non_critical_threshold;
  if (upper_critical_threshold)
    *upper_critical_threshold = tmp_upper_critical_threshold;
  if (upper_non_recoverable_threshold)
  *upper_non_recoverable_threshold = tmp_upper_non_recoverable_threshold;

  rv = 0;
 cleanup:
  _FIID_OBJ_DESTROY(obj_cmd_rs);
  if (rv < 0)
    {
      if (tmp_lower_non_critical_threshold)
        free(tmp_lower_non_critical_threshold);
      if (tmp_lower_critical_threshold)
        free(tmp_lower_critical_threshold);
      if (tmp_lower_non_recoverable_threshold)
        free(tmp_lower_non_recoverable_threshold);
      if (tmp_upper_non_critical_threshold)
        free(tmp_upper_non_critical_threshold);
      if (tmp_upper_critical_threshold)
        free(tmp_upper_critical_threshold);
      if (tmp_upper_non_recoverable_threshold)
        free(tmp_upper_non_recoverable_threshold);
    }
  return rv;
}

int
ipmi_sensors_output_verbose_thresholds (ipmi_sensors_state_data_t *state_data,
                                        uint8_t *sdr_record,
                                        unsigned int sdr_record_len)
{
  double *lower_non_critical_threshold = NULL;
  double *lower_critical_threshold = NULL;
  double *lower_non_recoverable_threshold = NULL;
  double *upper_non_critical_threshold = NULL;
  double *upper_critical_threshold = NULL;
  double *upper_non_recoverable_threshold = NULL;
  uint8_t sensor_unit;
  int rv = -1;
  
  assert(state_data);
  assert(sdr_record);
  assert(sdr_record_len);

  if (sdr_cache_get_sensor_unit (state_data->pstate,
                                 sdr_record,
                                 sdr_record_len,
                                 &sensor_unit) < 0)
    goto cleanup;

  if (ipmi_sensors_get_thresholds (state_data,
                                   sdr_record,
                                   sdr_record_len,
                                   &lower_non_critical_threshold,
                                   &lower_critical_threshold,
                                   &lower_non_recoverable_threshold,
                                   &upper_non_critical_threshold,
                                   &upper_critical_threshold,
                                   &upper_non_recoverable_threshold) < 0)
    goto cleanup;

  if (lower_critical_threshold)
    pstdout_printf (state_data->pstate,
                    "Lower Critical Threshold: %f %s\n", 
                    *lower_critical_threshold, 
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate, 
                    "Lower Critical Threshold: %s\n", 
                    "NA");

  if (upper_critical_threshold)
    pstdout_printf (state_data->pstate, 
                    "Upper Critical Threshold: %f %s\n", 
                    *upper_critical_threshold, 
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate, 
                    "Upper Critical Threshold: %s\n", 
                    "NA");

  if (lower_non_critical_threshold)
    pstdout_printf (state_data->pstate, 
                    "Lower Non-Critical Threshold: %f %s\n", 
                    *lower_non_critical_threshold, 
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate, 
                    "Lower Non-Critical Threshold: %s\n", 
                    "NA");

  if (upper_non_critical_threshold)
    pstdout_printf (state_data->pstate, 
                    "Upper Non-Critical Threshold: %f %s\n", 
                    *upper_non_critical_threshold, 
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate, 
                    "Upper Non-Critical Threshold: %s\n", 
                    "NA");

  if (lower_non_recoverable_threshold)
    pstdout_printf (state_data->pstate, 
                    "Lower Non-Recoverable Threshold: %f %s\n", 
                    *lower_non_recoverable_threshold, 
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate, 
                    "Lower Non-Recoverable Threshold: %s\n", 
                    "NA");

  if (upper_non_recoverable_threshold)
    pstdout_printf (state_data->pstate, 
                    "Upper Non-Recoverable Threshold: %f %s\n", 
                    *upper_non_recoverable_threshold, 
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate, 
                    "Upper Non-Recoverable Threshold: %s\n", 
                    "NA");

  rv = 0;
 cleanup:
  if (lower_non_critical_threshold)
    free(lower_non_critical_threshold);
  if (lower_critical_threshold)
    free(lower_critical_threshold);
  if (lower_non_recoverable_threshold)
    free(lower_non_recoverable_threshold);
  if (upper_non_critical_threshold)
    free(upper_non_critical_threshold);
  if (upper_critical_threshold)
    free(upper_critical_threshold);
  if (upper_non_recoverable_threshold)
    free(upper_non_recoverable_threshold);
  return rv;
}                              

int
ipmi_sensors_output_verbose_sensor_reading_ranges (ipmi_sensors_state_data_t *state_data,
                                                   uint8_t *sdr_record,
                                                   unsigned int sdr_record_len)
{
  double *nominal_reading = NULL;
  double *normal_maximum = NULL;
  double *normal_minimum = NULL;
  double *sensor_maximum_reading = NULL;
  double *sensor_minimum_reading = NULL;
  uint8_t sensor_unit;
  int rv = -1;

  assert(state_data);
  assert(sdr_record);
  assert(sdr_record_len);

  if (sdr_cache_get_sensor_unit (state_data->pstate,
                                 sdr_record,
                                 sdr_record_len,
                                 &sensor_unit) < 0)
    goto cleanup;

  if (sensor_minimum_reading)
    pstdout_printf (state_data->pstate,
                    "Sensor Min. Reading: %f %s\n",
                    *sensor_minimum_reading,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Min. Reading: %s\n",
                    "NA");

  if (sensor_maximum_reading)
    pstdout_printf (state_data->pstate,
                    "Sensor Max. Reading: %f %s\n",
                    *sensor_maximum_reading,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Max. Reading: %s\n",
                    "NA");

  if (normal_minimum)
    pstdout_printf (state_data->pstate,
                    "Normal Min.: %f %s\n",
                    *normal_minimum,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Normal Min.: %s\n",
                    "NA");

  if (normal_maximum)
    pstdout_printf (state_data->pstate,
                    "Normal Max.: %f %s\n",
                    *normal_maximum,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Normal Max.: %s\n",
                    "NA");

  if (nominal_reading)
    pstdout_printf (state_data->pstate,
                    "Nominal reading: %f %s\n",
                    *nominal_reading,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Nominal reading: %s\n",
                    "NA");

  if (reading)
    pstdout_printf (state_data->pstate,
                    "Sensor Reading: %f %s\n",
                    *reading,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Reading: %s\n",
                    "NA");
  
  rv = 0;
 cleanup:
  if (nominal_reading)
    free(nominal_reading);
  if (normal_maximum)
    free(normal_maximum);
  if (normal_minimum)
    free(normal_minimum);
  if (sensor_maximum_reading)
    free(sensor_maximum_reading);
  if (sensor_minimum_reading)
    free(sensor_minimum_reading);
  return rv;
}

int 
ipmi_sensors_output_verbose_sensor_reading (ipmi_sensors_state_data_t *state_data,
                                            uint8_t *sdr_record,
                                            unsigned int sdr_record_len,
                                            double *reading)
{
  uint8_t sensor_unit;

  assert(state_data);
  assert(sdr_record);
  assert(sdr_record_len);

  if (sdr_cache_get_sensor_unit (state_data->pstate,
                                 sdr_record,
                                 sdr_record_len,
                                 &sensor_unit) < 0)
    goto cleanup;

  if (reading)
    pstdout_printf (state_data->pstate,
                    "Sensor Reading: %f %s\n",
                    *reading,
                    ipmi_sensor_units[sensor_unit]);
  else
    pstdout_printf (state_data->pstate,
                    "Sensor Reading: %s\n",
                    "NA");

  return 0;
}
