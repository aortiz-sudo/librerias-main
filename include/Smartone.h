/**
 * \file    Smartone.h
 * 
 * \brief   Este archivo contiene la definición de la clase Smartone para el manejo del dispositivo satelital Smartone por UART.
 */

#ifndef _SMARTONE_H
#define _SMARTONE_H

#include "Satellite.h"

/**
 * \def     SMARTONE_RESET_BUFFER_LEN
 * \brief   Tamaño del buffer para el comando de reset.
 */
#define SMARTONE_RESET_BUFFER_LEN        5
/**
 * \def     SMARTONE_GET_ID_BUFFER_LEN
 * \brief   Tamaño del buffer para obtener el ID.
 */
#define SMARTONE_GET_ID_BUFFER_LEN       5
/**
 * \def     SMARTONE_RAW_BUFFER_LEN
 * \brief   Tamaño del buffer para mensajes crudos.
 */
#define SMARTONE_RAW_BUFFER_LEN         53
/**
 * \def     SMARTONE_TRUNC_BUFFER_LEN
 * \brief   Tamaño del buffer para mensajes truncados.
 */
#define SMARTONE_TRUNC_BUFFER_LEN       47
/**
 * \def     SMARTONE_RESPONSE_BUFFER_LEN
 * \brief   Tamaño del buffer para respuestas del dispositivo.
 */
#define SMARTONE_RESPONSE_BUFFER_LEN    10

/**
 * \def     SMARTONE_MESSAGE_HEADER
 * \brief   Header para mensajes Smartone.
 */
#define SMARTONE_MESSAGE_HEADER (uint8_t)0xAA
/**
 * \def     SMARTONE_RAW_MESSAGE
 * \brief   Identificador de mensaje RAW.
 */
#define SMARTONE_RAW_MESSAGE    (uint8_t)0x27
/**
 * \def     SMARTONE_TRUNC_MESSAGE
 * \brief   Identificador de mensaje truncado.
 */
#define SMARTONE_TRUNC_MESSAGE  (uint8_t)0x26
/**
 * \def     SMARTONE_ID_MESSAGE
 * \brief   Identificador de mensaje de ID.
 */
#define SMARTONE_ID_MESSAGE     (uint8_t)0x01
/**
 * \def     SMARTONE_RESET_MESSAGE
 * \brief   Identificador de mensaje de reset.
 */
#define SMARTONE_RESET_MESSAGE  (uint8_t)0xED

/**
 * \def     SMARTONE_ID_LENGTH
 * \brief   Longitud del ID del dispositivo Smartone.
 */
#define SMARTONE_ID_LENGTH  4

/**
 * \struct  smartone_param_struct
 * \brief   Estructura de parámetros para inicializar el dispositivo Smartone.
 * 
 * Hereda de uart_param_struct y establece valores por defecto para la velocidad de transmisión y el tipo de CRC.
 */
struct smartone_param_struct : uart_param_struct
{
    /**
     * \brief Constructor que inicializa los parámetros por defecto del Smartone.
     * 
     * - m_data_transmission_rate: 9600 baudios.
     * - m_crc: CRC_16.
     */
    smartone_param_struct()
    {
        this->m_data_transmission_rate = 9600;
        this->m_crc = CRC_16;
    }
};

/**
 * \class Smartone
 * 
 * \brief Clase para el manejo del dispositivo satelital Smartone por UART.
 * 
 * Permite enviar y recibir datos, obtener el ID del dispositivo, reiniciarlo y enviar mensajes RAW y truncados.
 */
class Smartone : public Satellite
{
    public:
        /**
         * \brief Destructor.
         */
        ~Smartone() { };

        /**
         * \brief               Envía datos al servidor.
         * \param p_data        Buffer con los datos a enviar.
         * \param p_data_length Longitud de los datos.
         * \returns             Cantidad de datos enviados.
         */
        int send_data_to_server(const uint8_t *p_data, size_t p_data_length) override;

        /**
         * \brief                   Obtiene datos del servidor.
         * \param p_buffer          Buffer donde se almacenarán los datos recibidos.
         * \param p_buffer_length   Longitud máxima del buffer.
         * \returns                 Cantidad de datos recibidos.
         */
        int get_data_from_server(uint8_t *p_buffer, size_t p_buffer_length) override;

        /**
         * \brief               Obtiene el ID del dispositivo Smartone.
         * \param p_id_buffer   Buffer donde se almacenará el ID.
         * \returns             Estado de la operación.
         */
        status get_id(uint8_t *p_id_buffer) override;

        /**
         * \brief Reinicia el dispositivo Smartone.
         */
        void reset_device() override;

        /**
         * \brief               Envía un mensaje crudo al dispositivo.
         * \param p_data        Buffer con los datos del mensaje.
         * \param p_data_length Longitud de los datos.
         * \returns             Estado de la operación.
         */
        status write_raw_message(const uint8_t *p_data, size_t p_data_length);

        /**
         * \brief                   Envía un mensaje truncado al dispositivo.
         * \param p_data            Buffer con los datos del mensaje.
         * \param p_data_length     Longitud de los datos.
         * \returns                 Estado de la operación.
         */
        status write_trunc_message(const uint8_t *p_data, size_t p_data_length);

    public:
        static const status NACK_MESSAGE = 5; ///< Estado para mensaje NACK (no reconocido).

    private:
        /**
         * \brief           Envía un comando al dispositivo Smartone.
         * \param p_command Estructura del comando.
         * \returns         Estado de la operación.
         */
        status send_command(command_struct *p_command) override;

        /**
         * \brief           Calcula el CRC de los datos enviados y recibidos.
         * \param p_data    Buffer de datos.
         * \param p_length  Longitud del buffer.
         * \param p_check   Indica si se revisa el CRC.
         * \returns         CRC calculado.
         */
        uint16_t crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check = false) override;

    private:
        uint8_t m_id[4] = { 0 }; ///< Buffer para almacenar el ID del dispositivo.
        uint8_t m_response[SMARTONE_RESPONSE_BUFFER_LEN] = { 0 }; ///< Buffer para almacenar la respuesta del dispositivo.
};

/**
 * \class Smartone_Builder
 * 
 * \brief Clase constructora para dispositivos Smartone.
 * 
 * Permite establecer los parámetros de configuración y obtener una instancia lista para su uso.
 */
class Smartone_Builder : public UART_Device_Builder
{
    public:
        /**
         * \brief Destructor.
         */
        ~Smartone_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo Smartone.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            smartone_param_struct *p = static_cast<smartone_param_struct *>(p_params);
            this->m_smartone_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_smartone_params.m_rx_pin = p->m_rx_pin;
            this->m_smartone_params.m_tx_pin = p->m_tx_pin;
            this->m_smartone_params.m_enable_pin = p->m_enable_pin;
            this->m_smartone_params.m_timeout = p->m_timeout;
            this->m_smartone_params.m_enable_state = p->m_enable_state;
            this->m_smartone_params.m_type = p->m_type;
            this->m_smartone_params.m_crc = p->m_crc;
            this->m_smartone_params.m_port = p->m_port;
            this->m_smartone_params.m_config = p->m_config;
        }

        /**
         * \brief   Obtiene una instancia del dispositivo Smartone construido.
         * \returns Puntero al dispositivo Smartone.
         */
        Device *get_device() override
        { 
            this->smartone_device.begin(&this->m_smartone_params);

            return &this->smartone_device;
        }
    
    private:
        smartone_param_struct m_smartone_params; ///< Parámetros del dispositivo Smartone.
        Smartone smartone_device;                ///< Instancia del dispositivo Smartone.
};

#endif