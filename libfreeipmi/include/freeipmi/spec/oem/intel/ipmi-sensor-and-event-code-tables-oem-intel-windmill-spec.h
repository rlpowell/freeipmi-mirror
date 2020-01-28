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

#ifndef IPMI_SENSOR_AND_EVENT_CODE_TABLES_OEM_INTEL_WINDMILL_SPEC_H
#define IPMI_SENSOR_AND_EVENT_CODE_TABLES_OEM_INTEL_WINDMILL_SPEC_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Intel Windmill
 * (Quanta Winterfell)
 * (Wiwynn Windmill)
 */

/* achu: These are Intel Node Manager events, but Intel Node Manager
 * does not exist on these boards.  So cutting & pasting into this
 * section to differentiate
 */
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_FIRMWARE_STATUS                                    0x00

#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_RECOVERY_GPIO_FORCED                   0x00
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_IMAGE_EXECUTION_FAILED                 0x01
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_FLASH_ERASE_ERROR                      0x02
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_FLASH_STATE_INFORMATION                0x03
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_INTERNAL_ERROR                         0x04
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_BMC_COLD_RESET_ERROR                   0x05
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_DIRECT_FLASH_UPDATE                    0x06
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_MANUFACTURING_ERROR                    0x07
#define IPMI_OEM_INTEL_WINDMILL_ME_FIRMWARE_HEALTH_EVENT_EVENT_DATA2_PERSISTENT_STORAGE_INTEGRITY_ERROR     0x08
/* No 0x09 - not available unlike newer Intel Node Manager */

#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA2_THROTTLING_BITMASK 0x03
#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA2_THROTTLING_SHIFT   0

#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA2_NATIVE_THROTTLING   0
#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA2_EXTERNAL_THROTTLING 1

#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA3_THROTTLING_CPU_VR_BITMASK 0xE0
#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA3_THROTTLING_CPU_VR_SHIFT   5

#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA3_THROTTLING_CHANNEL_NUMBER_BITMASK 0x18
#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA3_THROTTLING_CHANNEL_NUMBER_SHIFT   3

#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA3_THROTTLING_DIMM_BITMASK 0x07
#define IPMI_OEM_INTEL_WINDMILL_EVENT_DATA3_THROTTLING_DIMM_SHIFT   0

#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_CPU_NUMBER_BITMASK 0xE0
#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_CPU_NUMBER_SHIFT   5

#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_BITMASK 0x18
#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_SHIFT   3

#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_QPI 0
#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_LLC 1

#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_EXTRA_BITMASK 0x07
#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_EXTRA_SHIFT   0

#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_EXTRA_QPI0 0
#define IPMI_SENSOR_TYPE_PROCESSOR_EVENT_DATA2_OEM_INTEL_WINDMILL_MACHINE_CHECK_ERROR_SOURCE_EXTRA_QPI1 1

#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA2_OEM_INTEL_WINDMILL_DEVICE_NUMBER_BITMASK   0xF8
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA2_OEM_INTEL_WINDMILL_DEVICE_NUMBER_SHIFT     3

#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA2_OEM_INTEL_WINDMILL_FUNCTION_NUMBER_BITMASK 0x07
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA2_OEM_INTEL_WINDMILL_FUNCTION_NUMBER_SHIFT   0

/* These are from WiWynn doc */
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_CPU_MISMATCH                0x0057
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_CPU_SELF_TEST_FAILED        0x0058
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_CPU_CACHE_ERROR             0x0158
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_CPU_MICROCODE_UPDATE_FAILED 0x0059
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_CPU_NO_MICROCODE            0x0159
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_CPU_INTERNAL_ERROR          0x005A
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_RESET_NOT_AVAILABLE1        0x005B
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_RESET_NOT_AVAILABLE2        0x00F8
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_RECOVERY_NO_CAPSULE         0x00F9
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_SB_PWR_FLR                  0x5724
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_PEI_SB_SYSPWR_FLR               0x5723
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_CLEAR_CMOS                  0x5720
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_NB_ERROR                    0x00D1
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_ARCH_PROTOCOL_NOT_AVAILABLE 0x00D3
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_PCI_BUS_OUT_OF_RESOURCES    0x00D4
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_LEGACY_OPROM_NO_SPACE       0x00D5
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_NO_CON_OUT                  0x00D6
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_NO_CON_IN                   0x00D7
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_FLASH_UPDATE_FAILED         0x00DB
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_DXE_RESET_NOT_AVAILABLE         0x00DC
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_ME_RECOVERED_VIA_GR             0x5725

/* These are from a Quanta doc */
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_CMOS_CLEAR                                  0x5120
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_THERMAL_TRIP                                0x5122
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_SYS_PWROK_DROPS_UNEXPECTEDLY                0x5123
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_AC_LOST                                     0x5124
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_RECOVER_ME_FROM_ABNORMAL_MODE               0x5125
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_BACKUP_IMAGE_LOADED_DIRECT_FW_UPDATE_NEEDED 0x5126
#define IPMI_SENSOR_TYPE_SYSTEM_FIRMWARE_PROGRESS_OEM_INTEL_WINDMILL_POST_ERROR_CODE_ROCVER_HECI_FROM_ABNORMAL_MODE              0x5128

#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_CPU_NUMBER_BITMASK 0xE0
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_CPU_NUMBER_SHIFT   5

#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_SOURCE_BITMASK 0x07
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_SOURCE_SHIFT   0

#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_SOURCE_IRP0     0
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_SOURCE_IRP1     1
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_SOURCE_IIO_CORE 2
#define IPMI_SENSOR_TYPE_CRITICAL_INTERRUPT_EVENT_DATA3_OEM_INTEL_WINDMILL_SOURCE_VT_D     3

/* achu: doc lists bit [1], not bit [0], but given rollover is bit [8], and from testing, assuming it's bit [0] */
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_SEL_CLEAR_BITMASK 0x01
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_SEL_CLEAR_SHIFT   0

#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_SEL_ROLLOVER_BITMASK 0x10
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_SEL_ROLLOVER_SHIFT   4

#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA2_OEM_INTEL_WINDMILL_LOGICAL_RANK_BITMASK 0x03
#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA2_OEM_INTEL_WINDMILL_LOGICAL_RANK_SHIFT   0

#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA3_OEM_INTEL_WINDMILL_CPU_NUMBER_BITMASK 0xE0
#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA3_OEM_INTEL_WINDMILL_CPU_NUMBER_SHIFT   5

#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA3_OEM_INTEL_WINDMILL_CHANNEL_NUMBER_BITMASK 0x18
#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA3_OEM_INTEL_WINDMILL_CHANNEL_NUMBER_SHIFT   3

#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA3_OEM_INTEL_WINDMILL_DIMM_BITMASK 0x07
#define IPMI_SENSOR_TYPE_MEMORY_EVENT_DATA3_OEM_INTEL_WINDMILL_DIMM_SHIFT   0

#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_CHASSIS_POWER_STATUS_POWER_DOWN        0
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_CHASSIS_POWER_STATUS_POWER_CYCLE_RESET 1
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_CHASSIS_POWER_STATUS_POWER_ON          2
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_CHASSIS_POWER_STATUS_AC_LOST           4

#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_LOW_NONE_OF_THE_ABOVE 0
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_LOW_CML_ERROR         1
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_LOW_VIN_UV_FAULT      3
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_LOW_IOUT_OC_FAULT     4
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_LOW_HOTSWAP_OFF       6

#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_HIGH_POWER_GOOD   3
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_HIGH_MFR_STATUS   4
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_HIGH_INPUT_STATUS 5
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_HIGH_IOUT_STATUS  6
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_HIGH_VOUT_STATUS  7

#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_IOUT_WARN2         0
/* achu: HS_SHUTDOWN_CAUSE1 & HS_SHUTDOWN_CAUSE2 list 4 error messages
 * with <00>, <01>, <10>, & <11> listed next to them.  I have no idea
 * where these other bits come from.
 */
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_HS_SHUTDOWN_CAUSE1 1
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_HS_SHUTDOWN_CAUSE2 2
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_HS_INLIM           3
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_OV_CMP_OUT         5
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_UV_CMP_OUT         6
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_MFR_SPECIFIC_FET_HEALTH_BAD     7

#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_INPUT_PIN_OP_WARN  0
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_INPUT_VIN_UV_FAULT 4
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_INPUT_VIN_UV_WARN  5
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_INPUT_VIN_OV_WARN  6
#define IPMI_SENSOR_TYPE_OEM_INTEL_WINDMILL_HOT_SWAP_CONTROLLER_0_STATUS_INPUT_VIN_OV_FAULT 7

#ifdef __cplusplus
}
#endif

#endif /* IPMI_SENSOR_AND_EVENT_CODE_TABLES_OEM_INTEL_WINDMILL_SPEC_H */