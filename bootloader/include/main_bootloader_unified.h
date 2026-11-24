/*
 * main_bootloader_unified.h
 * 
 * Bootloader I2C Unificado para ATMega328PB
 * Compatible con protocolo ESP32
 * 
 * Author: [Corregido y mejorado]
 * Version: 2.0
 */

#ifndef MAIN_BOOTLOADER_H
#define MAIN_BOOTLOADER_H

#define F_CPU 8000000UL

#include <avr/io.h>
#include <avr/wdt.h>
#include <util/twi.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/atomic.h>

/* Configuración del bootloader */
#define USER_APP_START_ADDR     0x0000
#define USER_APP_END_ADDR       0x7000  // 28KB (antes del bootloader)
#define BOOTLOADER_START_ADDR   0x7000  // Bootloader en últimos 4KB
#define BOOTLOADER_SIZE         4096
#define BOOTLOADER_PAGES        32
#define BOOTLOADER_END_ADDR     0x7FFF
#define MAGIC_NUMBER            0x55AA  // Número mágico para validar firmware

/* Direcciones I2C - SE DEBE DEFINIR UNA ANTES DE COMPILAR */
#ifndef SLAVE_ADDRESS
    #error "Debe definir SLAVE_ADDRESS como 0x40 (printer) o 0x41 (display)"
    // Usar: -DSLAVE_ADDRESS=0x40 en build_flags de platformio.ini
#endif

/* Timeouts */
#define BOOTLOADER_TIMEOUT_MS   5000    // 5 segundos sin comando = arrancar app
#define I2C_TIMEOUT_MS          2500    // Timeout para operaciones I2C

/* Tamaños de buffer */
#define MAX_PAYLOAD_SIZE        128     // Tamaño de página SPM
#define I2C_BUFFER_SIZE         (MAX_PAYLOAD_SIZE + 2)

/* Comandos I2C - Compatible con ESP32 */
typedef enum
{
    I2C_WATCHDOG_RESET_CMD      = 0x00,
    I2C_FLASH_TO_ADDRESS_CMD    = 0x3C,
    I2C_CRC_CMD                 = 0x3D,
    I2C_START_FLASHING_CMD      = 0x3E,
    I2C_START_APPLICATION_CMD   = 0x3F,
} bootloader_command_t;

/* Estados del bootloader */
typedef enum
{
    STATE_IDLE,
    STATE_READY_TO_FLASH,
    STATE_FLASHING,
    STATE_VERIFYING,
    STATE_ERROR,
    STATE_SUCCESS
} bootloader_state_t;

/* Códigos de error */
typedef enum
{
    ERROR_NONE = 0,
    ERROR_INVALID_SIZE,
    ERROR_INVALID_ADDRESS,
    ERROR_WRITE_FAILED,
    ERROR_CRC_MISMATCH,
    ERROR_TIMEOUT,
    ERROR_BUFFER_OVERFLOW
} error_code_t;

/* Estructura para datos recibidos */
typedef struct 
{
    uint8_t buffer[I2C_BUFFER_SIZE];
    volatile uint16_t counter;
    volatile bool ready;
    uint8_t command;
} i2c_data_t;

/* Configuración de LEDs - Ajustar según hardware */
#ifdef PRINTER_HARDWARE
    #define LED_STATUS_DDR      DDRB
    #define LED_STATUS_PORT     PORTB
    #define LED_STATUS_PIN      PORTB3
    
    #define LED_ERROR_DDR       DDRD
    #define LED_ERROR_PORT      PORTD
    #define LED_ERROR_PIN       PORTD5
#else  // DISPLAY_HARDWARE
    #define LED_STATUS_DDR      DDRD
    #define LED_STATUS_PORT     PORTD
    #define LED_STATUS_PIN      PORTD1
    
    #define LED_ERROR_DDR       DDRD
    #define LED_ERROR_PORT      PORTD
    #define LED_ERROR_PIN       PORTD1  // Mismo LED para ambos
#endif

/* Macros para LEDs */
#define LED_STATUS_INIT()   (LED_STATUS_DDR |= (1 << LED_STATUS_PIN))
#define LED_STATUS_ON()     (LED_STATUS_PORT |= (1 << LED_STATUS_PIN))
#define LED_STATUS_OFF()    (LED_STATUS_PORT &= ~(1 << LED_STATUS_PIN))
#define LED_STATUS_TOGGLE() (LED_STATUS_PORT ^= (1 << LED_STATUS_PIN))

#define LED_ERROR_INIT()    (LED_ERROR_DDR |= (1 << LED_ERROR_PIN))
#define LED_ERROR_ON()      (LED_ERROR_PORT |= (1 << LED_ERROR_PIN))
#define LED_ERROR_OFF()     (LED_ERROR_PORT &= ~(1 << LED_ERROR_PIN))

#endif // MAIN_BOOTLOADER_H