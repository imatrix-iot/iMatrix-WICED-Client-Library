/*
 * Copyright 2017, Sierra Telecom. All Rights Reserved.
 *
 * This software, associated documentation and materials ("Software"),
 * is owned by Sierra Telecom ("Sierra") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Sierra hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Sierra's
 * integrated circuit products. Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Sierra.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Sierra
 * reserves the right to make changes to the Software without notice. Sierra
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Sierra does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Sierra product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Sierra's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Sierra against all liability.
 */
/** @file cli_status.c
 *
 *
 *
 */


#include <stdint.h>
#include <stdbool.h>
#include <stdbool.h>
#include <ctype.h>
#include "wiced.h"

#include "../storage.h"
#include "../version.h"
#include "interface.h"
#include "./ble/ble_manager.h"
#include "../coap/que_manager.h"
#include "../device/config.h"
#include "../device/hal_wifi.h"
#include "../device/hal_leds.h"
#include "../device/icb_def.h"
#include "../device/imx_LEDS.h"
#include "../device/var_data.h"
#include "../imatrix_upload/imatrix_upload.h"
#include "../wifi/wifi.h"
#include "cli_status.h"

/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define FEET_IN_1METER	    3.28084
#define VAR_PRINT_LENGTH    16
/******************************************************
 *                   Enumerations
 ******************************************************/

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/
extern IOT_Device_Config_t device_config;
extern iMatrix_Control_Block_t icb;
extern control_sensor_data_t *sd;
extern control_sensor_data_t *cd;
extern char *imx_data_types[ IMX_NO_DATA_TYPES ];
/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/

/******************************************************
 *               Function Definitions
 ******************************************************/
/**
  * @brief Print the status of the device
  * @param  None
  * @retval : None
  */
void cli_status( uint16_t arg )
{
	UNUSED_PARAMETER(arg);

	uint16_t i;
	uint32_t channel;
	int rssi, noise;
	wiced_time_t current_time;
	wiced_iso8601_time_t iso8601_time;
	wiced_utc_time_t utc_time;
    uint32_t uptime, days, hours, minutes, seconds;

    wiced_time_get_utc_time( &utc_time );
    uptime = utc_time - icb.boot_time;

    days = uptime / 86400;
    hours = ( uptime % 86400 ) / 3600 ;
    minutes = ( uptime % 3600 ) / 60;
    seconds = uptime % 60;


    imx_cli_print( "\r\nProduct Name: %s, Device Name: %s - ", device_config.product_name, device_config.device_name  );
    imx_cli_print( "Serial Number: %08lX%08lX%08lX - iMatrix assigned: %s\r\n", device_config.sn.serial1, device_config.sn.serial2, device_config.sn.serial3, device_config.device_serial_number );
    imx_cli_print( "Running iMatrix version:" );
    imx_cli_print( IMX_VERSION_FORMAT, MajorVersion, MinorVersion, IMATRIX_BUILD );
    imx_cli_print( ", Running Host version:" );
    imx_cli_print( IMX_VERSION_FORMAT, device_config.host_major_version, device_config.host_minor_version, device_config.host_build_version );

    wiced_time_get_utc_time( &utc_time );
    wiced_time_get_iso8601_time( &iso8601_time );
    imx_cli_print( "\r\nSystem UTC time is: %lu -> %.26s - ", utc_time, (char*)&iso8601_time );

    if( icb.boot_time != icb.fake_utc_boot_time ) {
        wiced_time_convert_utc_ms_to_iso8601( (wiced_utc_time_ms_t)icb.boot_time * 1000, &iso8601_time );
        imx_cli_print( "Booted at: %.26s, ", (char*)&iso8601_time );
    } else {
        wiced_time_convert_utc_ms_to_iso8601( (wiced_utc_time_ms_t)icb.fake_utc_boot_time * 1000, &iso8601_time );
        imx_cli_print( "NTP Has not set time yet, using last NTP time of: %.26s, ", (char*)&iso8601_time );
    }
    imx_cli_print( "System up time %u Days, %02lu Hours %02lu Minutes %02lu Seconds", days, hours, minutes, seconds );

    imx_cli_print( "Last NTP Updated time: %lu, Reboot Counter: %lu, Valid Config: 0x%08x\r\n", (uint32_t) device_config.last_ntp_updated_time, device_config.reboots, device_config.valid_config );
	imx_cli_print( "Device location: Longitude: %f, Latitude: %f, Elevation: %fm (%6.2fft.)\r\n", icb.longitude, icb.latitude, icb.elevation, ( icb.elevation * FEET_IN_1METER )  );

	// Display Wi Fi details from connection attempts up through the last one called by iMatrix code using the icb counters and
	// include additional wifi connection attempts since the last iMatrix connection by using wiced_..wifi_joins() functions.

	imx_cli_print( "Wi Fi Details: Successful Connections: %lu, Failed Attempts: %lu\r\n", icb.wifi_success_connect_count + wiced_successful_wifi_joins(),
	        icb.wifi_failed_connect_count + wiced_failed_wifi_joins());
	imx_cli_print( "Device is: " );
	if( icb.wifi_up == true ) {
		imx_cli_print( "Online, " );
        imx_cli_print( "IP: %u.%u.%u.%u, ",
                (unsigned int)((GET_IPV4_ADDRESS( icb.my_ip ) >> 24) & 0xFF),
                (unsigned int)((GET_IPV4_ADDRESS( icb.my_ip ) >> 16) & 0xFF),
                (unsigned int)((GET_IPV4_ADDRESS( icb.my_ip ) >>  8) & 0xFF),
                (unsigned int)((GET_IPV4_ADDRESS( icb.my_ip ) >>  0) & 0xFF ) );
		if( device_config.AP_setup_mode == true ) {
			imx_cli_print( "Access Point / Setup mode, SSID: %s, WPA2PSK: ->%s<-, Security Mode: 0x%x, ", device_config.ap_ssid, device_config.ap_wpa, device_config.ap_security_mode );
			/*
			 * Add Code to list clients
			 */
			imx_cli_print( "Client list not yet implemented" );
		} else {
			rssi = hal_get_wifi_rssi();
			noise = hal_get_wifi_rf_noise();
			channel = hal_get_wifi_channel();
    		imx_cli_print( "Secured with: " );
		    switch( device_config.st_security_mode ) {
		    	case IMX_SECURITY_8021X_EAP_TLS :
		    		imx_cli_print( "802.1X EAP TLS, " );
		    		break;
		    	case IMX_SECURITY_8021X_PEAP :
		    		imx_cli_print( "802.1X PEAP TLS, " );
		    		break;
		    	case IMX_SECURITY_WEP :
		    		imx_cli_print( "WEP, " );
		    		break;
		    	case IMX_SECURITY_WPA2 :
		    	default :
		    		imx_cli_print( "WPA2PSK, " );
		    		break;
		    }
			imx_cli_print( "Connected to SSID: %s, Channel: %lu, RSSI: %lddB, Noise: %lddB, S/N: %ddB", device_config.st_ssid, channel, rssi, noise, rssi - noise);
		}
	} else
		imx_cli_print( "Offline" );
	imx_cli_print( "\r\n" );
	for( i = 0; i < NO_PROTOCOL_STATS; i++ ) {
	    if( i == UDP_STATS )
	        imx_cli_print( "    UDP Statistics:" );
	    else if( i == TCP_STATS )
            imx_cli_print( "    TCP Statistics:" );
	    imx_cli_print( " Packets - Received: %lu, Multicast: %lu, Unicast Received: %lu, Errors: %lu",
	            icb.ip_stats[ i ].packets_received, icb.ip_stats[ i ].packets_multicast_received, icb.ip_stats[ i ].packets_unitcast_received, icb.ip_stats[ i ].packets_received_errors );
	    imx_cli_print( " Creation Failure: %lu, Fail to Send: %lu, Packets Sent: %lu, Last Receive Error: %lu\r\n",
	            icb.ip_stats[ i ].packet_creation_failure, icb.ip_stats[ i ].fail_to_send_packet, icb.ip_stats[ i ].packets_sent, icb.ip_stats[ i ].rec_error );
	}

    imx_cli_print( "AT Commands processed: %lu, Errors: %lu, Verbose mode: ", icb.AT_commands_processed, icb.AT_command_errors );
    switch( device_config.AT_verbose ) {
    case IMX_AT_VERBOSE_NONE :
        imx_cli_print( "None" );
        break;
    case IMX_AT_VERBOSE_STANDARD :
        imx_cli_print( "AT Commands & CLI Responses ONLY" );
        break;
    case IMX_AT_VERBOSE_STANDARD_STATUS :
        imx_cli_print( "AT Responses & Status Messages" );
        break;
    }

    imx_cli_print( "\r\n" );
    /*
     * Show free message pools
     */
    print_msg_errors();

    if( icb.imatrix_no_packet_avail == true )
        imx_cli_print( "*** iMatrix Out of Packets: " );
    print_free_msg_sizes();
    /*
     * Show Variable length pools
     */
    print_var_pools();
    /*
     * Use current time to determine next sample time
     */
	wiced_time_get_time( &current_time );
	/*
	 * Display status of controls
	 */
	imx_peripheral_type_t type;
	control_sensor_data_t *csd;
	imx_control_sensor_block_t *csb;
	uint16_t no_items;

	for( type = 0; type < IMX_NO_PERIPHERAL_TYPES; type++ ) {
	    if( type == IMX_CONTROLS ) {
	        csb = &device_config.ccb[ 0 ];
	        csd = &cd[ 0 ];
	    } else {
	        csb = &device_config.scb[ 0 ];
            csd = &sd[ 0 ];
	    }
        imx_cli_print( "%u %s: Current Status @: %lu mSec\r\n", ( type == IMX_CONTROLS ) ? device_config.no_controls : device_config.no_sensors, ( type == IMX_CONTROLS ) ? "Controls" : "Sensors", current_time );
        no_items = ( type == IMX_CONTROLS ) ? device_config.no_controls : device_config.no_sensors;

        for( i = 0; i < no_items; i++ ) {
            if( csb[ i ].enabled == true ) {
                imx_cli_print( "  No: %2u(%s), %32s, ID: 0x%08lx (%10lu), ", i, csb[ i ].read_only == true ? "R  " : "R/W", csb[ i ].name, csb[ i ].id, csb[ i ].id );
                if( csd[ i ].valid == true ) {
                    imx_cli_print( "Current setting: " );
                    imx_cli_print( "(%s) ", imx_data_types[ csb[ i ].data_type ] );
                    switch( csb[ i ].data_type ) {
                        case IMX_UINT32 :
                            imx_cli_print( "0x%08lx - %lu", csd[ i ].last_value.uint_32bit, csd[ i ].last_value.uint_32bit );
                            break;
                        case IMX_INT32 :
                            imx_cli_print( "%ld", csd[ i ].last_value.int_32bit );
                            break;
                        case IMX_FLOAT :
                            imx_cli_print( "%0.6f", csd[ i ].last_value.float_32bit );
                            break;
                        case IMX_VARIABLE_LENGTH :
                            imx_cli_print( "[%u] ", csd[ i ].last_value.var_data->length );
                            print_var_data( VR_DATA_STRING, csd[ i ].last_value.var_data );
                            break;
                    }
                    imx_cli_print( ", Errors: %lu, ", csd[ i ].errors );
                    if( csb[ i ].sample_rate == 0 )
                        imx_cli_print( "Event Driven" );
                    else {
                        if( csb[ i ].sample_rate >= 1000 )
                            imx_cli_print( "Sample Every: %4.1f Sec", ( (float) csb[ i ].sample_rate ) / 1000.0 );
                        else
                            imx_cli_print( "Sample Every: %5u mSec", csb[ i ].sample_rate );
                    }
                } else
                    imx_cli_print( "No Data Recorded");
                imx_cli_print( "\r\n" );
            }
        }
	}

	print_led_status();
	/*
	 * BLE Scan Status
	 */
//	print_ble_scan_results( 1 );

}

void print_var_data( var_data_types_t data_type, var_data_entry_t *var_data )
{
    uint16_t i, string_length;
    wiced_mac_t *bssid;

    if( var_data == NULL ) {
        imx_cli_print( "None" );
        return;
    }


    if( data_type == VR_DATA_STRING ) {
        /*
         * Verify it is only printable
         */
        string_length = strlen( (char * ) var_data->data );
        if( string_length == 0 ) {
            imx_cli_print( "<Empty String>" );
            return;
        }
        for( i = 0; i < string_length; i++ )
            if( !isprint( (int) var_data->data[ i ] ) ) {
                data_type = VR_DATA_BINARY;
                break;  // No its not a string
            }
    }
    switch( data_type ) {
        case VR_DATA_STRING :
            imx_cli_print( "String: %s", (char *) var_data->data );
            break;
        case VR_DATA_MAC_ADDRESS :
            bssid = ( wiced_mac_t *) var_data->data;
            imx_cli_print( " BSSID: %02x:%02x:%02x:%02x:%02x:%02x", bssid->octet[ 0 ], bssid->octet[ 1 ], bssid->octet[ 2 ], bssid->octet[ 3 ],
                            bssid->octet[ 4 ], bssid->octet[ 5 ] );
            break;
        case VR_DATA_BINARY :
        default :
            /*
             * Just print up to the first 16 Characters as Hex and Char
             */
            imx_cli_print( "Binary: " );
            for( i = 0; ( ( i < VAR_PRINT_LENGTH ) && ( i < var_data->length ) ); i++ )
                imx_cli_print( " 0x%02X", var_data->data[ i ] );
            imx_cli_print( "  \"" );
            for( i = 0; ( ( i < VAR_PRINT_LENGTH ) && ( i < var_data->length ) ); i++ )
                imx_cli_print( " %c", ( isprint( (int) var_data->data[ i ] ) == false ) ? '*' : (char) var_data->data[ i ] );
            imx_cli_print( "\"" );
            break;
    }

}
