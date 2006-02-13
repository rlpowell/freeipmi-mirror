/* 
   ipmi-error.c - IPMI error handling

   Copyright (C) 2003, 2004, 2005 FreeIPMI Core Team

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
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  

*/

#include "freeipmi.h"

#define SNPRINTF_RETURN(arg...)    \
do				   \
{				   \
  snprintf (errstr, len, arg);	   \
  return 0;                        \
} while (0)

int8_t 
ipmi_strerror_r (uint8_t cmd, 
		 uint8_t comp_code, 
		 char *errstr, 
		 size_t len)
{
  if (errstr == NULL)
    {
      errno = EINVAL;
      return (-1);
    }
  
  switch (comp_code)
    {
    case IPMI_COMP_CODE_COMMAND_SUCCESS:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_SUCCESS_STR);

    case IPMI_COMP_CODE_NODE_BUSY:
      SNPRINTF_RETURN (IPMI_COMP_CODE_NODE_BUSY_STR);

    case IPMI_COMP_CODE_COMMAND_INVALID:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_INVALID_STR);

    case IPMI_COMP_CODE_COMMAND_INVALID_FOR_LUN:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_INVALID_FOR_LUN_STR);

    case IPMI_COMP_CODE_COMMAND_TIMEOUT:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_TIMEOUT_STR);

    case IPMI_COMP_CODE_OUT_OF_SPACE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_OUT_OF_SPACE_STR);

    case IPMI_COMP_CODE_RESERVATION_CANCELLED:
      SNPRINTF_RETURN (IPMI_COMP_CODE_RESERVATION_CANCELLED_STR);

    case IPMI_COMP_CODE_REQUEST_DATA_TRUNCATED:
      SNPRINTF_RETURN (IPMI_COMP_CODE_REQUEST_DATA_TRUNCATED_STR);

    case IPMI_COMP_CODE_REQUEST_DATA_LENGTH_INVALID:
      SNPRINTF_RETURN (IPMI_COMP_CODE_REQUEST_DATA_LENGTH_INVALID_STR);

    case IPMI_COMP_CODE_REQUEST_DATA_LENGTH_LIMIT_EXCEEDED:
      SNPRINTF_RETURN (IPMI_COMP_CODE_REQUEST_DATA_LENGTH_LIMIT_EXCEEDED_STR);

    case IPMI_COMP_CODE_PARAMETER_OUT_OF_RANGE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_PARAMETER_OUT_OF_RANGE_STR);

    case IPMI_COMP_CODE_CANNOT_RETURN_REQUESTED_NUMBER_OF_BYTES:
      SNPRINTF_RETURN (IPMI_COMP_CODE_CANNOT_RETURN_REQUESTED_NUMBER_OF_BYTES_STR);

    case IPMI_COMP_CODE_REQUEST_SENSOR_DATA_OR_RECORD_NOT_PRESENT:
      SNPRINTF_RETURN (IPMI_COMP_CODE_REQUEST_SENSOR_DATA_OR_RECORD_NOT_PRESENT_STR);

    case IPMI_COMP_CODE_REQUEST_INVALID_DATA_FIELD:
      SNPRINTF_RETURN (IPMI_COMP_CODE_REQUEST_INVALID_DATA_FIELD_STR);

    case IPMI_COMP_CODE_COMMAND_ILLEGAL_FOR_SENSOR_OR_RECORD_TYPE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_ILLEGAL_FOR_SENSOR_OR_RECORD_TYPE_STR);

    case IPMI_COMP_CODE_COMMAND_CANNOT_RESPOND:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_CANNOT_RESPOND_STR);

    case IPMI_COMP_CODE_COMMAND_DUPLICATE_REQUEST:
      SNPRINTF_RETURN (IPMI_COMP_CODE_COMMAND_DUPLICATE_REQUEST_STR);

    case IPMI_COMP_CODE_SDR_UPDATE_MODE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_SDR_UPDATE_MODE_STR);

    case IPMI_COMP_CODE_FIRMWARE_UPDATE_MODE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_FIRMWARE_UPDATE_MODE_STR);

    case IPMI_COMP_CODE_BMC_INIT_MODE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_BMC_INIT_MODE_STR);

    case IPMI_COMP_CODE_DESTINATION_UNAVAILABLE:
      SNPRINTF_RETURN (IPMI_COMP_CODE_DESTINATION_UNAVAILABLE_STR);

    case IPMI_COMP_CODE_INSUFFICIENT_PRIVILEGE_LEVEL:
      SNPRINTF_RETURN (IPMI_COMP_CODE_INSUFFICIENT_PRIVILEGE_LEVEL_STR);

    case IPMI_COMP_CODE_REQUEST_PARAMETER_NOT_SUPPORTED:
      SNPRINTF_RETURN (IPMI_COMP_CODE_REQUEST_PARAMETER_NOT_SUPPORTED_STR);

    case IPMI_COMP_CODE_UNSPECIFIED_ERROR:
      SNPRINTF_RETURN (IPMI_COMP_CODE_UNSPECIFIED_ERROR_STR);
    }

  /* OEM completion codes */
  if ((comp_code >= 0x01) && (comp_code <= 0x7E))
    SNPRINTF_RETURN ("Device specific (OEM) completion code %02Xh.", comp_code);

  /* Command specific completion codes */
  if ((comp_code >= 0x80) && (comp_code <= 0xBE)) 
    {
      switch (cmd) 
	{
	case IPMI_CMD_GET_SESSION_CHALLENGE:
	  switch (comp_code)
            {
            case IPMI_COMP_CODE_INVALID_USERNAME:
              SNPRINTF_RETURN (IPMI_COMP_CODE_INVALID_USERNAME_STR);
	      
            case IPMI_COMP_CODE_NULL_USERNAME_NOT_ENABLED:
              SNPRINTF_RETURN (IPMI_COMP_CODE_NULL_USERNAME_NOT_ENABLED_STR);
            }
	  break;
	case IPMI_CMD_ACTIVATE_SESSION:
	  switch (comp_code)
            {
            case IPMI_COMP_CODE_NO_SESSION_SLOT_AVAILABLE:
              SNPRINTF_RETURN (IPMI_COMP_CODE_NO_SESSION_SLOT_AVAILABLE_STR);
	      
            case IPMI_COMP_CODE_NO_SLOT_AVAILABLE_FOR_GIVEN_USER:
              SNPRINTF_RETURN (IPMI_COMP_CODE_NO_SLOT_AVAILABLE_FOR_GIVEN_USER_STR);
	      
            case IPMI_COMP_CODE_NO_SLOT_AVAILABLE_TO_SUPPORT_USER:
              SNPRINTF_RETURN (IPMI_COMP_CODE_NO_SLOT_AVAILALBE_TO_SUPPORT_USER_STR);
	      
            case IPMI_COMP_CODE_SESSION_SEQ_NUM_OUT_OF_RANGE:
              SNPRINTF_RETURN (IPMI_COMP_CODE_SESSION_SEQ_NUM_OUT_OF_RANGE_STR);
	      
            case IPMI_COMP_CODE_INVALID_SESSION_ID:
              SNPRINTF_RETURN (IPMI_COMP_CODE_INVALID_SESSION_ID_STR);
	      
            case IPMI_COMP_CODE_EXCEEDS_PRIV_LEVEL:
              SNPRINTF_RETURN (IPMI_COMP_CODE_EXCEEDS_PRIV_LEVEL_STR);
            }
	  break;
	case IPMI_CMD_SET_SESSION_PRIV_LEVEL: 
          switch (comp_code)
            {
            case IPMI_COMP_CODE_RQ_LEVEL_NOT_AVAILABLE_FOR_USER:
              SNPRINTF_RETURN (IPMI_COMP_CODE_RQ_LEVEL_NOT_AVAILABLE_FOR_USER_STR);
            case IPMI_COMP_CODE_RQ_LEVEL_EXCEEDS_USER_PRIV_LIMIT:
              SNPRINTF_RETURN (IPMI_COMP_CODE_RQ_LEVEL_EXCEEDS_USER_PRIV_LIMIT_STR);
            case IPMI_COMP_CODE_CANNOT_DISABLE_USER_LEVEL_AUTH:
              SNPRINTF_RETURN (IPMI_COMP_CODE_CANNOT_DISABLE_USER_LEVEL_AUTH_STR);
            }
	  break;
	case IPMI_CMD_CLOSE_SESSION:
          switch (comp_code)
            {
            case IPMI_COMP_CODE_INVALID_SESSION_ID_IN_RQ:
              SNPRINTF_RETURN (IPMI_COMP_CODE_INVALID_SESSION_ID_IN_RQ_STR);
            }
	  break;
	case IPMI_CMD_DELETE_SEL_ENTRY:
          switch (comp_code)
            {
            case IPMI_COMP_CODE_SEL_OPERATION_NOT_SUPPORTED:
              SNPRINTF_RETURN (IPMI_COMP_CODE_SEL_OPERATION_NOT_SUPPORTED_STR);
            case IPMI_COMP_CODE_SEL_ERASE_IN_PROGRESS:
              SNPRINTF_RETURN (IPMI_COMP_CODE_SEL_ERASE_IN_PROGRESS_STR);
            }
	  break;
	}
      SNPRINTF_RETURN ("No error message found for command "
		       "%02Xh and completion code %02Xh.  "
		       "Please report to <freeipmi-devel@gnu.org>", 
		       cmd, 
		       comp_code);
    }
  
  SNPRINTF_RETURN ("Unknown completion code %02Xh for command %02Xh.", 
		   comp_code, 
		   cmd);
}

int8_t 
ipmi_strerror_cmd_r (fiid_obj_t obj_cmd, 
		     char *errstr, 
		     size_t len)
{
  uint8_t cmd, comp_code;
  
  if (obj_cmd == NULL || errstr == NULL)
    {
      errno = EINVAL;
      return (-1);
    }
  
  cmd = obj_cmd[0];
  comp_code = obj_cmd[1];
  
  return ipmi_strerror_r (cmd, comp_code, errstr, len); 
}

int8_t 
ipmi_kcs_strstatus_r (uint8_t status_code, 
		      char *errstr, 
		      size_t len)
{
  if (errstr == NULL)
    {
      errno = EINVAL;
      return (-1);
    }
  
  switch (status_code)
    {
    case IPMI_KCS_STATUS_NO_ERROR:
      SNPRINTF_RETURN (IPMI_KCS_STATUS_NO_ERROR_STR);
      
    case IPMI_KCS_STATUS_ABORTED_BY_CMD:
      SNPRINTF_RETURN (IPMI_KCS_STATUS_ABORTED_BY_CMD_STR);
      
    case IPMI_KCS_STATUS_ILLEGAL_CTRL_CODE:
      SNPRINTF_RETURN (IPMI_KCS_STATUS_ILLEGAL_CTRL_CODE_STR);
      
    case IPMI_KCS_STATUS_LEN_ERROR:
      SNPRINTF_RETURN (IPMI_KCS_STATUS_LEN_ERROR_STR); 
      
    case IPMI_KCS_STATUS_UNSPECIFIED_ERROR:
      SNPRINTF_RETURN (IPMI_KCS_STATUS_UNSPECIFIED_ERROR_STR); 
    }
  
  if ((status_code >= IPMI_KCS_STATUS_OEM_ERROR_BEGIN) &&
      (status_code <= IPMI_KCS_STATUS_OEM_ERROR_END))
    {
      SNPRINTF_RETURN ("OEM status code %02Xh.", status_code);
    }
  
  SNPRINTF_RETURN ("Unknown KCS interface status code %02Xh.", status_code);
};

int8_t 
mcpplus_status_strerror_r(uint8_t rmcpplus_status_code,
                          char *errstr,
                          size_t len)
{
  if (errstr == NULL)
    {
      errno = EINVAL;
      return (-1);
    }
  
  switch (rmcpplus_status_code) 
    {
    case RMCPPLUS_STATUS_NO_ERRORS:
      SNPRINTF_RETURN ("No errors.");
    case RMCPPLUS_STATUS_INSUFFICIENT_RESOURCES_TO_CREATE_A_SESSION:
      SNPRINTF_RETURN ("Insufficient resources to create a session.");
    case RMCPPLUS_STATUS_INVALID_SESSION_ID:
      SNPRINTF_RETURN ("Invalid Session ID.");
    case RMCPPLUS_STATUS_INVALID_PAYLOAD_TYPE:
      SNPRINTF_RETURN ("Invalid payload type.");
    case RMCPPLUS_STATUS_INVALID_AUTHENTICATION_ALGORITHM:
      SNPRINTF_RETURN ("Invalid authentication algorithm.");
    case RMCPPLUS_STATUS_INVALID_INTEGRITY_ALGORITHM:
      SNPRINTF_RETURN ("Invalid integrity algorithm.");
    case RMCPPLUS_STATUS_NO_MATCHING_AUTHENTICATION_PAYLOAD:
      SNPRINTF_RETURN ("No matching authentication payload.");
    case RMCPPLUS_STATUS_NO_MATCHING_INTEGRITY_PAYLOAD:
      SNPRINTF_RETURN ("No mathing integrity payload.");
    case RMCPPLUS_STATUS_INACTIVE_SESSION_ID:
      SNPRINTF_RETURN ("Inactive Session ID");
    case RMCPPLUS_STATUS_INVALID_ROLE:
      SNPRINTF_RETURN ("Invalid role.");
    case RMCPPLUS_STATUS_UNAUTHORIZED_ROLE_OR_PRIVILEGE_LEVEL_REQUESTED:
      SNPRINTF_RETURN ("Unauthorized role or privelege level requested.");
    case RMCPPLUS_STATUS_INSUFFICIENT_RESOURCES_TO_CREATE_A_SESSION_AT_THE_REQUESTED_TIME:
      SNPRINTF_RETURN ("Insufficient resources to create a session "
                       "at the requested time.");
    case RMCPPLUS_STATUS_INVALID_NAME_LENGTH:
      SNPRINTF_RETURN ("Invalid name length.");
    case RMCPPLUS_STATUS_UNAUTHORIZED_NAME:
      SNPRINTF_RETURN ("Unauthorized name.");
    case RMCPPLUS_STATUS_UNAUTHORIZED_GUID:
      SNPRINTF_RETURN ("Unauthorized GUID. (GUID that BMC submitted in "
                       "RAKP Message 2 was not accepted by remote console).");
    case RMCPPLUS_STATUS_INVALID_INTEGRITY_CHECK_VALUE:
      SNPRINTF_RETURN ("Invalid integrity check value.");
    case RMCPPLUS_STATUS_INVALID_CONFIDENTIALITY_ALGORITHM:
      SNPRINTF_RETURN ("Invalid confidentiality algorithm.");
    case RMCPPLUS_STATUS_NO_CIPHER_SUITE_MATCH_WITH_PROPOSED_SECURITY_ALGORITHMS:
      SNPRINTF_RETURN ("No Cipher Suite match with proposed security algorithms.");
    case RMCPPLUS_STATUS_ILLEGAL_OR_UNRECOGNIZED_PARAMETER:
      SNPRINTF_RETURN ("Illegal or Unrecognized parameter.");
    }

  SNPRINTF_RETURN ("Unknown rmcp+ or rakp status code %02Xh.", 
                   rmcpplus_status_code);
}
