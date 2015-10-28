/*
 * Copyright (C) 2003-2015 FreeIPMI Core Team
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#include <stdarg.h>
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
#include <errno.h>

#include "freeipmi/sel/ipmi-sel.h"

#include "freeipmi/spec/ipmi-iana-enterprise-numbers-spec.h"
#include "freeipmi/spec/ipmi-product-id-spec.h"

#include "ipmi-sel-common.h"
#include "ipmi-sel-defs.h"
#include "ipmi-sel-string.h"
#include "ipmi-sel-string-supermicro.h"
#include "ipmi-sel-string-supermicro-common.h"
#include "ipmi-sel-string-supermicro-x8dtu.h"
#include "ipmi-sel-trace.h"
#include "ipmi-sel-util.h"

#include "freeipmi-portability.h"

/* return (0) - no OEM match
 * return (1) - OEM match
 * return (-1) - error, cleanup and return error
 */
int
sel_string_output_supermicro_x8dtu_event_data1_class_oem (ipmi_sel_ctx_t ctx,
							  struct ipmi_sel_entry *sel_entry,
							  uint8_t sel_record_type,
							  char *tmpbuf,
							  unsigned int tmpbuflen,
							  unsigned int flags,
							  unsigned int *wlen,
							  struct ipmi_sel_system_event_record_data *system_event_record_data)
{
  int ret;

  assert (ctx);
  assert (ctx->magic == IPMI_SEL_CTX_MAGIC);
  assert (ctx->manufacturer_id == IPMI_IANA_ENTERPRISE_ID_SUPERMICRO
	  || ctx->manufacturer_id == IPMI_IANA_ENTERPRISE_ID_SUPERMICRO_WORKAROUND);
  assert (sel_entry);
  assert (tmpbuf);
  assert (tmpbuflen);
  assert (!(flags & ~IPMI_SEL_STRING_FLAGS_MASK));
  assert (flags & IPMI_SEL_STRING_FLAGS_INTERPRET_OEM_DATA);
  assert (wlen);
  assert (system_event_record_data);
  assert (ctx->product_id == IPMI_SUPERMICRO_PRODUCT_ID_X8DTU);

  /* achu: Via reverse engineering, see 
   *
   * "Supermicro X8DTG-QF System Event Log" thread in late
   * January/early February 2012.
   */

  if ((ret = sel_string_output_supermicro_overheat_event_data1_class_oem (ctx,
									  sel_entry,
									  sel_record_type,
									  tmpbuf,
									  tmpbuflen,
									  flags,
									  wlen,
									  system_event_record_data)) < 0)

    if (ret)
      return (1);

  return (0);
}
