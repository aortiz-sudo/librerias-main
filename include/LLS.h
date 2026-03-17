/**
 * \file    LLS.h
 * 
 * \brief   Este archivo contiene la definición de la clase LLS para el manejo de sensores de nivel de combustible LLS por UART.
 */

#ifndef _LLS_H
#define _LLS_H

#include "UART_Device.h"

/*--------- Definiciones de constantes y macros para LLS ---------*/
#define LLS_MAX_BUFFER_LEN 24      ///< Longitud del buffer ASCII.
#define ASCII_FUEL_POS       14      ///< Posición del nivel de combustible en ASCII.
#define ASCII_TEMP_POS        9      ///< Posición de la temperatura en ASCII.
#define ASCII_FREQ_POS        2      ///< Posición de la frecuencia en ASCII.
#define HEX_FUEL_POS          4      ///< Posición del nivel de combustible en HEX.
#define HEX_TEMP_POS          3      ///< Posición de la temperatura en HEX.
#define HEX_FREQ_POS          6      ///< Posición de la frecuencia en HEX.
#define LLS_TX_MESSAGE_HEADER (uint8_t)0x31 ///< Header de mensaje TX.
#define LLS_RX_MESSAGE_HEADER (uint8_t)0x3E ///< Header de mensaje RX.

#define LLS_PERIODIC_DATA_CMD        (uint8_t)0x07 ///< Comando para datos periódicos.
#define LLS_SINGLE_DATA_CMD          (uint8_t)0x06 ///< Comando para datos únicos.
#define LLS_INTERVAL_ADJUSTMENT_CMD  (uint8_t)0x13 ///< Comando para ajustar intervalo.
#define LLS_OUTPUT_MODE_CMD          (uint8_t)0x17 ///< Comando para modo de salida.

#define LLS_CMD_SUCCESS (uint8_t)0x00 ///< Comando exitoso.
#define LLS_CMD_FAILED  (uint8_t)0x01 ///< Comando fallido.

#define LLS_NO_OUTPUT     (uint8_t)0x00 ///< Sin salida.
#define LLS_BINARY_OUTPUT (uint8_t)0x01 ///< Salida binaria.
#define LLS_ASCII_OUTPUT  (uint8_t)0x02 ///< Salida ASCII.

#define MAX_NUMBER_OF_SLAVES 4 ///< Número máximo de sensores esclavos.

/**
 * \struct lls_param_struct
 * \brief  Estructura de parámetros para inicializar el sensor LLS.
 */
struct lls_param_struct : uart_param_struct
{
    lls_param_struct() { this->m_crc = CRC_8; }
    bool m_periodic; ///< Indica si se usan datos periódicos.
    bool m_ascii;    ///< Indica si la salida es ASCII.
};

/**
 * \struct lls_data
 * \brief  Estructura para almacenar datos de un sensor LLS.
 */
struct lls_data
{
    uint8_t address;         ///< Dirección del sensor.
    int8_t temp;             ///< Temperatura medida.
    uint16_t fuel_level;     ///< Nivel de combustible.
    uint16_t frequency_value;///< Valor de frecuencia.
};

/**
 * \class   LLS
 * 
 * \brief   Clase para el manejo de sensores de nivel de combustible LLS por UART.
 */
class LLS : public UART_Device
{
    public:
        ~LLS() { }

        /**
         * \brief           Establece los parámetros del dispositivo LLS.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(const device_param_struct *p_params) override;

        /**
         * \brief           Obtiene los parámetros actuales del dispositivo LLS.
         * \param p_params  Parámetros del dispositivo.
         */
        void get_device_parameters(device_param_struct *p_params) override;

        /**
         * \brief                   Lee los datos del sensor LLS en la dirección especificada.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Estado de la operación.
         */
        status read_lls_data(uint8_t p_slave_address);

        /**
         * \brief                   Obtiene el nivel de combustible del sensor.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Nivel de combustible.
         */
        uint16_t get_fuel_level(uint8_t p_slave_address);

        /**
         * \brief                   Obtiene la temperatura del sensor.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Temperatura medida.
         */
        int8_t get_temperature(uint8_t p_slave_address);

        /**
         * \brief                   Obtiene el valor de frecuencia del sensor.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Valor de frecuencia.
         */
        uint16_t get_frequency_value(uint8_t p_slave_address);

        /**
         * \brief                   Establece el intervalo de salida de datos.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \param p_seconds         Intervalo en segundos.
         * \returns                 Estado de la operación.
         */
        status set_output_interval(uint8_t p_slave_address, uint8_t p_seconds);

        /**
         * \brief                   Configura la salida binaria en el sensor.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Estado de la operación.
         */
        status set_binary_output(uint8_t p_slave_address);

        /**
         * \brief                   Configura la salida ASCII en el sensor.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Estado de la operación.
         */
        status set_ascii_output(uint8_t p_slave_address);

    public:
        static const status HEADER_ERROR            = 6;  ///< Error de encabezado.
        static const status FUEL_OUT_OF_RANGE_ERROR = 7;  ///< Error de nivel de combustible fuera de rango.
        static const status ADDRESS_ERROR           = 8;  ///< Error de dirección.
        static const status PERIODIC_ERROR          = 9;  ///< Error de datos periódicos.
        static const status COMMAND_FAILED_ERROR    = 10; ///< Error de comando fallido.
        static const status COMMAND_SUCCESS         = 11; ///< Comando exitoso.

    private:
        /**
         * \brief               Calcula el CRC de los datos enviados y recibidos.
         * \param p_data        Buffer de datos.
         * \param p_length      Longitud del búfer.
         * \param p_check       Indica si se revisa el CRC.
         * \returns             CRC calculado.
         */
        uint16_t crc_calculation(const uint8_t *p_data, size_t p_length, bool p_check = false) override;

        /**
         * \brief           Envía un comando al sensor LLS.
         * \param p_command Estructura del comando.
         * \returns         Estado de la operación.
         */
        status send_command(command_struct *p_command) override;

        /**
         * \brief                   Procesa los datos recibidos del sensor.
         * \param p_data            Buffer de datos.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \returns                 Estado de la operación.
         */
        status parse_data(uint8_t *p_data, uint8_t p_slave_address);

        /**
         * \brief                   Establece el modo de salida del sensor.
         * \param p_slave_address   Dirección del sensor esclavo.
         * \param p_mode            Modo de salida.
         * \returns                 Estado de la operación.
         */
        status set_output_mode(uint8_t p_slave_address, uint8_t p_mode);

    private:
        bool m_periodic = false;                                 ///< Indica si se usan datos periódicos.
        bool m_ascii = false;                                    ///< Indica si la salida es ASCII.
        uint8_t m_lls_buffer[LLS_MAX_BUFFER_LEN] = { 0 };       ///< Buffer para datos recibidos.
        lls_data m_lls_data[MAX_NUMBER_OF_SLAVES] = {0 };        ///< Datos de los sensores esclavos.
};

/**
 * \class   LLS_Builder
 * 
 * \brief   Clase constructora para dispositivos LLS.
 */
class LLS_Builder : public UART_Device_Builder
{
    public:
        ~LLS_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo LLS.
         * \param _params   Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *_params) override
        {
            lls_param_struct *p = static_cast<lls_param_struct *>(_params);
            this->m_lls_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_lls_params.m_rx_pin = p->m_rx_pin;
            this->m_lls_params.m_tx_pin = p->m_tx_pin;
            this->m_lls_params.m_enable_pin = p->m_enable_pin;
            this->m_lls_params.m_timeout = p->m_timeout;
            this->m_lls_params.m_enable_state = p->m_enable_state;
            this->m_lls_params.m_type = p->m_type;
            this->m_lls_params.m_crc = p->m_crc;
            this->m_lls_params.m_port = p->m_port;
            this->m_lls_params.m_config = p->m_config;
            this->m_lls_params.m_periodic = p->m_periodic;
            this->m_lls_params.m_ascii = p->m_ascii;
        }

        /**
         * \brief   Obtiene una instancia del dispositivo LLS construido.
         * \returns Puntero al dispositivo LLS.
         */
        Device *get_device() override
        {
            this->lls_device.begin(&m_lls_params);
            return &(this->lls_device);
        }
    
    private:
        lls_param_struct m_lls_params; ///< Parámetros del dispositivo LLS.
        LLS lls_device;                ///< Instancia del dispositivo LLS.
};

#endif