/**
 * \file    Device.h
 * 
 * \brief   Este archivo contiene a la interfaz Device asi como su clase constructora.
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>

#if defined(ESP32)
    #include "Logger.h"
#endif


/**
 * \brief   Tipos de estados que pueden tener los dispositivos.
 */
typedef uint8_t status;

/**
 * \brief   Tipos de comunicación que pueden utilizar los dispositivos.
 */
typedef enum : uint8_t
{
    DEFAULT_DEVICE,
    UART_DEVICE,    ///< Comunicación UART.
    I2C_DEVICE,     ///< Comunicación I2C.
} device_type;

/*--------- Establece los tipos de CRC se utilizan los dispositivos ---------*/
/**
 * \brief   Tipos de CRC que pueden utilizar los dispositivos.
 */
typedef enum : uint8_t
{
    NO_CRC, ///< Sin CRC.
    CRC_8,  ///< CRC 8 bits.
    CRC_16, ///< CRC 16 bits.
} crc_type;

/**
 * \brief   Estructura de parámetros para inicializar los dispositivos.
 */
struct device_param_struct
{
    device_param_struct() { }  
    uint32_t m_data_transmission_rate; ///< Velocidad de transmision de datos
    device_type m_type; ///< Tipo de comunicacion
    crc_type m_crc; ///< Tipo de CRC
    Stream *m_port; ///< Puerto por donde se enviaran y recibiran los datos
};

/**
 * \class   Device
 * 
 * \brief   Clase hija de la clase Print.
 * 
 * Establece una plantilla para los distintos tipos de 
 * dispositivos que se utilizan en el proyecto.
 */
class Device : public Print
{
    public:
        virtual ~Device() { };

        /**
         * \brief           Función para inicializar el dispositivo.
         * \param p_params  Parámetros utilizados para inicializar el dispositivo.
         */
        virtual void begin(const device_param_struct *p_params) = 0;

        /**
         * \brief           Función para establecer los parámetros del dispositivo.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void set_device_parameters(const device_param_struct *p_params) = 0;

        /**
         * \brief           Función para obtener los parámetros que se utilizaron para inicializar el dispositivo.
         * \param p_params  Estructura en donde se guardarán los dispostivos.
         */
        virtual void get_device_parameters(device_param_struct *p_params) = 0;

        /**
         * \brief               Función para enviar datos al dispositivo.
         * \param p_data        Buffer de datos que se quiere enviar.
         * \param p_data_length Número de bytes que se que quiere enviar.
         * \returns             Número de datos enviados.
         */
        virtual size_t write_data(uint8_t *p_data, size_t p_data_length) = 0;

        /**
         * \brief                  Función para recibir datos del dispositivo.
         * \param p_buffer          Buffer en dónde se guardarán los datos recibidos.
         * \param p_buffer_length   Número máximo de bytes a recibir.
         * \returns                 Número de bytes recibidos.
         */
        virtual size_t read_data(uint8_t *p_buffer, size_t p_buffer_length) = 0;

        /*------- Implementacion de Función para compatibilidad -------*/
        /**
         * \brief           Escribe un byte en el dispositivo (heredado de Print).
         * \param p_data    Byte a enviar.
         * \returns         Número de bytes escritos.
         */
        size_t write(uint8_t p_data);

        /**
         * \brief           Escribe un buffer de datos en el dispositivo (heredado de Print).
         * \param p_data    Buffer de datos a enviar.
         * \param p_length  Número de bytes a enviar.
         * \returns         Número de bytes escritos.
         */
        size_t write(uint8_t *p_data, size_t p_length);
        
        /**
         * \brief           Función utilizada para cambiar el estado del dispositivo.
         * \param p_status  Estado al que se quiere cambiar.
         * \returns         El mismo estado al que se estableció.
         */
        inline status set_status(status p_status)
        {
            this->m_status_flag = p_status;
            return p_status;
        }

        /**
         * \brief   Función para obtener el estado actual del dispositivo.
         * \returns Estado del dispositivo.
         */
        inline status get_status() const
        {
            return m_status_flag;
        }

        /**
         * \brief       Función para establecer el tipo de CRC que utiliza el dispositvo.
         * \param p_crc Tipo de CRC que se quiere utilizar.
         */
        inline void set_crc_type(crc_type p_crc)
        {
            this->m_crc = p_crc;
        }

        /**
         * \brief   Función para obtener el tipo de CRC que utiliza el dispositivo.
         * \returns El tipo de CRC que utiliza el dispositivo.
         */
        inline crc_type get_crc_type() const
        {
            return this->m_crc;
        }

        /**
         * \brief           Función para establecer el puerto que utilizara el disposivo.
         * \param p_port    Puerto que utilizará el dispositvo.
         */
        inline void set_port(Stream *p_port)
        {
            this->m_port = p_port;
        }

        /**
         * \brief   Función para obtener el puerto que utiliza el dispositivo.
         * \returns Puerto que utiliza el dispositvo.
         */
        
        inline Stream *get_port() const 
        {
            return this->m_port;
        }

        /**
         * \brief           Función para establecer el tipo de comunicación del dipositivo.
         * \param p_type    Tipo de comunicación.
         */
        inline void set_type(device_type p_type)
        {
            this->m_type = p_type;
        }

        /**
         * \brief   Función para obtener el tipo de comunicación que utiliza el dispositivo.
         * \returns El tipo de comunicación que utiliza el dispositivo.
         */
        inline device_type get_type() const
        {
            return this->m_type;
        }

        /**
         * \brief                           Función para establecer la velocidad de transmisión de datos del dispositivo.
         * \param p_data_transmission_rate  Velocidad de transmisión.
         */
        inline void set_transmission_rate(uint32_t p_data_transmission_rate)
        {
            this->m_data_transmission_rate = p_data_transmission_rate;
        }

        /**
         * \brief   Función para obtener la velocidad de transmisión de datos.
         * \returns La velocidad de transmición de datos del dispositivo.
         */
        inline uint32_t get_transmission_rate() const
        {
            return this->m_data_transmission_rate;
        }
    
    public:
        static const status NO_DEVICE_ERROR           = 0; ///< No se detectó algún error.
        static const status TIMEOUT_ERROR             = 1; ///< Se agotó el tiempo de espera para alguna respuesta.
        static const status CRC_ERROR                 = 2; ///< Se detectó algún error en el checksum de los datos.
        static const status DATA_OUT_OF_RANGE_ERROR   = 3; ///< El valor que se recibió es más alto de lo que está establecido.
        static const status DATA_NOT_READY_ERROR      = 4; ///< Los datos aún no están listos.
        static const status BUFFER_OVERFLOW_ERROR     = 5; ///< Sobrecarga de datos en el buffer.

    protected:

        //Estructura de un comando.
        struct command_struct
        {
            uint8_t type;           ///< Tipo de comando.
            uint8_t *data;          ///< Datos adicionales para enviar junto con el comando.
            size_t length;          ///< Cantidad de datos adicionales.
            uint8_t *response;      ///< Buffer para guardar la respuesta del dispositivo.
            size_t buffer_len;      ///< Tamaño del buffer de respuesta.
            uint8_t address;        ///< Dirección del dispositivo.
            size_t response_length;  ///< Número de bytes recibidos.
        };

    protected:

        /**
         * \brief   Función que estructura los datos de un comando para enviarlos.
         * \returns El estado que se generó al enviar el comando.
         */
        virtual status send_command(command_struct *p_command) = 0;

    private:
        status m_status_flag = NO_DEVICE_ERROR; ///< Guarda el estado en el que se encuentra el dispositivo.
        crc_type m_crc = NO_CRC;                ///< Tipo de CRC que utiliza el dispositvo.
        device_type m_type = DEFAULT_DEVICE;    ///< Tipo de comunicación del dispostivo.    
        uint32_t m_data_transmission_rate = 0;  ///< Velocidad de transmisión de datos.
        Stream *m_port = nullptr;               ///< Puerto de comunicación.
};

/**
 * \class Device_Builder
 * 
 * \brief Clase abstracta para la construcción de objetos Device.
 * 
 * Proporciona una interfaz para establecer los parámetros del dispositivo y obtener una instancia del mismo.
 * Las clases derivadas deben implementar los métodos virtuales para configurar y construir dispositivos específicos.
 */
class Device_Builder
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~Device_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void set_device_parameters(device_param_struct *p_params) = 0;

        /**
         * \brief   Obtiene una instancia del dispositivo construido.
         * \returns Puntero al objeto Device.
         */
        virtual Device *get_device() = 0;
};

#endif