/* 
   bmc-conf2.h: BMC Config functions
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA. 
*/

#ifndef _BMC_CONFIG_API_H
#define _BMC_CONFIG_API_H

#include "bmc-config.h"
#include "bmc-common.h"

struct authentication_type
{
  uint8_t type_none;
  uint8_t type_md2;
  uint8_t type_md5;
  uint8_t type_straight_password;
  uint8_t type_oem_proprietary;
};

struct bmc_authentication_level
{
  struct authentication_type callback;
  struct authentication_type user;
  struct authentication_type operator;
  struct authentication_type admin;
  struct authentication_type oem;
};

int get_bmc_max_users (ipmi_device_t dev,
                       uint8_t *max_users);
bmc_err_t set_bmc_username (ipmi_device_t dev, 
                            uint8_t userid, 
                            uint8_t *username);
bmc_err_t set_bmc_enable_user (ipmi_device_t dev, 
                               uint8_t userid, 
                               int user_status);
bmc_err_t set_bmc_user_password (ipmi_device_t dev, 
                                 uint8_t userid, 
                                 uint8_t *password);
bmc_err_t set_bmc_user_password20 (ipmi_device_t dev, 
                                   uint8_t userid, 
                                   uint8_t *password);
bmc_err_t set_bmc_user_lan_channel_access (ipmi_device_t dev, 
                                           uint8_t userid, 
                                           uint8_t lan_user_ipmi_messaging, 
                                           uint8_t lan_user_link_authentication, 
                                           uint8_t lan_user_restricted_to_callback, 
                                           uint8_t lan_privilege_limit, 
                                           uint8_t lan_session_limit);
bmc_err_t set_bmc_user_payload_access (ipmi_device_t dev,
                                       uint8_t userid,
                                       uint8_t operation,
                                       uint8_t standard_payload_1,
                                       uint8_t standard_payload_2,
                                       uint8_t standard_payload_3,
                                       uint8_t standard_payload_4,
                                       uint8_t standard_payload_5,
                                       uint8_t standard_payload_6,
                                       uint8_t standard_payload_7,
                                       uint8_t oem_payload_0,
                                       uint8_t oem_payload_1,
                                       uint8_t oem_payload_2,
                                       uint8_t oem_payload_3,
                                       uint8_t oem_payload_4,
                                       uint8_t oem_payload_5,
                                       uint8_t oem_payload_6,
                                       uint8_t oem_payload_7);
bmc_err_t set_bmc_user_serial_channel_access (ipmi_device_t dev, 
                                              uint8_t userid, 
                                              uint8_t serial_user_ipmi_messaging, 
                                              uint8_t serial_user_link_authentication, 
                                              uint8_t serial_user_restricted_to_callback, 
                                              uint8_t serial_privilege_limit, 
                                              uint8_t serial_session_limit);
bmc_err_t set_bmc_lan_channel_volatile_access (ipmi_device_t dev, 
                                               uint8_t access_mode, 
                                               uint8_t user_level_authentication, 
                                               uint8_t per_message_authentication, 
                                               uint8_t pef_alerting, 
                                               uint8_t channel_privilege_limit);
bmc_err_t set_bmc_lan_channel_non_volatile_access (ipmi_device_t dev, 
                                                   uint8_t access_mode, 
                                                   uint8_t user_level_authentication, 
                                                   uint8_t per_message_authentication, 
                                                   uint8_t pef_alerting, 
                                                   uint8_t channel_privilege_limit);
bmc_err_t set_bmc_lan_conf_ip_address_source (ipmi_device_t dev, 
                                              uint8_t ip_address_source);
bmc_err_t set_bmc_lan_conf_ip_address (ipmi_device_t dev, 
                                       char *ip_address);
bmc_err_t set_bmc_lan_conf_mac_address (ipmi_device_t dev, 
                                        char *mac_address);
bmc_err_t set_bmc_lan_conf_subnet_mask (ipmi_device_t dev, 
                                        char *subnet_mask);
bmc_err_t set_bmc_lan_conf_default_gateway_address (ipmi_device_t dev, 
                                                    char *default_gateway_address);
bmc_err_t set_bmc_lan_conf_default_gateway_mac_address (ipmi_device_t dev, 
                                                        char *default_gateway_mac_address);
bmc_err_t set_bmc_lan_conf_backup_gateway_address (ipmi_device_t dev, 
                                                   char *backup_gateway_address);
bmc_err_t set_bmc_lan_conf_backup_gateway_mac_address (ipmi_device_t dev, 
                                                       char *backup_gateway_mac_address);
bmc_err_t set_bmc_lan_conf_vlan_id (ipmi_device_t dev, 
                                    uint32_t vlan_id,
                                    uint8_t vlan_id_enable);
bmc_err_t set_bmc_lan_conf_vlan_priority (ipmi_device_t dev, 
                                          uint8_t vlan_priority);

bmc_err_t set_bmc_lan_conf_authentication_type_enables (ipmi_device_t dev, 
                                                        struct bmc_authentication_level *bmc_authentication_level);
bmc_err_t set_bmc_lan_conf_bmc_generated_arp_control (ipmi_device_t dev, 
                                                      uint8_t bmc_generated_gratuitous_arps,
                                                      uint8_t bmc_generated_arp_responses);
bmc_err_t set_bmc_lan_conf_gratuitous_arp_interval (ipmi_device_t dev, 
                                                    uint8_t gratuitous_arp_interval);
bmc_err_t set_bmc_serial_channel_volatile_access (ipmi_device_t dev, 
                                                  uint8_t access_mode, 
                                                  uint8_t user_level_authentication, 
                                                  uint8_t per_message_authentication, 
                                                  uint8_t pef_alerting, 
                                                  uint8_t channel_privilege_limit);
bmc_err_t set_bmc_serial_channel_non_volatile_access (ipmi_device_t dev, 
                                                      uint8_t access_mode, 
                                                      uint8_t user_level_authentication, 
                                                      uint8_t per_message_authentication, 
                                                      uint8_t pef_alerting, 
                                                      uint8_t channel_privilege_limit);
bmc_err_t set_bmc_serial_conf_connection_mode (ipmi_device_t dev, 
                                               uint8_t basic_mode, 
                                               uint8_t ppp_mode, 
                                               uint8_t terminal_mode, 
                                               uint8_t connect_mode);
bmc_err_t set_bmc_serial_conf_page_blackout_interval (ipmi_device_t dev, 
                                                      uint8_t page_blackout_interval);
bmc_err_t set_bmc_serial_conf_call_retry_interval (ipmi_device_t dev, 
                                                   uint8_t call_retry_interval);
bmc_err_t set_bmc_serial_conf_ipmi_messaging_comm_settings (ipmi_device_t dev, 
                                                            uint8_t dtr_hangup, 
                                                            uint8_t flow_control, 
                                                            uint8_t bit_rate);
bmc_err_t set_pef_control (ipmi_device_t dev, 
                           uint8_t pef, 
                           uint8_t pef_event_messages, 
                           uint8_t pef_startup_delay, 
                           uint8_t pef_alert_startup_delay);
bmc_err_t set_pef_action_global_control (ipmi_device_t dev, 
                                         uint8_t alert_action, 
                                         uint8_t power_down_action, 
                                         uint8_t reset_action, 
                                         uint8_t power_cycle_action, 
                                         uint8_t oem_action, 
                                         uint8_t diagnostic_interrupt);
bmc_err_t set_pef_startup_delay (ipmi_device_t dev, 
                                 uint8_t pef_startup_delay);
bmc_err_t set_pef_alert_startup_delay (ipmi_device_t dev, 
                                       uint8_t pef_alert_startup_delay);
bmc_err_t set_bmc_power_restore_policy (ipmi_device_t dev, 
                                        uint8_t power_restore_policy);

bmc_err_t set_sol_sol_enable(ipmi_device_t dev,
                             uint8_t sol_enable);
bmc_err_t set_sol_sol_authentication(ipmi_device_t dev,
                                     uint8_t sol_privilege_level,
                                     uint8_t force_sol_payload_authentication,
                                     uint8_t force_sol_payload_encryption);
bmc_err_t set_sol_character_accumulate_interval_and_send_threshold(ipmi_device_t dev,
                                                                   uint8_t character_accumulate_interval,
                                                                   uint8_t character_send_threshold);
bmc_err_t set_sol_sol_retry(ipmi_device_t dev,
                            uint8_t retry_count,
                            uint8_t retry_interval);
bmc_err_t set_sol_sol_non_volatile_bit_rate(ipmi_device_t dev,
                                            uint8_t bit_rate);
bmc_err_t set_sol_sol_volatile_bit_rate(ipmi_device_t dev,
                                        uint8_t bit_rate);
bmc_err_t set_sol_sol_payload_port_number(ipmi_device_t dev,
                                          uint16_t port_number);

bmc_err_t set_rmcpplus_cipher_suite_id_privilege (ipmi_device_t dev,
                                                  uint8_t cipher_suite_id,
                                                  uint8_t privilege);

bmc_err_t set_k_r(ipmi_device_t dev,
                  uint8_t *k_r,
                  uint32_t k_r_len);

bmc_err_t set_k_g(ipmi_device_t dev,
                  uint8_t *k_g,
                  uint32_t k_g_len);

/***********************************************************/
bmc_err_t get_bmc_username (ipmi_device_t dev, 
                            uint8_t userid, 
                            uint8_t *username,
                            uint32_t username_len);
bmc_err_t get_bmc_user_lan_channel_access (ipmi_device_t dev, 
                                           uint8_t userid, 
                                           uint8_t *user_ipmi_messaging, 
                                           uint8_t *user_link_authentication, 
                                           uint8_t *user_restricted_to_callback, 
                                           uint8_t *privilege_limit, 
                                           uint8_t *session_limit,
                                           uint8_t *user_id_enable_status);
bmc_err_t get_bmc_user_payload_access (ipmi_device_t dev,
                                       uint8_t userid,
                                       uint8_t *standard_payload_1,
                                       uint8_t *standard_payload_2,
                                       uint8_t *standard_payload_3,
                                       uint8_t *standard_payload_4,
                                       uint8_t *standard_payload_5,
                                       uint8_t *standard_payload_6,
                                       uint8_t *standard_payload_7,
                                       uint8_t *oem_payload_0,
                                       uint8_t *oem_payload_1,
                                       uint8_t *oem_payload_2,
                                       uint8_t *oem_payload_3,
                                       uint8_t *oem_payload_4,
                                       uint8_t *oem_payload_5,
                                       uint8_t *oem_payload_6,
                                       uint8_t *oem_payload_7);
bmc_err_t get_bmc_user_serial_channel_access (ipmi_device_t dev, 
                                              uint8_t userid, 
                                              uint8_t *user_ipmi_messaging, 
                                              uint8_t *user_link_authentication, 
                                              uint8_t *user_restricted_to_callback, 
                                              uint8_t *privilege_limit, 
                                              uint8_t *session_limit,
                                              uint8_t *user_id_enable_status);
bmc_err_t get_bmc_lan_channel_volatile_access (ipmi_device_t dev, 
                                               uint8_t *access_mode, 
                                               uint8_t *user_level_authentication, 
                                               uint8_t *per_message_authentication, 
                                               uint8_t *pef_alerting, 
                                               uint8_t *privilege_limit);
bmc_err_t get_bmc_lan_channel_non_volatile_access (ipmi_device_t dev, 
                                                   uint8_t *access_mode, 
                                                   uint8_t *user_level_authentication, 
                                                   uint8_t *per_message_authentication, 
                                                   uint8_t *pef_alerting, 
                                                   uint8_t *privilege_limit);
bmc_err_t get_bmc_lan_conf_ip_address_source (ipmi_device_t dev, 
                                              uint8_t *ip_address_source);
bmc_err_t get_bmc_lan_conf_ip_address (ipmi_device_t dev, 
                                       char *ip_address);
bmc_err_t get_bmc_lan_conf_mac_address (ipmi_device_t dev, 
                                        char *mac_address);
bmc_err_t get_bmc_lan_conf_subnet_mask (ipmi_device_t dev, 
                                        char *subnet_mask);
bmc_err_t get_bmc_lan_conf_default_gateway_address (ipmi_device_t dev, 
                                                    char *default_gateway_address);
bmc_err_t get_bmc_lan_conf_default_gateway_mac_address (ipmi_device_t dev, 
                                                        char *default_gateway_mac_address);
bmc_err_t get_bmc_lan_conf_backup_gateway_address (ipmi_device_t dev, 
                                                   char *backup_gateway_address);
bmc_err_t get_bmc_lan_conf_backup_gateway_mac_address (ipmi_device_t dev, 
                                                       char *backup_gateway_mac_address);
bmc_err_t get_bmc_lan_conf_authentication_type_enables (ipmi_device_t dev, 
                                                        struct bmc_authentication_level *bmc_authentication_level);
bmc_err_t get_bmc_lan_conf_bmc_generated_arp_control (ipmi_device_t dev, 
                                                      uint8_t *gratuitous_arps, 
                                                      uint8_t *arp_response);
bmc_err_t get_bmc_lan_conf_gratuitous_arp_interval (ipmi_device_t dev, 
                                                    uint8_t *gratuitous_arp_interval);

bmc_err_t get_bmc_serial_channel_volatile_access (ipmi_device_t dev, 
                                                  uint8_t *access_mode, 
                                                  uint8_t *user_level_authentication, 
                                                  uint8_t *per_message_authentication, 
                                                  uint8_t *pef_alerting, 
                                                  uint8_t *privilege_limit);
bmc_err_t get_bmc_serial_channel_non_volatile_access (ipmi_device_t dev, 
                                                      uint8_t *access_mode, 
                                                      uint8_t *user_level_authentication, 
                                                      uint8_t *per_message_authentication, 
                                                      uint8_t *pef_alerting, 
                                                      uint8_t *privilege_limit);
bmc_err_t get_bmc_serial_conf_connection_mode (ipmi_device_t dev, 
                                               uint8_t *basic_mode, 
                                               uint8_t *ppp_mode, 
                                               uint8_t *terminal_mode, 
                                               uint8_t *connect_mode);
bmc_err_t get_bmc_serial_conf_page_blackout_interval (ipmi_device_t dev, 
                                                      uint8_t *page_blackout_interval);
bmc_err_t get_bmc_serial_conf_call_retry_interval (ipmi_device_t dev, 
                                                   uint8_t *call_retry_interval);
bmc_err_t get_bmc_serial_conf_ipmi_messaging_comm_settings (ipmi_device_t dev, 
                                                            uint8_t *dtr_hangup, 
                                                            uint8_t *flow_control, 
                                                            uint8_t *bit_rate);
bmc_err_t get_bmc_power_restore_policy (ipmi_device_t dev, 
                                        uint8_t *power_restore_policy);
bmc_err_t get_bmc_lan_conf_vlan_id (ipmi_device_t dev, 
                                    uint32_t *vlan_id,
                                    uint8_t *vlan_id_enable);
bmc_err_t get_bmc_lan_conf_vlan_priority (ipmi_device_t dev, 
                                          uint8_t *vlan_priority);
bmc_err_t get_pef_control (ipmi_device_t dev, 
                           uint8_t *pef, 
                           uint8_t *pef_event_messages, 
                           uint8_t *pef_startup_delay, 
                           uint8_t *pef_alert_startup_delay);
bmc_err_t get_pef_action_global_control (ipmi_device_t dev, 
                                         uint8_t *alert_action, 
                                         uint8_t *power_down_action, 
                                         uint8_t *reset_action, 
                                         uint8_t *power_cycle_action, 
                                         uint8_t *oem_action, 
                                         uint8_t *diagnostic_interrupt);
bmc_err_t get_pef_startup_delay (ipmi_device_t dev, 
                                 uint8_t *pef_startup_delay);
bmc_err_t get_pef_alert_startup_delay (ipmi_device_t dev, 
                                       uint8_t *pef_alert_startup_delay);
bmc_err_t get_sol_sol_enable (ipmi_device_t dev,
                              uint8_t *sol_enable);
bmc_err_t get_sol_sol_authentication (ipmi_device_t dev,
                                      uint8_t *sol_privilege_level,
                                      uint8_t *force_sol_payload_authentication,
                                      uint8_t *force_sol_payload_encryption);
bmc_err_t get_sol_character_accumulate_interval_and_send_threshold (ipmi_device_t dev,
                                                                    uint8_t *character_accumulate_interval,
                                                                    uint8_t *character_send_threshold);
bmc_err_t get_sol_sol_retry (ipmi_device_t dev,
                             uint8_t *retry_count,
                             uint8_t *retry_interval);
bmc_err_t get_sol_sol_non_volatile_bit_rate (ipmi_device_t dev,
                                             uint8_t *bit_rate);
bmc_err_t get_sol_sol_volatile_bit_rate (ipmi_device_t dev,
                                         uint8_t *bit_rate);
bmc_err_t get_sol_sol_payload_port_number (ipmi_device_t dev,
                                           uint16_t *port_number);

bmc_err_t get_rmcpplus_cipher_suite_id_privilege (ipmi_device_t dev,
                                                  uint8_t cipher_suite_id,
                                                  uint8_t *privilege);

bmc_err_t get_k_r(ipmi_device_t dev,
                  uint8_t *k_r,
                  uint32_t k_r_len);

bmc_err_t get_k_g(ipmi_device_t dev,
                  uint8_t *k_g,
                  uint32_t k_g_len);

/***********************************************************/
bmc_diff_t check_bmc_user_password (ipmi_device_t dev, 
                                    uint8_t userid, 
                                    uint8_t *password);
bmc_diff_t check_bmc_user_password20 (ipmi_device_t dev, 
                                      uint8_t userid, 
                                      uint8_t *password);
#endif
