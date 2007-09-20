#ifndef _CONFIG_VALIDATE_H_
#define _CONFIG_VALIDATE_H_

#include <stdio.h>

#include "config-common.h"

config_validate_t config_yes_no_validate(const char *value);

config_validate_t config_number_range_three_bits(const char *value);

config_validate_t config_number_range_four_bits(const char *value);

config_validate_t config_number_range_seven_bits(const char *value);

config_validate_t config_number_range_one_byte(const char *value);

config_validate_t config_number_range_one_byte_non_zero(const char *value);

config_validate_t config_number_range_two_bytes(const char *value);

config_validate_t config_ip_address_validate(const char *value);

config_validate_t config_mac_address_validate(const char *value);

#endif /* _CONFIG_VALIDATE_H_ */
