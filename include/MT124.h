/**
 * \file    MT124.h
 * 
 * \brief   Este archivo contiene la definición de la clase MT124 para el manejo del lector de tags MT124 por UART.
 */

#ifndef _MT124_H
#define _MT124_H

#include "UART_Device.h"

/**
 * \def     MT124_BUFFER_LEN
 * \brief   Tamaño del buffer para datos del lector MT124.
 */
#define MT124_BUFFER_LEN 32

/**
 * \def     TAG_DATA_SIZE
 * \brief   Tamaño del buffer para datos de tag.
 */
#define TAG_DATA_SIZE     8

/**
 * \def     TAG_DATA_POS
 * \brief   Posición de inicio de los datos de tag en el buffer.
 */
#define TAG_DATA_POS      6

/**
 * \def     READER_STATUS_POS
 * \brief   Posición del estado del lector en el buffer.
 */
#define READER_STATUS_POS 5

/**
 * \def     READER_ADDR_H
 * \brief   Dirección alta del lector.
 */
#define READER_ADDR_H           (uint8_t)0x55
/**
 * \def     MT124_TX_MESSAGE_HEADER
 * \brief   Header para mensajes TX.
 */
#define MT124_TX_MESSAGE_HEADER (uint8_t)0x48
/**
 * \def     MT124_RX_MESSAGE_HEADER
 * \brief   Header para mensajes RX.
 */
#define MT124_RX_MESSAGE_HEADER (uint8_t)0x57
/**
 * \def     MT124_END_OF_MESSAGE
 * \brief   Byte de fin de mensaje.
 */
#define MT124_END_OF_MESSAGE    (uint8_t)0x03
/**
 * \def     MT124_CHECK_STATUS_CMD
 * \brief   Comando para verificar el estado del lector.
 */
#define MT124_CHECK_STATUS_CMD  (uint8_t)0x30
/**
 * \def     MT124_SET_ADDRESS_CMD
 * \brief   Comando para establecer la dirección del lector.
 */
#define MT124_SET_ADDRESS_CMD   (uint8_t)0x40
/**
 * \def     MT124_GET_TAG_DATA_CMD
 * \brief   Comando para obtener datos de tag.
 */
#define MT124_GET_TAG_DATA_CMD  (uint8_t)0x50

/**
 * \enum    reader_status
 * \brief   Estados posibles del lector MT124.
 */
typedef enum : uint8_t
{
  READER_NO_STATUS                    = 0x00, ///< Sin estado.
  READER_FAIL                         = 0x4E, ///< Fallo en la operación.
  READER_SUCCESS                      = 0x59, ///< Operación exitosa.
  READER_REQUEST_ADRRESS_SETTING      = 0x01, ///< Solicitud de configuración de dirección.
  READER_APPLYING_ADDRESS_INFO,               ///< Aplicando información de dirección.
  READER_ADDRESS_RECEIVED_SUCCESFULLY,        ///< Dirección recibida exitosamente.
  READER_ADDRESS_ASSIGNED,                    ///< Dirección asignada.
  READER_ADDRESS_SET,                         ///< Dirección establecida.
  READER_NO_TAG_DATA,                         ///< No hay datos de tag.
  READER_READING_TAG_SUCCESFULLY,             ///< Lectura de tag exitosa.
  READER_TAG_DATA_READY,                      ///< Datos de tag listos.
  READER_IDLE,                                ///< Lector en reposo.
  READER_LOW_BATTERY,                         ///< Batería baja.
  READER_ADDRESS_DOES_NOT_EXISTS              ///< La dirección no existe.
} reader_status;

/**
 * \struct  mt124_param_struct
 * \brief   Estructura de parámetros para inicializar el lector MT124.
 * 
 * Hereda de uart_param_struct y establece valores por defecto para CRC y velocidad de transmisión.
 */
struct mt124_param_struct : uart_param_struct
{
    /**
     * \brief Constructor que inicializa los parámetros por defecto del MT124.
     * 
     * - m_crc: CRC_8.
     * - m_data_transmission_rate: 115200 baudios.
     */
    mt124_param_struct()
    {
        this->m_crc = CRC_8;
        this->m_data_transmission_rate = 115200;
    }
};

/**
 * \class MT124
 * 
 * \brief Clase para el manejo del lector de tags MT124 por UART.
 * 
 * Permite configurar el lector, obtener datos de tag, consultar el estado y manejar la comunicación por UART.
 */
class MT124 : public UART_Device
{
    public:
        /**
         * \brief Destructor.
         */
        ~MT124() { }

        /**
         * \brief           Establece los parámetros del lector MT124.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(const device_param_struct *p_params) override;

        /**
         * \brief           Obtiene los parámetros actuales del lector MT124.
         * \param p_params  Estructura donde se guardarán los parámetros.
         */
        void get_device_parameters(device_param_struct *p_params) override;

        /**
         * \brief                   Obtiene los datos de tag del lector.
         * \param p_buffer          Doble puntero al buffer donde se guardarán los datos.
         * \param p_reader_address  Dirección del lector.
         * \returns                 Estado de la operación.
         */
        status get_tag_data(uint8_t **p_buffer, uint8_t p_reader_address);

        /**
         * \brief                   Obtiene el estado actual del lector.
         * \param p_reader_address  Dirección del lector.
         * \returns                 Estado del lector.
         */
        reader_status get_reader_status(uint8_t p_reader_address);

        /**
         * \brief                   Reinicia el estado del lector en la dirección especificada.
         * \param p_reader_address  Dirección del lector.
         */
        inline void reset_reader_status(uint8_t p_reader_address)
        {
            m_reader_status[p_reader_address] = READER_NO_STATUS;
        }

    public:
        static const status READER_ADDRESS_ERROR  = 6; ///< Error de dirección del lector.
        static const status NO_READING_ERROR      = 7; ///< Error de lectura.
        static const status TAG_ERROR             = 8; ///< Error de datos de tag.
        static const status READ_TAG_SUCCESSFULLY = 9; ///< Lectura de tag exitosa.

    private:
        /**
         * \brief           Calcula el CRC de los datos enviados y recibidos.
         * \param p_data    Buffer de datos.
         * \param p_length  Longitud del buffer.
         * \param p_check   Indica si se revisa el CRC.
         * \returns     CRC calculado.
         */
        uint16_t crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check = false) override;

        /**
         * \brief           Envía un comando al lector MT124.
         * \param p_command Estructura del comando.
         * \returns         Estado de la operación.
         */
        status send_command(command_struct *p_command) override;
        
    private:
        reader_status m_reader_status[16] = { (reader_status)0 }; ///< Estado de cada lector (hasta 16 lectores).
        uint8_t m_buffer[MT124_BUFFER_LEN] = { 0 }; ///< Buffer para datos recibidos.
        uint8_t m_tag_data[TAG_DATA_SIZE] = { 0 };  ///< Buffer para datos de tag.
};

/**
 * \class MT124_Builder
 * 
 * \brief Clase constructora para dispositivos MT124.
 * 
 * Permite establecer los parámetros de configuración y obtener una instancia lista para su uso.
 */
class MT124_Builder : public UART_Device_Builder
{
    public:
        /**
         * \brief Destructor.
         */
        ~MT124_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo MT124.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            mt124_param_struct *p = static_cast<mt124_param_struct *>(p_params);
            this->m_mt124_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_mt124_params.m_rx_pin = p->m_rx_pin;
            this->m_mt124_params.m_tx_pin = p->m_tx_pin;
            this->m_mt124_params.m_enable_pin = p->m_enable_pin;
            this->m_mt124_params.m_timeout = p->m_timeout;
            this->m_mt124_params.m_enable_state = p->m_enable_state;
            this->m_mt124_params.m_type = p->m_type;
            this->m_mt124_params.m_crc = p->m_crc;
            this->m_mt124_params.m_port = p->m_port;
            this->m_mt124_params.m_config = p->m_config;
        }

        /**
         * \brief   Obtiene una instancia del dispositivo MT124 construido.
         * \returns Puntero al dispositivo MT124.
         */
        Device *get_device() override
        { 
            this->mt124_device.begin(&this->m_mt124_params);

            return &(this->mt124_device);
        }

    private:
        mt124_param_struct m_mt124_params; ///< Parámetros del dispositivo MT124.
        MT124 mt124_device;                ///< Instancia del dispositivo MT124.
};

#endif