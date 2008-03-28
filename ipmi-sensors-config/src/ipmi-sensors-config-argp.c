/* 
   Copyright (C) 2007-2008 FreeIPMI Core Team
   
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#if STDC_HEADERS
#include <string.h>
#endif /* STDC_HEADERS */
#include <argp.h>
#include <ctype.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#if HAVE_FCNTL_H
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif /* HAVE_FCNTL_H */
#include <errno.h>

#include "tool-cmdline-common.h"
#include "ipmi-sensors-config.h"
#include "ipmi-sensors-config-argp.h"

static error_t parse_opt (int key, char *arg, struct argp_state *state);

const char *argp_program_version = 
"IPMI PEF [ipmi-sensors-config-" PACKAGE_VERSION "]\n"
"Copyright (C) 2007-2008 FreeIPMI Core Team\n"
"This program is free software; you may redistribute it under the terms of\n"
"the GNU General Public License.  This program has absolutely no warranty.";

const char *argp_program_bug_address = "<freeipmi-devel@gnu.org>";

static char doc[] = "IPMI-SENSORS-CONFIG - sensor configuration utility.";

static char args_doc[] = "";

static struct argp_option options[] = 
  {
    ARGP_COMMON_OPTIONS_DRIVER,
    ARGP_COMMON_OPTIONS_INBAND,
    ARGP_COMMON_OPTIONS_OUTOFBAND,
    ARGP_COMMON_OPTIONS_AUTHENTICATION_TYPE,
    ARGP_COMMON_OPTIONS_PRIVILEGE_LEVEL_OPERATOR,
    ARGP_COMMON_OPTIONS_WORKAROUND_FLAGS,
    ARGP_COMMON_SDR_OPTIONS,
    ARGP_COMMON_OPTIONS_DEBUG,
    {"checkout", CHECKOUT_KEY, 0, 0,
     "Fetch configuration information.", 30},
    {"commit", COMMIT_KEY, 0, 0,
     "Update configuration information from a config file or key pairs.", 31},
    {"diff", DIFF_KEY, 0, 0,
     "Show differences between stored information and a config file or key pairs.", 32},
    {"filename", FILENAME_KEY, "FILENAME", 0,
     "Specify a config file for checkout/commit/diff.", 33},
    {"key-pair", KEYPAIR_KEY, "KEY-PAIR", 0,
     "Specify KEY=VALUE pairs for checkout/commit/diff.", 34},
    {"section", SECTIONS_KEY, "SECTION", 0,
     "Specify a SECTION for checkout.", 35},
    {"listsections", LIST_SECTIONS_KEY, 0, 0,
     "List available sections for checkout.", 36},
    {"verbose", VERBOSE_KEY, 0, 0,
     "Print additional detailed information.", 37},
    { 0, }
  };

static struct argp argp = { options, parse_opt, args_doc, doc };

static error_t 
parse_opt (int key, char *arg, struct argp_state *state)
{
  struct ipmi_sensors_config_arguments *cmd_args = state->input;
  struct config_keypair *kp = NULL;
  struct config_section_str *sstr = NULL;
  char *section_name = NULL;
  char *key_name = NULL;
  char *value = NULL;
  error_t ret;
  
  switch (key)
    {
    case CHECKOUT_KEY:
      if (!cmd_args->config_args.action)
        cmd_args->config_args.action = CONFIG_ACTION_CHECKOUT;
      else
        cmd_args->config_args.action = -1;
      break;
    case COMMIT_KEY:
      if (!cmd_args->config_args.action)
        cmd_args->config_args.action = CONFIG_ACTION_COMMIT;
      else
        cmd_args->config_args.action = -1;
      break;
    case DIFF_KEY:
      if (!cmd_args->config_args.action)
        cmd_args->config_args.action = CONFIG_ACTION_DIFF;
      else
        cmd_args->config_args.action = -1;
      break;
    case FILENAME_KEY:
      if (cmd_args->config_args.filename) /* If specified more than once */
        free (cmd_args->config_args.filename);
      if (!(cmd_args->config_args.filename = strdup (arg)))
        {
          perror("strdup");
          exit(1);
        }
      break;
    case KEYPAIR_KEY:
      if (config_keypair_parse_string(arg,
                                      &section_name,
                                      &key_name,
                                      &value) < 0)
        {
          /* error printed in function call */
          exit(1);
        }
      if (!(kp = config_keypair_create(section_name,
                                       key_name,
                                       value)))
        {
          fprintf(stderr,
                  "config_keypair_create error\n");
          exit(1);
        }
      if (config_keypair_append(&(cmd_args->config_args.keypairs),
                                kp) < 0)
        {
          /* error printed in function call */
          exit(1);
        }
      if (section_name)
        free(section_name);
      section_name = NULL;
      if (key_name)
        free(key_name);
      key_name = NULL;
      if (value)
        free(value);
      value = NULL;
      kp = NULL;
      break;
    case SECTIONS_KEY:
      if (!(sstr = config_section_str_create(arg)))
        {
          fprintf(stderr,
                  "config_section_str_create error\n");
          exit(1);
        }
      if (config_section_str_append(&(cmd_args->config_args.section_strs),
                                    sstr) < 0)
        {
          /* error printed in function call */
          exit(1);
        }
      sstr = NULL;
      break;
    case LIST_SECTIONS_KEY:
      if (!cmd_args->config_args.action)
        cmd_args->config_args.action = CONFIG_ACTION_LIST_SECTIONS;
      else
        cmd_args->config_args.action = -1;
      break;
    case VERBOSE_KEY:
      cmd_args->config_args.verbose = 1;
      break;
    case ARGP_KEY_ARG:
      /* Too many arguments. */
      argp_usage (state);
      break;
    case ARGP_KEY_END:
      break;
    default:
      ret = common_parse_opt (key, arg, state, &(cmd_args->config_args.common));
      if (ret == ARGP_ERR_UNKNOWN)
        ret = sdr_parse_opt (key, arg, state, &(cmd_args->sdr));
      return ret;
    }
  
  return 0;
}

void 
ipmi_sensors_config_argp_parse (int argc, char **argv, struct ipmi_sensors_config_arguments *cmd_args)
{
  init_common_cmd_args (&(cmd_args->config_args.common));
  init_sdr_cmd_args (&(cmd_args->sdr));
  cmd_args->config_args.action = 0;
  cmd_args->config_args.verbose = 0;
  cmd_args->config_args.filename = NULL;
  cmd_args->config_args.keypairs = NULL;
  cmd_args->config_args.section_strs = NULL;

  /* OPERATOR is minimum for ipmi-sensors-config b/c its needed for many of the
   * ipmi cmds used
   */
  cmd_args->config_args.common.privilege_level = IPMI_PRIVILEGE_LEVEL_OPERATOR;
  argp_parse (&argp, argc, argv, ARGP_IN_ORDER, NULL, cmd_args);
  verify_sdr_cmd_args (&(cmd_args->sdr));
  verify_common_cmd_args (&(cmd_args->config_args.common));
}

int
ipmi_sensors_config_args_validate (struct ipmi_sensors_config_arguments *cmd_args)
{
  int ret = 0;

  // action is non 0 and -1
  if (! cmd_args->config_args.action || cmd_args->config_args.action == -1)
    {
      fprintf (stderr,
               "Exactly one of --checkout, --commit, --diff, or --listsections MUST be given\n");
      return -1;
    }
  
  // filename and keypair both given for checkout or diff
  if (cmd_args->config_args.filename && cmd_args->config_args.keypairs
      && cmd_args->config_args.action == CONFIG_ACTION_DIFF)
    {
      fprintf (stderr,
               "Both --filename or --keypair cannot be used\n");
      return -1;
    }

  // only one of keypairs or section can be given for checkout
  if (cmd_args->config_args.action == CONFIG_ACTION_CHECKOUT
      && (cmd_args->config_args.keypairs && cmd_args->config_args.section_strs))
    {
      fprintf (stderr,
               "Only one of --filename, --keypair, and --section can be used\n");
      return -1;
    }

  if (cmd_args->config_args.filename)
    {
      switch (cmd_args->config_args.action)
        {
        case CONFIG_ACTION_COMMIT: case CONFIG_ACTION_DIFF:
          if (access (cmd_args->config_args.filename, R_OK) != 0)
            {
              perror (cmd_args->config_args.filename);
              return -1;
            }
          break;
        case CONFIG_ACTION_CHECKOUT:
          if (access (cmd_args->config_args.filename, F_OK) == 0)
            {
              if (access (cmd_args->config_args.filename, W_OK) != 0)
                {
                  perror (cmd_args->config_args.filename);
                  return -1;
                }
            }
          else
            {
              int fd;
              fd = open (cmd_args->config_args.filename, O_CREAT, 0644);
              if (fd == -1)
                {
                  perror (cmd_args->config_args.filename);
                  return -1;
                }
              else
                {
                  close (fd);
                  unlink (cmd_args->config_args.filename);
                }
            }
          break;
        case CONFIG_ACTION_LIST_SECTIONS:
        case CONFIG_ACTION_INFO:
          /* do nothing - here to remove compile warning */
          break;
        }
    }

  return ret;
}

