/**
 * \file    main.h
 * 
 * \brief   Archivo de cabecera principal del proyecto. Define pines, direcciones, tamaños y posiciones de parámetros
 *          para la configuración y manejo de dispositivos como pantallas, impresoras, lectores, sensores y comunicación.
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <Update.h>
#include <DS3231.h>
#include "Keypad_I2C.h"
#include "LLS.h"
#include "I2C_Master_Device.h"
#include "MT124.h"
#include "Smartone.h"
#include "Printer.h"
#include "Display.h"
#include "Iridium.h"
#include "Screen.h"
#include "SD_Card.h"
#include "Ethernet_HTTP.h"
#include "Ethernet_SocketIO.h"
#include "Global_Client_Builder.h"
#include "Logger.h"
#include "json_parser.h"

extern "C"
{
  #include <esp_core_dump.h>
}

/*---------------- DEFINE UART PORTS ----------------*/
/**
 * \def   PORT_LLS
 * \brief Puerto UART para el sensor LLS.
 */
#define PORT_LLS        Serial1
/**
 * \def   PORT_MT124
 * \brief Puerto UART para el lector MT124.
 */
#define PORT_MT124      Serial2

/*-------- I2C SLAVE ADDRESSES ---------*/
/**
 * \def   KEYPAD_ADDR
 * \brief Dirección I2C del teclado.
 */
#define KEYPAD_ADDR     (uint8_t)0x38
/**
 * \def   PRINTER_ADDR
 * \brief Dirección I2C de la impresora.
 */
#define PRINTER_ADDR    (uint8_t)0x30
/**
 * \def   DISPLAY_ADDR
 * \brief Dirección I2C de la pantalla.
 */
#define DISPLAY_ADDR    (uint8_t)0x31
/**
 * \def   PRINTER_BOOTLOADER_ADDR
 * \brief Dirección I2C del bootloader de la impresora.
 */
#define PRINTER_BL_ADDR (uint8_t)0x40
/**
 * \def   DISPLAY_BOOTLOADER_ADDR
 * \brief Dirección I2C del bootloader de la pantalla.
 */
#define DISPLAY_BL_ADDR (uint8_t)0x41

/**
 * \def   RFID_READER_ADDR
 * \brief Dirección del lector RFID.
 */
#define RFID_READER_ADDR  (uint8_t)0x01
/**
 * \def   SMARTONE_STATE
 * \brief Estado lógico para el dispositivo Smartone.
 */
#define SMARTONE_STATE    (bool)LOW
/**
 * \def   IRIDIUM_STATE
 * \brief Estado lógico para el dispositivo Iridium.
 */
#define IRIDIUM_STATE     (bool)HIGH

/*---------------- PINS ----------------*/
/**
 * \def   CABINET_PIN
 * \brief Pin digital para el gabinete.
 */
#define CABINET_PIN      (int8_t)21

#ifndef CALIBRATION_PIN
  /**
   * \def   CALIBRATION_PIN
   * \brief Pin digital para calibración.
   */
  #define CALIBRATION_PIN  (int8_t)34
#endif

/**
 * \def   FLOWMETER_PIN_1
 * \brief Pin digital 1 para el flujómetro.
 */
#define FLOWMETER_PIN_1  (int8_t)15
/**
 * \def   FLOWMETER_PIN_2
 * \brief Pin digital 2 para el flujómetro.
 */
#define FLOWMETER_PIN_2  (int8_t)02
/**
 * \def   FLOWMETER_PIN_3
 * \brief Pin digital 3 para el flujómetro.
 */
#define FLOWMETER_PIN_3  (int8_t)00
/**
 * \def RELAY_PIN_1
 * \brief Pin digital para el primer relevador.
 */
#define RELAY_PIN_1      (int8_t)19
/**
 * \def   RELAY_PIN_2
 * \brief Pin digital para el segundo relevador.
 */
#define RELAY_PIN_2      (int8_t)18
/**
 * \def   RELAY_PIN_3
 * \brief Pin digital para el tercer relevador.
 */
#define RELAY_PIN_3      (int8_t)05

/*-------------- UART PINS -------------*/
/**
 * \def   RFID_RX_PIN
 * \brief Pin RX para el lector RFID.
 */
#define RFID_RX_PIN (int8_t)13
/**
 * \def   RFID_TX_PIN
 * \brief Pin TX para el lector RFID.
 */
#define RFID_TX_PIN (int8_t)12
/**
 * \def   LLS_RX_PIN
 * \brief Pin RX para el sensor LLS.
 */
#define LLS_RX_PIN  (int8_t)14
/**
 * \def   LLS_TX_PIN
 * \brief Pin TX para el sensor LLS.
 */
#define LLS_TX_PIN  (int8_t)27

/*-------------- SPI PINS --------------*/
/**
 * \def   CS_SD_CARD
 * \brief Pin Chip Select para la tarjeta SD.
 */
#define CS_SD_CARD       (int8_t)25
/**
 * \def   CS_W5500
 * \brief Pin Chip Select para el módulo Ethernet W5500.
 */
#define CS_W5500         (int8_t)26
/**
 * \def   SPI_MOSI
 * \brief Pin MOSI para SPI.
 */
#define SPI_MOSI         (int8_t)04
/**
 * \def   SPI_MISO
 * \brief Pin MISO para SPI.
 */
#define SPI_MISO         (int8_t)16
/**
 * \def   SPI_SCK
 * \brief Pin SCK para SPI.
 */
#define SPI_SCK          (int8_t)17

/*------------- I2C PINS ---------------*/
/**
 * \def   SDA_PIN
 * \brief Pin SDA para I2C.
 */
#define SDA_PIN          (int8_t)22
/**
 * \def   SCL_PIN
 * \brief Pin SCL para I2C.
 */
#define SCL_PIN          (int8_t)23

/*------------- RELAY STATE ---------------*/
/**
 * \def   RELAY_ON
 * \brief Estado lógico para activar el relevador.
 */
#define RELAY_ON    (bool)LOW
/**
 * \def   RELAY_OFF
 * \brief Estado lógico para desactivar el relevador.
 */
#define RELAY_OFF   (bool)HIGH

/*--------------- TAMAÑO MÁXIMO DE LOS PARÁMTETROS ---------------*/
/**
 * \def   SERVER_SIZE
 * \brief Tamaño máximo para el nombre del servidor.
 */
#define SERVER_SIZE         (uint8_t)0x40
/**
 * \def   ENDPOINT_SIZE
 * \brief Tamaño máximo para el endpoint.
 */
#define PRINTER_MAC_SIZE    (uint8_t)0x12
/**
 * \def   BUSINESS_NAME_SIZE
 * \brief Tamaño máximo para el nombre del negocio.
 */
#define BUSINESS_NAME_SIZE  (uint8_t)0x30
/**
 * \def   BUSINESS_ADDR_SIZE
 * \brief Tamaño máximo para la dirección del negocio.
 */
#define BUSINESS_ADDR_SIZE  (uint8_t)0x30
/**
 * \def   BUSINESS_AREA_SIZE
 * \brief Tamaño máximo para el área del negocio.
 */
#define BUSINESS_AREA_SIZE  (uint8_t)0x30
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
 * \def   BUSINESS_POST_SIZE
 * \brief Tamaño máximo para el código postal del negocio.
 */
#define BUSINESS_POST_SIZE  (uint8_t)0x0A
/**
 * \def   BUSINESS_RFC_SIZE
 * \brief Tamaño máximo para el RFC del negocio.
 */
#define BUSINESS_RFC_SIZE   (uint8_t)0x10
/**
 * \def   GITHUB_TOKEN_SIZE
 * \brief Tamaño máximo del token de github.
 */
#define PRODUCT_SIZE        (uint8_t)0x10
/**
 * \def   PRICE_SIZE
 * \brief Tamaño máximo para el precio.
 */
#define GITHUB_TOKEN_SIZE   (uint8_t)0x30
/**
 * \def   TICKET_NUMBER_SIZE
 * \brief Tamaño máximo para el número de ticket.
 */
#define TICKET_NUMBER_SIZE  (uint8_t)0x04
/**
 * \def   MAX_FUEL_LEVEL_SIZE
 * \brief Tamaño máximo para el nivel de combustible.
 */
#define MAX_FUEL_LEVEL_SIZE (uint8_t)0x02
/**
 * \def   PRODUCT_SIZE
 * \brief Tamaño máximo para el identificador de producto.
 */
#define PRICE_SIZE          (uint8_t)0x04
/**
 * \def   BRIGHTNESS_SIZE
 * \brief Tamaño máximo para el brillo.
 */
#define BRIGHTNESS_SIZE     (uint8_t)0x01
/**
 * \def   K_FACTOR_SIZE
 * \brief Tamaño máximo para el factor K.
 */
#define K_FACTOR_SIZE       (uint8_t)0x04
/**
 * \def   PORT_SIZE
 * \brief Tamaño máximo para el puerto.
 */
#define PORT_SIZE           (uint8_t)0x06

/**
 * \def   USER_PSWD_SIZE
 * \brief Tamaño máximo para la contraseña de usuario.
 */
#define USER_PSWD_SIZE      (uint8_t)0x05
/**
 * \def   USER_NAME_SIZE
 * \brief Tamaño máximo para el nombre de usuario.
 */
#define USER_NAME_SIZE      (uint8_t)0x20


/*---------------- POSICIÓN DE LOS PARÁMETROS EN EL ARCHIVO ----------------*/
/**
 * \def   SERVER_POS
 * \brief Posición del servidor en el archivo de parámetros.
 */
#define SERVER_POS          (uint8_t)0x00
/**
 * \def   DATA_ENDPOINT_POS
 * \brief Posición del endpoint de datos en el archivo de parámetros.
 */
#define PRINTER_MAC_POS     (uint8_t)0x01
/**
 * \def   ADMIN_PSWD_POS
 * \brief Posición de la contraseña de administrador en el archivo de parámetros.
 */
/**
 * \def   BUSINESS_NAME_POS
 * \brief Posición del nombre del negocio en el archivo de parámetros.
 */
#define BUSINESS_NAME_POS   (uint8_t)0x02
/**
 * \def   BUSINESS_ADDR_POS
 * \brief Posición de la dirección del negocio en el archivo de parámetros.
 */
#define BUSINESS_ADDR_POS   (uint8_t)0x03
/**
 * \def   BUSINESS_AREA_POS
 * \brief Posición del área del negocio en el archivo de parámetros.
 */
#define BUSINESS_AREA_POS   (uint8_t)0x04
/**
 * \def   BUSINESS_CITY_POS
 * \brief Posición de la ciudad del negocio en el archivo de parámetros.
 */
#define BUSINESS_CITY_POS   (uint8_t)0x05
/**
 * \def   BUSINESS_STATE_POS
 * \brief Posición del estado del negocio en el archivo de parámetros.
 */
#define BUSINESS_STATE_POS  (uint8_t)0x06
/**
 * \def   BUSINESS_POST_POS
 * \brief Posición del código postal del negocio en el archivo de parámetros.
 */
#define BUSINESS_POST_POS   (uint8_t)0x07
/**
 * \def   BUSINESS_RFC_POS
 * \brief Posición del RFC del negocio en el archivo de parámetros.
 */
#define BUSINESS_RFC_POS    (uint8_t)0x08

/**
 * \def   PRODUCT_POS
 * \brief Posición del identificador de producto en el archivo de parámetros.
 */
#define PRODUCT_POS         (uint8_t)0x09
/**
 * \def   NUMBER_OF_USERS_POS
 * \brief Número de usuarios que se encuentran registrados
 */
#define NUMBER_OF_USERS_POS (uint8_t)0x0A
/**
 * \def   TICKET_NUMBER_POS
 * \brief Posición del número de ticket en el archivo de parámetros.
 */
#define TICKET_NUMBER_POS   (uint8_t)0x0B
/**
 * \def   MAX_FUEL_LEVEL_POS
 * \brief Posición del nivel máximo de combustible en el archivo de parámetros.
 */
#define MAX_FUEL_LEVEL_POS  (uint8_t)0x0C

/**
 * \def   PRICE_POS
 * \brief Posición del precio en el archivo de parámetros.
 */
#define PRICE_POS           (uint8_t)0x0D
/**
 * \def   BRIGHTNESS_POS
 * \brief Posición del brillo en el archivo de parámetros.
 */
#define BRIGHTNESS_POS      (uint8_t)0x0E
/**
 * \def   K_FACTOR_POS
 * \brief Posición del factor K en el archivo de parámetros.
 */
#define K_FACTOR_POS        (uint8_t)0x0F
/**
 * \def   LLS_ASCII_POS
 * \brief Posición de los datos ASCII del sensor LLS en el archivo de parámetros.
 */
#define LLS_ASCII_POS       (uint8_t)0x10
/**
 * \def   PORT_POS
 * \brief Posición del puerto en el archivo de parámetros.
 */
#define PORT_POS            (uint8_t)0x11
/**
 * \def   OFFSET_POS
 * \brief Posición del factor de corrección.
 */
#define OFFSET_POS          (uint8_t)0x12
/**
 * \def   CLIENT_ID_POS
 * \brief Posición del ID del cliente.
 */
#define CLIENT_ID_POS       (uint8_t)0x13
/**
 * \def   PRODUCT_ID_POS
 * \brief Posicion del ID del producto.
 */
#define PRODUCT_ID_POS       (uint8_t)0x14
/**
 * \def   TOKEN_POS
 * \brief Posición del token de github
 */
#define GITHUB_TOKEN_POS        (uint8_t)0x00
/**
 * \def   ESP32_FW_VERSION_POS
 * \brief Posición de la version de firmware de la ESP32.
 */
#define ESP32_FW_VERSION_POS    (uint8_t)0x01
/**
 * \def   DISPLAY_FW_VERSION_POS
 * \brief Posición de la versión de firmware del controlador de la pantalla.
 */
#define DISPLAY_FW_VERSION_POS  (uint8_t)0x02
/**
 * \def   PRINTER_FW_VERSION_POS
 * \brief Posición de la versión de firmware del controlador de la impresora.
 */
#define PRINTER_FW_VERSION_POS  (uint8_t)0x03
/**
 * \def   ESP32_Index
 * \brief Posicion de la version de firmware en el archivo de GitHub de la ESP32.
 */
#define ESP32_Index 0
/**
 * \def   Display_Index
 * \brief Posicion de la version de firmware en el archivo de GitHub del MCU del Display.
 */
#define Display_Index 1
/**
 * \def   Printer_Index
 * \brief Posicion de la version de firmware en el archivo de GitHub del MCU de la impresora.
 */
#define Printer_Index 2

#define RTC_YEAR_INDEX    0
#define RTC_MONTH_INDEX   1
#define RTC_DAY_INDEX     2
#define RTC_HOUR_INDEX    4
#define RTC_MINUTE_INDEX  5
#define RTC_SECOND_INDEX  6

#define MAX_NUMBER_OF_USERS 32
#define MAX_NUMBER_OF_DISPATCHES 48
#define MAX_EVENT_JSON_LENGTH 256
#define MAX_UPDATE_JSON_LENGTH 64

#define TRIGGER_BIT       (1 << 0)
#define GENERAL_TASK_BIT  (1 << 1)
#define WS_TASK_BIT       (1 << 2)
#define HTTP_TASK_BIT     (1 << 3)

