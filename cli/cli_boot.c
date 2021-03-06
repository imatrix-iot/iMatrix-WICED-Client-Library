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
/** @file cli_reboot.c
 *
 *
 *
 */


#include <stdint.h>
#include <stdbool.h>

#include "wiced.h"

#include "../common.h"
#include "../device/icb_def.h"
#include "../ota_loader/ota_structure.h"
#include "../ota_loader/ota_loader.h"
#include "interface.h"

#include "cli_boot.h"
/******************************************************
 *                      Macros
 ******************************************************/

/******************************************************
 *                    Constants
 ******************************************************/

/******************************************************
 *                   Enumerations
 ******************************************************/

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
extern iMatrix_Control_Block_t icb;
/******************************************************
 *               Function Definitions
 ******************************************************/
/**
  * @brief Boot to an image in serial flash
  * @param  None
  * @retval : None
  */
void cli_boot( uint16_t arg )
{
    UNUSED_PARAMETER(arg);

    char *token;
    int bar;

    token = strtok(NULL, " " ); // Get boot image

    if( token ) {
        bar = atoi( token );
        if( ( bar == LEGACY_OTA ) || ( bar == APP0 ) || ( bar == APP1 ) || ( bar == FACTORY_RESET ) )
            reboot_to_image( bar );
        else
            imx_cli_print( "Image number must be be: %u or %u or %u or %u\r\n", FACTORY_RESET, LEGACY_OTA, APP0, APP1 );
    } else
        imx_cli_print( "Must supply boot image number\r\n" );
}
/**
  * @brief Reboot the device
  * @param  None
  * @retval : None
  */
void cli_reboot( uint16_t arg )
{
	UNUSED_PARAMETER(arg);

	char *token;
	token = strtok(NULL, " " );	// Get parameter number
	if( token ) {
		if( strcmp( token, "reset" ) == 0 ) {
			icb.boot_count = 0xFFFFFFFF;	// Next inc will be 0
//										clear_saved_status();
//										save_config();
		}
	}
	imx_cli_print( "Rebooting...\r\n" );
    wiced_rtos_delay_milliseconds( 2000 );
    wiced_framework_reboot();
}
