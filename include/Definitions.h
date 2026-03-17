/**
 * \file  Definitions.h
 * 
 * \brief Definiciones generales utilizadas en el proyecto.
 */
#ifndef _DEFINITIONS_H
#define _DEFINITIONS_H
#include <stdint.h>

/*---------  Comandos I2C para el control de la pantalla y la impresora  ---------*/
/**
 * \def   I2C_WATCHDOG_RESET_CMD
 * \brief Comando I2C para reiniciar los perifericos mediante el watchdog.
 */
#define I2C_WATCHDOG_RESET_CMD             (uint8_t)0x00
/**
 * \def   I2C_PRINTER_MAC_ADDRESS_CMD
 * \brief Comando I2C para enviar la dirección MAC a la impresora.
 */
#define I2C_PRINTER_MAC_ADDRESS_CMD        (uint8_t)0x01
/**
 * \def   I2C_PRINTER_PRINTER_PRINT_CMD
 * \brief Comando I2C para imprimir el último ticket.
 */
#define I2C_PRINTER_PRINTER_PRINT_CMD      (uint8_t)0x02
/**
 * \def   I2C_PRINTER_PRINT_DAY_CMD
 * \brief Comando I2C para imprimir el ticket del día.
 */
#define I2C_PRINTER_PRINT_DAY_CMD          (uint8_t)0x03
/**
 * \def   I2C_PRINTER_STRING_CMD
 * \brief Comando I2C para imprimir una cadena en la impresora.
 */
#define I2C_PRINTER_STRING_CMD             (uint8_t)0x04
/**
 * \def   I2C_PRINTER_INT_CMD
 * \brief Comando I2C para imprimir un entero en la impresora.
 */
#define I2C_PRINTER_INT_CMD                (uint8_t)0x05
/**
 * \def   I2C_PRINTER_FLOAT_CMD
 * \brief Comando I2C para imprimir un flotante en la impresora.
 */
#define I2C_PRINTER_FLOAT_CMD              (uint8_t)0x06
/**
 * \def   I2C_PRINTER_RTC_CMD
 * \brief Comando I2C para imprimir la fecha y hora en la impresora.
 */
#define I2C_PRINTER_RTC_CMD                (uint8_t)0x07
/**
 * \def   I2C_PRINTER_PRODUCT_CMD
 * \brief Comando I2C para imprimir el producto en la impresora.
 */
#define I2C_PRINTER_PRODUCT_CMD            (uint8_t)0x08
/**
 * \def   I2C_DISPLAY_CHANGE_PIC_CMD
 * \brief Comando I2C para cambiar la imagen en la pantalla.
 */
#define I2C_DISPLAY_CHANGE_PIC_CMD         (uint8_t)0x09
/**
 * \def   I2C_DISPLAY_CHANGE_BRIGHTNESS_CMD
 * \brief Comando I2C para cambiar el brillo de la pantalla.
 */
#define I2C_DISPLAY_CHANGE_BRIGHTNESS_CMD  (uint8_t)0x0A
/**
 * \def   I2C_DISPLAY_WRITE_TO_ADDRESS_CMD
 * \brief Comando I2C para escribir datos en una dirección específica de la pantalla.
 */
#define I2C_DISPLAY_WRITE_TO_ADDRESS_CMD   (uint8_t)0x0B
/**
 * \def   I2C_DISPLAY_SYSTEM_RESET_CMD
 * \brief Comando I2C para reiniciar el sistema de la pantalla.
 */
#define I2C_DISPLAY_SYSTEM_RESET_CMD       (uint8_t)0x0C
/**
 * \def I2C_DISPLAY_TOUCH_CMD
 * \brief Comando I2C para simular un toque en la pantalla.
 */
#define I2C_DISPLAY_TOUCH_CMD              (uint8_t)0x0D
/**
 * \def  I2C_SATELLITE_CMD
 * \brief Comando I2C para comunicación con dispositivos satelitales.
 */
#define I2C_SATELLITE_CMD                  (uint8_t)0x0E
/**
 * \def   I2C_PRINTER_RESET_CMD
 * \brief Comando I2C para reiniciar la impresora.
 */
#define I2C_PRINTER_RESET_CMD              (uint8_t)0x11
/**
 * \def   I2C_FLASH_TO_ADDRESS_CMD
 * \brief Comando I2C para guardar los datos en una dirección de la memoria flash del controlador.
 * 
 */
/*--------- Comandos I2C para el Bootloader Unificado ---------*/
/**
 * \def   I2C_FLASH_TO_ADDRESS_CMD
 * \brief Comando I2C para escribir una página de 128 bytes a flash.
 */
#define I2C_FLASH_TO_ADDRESS_CMD    (uint8_t)0x3C

/**
 * \def   I2C_CRC_CMD
 * \brief Comando I2C para enviar CRC y verificar firmware.
 */
#define I2C_CRC_CMD                 (uint8_t)0x3D

/**
 * \def   I2C_START_FLASHING_CMD
 * \brief Comando I2C para iniciar sesión de flasheo.
 */
#define I2C_START_FLASHING_CMD      (uint8_t)0x3E

/**
 * \def   I2C_START_APPLICATION_CMD
 * \brief Comando I2C para arrancar la aplicación.
 */
#define I2C_START_APPLICATION_CMD   (uint8_t)0x3F

/* ----------------- Respuestas del Bootloader --------------------- */
/**
 * \def   BOOTLOADER_ACK
 * \brief Respuesta de reconocimiento positivo del bootloader.
 */
#define BOOTLOADER_ACK              (uint8_t)0x06
 
/**
 * \def   BOOTLOADER_NACK
 * \brief Respuesta de reconocimiento negativo del bootloader.
 */
#define BOOTLOADER_NACK             (uint8_t)0x15

/*-----------  Direcciones para la manipualcion de los datos mostrados en la pantalla  -----------*/
/**
 * \def   DISPLAY_RTC_ADDRESS
 * \brief Dirección de memoria para la fecha y hora en la pantalla.
 */
#define DISPLAY_RTC_ADDRESS                 (uint16_t)0x0010
/**
 * \def   DISPLAY_PASSWORD_ADDRESS
 * \brief Dirección de memoria para la contraseña en la pantalla.
 */
#define DISPLAY_PASSWORD_ADDRESS            (uint16_t)0x4000
/**
 * \def   DISPLAY_FUEL_LEVEL_ADDRESS
 * \brief Dirección de memoria para el nivel de combustible.
 */
#define DISPLAY_FUEL_LEVEL_ADDRESS          (uint16_t)0x4010
/**
 * \def   DISPLAY_WRONG_PASS_ADDRESS
 * \brief Dirección de memoria para indicar contraseña incorrecta.
 */
#define DISPLAY_WRONG_PASS_ADDRESS          (uint16_t)0x4020
/**
 * \def   DISPLAY_FUEL_TEMP_ADDRESS
 * \brief Dirección de memoria para la temperatura del combustible.
 */
#define DISPLAY_FUEL_TEMP_ADDRESS           (uint16_t)0x4100
/**
 * \def   DISPLAY_VOLTAGE_VALUE_ADDRESS
 * \brief Dirección de memoria para el valor de voltaje.
 */
#define DISPLAY_VOLTAGE_VALUE_ADDRESS       (uint16_t)0x4120
/**
 * \def   DISPLAY_VOLUME_DISPATCH_ADDRESS
 * \brief Dirección de memoria para el volumen despachado.
 */
#define DISPLAY_VOLUME_DISPATCH_ADDRESS     (uint16_t)0x4200
/**
 * \def   DISPLAY_PRICE_DISPATCH_ADDRESS
 * \brief Dirección de memoria para el precio despachado.
 */
#define DISPLAY_PRICE_DISPATCH_ADDRESS      (uint16_t)0x4210
/**
 * \def   DISPLAY_PRODUCT_ADDRESS
 * \brief Dirección de memoria para el producto.
 */
#define DISPLAY_PRODUCT_ADDRESS             (uint16_t)0x4230
/**
 * \def   DISPLAY_TAG_MESSAGE_ADDRESS
 * \brief Dirección de memoria para mensajes de tag.
 */
#define DISPLAY_TAG_MESSAGE_ADDRESS         (uint16_t)0x4240
/**
 * \def   DISPLAY_DISPATCHED_VOLUME_ADDRESS
 * \brief Dirección de memoria para el volumen despachado final.
 */
#define DISPLAY_DISPATCHED_VOLUME_ADDRESS   (uint16_t)0x4300
/**
 * \def   DISPLAY_DISPATCHED_COST_ADDRESS
 * \brief Dirección de memoria para el costo despachado final.
 */
#define DISPLAY_DISPATCHED_COST_ADDRESS     (uint16_t)0x4310
/**
 * \def   DISPLAY_NEW_HOUR_ADDRESS
 * \brief Dirección de memoria para la nueva hora.
 */
#define DISPLAY_NEW_HOUR_ADDRESS            (uint16_t)0x4400
/**
 * \def   DISPLAY_NEW_MINUTE_ADDRESS
 * \brief Dirección de memoria para el nuevo minuto.
 */
#define DISPLAY_NEW_MINUTE_ADDRESS          (uint16_t)0x4410
/**
 * \def   DISPLAY_NEW_DAY_ADDRESS
 * \brief Dirección de memoria para el nuevo día.
 */
#define DISPLAY_NEW_DAY_ADDRESS             (uint16_t)0x4500
/**
 * \def   DISPLAY_NEW_MONTH_ADDRESS
 * \brief Dirección de memoria para el nuevo mes.
 */
#define DISPLAY_NEW_MONTH_ADDRESS           (uint16_t)0x4510
/**
 * \def   DISPLAY_NEW_YEAR_ADDRESS
 * \brief Dirección de memoria para el nuevo año.
 */
#define DISPLAY_NEW_YEAR_ADDRESS            (uint16_t)0x4520
/**
 * \def   DISPLAY_CURRENT_PRICE_ADDRESS
 * \brief Dirección de memoria para el precio actual.
 */
#define DISPLAY_CURRENT_PRICE_ADDRESS       (uint16_t)0x5600
/**
 * \def   DISPLAY_NEW_PRICE_ADDRESS
 * \brief Dirección de memoria para el nuevo precio.
 */
#define DISPLAY_NEW_PRICE_ADDRESS           (uint16_t)0x5610
/**
 * \def   DISPLAY_BUSINESS_NAME_ADDRESS
 * \brief Dirección de memoria para el nombre del negocio.
 */
#define DISPLAY_BUSINESS_NAME_ADDRESS       (uint16_t)0x5700
/**
 * \def  DISPLAY_BUSINESS_RFC_ADDRESS
 * \brief Dirección de memoria para el RFC del negocio.
 */
#define DISPLAY_BUSINESS_RFC_ADDRESS        (uint16_t)0x5800
/**
 * \def   DISPLAY_BUSINESS_ADDR_ADDRESS
 * \brief Dirección de memoria para la dirección del negocio.
 */
#define DISPLAY_BUSINESS_ADDR_ADDRESS       (uint16_t)0x5900
/**
 * \def   DISPLAY_BUSINESS_POSTAL_ADDRESS
 * \brief Dirección de memoria para el código postal del negocio.
 */
#define DISPLAY_BUSINESS_POSTAL_ADDRESS     (uint16_t)0x6000
/**
 * \def DISPLAY_BUSINESS_CITY_ADDRESS
 * \brief Dirección de memoria para la ciudad del negocio.
 */
#define DISPLAY_BUSINESS_CITY_ADDRESS       (uint16_t)0x6100
/**
 * \def   DISPLAY_BUSINESS_STATE_ADDRESS
 * \brief Dirección de memoria para el estado del negocio.
 */
#define DISPLAY_BUSINESS_STATE_ADDRESS      (uint16_t)0x6200
/**
 * \def   DISPLAY_MEASURED_FLOW_ADDRESS
 * \brief Dirección de memoria para el flujo medido.
 */
#define DISPLAY_MEASURED_FLOW_ADDRESS       (uint16_t)0x7000
/**
 * \def   DISPLAY_STANDARD_FLOW_ADDRESS
 * \brief Dirección de memoria para el flujo estándar.
 */
#define DISPLAY_STANDARD_FLOW_ADDRESS       (uint16_t)0x7010
/**
 * \def DISPLAY_TICKET_INFO_ADDRESS
 * \brief Dirección de memoria para la información del ticket.
 */
#define DISPLAY_TICKET_INFO_ADDRESS         (uint16_t)0x8020
/**
 * \def   DISPLAY_BRIGHTNESS_BAR_ADDRESS
 * \brief Dirección de memoria para la barra de brillo.
 */
#define DISPLAY_BRIGHTNESS_BAR_ADDRESS      (uint16_t)0x9000

/*-----------  Numeros de las imagenes utilizadas en la pantalla  -----------*/
/**
 * \def   INITIAL_SCREEN_PIC
 * \brief Número de imagen para la pantalla inicial.
 */
#define INITIAL_SCREEN_PIC                  (uint8_t)0x00
/**
 * \def   LOG_IN_SCREEN_PIC
 * \brief Número de imagen para la pantalla de inicio de sesión.
 */
#define LOG_IN_SCREEN_PIC                   (uint8_t)0x01
/**
 * \def   MAIN_MENU_NCAL_PIC
 * \brief Número de imagen para el menú principal sin calibración.
 */
#define MAIN_MENU_NCAL_PIC                  (uint8_t)0x02
/**
 * \def   MAIN_MENU_NCAL_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú principal sin calibración.
 */
#define MAIN_MENU_NCAL_OPTION1_PIC          (uint8_t)0x03
/**
 * \def   MAIN_MENU_NCAL_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú principal sin calibración.
 */
#define MAIN_MENU_NCAL_OPTION2_PIC          (uint8_t)0x04
/**
 * \def   MAIN_MENU_NCAL_OPTION3_PIC
 * \brief Número de imagen para la opción 3 del menú principal sin calibración.
 */
#define MAIN_MENU_NCAL_OPTION3_PIC          (uint8_t)0x05
/**
 * \def   MAIN_MENU_WCAL_PIC
 * \brief Número de imagen para el menú principal con calibración.
 */
#define MAIN_MENU_WCAL_PIC                  (uint8_t)0x06
/**
 * \def   MAIN_MENU_WCAL_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú principal con calibración.
 */
#define MAIN_MENU_WCAL_OPTION1_PIC          (uint8_t)0x07
/**
 * \def   MAIN_MENU_WCAL_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú principal con calibración.
 */
#define MAIN_MENU_WCAL_OPTION2_PIC          (uint8_t)0x08
/**
 * \def   MAIN_MENU_WCAL_OPTION3_PIC
 * \brief Número de imagen para la opción 3 del menú principal con calibración.
 */
#define MAIN_MENU_WCAL_OPTION3_PIC          (uint8_t)0x09
/**
 * \def   MAIN_MENU_WCAL_OPTION4_PIC
 * \brief Número de imagen para la opción 4 del menú principal con calibración.
 */
#define MAIN_MENU_WCAL_OPTION4_PIC          (uint8_t)0x0A
/**
 * \def   DISPATCH_MENU_PIC
 * \brief Número de imagen para el menú de despacho.
 */
#define DISPATCH_MENU_PIC                   (uint8_t)0x0B
/**
 * \def   DISPATCH_MENU_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú de despacho.
 */
#define DISPATCH_MENU_OPTION1_PIC           (uint8_t)0x0C
/**
 * \def   DISPATCH_MENU_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú de despacho.
 */
#define DISPATCH_MENU_OPTION2_PIC           (uint8_t)0x0D
/**
 * \def   QUANTITY_DISPATCH_MENU_PIC
 * \brief Número de imagen para el menú de despacho por cantidad.
 */
#define QUANTITY_DISPATCH_MENU_PIC          (uint8_t)0x0F
/**
 * \def   QUANTITY_DISPATCH_MENU_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú de despacho por cantidad.
 */
#define QUANTITY_DISPATCH_MENU_OPTION1_PIC  (uint8_t)0x10
/**
 * \def   QUANTITY_DISPATCH_MENU_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú de despacho por cantidad.
 */
#define QUANTITY_DISPATCH_MENU_OPTION2_PIC  (uint8_t)0x11
/**
 * \def   DISPATCHING_PRESS_TO_START_PIC
 * \brief Número de imagen para la pantalla de "Iniciar despacho".
 */
#define DISPATCHING_PRESS_TO_START_PIC          (uint8_t)0x12
/**
 * \def   DISPATCHING_STARTED_PIC
 * \brief Número de imagen para el inicio de despacho.
 */
#define DISPATCHING_STARTED_PIC             (uint8_t)0x13
/**
 * \def   DISPATCHING_IDLE_PIC
 * \brief Número de imagen para el estado de despacho inactivo.
 */
#define DISPATCHING_IDLE_PIC                (uint8_t)0x14
/**
 * \def   DISPATCHING_FINISH_PIC
 * \brief Número de imagen para el fin de despacho.
 */
#define DISPATCHING_FINISH_PIC              (uint8_t)0x15
/**
 * \def   PRINT_TICKET_MENU_PIC
 * \brief Número de imagen para el menú de impresión de tickets.
 */
#define PRINT_TICKET_MENU_PIC               (uint8_t)0x16
/**
 * \def   PRINT_TICKET_MENU_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú de impresión de tickets.
 */
#define PRINT_TICKET_MENU_OPTION1_PIC       (uint8_t)0x17
/**
 * \def   PRINT_TICKET_MENU_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú de impresión de tickets.
 */
#define PRINT_TICKET_MENU_OPTION2_PIC       (uint8_t)0x18
/**
 * \def   CONFIGURATION_MENU_PIC
 * \brief Número de imagen para el menú de configuración.
 */
#define CONFIGURATION_MENU_PIC              (uint8_t)0x19
/**
 * \def   CONFIGURATION_MENU_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú de configuración.
 */
#define CONFIGURATION_MENU_OPTION1_PIC      (uint8_t)0x1A
/**
 * \def   CONFIGURATION_MENU_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú de configuración.
 */
#define CONFIGURATION_MENU_OPTION2_PIC      (uint8_t)0x1B
/**
 * \def   CONFIGURATION_MENU_OPTION3_PIC
 * \brief Número de imagen para la opción 3 del menú de configuración.
 */
#define CONFIGURATION_MENU_OPTION3_PIC      (uint8_t)0x1C
/**
 * \def   CONFIGURATION_MENU_OPTION4_PIC
 * \brief Número de imagen para la opción 4 del menú de configuración.
 */
#define CONFIGURATION_MENU_OPTION4_PIC      (uint8_t)0x1D
/**
 * \def   DATE_MENU_HOUR_PIC
 * \brief Número de imagen para el menú de hora.
 */
#define DATE_MENU_HOUR_PIC                  (uint8_t)0x1E
/**
 * \def   DATE_MENU_MINUTE_PIC
 * \brief Número de imagen para el menú de minutos.
 */
#define DATE_MENU_MINUTE_PIC                (uint8_t)0x1F
/**
 * \def   DATE_MENU_DAY_PIC
 * \brief Número de imagen para el menú de día.
 */
#define DATE_MENU_DAY_PIC                   (uint8_t)0x20
/**
 * \def   DATE_MENU_MONTH_PIC
 * \brief Número de imagen para el menú de mes.
 */
#define DATE_MENU_MONTH_PIC                 (uint8_t)0x21
/**
 * \def   DATE_MENU_YEAR_PIC
 * \brief Número de imagen para el menú de año.
 */
#define DATE_MENU_YEAR_PIC                  (uint8_t)0x22
/**
 * \def   PRODUCT_PRICE_PIC
 * \brief Número de imagen para el menú de precio de producto.
 */
#define PRODUCT_PRICE_PIC                   (uint8_t)0x23
/**
 * \def   TICKET_INFO_PIC
 * \brief Número de imagen para la información del ticket.
 */
#define TICKET_INFO_PIC                     (uint8_t)0x24
/**
 * \def   SCREEN_BRIGHTNESS_PIC
 * \brief Número de imagen para el menú de brillo de pantalla.
 */
#define SCREEN_BRIGHTNESS_PIC               (uint8_t)0x25
/**
 * \def   CALIBRATION_MENU_PIC
 * \brief Número de imagen para el menú de calibración.
 */
#define CALIBRATION_MENU_PIC                (uint8_t)0x26
/**
 * \def   CALIBRATION_MENU_OPTION1_PIC
 * \brief Número de imagen para la opción 1 del menú de calibración.
 */
#define CALIBRATION_MENU_OPTION1_PIC        (uint8_t)0x27
/**
 * \def   CALIBRATION_MENU_OPTION2_PIC
 * \brief Número de imagen para la opción 2 del menú de calibración.
 */
#define CALIBRATION_MENU_OPTION2_PIC        (uint8_t)0x28

/*--------- Comandos utilizados para el control de la pantalla ---------*/
/**
 * \def   WRITE_COMMAND
 * \brief Comando para escribir datos en la pantalla.
 */
#define WRITE_COMMAND               (uint8_t)0x82
/**
 * \def   READ_COMMAND
 * \brief Comando para leer datos de la pantalla.
 */
#define READ_COMMAND                (uint8_t)0x83
/**
 * \def   FRAME_HEADER_H
 * \brief Byte alto del encabezado.
 */
#define FRAME_HEADER_H              (uint8_t)0x5A
/**
 * \def   FRAME_HEADER_L
 * \brief Byte bajo del encabezado.
 */
#define FRAME_HEADER_L              (uint8_t)0xA5
/**
 * \def   SYSTEM_RESET_CMD_H
 * \brief Comando alto para resetear el sistema.
 */
#define SYSTEM_RESET_CMD_H          (uint16_t)0x55AA
/**
 * \def   SYSTEM_RESET_CMD_L
 * \brief Comando bajo para resetear el sistema.
 */
#define SYSTEM_RESET_CMD_L          (uint16_t)0x5AA5
/**
 * \def   SET_CRC_TRUE_CMD
 * \brief Comando para activar el CRC.
 */
#define SET_CRC_TRUE_CMD            (uint16_t)0x5A80
/**
 * \def   SET_CRC_FALSE_CMD
 * \brief Comando para desactivar el CRC.
 */
#define SET_CRC_FALSE_CMD           (uint16_t)0x5A00
/**
 * \def   ENABLE_PAGE_SWITCH_CMD
 * \brief Comando para habilitar el cambio de página.
 */
#define ENABLE_PAGE_SWITCH_CMD      (uint16_t)0x5A01
/**
 * \def   ENABLE_TOUCH_SIM_CMD
 * \brief Comando para habilitar la simulación de touch.
 */
#define ENABLE_TOUCH_SIM_CMD        (uint16_t)0x5AA5

/*--------- Dirección de los regristros utilizado para el control de la pantalla ---------*/
/**
 * \def   SYSTEM_RESET_REG
 * \brief Dirección de registro para resetear el sistema.
 */
#define SYSTEM_RESET_REG            (uint16_t)0x0004
/**
 * \def   UART_CONFIG_REG
 * \brief Dirección de registro para configuración UART.
 */
#define UART_CONFIG_REG             (uint16_t)0x000C
/**
 * \def   RTC_CONFIG_REG
 * \brief Dirección de registro para configuración RTC.
 */
#define RTC_CONFIG_REG              (uint16_t)0x0010
/**
 * \def   PIC_NOW_REG
 * \brief Dirección de registro para la imagen actual.
 */
#define PIC_NOW_REG                 (uint16_t)0x0014
/**
 * \def   LCD_HOR_REG
 * \brief Dirección de registro para la resolución horizontal.
 */
#define LCD_HOR_REG                 (uint16_t)0x007A
/**
 * \def   LCD_VER_REG
 * \brief Dirección de registro para la resolución vertical.
 */
#define LCD_VER_REG                 (uint16_t)0x007B
/**
 * \def   SYSTEM_CONFIG_REG
 * \brief Dirección de registro para configuración del sistema.
 */
#define SYSTEM_CONFIG_REG           (uint16_t)0x0080
/**
 * \def   LED_CONFIG_REG
 * \brief Dirección de registro para configuración del LED.
 */
#define LED_CONFIG_REG              (uint16_t)0x0082
/**
 * \def   PIC_SET_REG
 * \brief Dirección de registro para establecer imagen.
 */
#define PIC_SET_REG                 (uint16_t)0x0084
/**
 * \def   TP_SIMULATION_REG
 * \brief Dirección de registro para simulación de touch.
 */
#define TP_SIMULATION_REG           (uint16_t)0x00D4
/**
 * \def   BACKGROUND_SWITCH_REG
 * \brief Dirección de registro para cambio de fondo.
 */
#define BACKGROUND_SWITCH_REG       (uint16_t)0x00DE

/*--------- Enumeración para establecer los distintos tipos de touch en la pantalla ---------*/
/**
 * \enum  press_mode
 * \brief Tipos de eventos de touch en la pantalla.
 */
typedef enum : uint16_t
{
    NO_TOUCH,           ///< Sin toque.
    TOUCH_PRESSED,      ///< Toque presionado.
    TOUCH_RELEASED,     ///< Toque liberado.
    TOUCH_KEEP_PRESSING,///< Mantener toque presionado.
    TOUCH               ///< Evento de toque.
} press_mode;

/*--------- Estructura donde se almacenan los datos que se muestran en el ticket ---------*/
/**
 * \struct  ticket_struct
 * \brief   Estructura para almacenar los datos de un ticket de despacho.
 */
typedef struct
{
    uint32_t ticket_number;            ///< Número de ticket.
    float liters;                      ///< Litros despachados.
    float price;                       ///< Precio por litro.
    float total_amount;                ///< Importe total.
    float IVA;                         ///< IVA aplicado.
    float total;                       ///< Total con IVA.
    const char *business_name;         ///< Nombre del negocio.
    const char *business_address;      ///< Dirección del negocio.
    const char *business_area;         ///< Área del negocio.
    const char *business_postal_code;  ///< Código postal del negocio.
    const char *business_RFC;          ///< RFC del negocio.
    const char *business_city;         ///< Ciudad del negocio.
    const char *business_state;        ///< Estado del negocio.
    const char *user_name;             ///< Nombre del usuario.
    const char *product;               ///< Producto despachado.
    uint8_t *rtc_array;                ///< Array con fecha y hora.
} ticket_struct;

#endif
