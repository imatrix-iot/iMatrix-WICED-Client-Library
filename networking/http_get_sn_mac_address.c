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
 * so agrees to indemnity Sierra against all liability.
 */
/*
 * http_get_sn_mac_address.c
 *
 *  Created on: April 11, 2017
 *      Author: greg.phillips
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <ctype.h>

#include "wiced.h"
#include "spi_flash.h"
#include "wiced_apps_common.h"

#include "../defines.h"
#include "../system.h"
#include "../system.h"
#include "../hal.h"
#include "../cli/interface.h"
#include "../device/config.h"
#include "../device/dcb_def.h"
#include "../json/mjson.h"
#include "../hal_support.h"
#include "../ota_loader/ota_structure.h"
#include "utility.h"
#include "http_get_sn_mac_address.h"



/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/
#define MAC_LENGTH	20
/******************************************************
 *                   Enumerations
 ******************************************************/
enum load_power_t {
	GET_MAC_DNS,
	GET_MAC_OPEN_SOCKET,
	GET_MAC_ESTABLISH_CONNECTION,
	GET_MAC_SEND_REQUEST,
	GET_MAC_PARSE_HEADER,
	GET_MAC_RECEIVE_STREAM,
	GET_MAC_DATA_TIMEOUT,
	GET_MAC_CLOSE_CONNECTION,
	GET_MAC_CLOSE_SOCKET,
	GET_MAC_DONE,
};

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/

/******************************************************
 *               Function Declarations
 ******************************************************/

/******************************************************
 *               Variable Definitions
 ******************************************************/
extern struct OTA_CONFIGURATION ota_loader_config;
extern IOT_Device_Config_t device_config;
extern dcb_t dcb;
/******************************************************
 *               Function Definitions
 ******************************************************/
void get_sn_mac( uint16_t arg )
{
	UNUSED_PARAMETER(arg);

	/*
	 *	Get the Serial Number and MAC from the server
	 */
	if( http_get_sn_mac_address() == true ) {
	    platform_dct_wifi_config_t* dct_wifi = NULL;
	    dct_wifi = (platform_dct_wifi_config_t*) wiced_dct_get_current_address( DCT_WIFI_CONFIG_SECTION );

		cli_print( "Successfully got SN: %s, PW: %s, and MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", device_config.device_serial_number, device_config.password,
	            dct_wifi->mac_address.octet[ 0 ],
	            dct_wifi->mac_address.octet[ 1 ],
	            dct_wifi->mac_address.octet[ 2 ],
	            dct_wifi->mac_address.octet[ 3 ],
	            dct_wifi->mac_address.octet[ 4 ],
	            dct_wifi->mac_address.octet[ 5 ] );
	} else {
		cli_print( "failed to get SN, PW and MAC\r\n" );
	}

}
/**
  * @brief	HTTP GET the power settings from the database
  * @param  Part Number, power levels for off, 50%, 100% and pointer to status
  * @retval : true - Done - false - Still processing
  */
uint16_t http_get_sn_mac_address( void )
{
	int values[6];
	int i;
	uint32_t buffer_length;
	uint16_t state, retry_count;
	wiced_result_t result;
	char local_buffer[ BUFFER_LENGTH ];
	char *content_start, *content_end, ch;
	char string_mac_address[ MAC_LENGTH ];
	char string_serial_number[ DEVICE_SERIAL_NUMBER_LENGTH + 1 ];
	char string_password[ PASSWORD_LENGTH + 1 ];
    struct json_attr_t json_attrs[] = {
            {"mac",  t_string, .addr.string = string_mac_address, .len = MAC_LENGTH },
            {"sn",  t_string, .addr.string = string_serial_number, .len = DEVICE_SERIAL_NUMBER_LENGTH },
            {"pw",  t_string, .addr.string = string_password, .len = PASSWORD_LENGTH },
            {NULL}
    };

    if( dcb.wifi_up == false ) {
    	print_status( "Wi-Fi Offline, retry when online\r\n" );
    	return WICED_ERROR;
    }
	state = GET_MAC_DNS;
	retry_count = 0;
	ota_loader_config.good_get_power = false;

	while( true ) {
		switch( state ) {
			case GET_MAC_DNS :
			    print_status( "DNS Lookup for site: %s\r\n", get_manufacturing_site() );
			    if( get_site_ip( get_manufacturing_site(), &ota_loader_config.address ) == true ) {
                    print_status("IP address %lu.%lu.%lu.%lu\r\n",
                            ( ota_loader_config.address.ip.v4 >> 24) & 0xFF,
                            ( ota_loader_config.address.ip.v4 >> 16) & 0xFF,
                            ( ota_loader_config.address.ip.v4 >> 8 ) & 0xFF,
                            ( ota_loader_config.address.ip.v4 & 0xFF) );
                    state = GET_MAC_OPEN_SOCKET;
                    break;
				} else {
					print_status( "Failed to get IP address, aborting getting SN & MAC Address for %s\r\n", device_config.product_name );
					return false;
				}
				break;
			case GET_MAC_OPEN_SOCKET :
				result = wiced_tcp_create_socket( &ota_loader_config.socket, WICED_STA_INTERFACE );
				if( result != WICED_TCPIP_SUCCESS ) {
					print_status( "Failed to create socket on STA Interface, aborting\r\n" );
					return false;
				}
				state = GET_MAC_ESTABLISH_CONNECTION;
				break;
			case GET_MAC_ESTABLISH_CONNECTION :
				retry_count = 0;
				while( retry_count < MAX_CONNECTION_RETRY_COUNT ) {
					result = wiced_tcp_connect( &ota_loader_config.socket, &ota_loader_config.address, 80, 1000 );
					if( result == WICED_TCPIP_SUCCESS ) {
						print_status( "Successfully Connected to: %s on Port: 80\r\n", get_manufacturing_site() );
						/*
						 * Create the stream
						 */
						result = wiced_tcp_stream_init( &ota_loader_config.tcp_stream, &ota_loader_config.socket );
						if( result == WICED_TCPIP_SUCCESS ) {
							state = GET_MAC_SEND_REQUEST;
							break;
						} else {
							print_status( "Failed to connect to stream\r\n" );
						}
					}
				}
				if( retry_count >= MAX_CONNECTION_RETRY_COUNT ) {
					print_status( "Failed to Connect to: %s on Port: 80, aborting OTA loader\r\n", get_manufacturing_site() );
					state = GET_MAC_CLOSE_SOCKET;
				}
				break;
			case GET_MAC_SEND_REQUEST :		// Create query request and send
				memset( local_buffer, 0x00, BUFFER_LENGTH );
				strcat( local_buffer, "GET /device?" );
				sprintf( &local_buffer[ strlen( local_buffer ) ], "cpuid=0x%08lX%08lX%08lX&productid=0x%08lX", device_config.sn.serial1, device_config.sn.serial2, device_config.sn.serial3, device_config.product_id );
				strcat( local_buffer, " HTTP/1.1\nHost: " );
				strcat( local_buffer, get_manufacturing_site() );
				strcat( local_buffer, "\r\n\r\n" );
			    print_status( "Sending query: %s\r\n", local_buffer );
			    result = wiced_tcp_stream_write( &ota_loader_config.tcp_stream, local_buffer , (uint32_t) strlen( local_buffer ) );
			    if ( result == WICED_TCPIP_SUCCESS ) {
			    	result =  wiced_tcp_stream_flush( &ota_loader_config.tcp_stream );
			    	if( result == WICED_TCPIP_SUCCESS ) {
			    		wiced_time_get_utc_time( &ota_loader_config.last_recv_packet_utc_time );
			    		ota_loader_config.data_retry_count = 0;
			    		state = GET_MAC_PARSE_HEADER;
			    		break;
			    	}
			    }
			    if( result != WICED_TCPIP_SUCCESS ) {
			    	print_status( "FAILED to send request.\r\n" );
			    	state = GET_MAC_CLOSE_SOCKET;
			    }
			    break;
			case GET_MAC_PARSE_HEADER :
				result = wiced_tcp_stream_read_with_count( &ota_loader_config.tcp_stream, local_buffer, BUFFER_LENGTH, 2000, &buffer_length );
				if( result == WICED_TCPIP_SUCCESS ) { // Got some data process it - This will be the header
					print_status( "\r\nReceived: %lu Bytes\r\n", buffer_length );
					/*
					uint32_t i;
					for( i = 0; i < buffer_length; i++ )
						if( isprint( (uint16_t ) local_buffer[ i ] ) || ( local_buffer[ i ] == '\r' ) || ( local_buffer[ i ] == '\n' ) )
							print_status( "%c", local_buffer[ i ] );
						else
							print_status( "." );
					*/
					/*
					 * Look for 200 OK, Content length and two CR/LF - make sure we are getting what we asked for
					 */
					if( strstr( local_buffer, HTTP_RESPONSE_GOOD ) && strstr( local_buffer, CRLFCRLF ) ) {
						/*
						 * Header has all elements we need. Pull out the content length and save off the rest
						 */
						if( strstr( local_buffer, ACCEPT_RANGES ) )
							ota_loader_config.accept_ranges = true;
						else
							ota_loader_config.accept_ranges = false;
						/*
						 * Find the end of the header
						 */
						content_start = strstr( local_buffer, "{" );
						content_end = strstr( local_buffer, "}" ) + 1;
						if( content_start == (char *) 0 || content_end == (char *) 1 ) {
							print_status( "JSON not found in body\r\n" );
							state = GET_MAC_CLOSE_CONNECTION;
						} else {
							*content_end = 0x00;	// Null terminate the string

							print_status( "JSON: %s\r\n", content_start );
						    /*
						     * Process the passed URI Query
						     */
						    result = json_read_object( content_start, json_attrs, NULL );

						    if( result ) {
						        print_status( "JSON parsing failed, Result: %u\r\n", result );
						        return( false );
						    }
						    if( 6 == sscanf( string_mac_address, "%x:%x:%x:%x:%x:%x%c",
						        &values[0], &values[1], &values[2], &values[3], &values[4], &values[5], &ch ) ) {

						        platform_dct_wifi_config_t* dct_wifi = NULL;
						        platform_dct_wifi_config_t wifi;

						        dct_wifi = (platform_dct_wifi_config_t*) wiced_dct_get_current_address( DCT_WIFI_CONFIG_SECTION );
						        memmove( &wifi, dct_wifi, sizeof( platform_dct_wifi_config_t ) );

                                for( i = 0; i < 6; ++i )
                                    wifi.mac_address.octet[ i ] = (uint8_t) values[ i ];

						        result = wiced_dct_write( &wifi, DCT_WIFI_CONFIG_SECTION, 0, sizeof(platform_dct_wifi_config_t) );

						        print_status( "Setting MAC to: " );
						        print_mac_address( (wiced_mac_t*) &wifi.mac_address );
						        print_status( "\r\n" );

						        memset(  device_config.device_serial_number, 0x00, DEVICE_SERIAL_NUMBER_LENGTH );
                                memset(  device_config.password, 0x00, PASSWORD_LENGTH );
						        strncpy( device_config.device_serial_number, string_serial_number, DEVICE_SERIAL_NUMBER_LENGTH );
                                strncpy( device_config.password, string_password, PASSWORD_LENGTH );

						        save_config();
						    } else {
						        print_status( "Invalid MAC Address Product Name, ID: %s, 0x%08lx - Call for HELP +1 888 545 1007\r\n", device_config.product_name, device_config.product_id ); /* invalid mac */
						        return false;
						    }

						    /*
						     * We are done one way or the other
						     *
						     */
						    ota_loader_config.good_get_power = true;
						}
					} else if ( strstr( local_buffer, HTTP_RESPONSE_NOT_FOUND ) && strstr( local_buffer, CRLFCRLF ) )
						print_status( "MAC URL Not found\r\n" );
				}
				state = GET_MAC_CLOSE_CONNECTION;
				break;
			case GET_MAC_CLOSE_CONNECTION :
				state = GET_MAC_CLOSE_SOCKET;
				break;
			case GET_MAC_CLOSE_SOCKET :
			    wiced_tcp_disconnect( &ota_loader_config.socket );
			    wiced_tcp_delete_socket( &ota_loader_config.socket );
			    if( ota_loader_config.good_get_power ) {
			    	print_status("Got the MAC Address.\r\n");

			    	return( true );
			    }
			    else {
			    	print_status("Get MAC Address failed.\r\n");
			    	return( false );
			    }
				break;
		}
	}

	return true;
}