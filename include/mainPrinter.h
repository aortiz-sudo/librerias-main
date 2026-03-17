/**
 * \file    mainPrinter.h
 * 
 * \brief   Archivo de cabecera para la configuración y manejo de la impresora principal.
 *          Define pines, buffers, direcciones y parámetros para la comunicación I2C y UART,
 *          así como constantes para el manejo de datos de negocio y ticket.
 */

#include <Arduino.h>
#include <Wire.h>
#include "Definitions.h"
//#include "HC05.h"

/**
 * \def   PORT_SAT
 * \brief Puerto serie utilizado para comunicación con el dispositivo satelital (Smartone).
 * 
 * En plataformas AVR_ATMEGA328PB se usa Serial1, en otras se usa SoftwareSerial.
 */
#if defined (__AVR_ATMEGA328PB__)
  #define PORT_SAT Serial1
#else
  #include <SoftwareSerial.h>
  /**
   * \def   pin_smartone_rx
   * \brief Pin RX para el módulo Smartone en SoftwareSerial.
   */
  #define pin_smartone_rx (int8_t)12
  /**
   * \def   pin_smartone_tx
   * \brief Pin TX para el módulo Smartone en SoftwareSerial.
   */
  #define pin_smartone_tx (int8_t)11

  SoftwareSerial PORT_SAT(pin_smartone_rx, pin_smartone_tx);
#endif

/**
 * \def   I2C_BUFFER_LEN
 * \brief Tamaño del buffer para la comunicación I2C.
 */
#define I2C_BUFFER_LEN 64

/**
 * \def   I2C_SLAVE_ADDR
 * \brief Dirección I2C..
 */
#define I2C_SLAVE_ADDR (uint8_t)0x30

/*--- Direcciones de los datos de negocio en la memoria ---*/
/**
 * \def   BUSINESS_NAME_ADDR
 * \brief Dirección de memoria para el nombre del negocio.
 */
#define BUSINESS_NAME_ADDR    (uint8_t)0x00
/**
 * \def   BUSINESS_ADDRESS_ADDR
 * \brief Dirección de memoria para la dirección del negocio.
 */
#define BUSINESS_ADDRESS_ADDR (uint8_t)0x01
/**
 * \def   BUSINESS_AREA_ADDR
 * \brief Dirección de memoria para el área del negocio.
 */
#define BUSINESS_AREA_ADDR    (uint8_t)0x02
/**
 * \def   BUSINESS_POST_ADDR
 * \brief Dirección de memoria para el código postal del negocio.
 */
#define BUSINESS_POST_ADDR    (uint8_t)0x03
/**
 * \def   BUSINEES_RFC_ADDR
 * \brief Dirección de memoria para el RFC del negocio.
 */
#define BUSINEES_RFC_ADDR     (uint8_t)0x04

/*--- Tamaños máximos de los datos de negocio y usuario ---*/
/**
 * \def   BUSINESS_NAME_SIZE
 * \brief Tamaño máximo para el nombre del negocio.
 */
#define BUSINESS_NAME_SIZE  (uint8_t)0x20
/**
 * \def   BUSINESS_ADDR_SIZE
 * \brief Tamaño máximo para la dirección del negocio.
 */
#define BUSINESS_ADDR_SIZE  (uint8_t)0x20
/**
 * \def   BUSINESS_AREA_SIZE
 * \brief Tamaño máximo para el área del negocio.
 */
#define BUSINESS_AREA_SIZE  (uint8_t)0x20
/**
 * \def   BUSINESS_POST_SIZE
 * \brief Tamaño máximo para el código postal del negocio.
 */
#define BUSINESS_POST_SIZE  (uint8_t)0x0A
/**
 * \def   BUSINESS_CITY_SIZE
 * \brief Tamaño máximo para la ciudad del negocio.
 */
#define BUSINESS_CITY_SIZE  (uint8_t)0x20
/**
 * \def   BUSINESS_STATE_SIZE
 * \brief Tamaño máximo para el estado del negocio.
 */
#define BUSINESS_STATE_SIZE (uint8_t)0x20
/**
 * \def   BUSINESS_RFC_SIZE
 * \brief Tamaño máximo para el RFC del negocio.
 */
#define BUSINESS_RFC_SIZE   (uint8_t)0x10
/**
 * \def   USER_NAME_SIZE
 * \brief Tamaño máximo para el nombre de usuario.
 */
#define USER_NAME_SIZE      (uint8_t)0x20

/*--- Índices para los datos de ticket ---*/
/**
 * \def   LITERS_INDEX
 * \brief Índice para los litros despachados en el ticket.
 */
#define LITERS_INDEX       0
/**
 * \def   PRICE_INDEX
 * \brief Índice para el precio por litro en el ticket.
 */
#define PRICE_INDEX        1
/**
 * \def   TOTAL_AMOUNT_INDEX
 * \brief Índice para el importe total en el ticket.
 */
#define TOTAL_AMOUNT_INDEX 2
/**
 * \def  IVA_INDEX
 * \brief Índice para el IVA en el ticket.
 */
#define IVA_INDEX          3
/**
 * \def   TOTAL_INDEX
 * \brief Índice para el total con IVA en el ticket.
 */
#define TOTAL_INDEX        4

/*--- Pines para control y monitoreo de la impresora y satelital ---*/
/**
 * \def   enable_at_pin
 * \brief Pin digital para habilitar el modo AT del módulo Bluetooth.
 */
#define enable_at_pin (int8_t)5
/**
 * \def   state_pin
 * \brief Pin digital para leer el estado del módulo Bluetooth.
 */
#define state_pin     (int8_t)6
/**
 * \def   battery_pin
 * \brief Pin analógico para monitoreo de batería.
 */
#define battery_pin   (uint8_t)A2
/**
 * \def   sat_out_pin
 * \brief Pin leer salida satelital.
 */
#define sat_out_pin   (uint8_t)A0

/*--- Constantes para el divisor de voltaje ---*/
/**
 * \def   R1
 * \brief Valor de la resistencia R1 para el divisor de voltaje.
 */
#define R1 560.0
/**
 * \def   R2
 * \brief Valor de la resistencia R2 para el divisor de voltaje.
 */
#define R2 220.0