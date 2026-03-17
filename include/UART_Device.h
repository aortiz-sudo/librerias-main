/**
 * \file    UART_Device.h
 * 
 * \brief   Este archivo contiene la definición de la clase UART_Device y estructuras relacionadas para el manejo de dispositivos UART.
 */

#ifndef _UART_DEVICE_H
#define _UART_DEVICE_H

#include "Device.h"

/**
 * \struct uart_param_struct
 * \brief Estructura de parámetros para inicializar dispositivos UART.
 * 
 * Hereda de device_param_struct y añade parámetros específicos para UART como pines, configuración y tiempos.
 */
struct uart_param_struct : device_param_struct
{
    /**
     * \brief Constructor que inicializa el tipo de dispositivo como UART.
     */
    uart_param_struct() { this->m_type = UART_DEVICE; }
    
    uint32_t m_config = SERIAL_8N1;   ///< Configuración de la comunicación UART (por defecto SERIAL_8N1).
    int8_t m_rx_pin = -1;             ///< Pin RX del dispositivo UART.
    int8_t m_tx_pin = -1;             ///< Pin TX del dispositivo UART.
    int8_t m_enable_pin = -1;         ///< Pin para habilitar el dispositivo UART.
    bool m_enable_state = LOW;        ///< Estado lógico para habilitar el dispositivo.
    uint32_t m_enable_delay = 50;     ///< Retardo en milisegundos al habilitar el dispositivo.
    uint32_t m_timeout = 50;          ///< Tiempo de espera para operaciones UART.
};

/**
 * \class UART_Device
 * 
 * \brief Clase base para el manejo de dispositivos UART.
 * 
 * Hereda de Device y proporciona métodos para inicialización, escritura, lectura y configuración de dispositivos UART.
 */
class UART_Device : public Device
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~UART_Device() { }
        
        /**
         * \brief           Inicializa el dispositivo UART con los parámetros especificados.
         * \param p_params  Parámetros de configuración.
         */
        virtual void begin(const device_param_struct *p_params) override;

        /**
         * \brief           Establece los parámetros del dispositivo UART.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void set_device_parameters(const device_param_struct *p_params) override;

        /**
         * \brief           Obtiene los parámetros actuales del dispositivo UART.
         * \param p_params  Estructura donde se guardarán los parámetros.
         */
        virtual void get_device_parameters(device_param_struct *p_params) override;

        /**
         * \brief               Escribe datos en el dispositivo UART.
         * \param p_data        Buffer de datos a enviar.
         * \param p_data_length Número de bytes a enviar.
         * \returns             Número de datos enviados.
         */
        virtual size_t write_data(uint8_t *p_data, size_t p_data_length) override;

        /**
         * \brief                   Lee datos del dispositivo UART.
         * \param p_buffer          Buffer donde se guardarán los datos recibidos.
         * \param p_buffer_length   Número máximo de bytes a recibir.
         * \returns                 Número de bytes recibidos.
         */
        virtual size_t read_data(uint8_t *p_buffer, size_t p_buffer_length) override;

        /**
         * \brief Reinicia el puerto serie con la configuración actual.
         */
        void reset_serial_port();

        /**
         * \brief               Reinicia el puerto serie con una configuración específica.
         * \param p_BAUD_RATE   Velocidad de transmisión.
         * \param p_config      Configuración UART (por defecto SERIAL_8N1).
         */
        void reset_serial_port(unsigned long p_BAUD_RATE, int p_config = SERIAL_8N1);

        /**
         * \brief           Establece el tiempo de espera para operaciones UART.
         * \param p_timeout Tiempo de espera en milisegundos.
         */
        inline void set_timeout(uint32_t p_timeout)
        {
            this->m_timeout = p_timeout;
        }

        /**
         * \brief   Obtiene el tiempo de espera actual para operaciones UART.
         * \returns Tiempo de espera en milisegundos.
         */
        inline uint32_t get_timeout()
        {
            return this->m_timeout;
        }

    protected:
        uint32_t m_config = SERIAL_8N1; ///< Configuración UART.
        int8_t m_rx_pin = -1;        ///< Pin RX.
        int8_t m_tx_pin = -1;        ///< Pin TX.
        int8_t m_enable_pin = -1;    ///< Pin de habilitación.
        bool m_enable_state = LOW;   ///< Estado lógico de habilitación.
        uint32_t m_enable_delay = 0; ///< Retardo al habilitar.
        uint32_t m_timeout = 0;      ///< Tiempo de espera.

    protected:
        /**
         * \brief           Envía un comando al dispositivo UART.
         * \param p_command Estructura del comando.
         * \returns         Estado de la operación.
         */
        virtual status send_command(command_struct *p_command) = 0;

        /**
         * \brief           Calcula el CRC de los datos enviados y recibidos.
         * \param p_data    Buffer de datos.
         * \param p_length  Longitud del buffer.
         * \param p_check   Indica si se revisa el CRC.
         * \returns         CRC calculado.
         */
        virtual uint16_t crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check = false) = 0;

        /**
         * \brief Activa el pin de habilitación del dispositivo UART.
         */
        void activate_enable_pin();

        /**
         * \brief Desactiva el pin de habilitación del dispositivo UART.
         */
        void deactivate_enable_pin();  
};

/**
 * \class UART_Device_Builder
 * 
 * \brief Clase abstracta para la construcción de objetos UART_Device.
 * 
 * Proporciona una interfaz para establecer los parámetros y obtener una instancia del dispositivo UART.
 */
class UART_Device_Builder : public Device_Builder
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~UART_Device_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo UART.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void set_device_parameters(device_param_struct *p_params) = 0;

        /**
         * \brief   Obtiene una instancia del dispositivo UART construido.
         * \returns Puntero al objeto UART_Device.
         */
        virtual Device *get_device() = 0;
};

#endif