/**
 * \file    Iridium.h
 *
 * \brief   Este archivo contiene la definición de la clase Iridium para el manejo del módem satelital Iridium (SBD) por UART mediante comandos AT.
 */

#ifndef _IRIDIUM_H
#define _IRIDIUM_H

#include "Satellite.h"
#include "string_handlers.h"

/*--------- Comandos AT para el control del módem Iridium ---------*/
#define REPEAT_LAST_CMD             "A/"        ///< Repite el último comando ejecutado.
#define ECHO_CMD                    "ATEn"      ///< Activa/desactiva el eco de comandos.
#define QUIET_MODE_CMD              "ATQn"      ///< Activa/desactiva el modo silencioso.
#define VERBOSE_MODE_CMD            "ATVn"      ///< Activa/desactiva el modo detallado de respuestas.
#define SOFT_RESET_CMD              "ATZn"      ///< Reinicio por software del módem.
#define DTR_OPTION_CMD              "AT&Dn"     ///< Configura el comportamiento de la señal DTR.
#define RESTORE_SETTINGS_CMD        "AT&Fn"     ///< Restaura la configuración de fábrica.
#define FLOW_CONTROL_CMD            "AT&Kn"     ///< Configura el control de flujo.
#define GET_CONFIG_CMD              "AT&V"      ///< Obtiene la configuración actual.
#define SET_CONFIG_CMD              "AT&Wn"     ///< Guarda la configuración actual en un perfil.
#define SET_PROFILE_CMD             "AT&Yn"     ///< Establece el perfil por defecto al arranque.
#define DISPLAY_REGISTERS_CMD       "AT%R"      ///< Muestra los registros S del módem.
#define FLUSH_EEPROM_CMD            "AT*F"      ///< Vacía la memoria EEPROM.
#define RADIO_ACTIVITY_CMD          "AT*Rn"     ///< Activa/desactiva la actividad del radio.
#define GET_RTC_CMD                 "AT+CCLK"   ///< Obtiene el reloj de tiempo real.
#define GET_SERIAL_NUMBER_CMD       "AT+CGSN"   ///< Obtiene el número de serie (IMEI).
#define INDICATOR_EVENT_CMD         "AT+CIER"   ///< Configura los reportes de eventos del indicador.
#define RING_INDICATION_STATUS_CMD  "AT+CRIS"   ///< Consulta el estado de la indicación de llamada.
#define SIGNAL_QUALITY_STATUS_CMD   "AT+CSQ"    ///< Consulta la calidad de la señal.
#define SET_BAUDRATE_CMD            "AT+IPR"    ///< Establece la velocidad de transmisión.
#define WRITE_BIN_DATA_CMD          "AT+SBDWB"  ///< Escribe datos binarios en el buffer de envío.
#define READ_BIN_DATA_CMD           "AT+SBDRB"  ///< Lee datos binarios del buffer de recepción.
#define WRITE_TEXT_MSG_CMD          "AT+SBDWT"  ///< Escribe un mensaje de texto en el buffer de envío.
#define READ_TEXT_MSG_CMD           "AT+SDBRT"  ///< Lee un mensaje de texto del buffer de recepción.
#define INITIATE_SBD_SESSION_CMD    "AT+SBDI"   ///< Inicia una sesión SBD.
#define INITIATE_SBD_E_SESSION_CMD  "AT+SBDIX"  ///< Inicia una sesión SBD extendida.
#define CLEAR_SBD_MSG_BUFFER_CMD    "AT+SBDD"   ///< Limpia los buffers de mensajes SBD.

/*--------- Longitudes máximas de los mensajes SBD ---------*/
#define MAX_TX_BIN_DATA_LEN     340     ///< Longitud máxima de datos binarios a transmitir.
#define MAX_RX_BIN_DATA_LEN     270     ///< Longitud máxima de datos binarios a recibir.
#define MAX_TX_TEXT_MESSAGE_LEN 120     ///< Longitud máxima de un mensaje de texto a transmitir.
#define MAX_RX_TEXT_MESSAGE_LEN 270     ///< Longitud máxima de un mensaje de texto a recibir.

/*--------- Opciones para limpiar los buffers de mensajes SBD ---------*/
#define CLEAR_MOBILE_ORIGINATED_BUFFER (uint8_t)0   ///< Limpia el buffer de mensajes originados (MO).
#define CLEAR_MOBILE_TERMINATED_BUFFER (uint8_t)1   ///< Limpia el buffer de mensajes terminados (MT).
#define CLEAR_BOTH_BUFFERS             (uint8_t)2   ///< Limpia ambos buffers de mensajes.

#define ENABLE  (uint8_t)1  ///< Valor para habilitar una opción.
#define DISABLE (uint8_t)0  ///< Valor para deshabilitar una opción.

#define IRIDIUM_BINARY_MESSAGE (uint8_t)0x00   ///< Identificador de mensaje binario.
#define IRIDIUM_TEXT_MESSAGE   (uint8_t)0x01   ///< Identificador de mensaje de texto.

/**
 * \struct  iridium_param_struct
 * \brief   Estructura de parámetros para inicializar el módem Iridium.
 *
 * Hereda de uart_param_struct y establece valores por defecto para la velocidad de transmisión y el tipo de CRC.
 */
struct iridium_param_struct : uart_param_struct
{
    /**
     * \brief Constructor que inicializa los parámetros por defecto del Iridium.
     *
     * - m_data_transmission_rate: 19200 baudios.
     * - m_crc: CRC_16.
     */
    iridium_param_struct()
    {
        this->m_data_transmission_rate = 19200;
        this->m_crc = CRC_16;
    }
};

/**
 * \enum    iridium_baudrate
 * \brief   Velocidades de transmisión soportadas por el módem Iridium.
 */
typedef enum : uint8_t
{
    _0,             ///< Velocidad automática.
    _600_BPS,       ///< 600 bps.
    _1200_BPS,      ///< 1200 bps.
    _4800_BPS,      ///< 4800 bps.
    _9600_BPS,      ///< 9600 bps.
    _19200_BPS,     ///< 19200 bps.
    _38400_BPS,     ///< 38400 bps.
    _57600_BPS,     ///< 57600 bps.
    _115200_BPS,    ///< 115200 bps.
} iridium_baudrate;

/**
 * \class Iridium
 *
 * \brief Clase para el manejo del módem satelital Iridium por UART.
 *
 * Permite enviar y recibir datos mediante el servicio SBD, obtener el ID del dispositivo y reiniciarlo.
 */
class Iridium : public Satellite
{
    public:
        /**
         * \brief               Envía datos al servidor mediante el servicio SBD.
         * \param p_data        Buffer con los datos a enviar.
         * \param p_data_length Longitud de los datos.
         * \returns             Cantidad de datos enviados o código de estado.
         */
        int send_data_to_server(const uint8_t *p_data, size_t p_data_length) override;

        /**
         * \brief                   Obtiene datos del servidor mediante el servicio SBD.
         * \param p_data            Buffer donde se almacenarán los datos recibidos.
         * \param p_buffer_length   Longitud máxima del buffer.
         * \returns                 Cantidad de datos recibidos o código de estado.
         */
        int get_data_from_server(uint8_t *p_data, size_t p_buffer_length) override;

        /**
         * \brief               Obtiene el ID (número de serie/IMEI) del módem Iridium.
         * \param p_id_buffer   Buffer donde se almacenará el ID.
         * \returns             Estado de la operación.
         */
        status get_id(uint8_t *p_id_buffer);

        /**
         * \brief Reinicia el módem Iridium.
         */
        void reset_device() override;

    protected:
        /**
         * \brief           Envía un comando al módem Iridium.
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
};

/**
 * \class Iridium_Builder
 *
 * \brief Clase constructora para dispositivos Iridium.
 *
 * Permite establecer los parámetros de configuración y obtener una instancia lista para su uso.
 */
class Iridium_Builder : public UART_Device_Builder
{
    public:
        /**
         * \brief           Establece los parámetros del dispositivo Iridium.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            iridium_param_struct *p = static_cast<iridium_param_struct*>(p_params);
            this->m_iridium_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_iridium_params.m_rx_pin = p->m_rx_pin;
            this->m_iridium_params.m_tx_pin = p->m_tx_pin;
            this->m_iridium_params.m_enable_pin = p->m_enable_pin;
            this->m_iridium_params.m_timeout = p->m_timeout;
            this->m_iridium_params.m_enable_state = p->m_enable_state;
            this->m_iridium_params.m_type = p->m_type;
            this->m_iridium_params.m_crc = p->m_crc;
            this->m_iridium_params.m_port = p->m_port;
            this->m_iridium_params.m_config = p->m_config;
        }

        /**
         * \brief   Obtiene una instancia del dispositivo Iridium construido.
         * \returns Puntero al dispositivo Iridium.
         */
        Device *get_device() override
        {
            this->iridium_device.begin(&this->m_iridium_params);

            return &this->iridium_device;
        }

    private:
        iridium_param_struct m_iridium_params;  ///< Parámetros del dispositivo Iridium.
        Iridium iridium_device;                 ///< Instancia del dispositivo Iridium.
};
#endif
