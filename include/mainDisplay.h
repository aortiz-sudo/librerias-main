/**
 * \file    mainDisplay.h
 * 
 * \brief   Archivo de cabecera para la configuración y manejo de la pantalla principal.
 *          Define pines, buffer y dirección I2C para comunicación con maestro.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Definitions.h"
#include "Display.h"


/**
 * \def     FIRMWARE_VERSION
 * \brief   Versión del firmware para la pantalla.
*/
#define FIRMWARE_VERSION "1.0.0"  ///< Versión del firmware.
/**
 * \def     PORT_DISPLAY
 * \brief   Puerto serie utilizado para la pantalla principal.
 */
#define PORT_DISPLAY Serial

/**
 * \def     I2C_BUFFER_LEN
 * \brief   Tamaño del buffer para la comunicación I2C con la pantalla.
 */
#define I2C_BUFFER_LEN 64

/**
 * \def     I2C_SLAVE_ADDR
 * \brief   Dirección I2C para la comunicación con el maestro.
 */
#define I2C_SLAVE_ADDR (uint8_t)0x31