#ifndef GLOBALDEF_H
#define GLOBALDEF_H

/*============================================================================*/
// Return Code

// Success
#define XVD_API_SUCCESS											0

// The detector is not connected
#define XVD_API_ERROR_DETECTOR_DISCONNECTED						1

// No need to stop because the detector is not exposing
#define XVD_API_ERROR_NO_NEED_TO_STOP_EXPOSURE					2

// The detector already exposed and is receiving image data
#define XVD_API_ERROR_DETECTOR_ALREADY_EXPOSED					3

// Failed to send STOP_EXPOSURE command
#define XVD_API_ERROR_FAILED_TO_SEND_STOP_EXPOSURE_COMMAND		4

// Failed to turn on detector
#define XVD_API_ERROR_FAILED_TO_TURN_ON_DETECTOR				5

// Failed to turn off detector
#define XVD_API_ERROR_FAILED_TO_TURN_OFF_DETECTOR				6

// Failed to configure detector
#define XVD_API_ERROR_FAILED_TO_CONFIGURE_DETECTOR				7

// Failed to send EXPOSURE command
#define XVD_API_ERROR_FAILED_TO_SEND_EXPOSURE_COMMAND			8

// Failed to create internal receive image data thread
#define XVD_API_ERROR_FAILED_TO_CREATE_RECV_IMAGE_DATA_THREAD	9

// Failed to get detector id
#define XVD_API_ERROR_FAILED_TO_GET_DETECTOR_ID					10

// Unsupported detector type
#define XVD_API_ERROR_UNSUPPORTED_DETECTOR_TYPE					11

// Image process module is not loaded
#define XVD_API_ERROR_IMAGE_PROCESS_MODULE_NOT_LOADED			12

// Failed to load calibration file
#define	XVD_API_ERROR_FAILED_TO_LOAD_CALIBRATION_FILE			13

// Calibration file does not exist
#define XVD_API_ERROR_CALIBRATION_FILE_NOT_FOUND				14

// Failed to set low power mode for the detector
#define XVD_API_ERROR_FAILED_TO_SET_LOW_POWER_MODE				15

// Failed to init detector, reason: Configuration file doesn't exist
#define XVD_API_ERROR_CFG_FILE_NOT_FOUND						16

// Failed to start device detect thread
#define XVD_API_ERROR_FAILED_TO_START_DEVICE_DETECT_THREAD		17

// Failed to generate calibration file, reason: Input parameter is NULL
#define XVD_API_ERROR_FAILED_TO_GENERATE_CALIBRATION_FILE_INPUT_PARAM_ERROR	18

// Failed to generate calibration file, reason: No raw files or failed to open raw files
#define XVD_API_ERROR_FAILED_TO_GENERATE_CALIBRATION_FILE_RAW_FILE_ERROR	19

// Failed to generate calibration file, reason: Algorithm returns error
#define XVD_API_ERROR_FAILED_TO_GENERATE_CALIBRATION_FILE_BY_ALGORITHM		20


/*============================================================================*/

#endif // GLOBALDEF_H
