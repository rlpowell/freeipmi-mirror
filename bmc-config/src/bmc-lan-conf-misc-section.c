#include "bmc-config.h"
#include "bmc-common.h"
#include "bmc-config-api.h"
#include "bmc-diff.h"
#include "bmc-map.h"
#include "bmc-sections.h"


static bmc_err_t
enable_gratuitous_arps_checkout (const struct bmc_config_arguments *args,
				 const struct section *sect,
				 struct keyvalue *kv)
{
  uint8_t enable_arp;
  uint8_t reply_arp;
  bmc_err_t ret;

  ret = get_bmc_lan_conf_bmc_generated_arp_control (args->dev,
						    &enable_arp,
						    &reply_arp);
  if (ret != 0)
    return -1;

  if (kv->value)
    free (kv->value);
  
  if (enable_arp)
    {
      if (!(kv->value = strdup ("Yes")))
        {
          perror("strdup");
          return -1;
        }
    }
  else
    {
      if (!(kv->value = strdup ("No")))
        {
          perror("strdup");
          return -1;
        }
    }
  return BMC_ERR_SUCCESS;
}

static bmc_err_t
enable_gratuitous_arps_commit (const struct bmc_config_arguments *args,
			       const struct section *sect,
			       const struct keyvalue *kv)
{
  int ret;
  uint8_t enable_arp;
  uint8_t reply_arp;
  ret = get_bmc_lan_conf_bmc_generated_arp_control (args->dev,
						    &enable_arp,
						    &reply_arp);
  if (ret != 0)
    return -1;

  enable_arp = same (kv->value, "yes");

  return set_bmc_lan_conf_bmc_generated_arp_control (args->dev,
						     enable_arp,
						     reply_arp);
}

static bmc_diff_t
enable_gratuitous_arps_diff (const struct bmc_config_arguments *args,
			     const struct section *sect,
			     const struct keyvalue *kv)
{
  uint8_t enable_arp;
  uint8_t reply_arp;
  bmc_err_t rc;
  bmc_diff_t ret;

  if ((rc = get_bmc_lan_conf_bmc_generated_arp_control (args->dev,
                                                        &enable_arp,
                                                        &reply_arp)) != BMC_ERR_SUCCESS)
    {
      if (rc == BMC_ERR_NON_FATAL_ERROR)
        return BMC_DIFF_NON_FATAL_ERROR;
      return BMC_DIFF_FATAL_ERROR;
    }

  if (enable_arp == same (kv->value, "yes"))
    ret = BMC_DIFF_SAME;
  else
    {
      ret = BMC_DIFF_DIFFERENT; 
      report_diff (sect->section_name,
                   kv->key,
                   kv->value,
                   (enable_arp) ? "Yes" : "No");
    }
  return ret;
}

static bmc_validate_t
enable_gratuitous_arps_validate (const struct bmc_config_arguments *args,
				 const struct section *sect,
				 const char *value)
{
  if (value && (same (value, "yes") || same (value, "no")))
    return BMC_VALIDATE_VALID_VALUE;
  return BMC_VALIDATE_INVALID_VALUE;
}

/* reply */

static bmc_err_t
enable_arp_response_checkout (const struct bmc_config_arguments *args,
			      const struct section *sect,
			      struct keyvalue *kv)
{
  uint8_t enable_arp;
  uint8_t reply_arp;
  bmc_err_t ret;

  if ((ret = get_bmc_lan_conf_bmc_generated_arp_control (args->dev,
                                                         &enable_arp,
                                                         &reply_arp)) != BMC_ERR_SUCCESS)
    return ret;

  if (kv->value)
    free (kv->value);
  
  if (reply_arp)
    {
      if (!(kv->value = strdup ("Yes")))
        {
          perror("strdup");
          return BMC_ERR_FATAL_ERROR;
        }
    }
  else
    {
      if (!(kv->value = strdup ("No")))
        {
          perror("strdup");
          return BMC_ERR_FATAL_ERROR;
        }
    }
  return BMC_ERR_SUCCESS;
}

static bmc_err_t
enable_arp_response_commit (const struct bmc_config_arguments *args,
			    const struct section *sect,
			    const struct keyvalue *kv)
{
  uint8_t enable_arp;
  uint8_t reply_arp;
  bmc_err_t ret;
  
  if ((ret = get_bmc_lan_conf_bmc_generated_arp_control (args->dev,
                                                         &enable_arp,
                                                         &reply_arp)) != BMC_ERR_SUCCESS)
    return ret;

  reply_arp = same (kv->value, "yes");

  return set_bmc_lan_conf_bmc_generated_arp_control (args->dev,
						     enable_arp,
						     reply_arp);
}

static bmc_diff_t
enable_arp_response_diff (const struct bmc_config_arguments *args,
			  const struct section *sect,
			  const struct keyvalue *kv)
{
  uint8_t enable_arp;
  uint8_t reply_arp;
  bmc_err_t rc;
  bmc_diff_t ret;

  if ((rc = get_bmc_lan_conf_bmc_generated_arp_control (args->dev,
                                                        &enable_arp,
                                                        &reply_arp)) != BMC_ERR_SUCCESS)
    {
      if (rc == BMC_ERR_NON_FATAL_ERROR)
        return BMC_DIFF_NON_FATAL_ERROR;
      return BMC_DIFF_FATAL_ERROR;
    }

  if (reply_arp == same (kv->value, "yes"))
    ret = BMC_DIFF_SAME;
  else
    {
      ret = BMC_DIFF_DIFFERENT; 
      report_diff (sect->section_name,
                   kv->key,
                   kv->value,
                   (reply_arp) ? "Yes" : "No");
    }

  return ret;
}

static bmc_validate_t
enable_arp_response_validate (const struct bmc_config_arguments *args,
			      const struct section *sect,
			      const char *value)
{
  if (value && (same (value, "yes") || same (value, "no")))
    return BMC_VALIDATE_VALID_VALUE;
  return BMC_VALIDATE_INVALID_VALUE;
}

static bmc_err_t
gratuitous_arp_interval_checkout (const struct bmc_config_arguments *args,
				  const struct section *sect,
				  struct keyvalue *kv)
{
  uint8_t interval;
  bmc_err_t ret;

  if ((ret = get_bmc_lan_conf_gratuitous_arp_interval (args->dev,
                                                       &interval)) != BMC_ERR_SUCCESS)
    return ret;

  if (kv->value)
    free (kv->value);

  if (asprintf (&kv->value, "%d", interval) < 0)
    {
      perror("asprintf");
      return BMC_ERR_FATAL_ERROR;
    }
  return BMC_ERR_SUCCESS;
}

static bmc_err_t
gratuitous_arp_interval_commit (const struct bmc_config_arguments *args,
				const struct section *sect,
				const struct keyvalue *kv)
{
  return set_bmc_lan_conf_gratuitous_arp_interval (args->dev,
                                                   atoi (kv->value));
}

static bmc_diff_t
gratuitous_arp_interval_diff (const struct bmc_config_arguments *args,
			      const struct section *sect,
			      const struct keyvalue *kv)
{
  uint8_t interval;
  bmc_err_t rc;
  bmc_diff_t ret;

  if ((ret = get_bmc_lan_conf_gratuitous_arp_interval (args->dev,
                                                       &interval)) != BMC_ERR_SUCCESS)
    return ret;

  if (interval == atoi (kv->value))
    ret = BMC_DIFF_SAME;
  else
    {
      char num[32];
      ret = BMC_DIFF_DIFFERENT; 
      sprintf (num, "%d", interval);
      report_diff (sect->section_name,
                   kv->key,
                   kv->value,
                   num);
    }
  return ret;
}

static bmc_validate_t
gratuitous_arp_interval_validate (const struct bmc_config_arguments *args,
				  const struct section *sect,
				  const char *value)
{
  char *endptr;
  long int num;

  num = strtol (value, &endptr, 0);
  
  if (*endptr)
    return BMC_VALIDATE_INVALID_VALUE;

  if (num < 0 || num > 255)
    return BMC_VALIDATE_INVALID_VALUE;

  return BMC_VALIDATE_VALID_VALUE;
}

struct section *
bmc_lan_conf_misc_section_get (struct bmc_config_arguments *args)
{
  struct section *lan_conf_misc_section = NULL;

  if (!(lan_conf_misc_section = bmc_section_create ("Lan_Conf_Misc")))
    goto cleanup;

  if (bmc_section_add_keyvalue (lan_conf_misc_section,
				"Enable_Gratuitous_ARPs",
				"Possible values: Yes/No",
				0,
				enable_gratuitous_arps_checkout,
				enable_gratuitous_arps_commit,
				enable_gratuitous_arps_diff,
				enable_gratuitous_arps_validate) < 0)
    goto cleanup;

  if (bmc_section_add_keyvalue (lan_conf_misc_section,
				"Enable_ARP_Response",
				"Possible values: Yes/No",
				0,
				enable_arp_response_checkout,
				enable_arp_response_commit,
				enable_arp_response_diff,
				enable_arp_response_validate) < 0)
    goto cleanup;

  if (bmc_section_add_keyvalue (lan_conf_misc_section,
				"Gratuitous_ARP_Interval",
				"Give a number (x 500ms)",
				0,
				gratuitous_arp_interval_checkout,
				gratuitous_arp_interval_commit,
				gratuitous_arp_interval_diff,
				gratuitous_arp_interval_validate) < 0)
    goto cleanup;

  return lan_conf_misc_section;

 cleanup:
  if (lan_conf_misc_section)
    bmc_section_destroy(lan_conf_misc_section);
  return NULL;
}
