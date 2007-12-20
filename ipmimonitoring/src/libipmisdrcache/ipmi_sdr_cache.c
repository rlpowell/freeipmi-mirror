/*****************************************************************************\
 *  $Id: ipmi_sdr_cache.c,v 1.10.6.2 2007-12-20 22:59:42 chu11 Exp $
 *****************************************************************************
 *  Copyright (C) 2006-2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-222073
 *
 *  This file is part of Ipmimonitoring, an IPMI sensor monitoring
 *  library.  For details, see http://www.llnl.gov/linux/.
 *
 *  Ipmimonitoring is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmimonitoring is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmimonitoring.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/mman.h>
#include <assert.h>
#include <errno.h>

#include "ipmi_sdr_cache.h"
#include "ipmi_sdr_cache_common.h"
#include "ipmi_sdr_cache_defs.h"

static char *ipmi_sdr_cache_errmsgs[] =
  {
    "success",
    "context null",
    "context invalid",
    "invalid parmaeters",
    "out of memory",
    "filename invalid",
    "file system error",
    "filename path permission error",
    "SDR cache exists",
    "SDR cache create initialization already called",
    "SDR cache context set for reading",
    "cache creation not initialized",
    "number of records written has reached record count",
    "invalid SDR record length",
    "SDR record with an identical record id already written",
    "SDR record with an identical sensor number already written",
    "incomplete number of records written",
    "SDR cache reading initialization already called",
    "SDR cache context set for creation",
    "cache reading not initialized",
    "SDR cache does not exist",
    "SDR cache context set for creation",
    "SDR cache context set for reading",
    "SDR cache invalid",
    "not found",
    "internal IPMI error",
    "buffer overflow",
    "internal error",
    "errnum out of range",
    NULL
  };

ipmi_sdr_cache_ctx_t
ipmi_sdr_cache_ctx_create(void)
{
  struct ipmi_sdr_cache_ctx *c = NULL;

  if (!(c = (ipmi_sdr_cache_ctx_t)malloc(sizeof(struct ipmi_sdr_cache_ctx))))
    return NULL;
  c->magic = IPMI_SDR_CACHE_MAGIC;
  ipmi_sdr_cache_init_ctx(c);
  return c;
}

void
ipmi_sdr_cache_ctx_destroy(ipmi_sdr_cache_ctx_t c)
{
  if (!c || c->magic != IPMI_SDR_CACHE_MAGIC)
    return;

  if (c->fd >= 0)
    {
      /* If the cache create never completed, try to remove the file */
      if (c->operation == IPMI_SDR_CACHE_OPERATION_CREATE_CACHE)
	unlink(c->filename);
      close(c->fd);
    }
  if (c->sdr_cache)
    munmap(c->sdr_cache, c->file_size);
  if (c->record_ids)
    free(c->record_ids);
  if (c->sensor_numbers)
    free(c->sensor_numbers);

  c->magic = ~IPMI_SDR_CACHE_MAGIC;
  c->operation = IPMI_SDR_CACHE_OPERATION_UNINITIALIZED;
  free(c);
}

int 
ipmi_sdr_cache_ctx_errnum(ipmi_sdr_cache_ctx_t c)
{
  if (!c)
    return IPMI_SDR_CACHE_ERR_CONTEXT_NULL;
  else if (c->magic != IPMI_SDR_CACHE_MAGIC)
    return IPMI_SDR_CACHE_ERR_CONTEXT_INVALID;
  else
    return c->errnum;
}

char *
ipmi_sdr_cache_ctx_strerror(int errnum)
{
  if (errnum >= IPMI_SDR_CACHE_ERR_SUCCESS && errnum <= IPMI_SDR_CACHE_ERR_ERRNUMRANGE)
    return ipmi_sdr_cache_errmsgs[errnum];
  else
    return ipmi_sdr_cache_errmsgs[IPMI_SDR_CACHE_ERR_ERRNUMRANGE];
}
