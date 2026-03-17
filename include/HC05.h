/**
 * \file    HC05.h
 * 
 * \brief   Este archivo contiene la definición de la clase HC05 para el manejo del módulo Bluetooth HC-05 por UART.
 */

#ifndef _HC05_H
#define _HC05_H

#include "UART_Device.h"
#include "string_handlers.h"

/**
 * \def SLAVE
 * \brief Modo esclavo del HC-05.
 */
#define SLAVE       (uint8_t)0
/**
 * \def MASTER
 * \brief Modo maestro del HC-05.
 */
#define MASTER      (uint8_t)1
/**
 * \def SLAVE_LOOP
 * \brief Modo esclavo en bucle del HC-05.
 */
#define SLAVE_LOOP  (uint8_t)2

// Comandos AT para configuración y control del HC-05
#define TEST_CMD        "AT"            ///< Comando de prueba.
#define RESET_CMD       "AT+RESET"      ///< Comando para reiniciar el módulo.
#define ADDR_CMD        "AT+ADDR"       ///< Comando para obtener la dirección MAC.
#define NAME_CMD        "AT+NAME"       ///< Comando para obtener/establecer el nombre.
#define ROLE_CMD        "AT+ROLE"       ///< Comando para establecer el rol.
#define UART_CMD        "AT+UART"       ///< Comando para configurar UART.
#define CMODE_CMD       "AT+CMODE"      ///< Comando para modo de conexión.
#define BIND_CMD        "AT+BIND"       ///< Comando para vincular dispositivo.
#define STATE_CMD       "AT+STATE"      ///< Comando para obtener el estado.
#define PAIR_CMD        "AT+PAIR"       ///< Comando para emparejar dispositivo.
#define LINK_CMD        "AT+LINK"       ///< Comando para conectar dispositivo.
#define INQM_CMD        "AT+INQM"       ///< Comando para configurar modo de consulta.
#define INQ_CMD         "AT+INQ"        ///< Comando para consultar dispositivos.
#define PSWD_CMD        "AT+PSWD"       ///< Comando para establecer contraseña.
#define RNAME_CMD       "AT+RNAME"      ///< Comando para obtener nombre remoto.
#define CLEAR_CMD       "AT+RMAAD"      ///< Comando para borrar dispositivos vinculados.
#define INIT_CMD        "AT+INIT"       ///< Comando para inicializar SPP.
#define RESTORE_CMD     "AT+ORGL"       ///< Comando para restaurar configuración de fábrica.
#define DISCONNECT_CMD  "AT+DISC"       ///< Comando para desconectar.
#define SEEK_DEVICE_CMD "AT+FSAD"       ///< Comando para buscar dispositivo específico.

#define SUCCESS_RESPONSE    "OK"        ///< Respuesta de éxito.
#define FAIL_RESPONSE       "FAIL"      ///< Respuesta de fallo.

#define HC05_BUFFER_LEN 32              ///< Tamaño del buffer para respuestas.

/**
 * \enum    state
 * \brief   Estados posibles del módulo HC-05.
 */
typedef enum : uint8_t
{
    INITIALIZED,    ///< Inicializado.
    READY,          ///< Listo.
    PAIRABLE,       ///< Emparejable.
    PAIRED,         ///< Emparejado.
    INQUIRING,      ///< Consultando dispositivos.
    CONNECTING,     ///< Conectando.
    CONNECTED,      ///< Conectado.
    DISCONNECTED,   ///< Desconectado.
    UNKNOWN         ///< Estado desconocido.
} state;

/**
 * \enum    connection_mode
 * \brief   Modos de conexión soportados por el HC-05.
 */
typedef enum : uint8_t
{
    _SPECIFIC,      ///< Conexión a dispositivo específico.
    _ANY,           ///< Conexión a cualquier dispositivo.
    _SLAVE_LOOP     ///< Modo esclavo en bucle.
} connection_mode;

/**
 * \struct  inquire_mode
 * \brief   Estructura para configurar el modo de consulta de dispositivos Bluetooth.
 */
typedef struct
{
    bool access_mode;           ///< Modo de acceso (público/privado).
    uint8_t max_bt_devices;     ///< Máximo de dispositivos Bluetooth a consultar.
    uint8_t max_inquiring_time; ///< Tiempo máximo de consulta.
} inquire_mode;

/**
 * \struct  hc05_param_struct
 * \brief   Estructura de parámetros para inicializar el módulo HC-05.
 * 
 * Hereda de uart_param_struct y añade dirección MAC y pin de estado.
 */
struct hc05_param_struct : uart_param_struct
{
    hc05_param_struct() { this->m_crc = NO_CRC; }
    char m_mac_address[18];     ///< Dirección MAC del módulo.
    int8_t m_state_pin = -1;    ///< Pin de estado del módulo.
};

/**
 * \class HC05
 * 
 * \brief Clase para el manejo del módulo Bluetooth HC-05 por UART.
 * 
 * Permite configurar, conectar, emparejar y consultar dispositivos Bluetooth mediante comandos AT.
 */
class HC05 : public UART_Device
{
    public:
        ~HC05() { }

        /**
         * \brief           Inicializa el módulo HC-05 con los parámetros especificados.
         * \param p_params  Parámetros de configuración.
         */
        void begin(const device_param_struct *p_params) override;

        /**
         * \brief               Formatea la dirección MAC recibida.
         * \param p_mac_address Cadena con la dirección MAC.
         */
        void format_mac_address(const char *p_mac_address);

        /**
         * \brief           Establece los parámetros del dispositivo HC-05.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(const device_param_struct *p_params);

        /**
         * \brief           Obtiene los parámetros actuales del dispositivo HC-05.
         * \param p_params  Parámetros del dispositivo.
         */
        void get_device_parameters(device_param_struct *p_params);

        /**
         * \brief               Configura la comunicación UART del módulo.
         * \param p_BAUD_RATE   Velocidad de transmisión.
         * \param p_stop_bit    Bit de parada.
         * \param p_parity      Paridad.
         * \returns             true: El comando se ejecutó exitosamente.
         */
        bool config_uart(uint32_t p_BAUD_RATE, bool p_stop_bit = 1, uint8_t p_parity = 0);

        /**
         * \brief           Establece el rol del módulo (maestro/esclavo).
         * \param p_role    Rol a establecer.
         * \returns         true: El comando se ejecutó exitosamente.
         */
        bool set_role(uint8_t p_role);

        /**
         * \brief   Obtiene el rol actual del módulo.
         * \returns Rol actual.
         */
        int8_t get_role();

        /**
         * \brief   Indica si el módulo está conectado.
         * \returns true: Está conectado.
         */
        bool connected();

        /**
         * \brief               Conecta al dispositivo especificado usando dirección MAC y contraseña.
         * \param p_mac_address Dirección MAC del dispositivo.
         * \param p_password    Contraseña.
         * \returns             true: El comando se ejecutó exitosamente.
         */
        bool connect(const char *p_mac_address, const char *p_password);

        /**
         * \brief               Conecta al dispositivo especificado usando dirección MAC.
         * \param p_mac_address Dirección MAC del dispositivo.
         * \returns             true: El comando se ejecutó exitosamente.
         */
        bool connect(const char *p_mac_address);

        /**
         * \brief               Empareja con el dispositivo especificado.
         * \param p_mac_address Dirección MAC del dispositivo.
         * \param p_time        Tiempo de emparejamiento.
         * \returns             true: El comando se ejecutó exitosamente.
         */
        bool pair(const char *p_mac_address, uint8_t p_time);

        /**
         * \brief           Empareja con cualquier dispositivo disponible.
         * \param p_time    Tiempo de emparejamiento.
         * \returns         true: El comando se ejecutó exitosamente.
         */
        bool pair(uint8_t p_time);

        /**
         * \brief               Conecta al dispositivo vinculado.
         * \param p_mac_address Dirección MAC del dispositivo.
         * \returns             true: El comando se ejecutó exitosamente.
         */
        bool link(const char *p_mac_address);

        /**
         * \brief Vincula el dispositivo especificado.
         * \param p_mac_address Dirección MAC del dispositivo.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool bind(const char *p_mac_address);

        /**
         * \brief Busca un dispositivo específico por dirección MAC.
         * \param p_mac_address Dirección MAC del dispositivo.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool seek_device(const char *p_mac_address);

        /**
         * \brief Obtiene el dispositivo vinculado.
         * \param p_mac_addres Buffer para guardar la dirección MAC.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool get_binded_device(char *p_mac_addres);

        /**
         * \brief Establece la contraseña del módulo.
         * \param p_password Contraseña a establecer.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool set_password(const char *p_password);

        /**
         * \brief Obtiene la contraseña actual del módulo.
         * \param p_password Buffer para guardar la contraseña.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool get_password(char *p_password);

        /**
         * \brief Elimina las conexiones guardadas en el módulo.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool delete_saved_connections();

        /**
         * \brief Configura el modo de consulta de dispositivos.
         * \param p_inqm Estructura con la configuración.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool config_inquire(inquire_mode p_inqm);

        /**
         * \brief   Obtiene la configuración actual del modo de consulta.
         * \returns Estructura con la configuración.
         */
        inquire_mode get_inquire_mode();

        /**
         * \brief Configura el modo de conexión del módulo.
         * \param p_mode Modo de conexión.
         * \returns true: El comando se ejecutó exitosamente.
         */
        bool config_connection_mode(connection_mode p_mode);

        /**
         * \brief   Obtiene el modo de conexión actual.
         * \returns Modo de conexión.
         */
        connection_mode get_connection_mode();

        /**
         * \brief Reinicia el módulo HC-05.
         */
        void reset();

        /**
         * \brief Restaura la configuración por defecto del módulo.
         */
        void default_config();

        /**
         * \brief   Verifica el estado actual del módulo.
         * \returns Estado actual.
         */
        state check_state();

    public:
        static const status COMMAND_FAIL  = 6; ///< Error al ejecutar el comando.
        static const status DISCONNECTED  = 7; ///< El módulo está desconectado.
        static const status LONG_PASSWORD = 8; ///< Contraseña demasiado larga.
        static const status CONNECT_ERROR = 9; ///< Error al conectar.

    protected:
        char m_response[HC05_BUFFER_LEN]; ///< Buffer para respuestas del módulo.

    private:
        state m_state = DISCONNECTED;       ///< Estado actual del módulo.
        int8_t m_state_pin = false;         ///< Pin de estado del módulo.

        const char *m_state_str[9]
        {
            "INITIALIZED", "READY", "PAIRABLE", "PAIRED", "INQUIRING", 
            "CONNECTING", "CONNECTED", "DISCONNECTED", "UNKNOWN"
        }; ///< Arreglo de cadenas para los estados.

        char m_mac_address[15] = "";           ///< Dirección MAC local.

    private:
        /**
         * \brief           Envía un comando al módulo HC-05.
         * \param p_command Estructura del comando.
         * \returns         Estado de la operación.
         */
        status send_command(command_struct *p_command) override;

        /*---------- Implementación para compatibilidad ----------*/
        /**
         * \brief           Calcula el CRC de los datos enviados y recibidos.
         * \param p_data    Buffer de datos.
         * \param p_length  Longitud del buffer.
         * \param p_check   Indica si se revisa el CRC.
         * \returns         CRC calculado.
         */
        uint16_t crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check = false) { return 0; }
};

/**
 * \class HC05_Builder
 * 
 * \brief Clase constructora para dispositivos HC05.
 * 
 * Permite establecer los parámetros de configuración y obtener una instancia lista para su uso.
 */
class HC05_Builder : public UART_Device_Builder
{
    public:
        ~HC05_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo HC05.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            hc05_param_struct *p = static_cast<hc05_param_struct *>(p_params);
            this->m_hc05_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_hc05_params.m_rx_pin = p->m_rx_pin;
            this->m_hc05_params.m_tx_pin = p->m_tx_pin;
            this->m_hc05_params.m_enable_pin = p->m_enable_pin;
            this->m_hc05_params.m_timeout = p->m_timeout;
            this->m_hc05_params.m_enable_state = p->m_enable_state;
            this->m_hc05_params.m_type = p->m_type;
            this->m_hc05_params.m_crc = p->m_crc;
            this->m_hc05_params.m_port = p->m_port;
            this->m_hc05_params.m_config = p->m_config;
            this->m_hc05_params.m_state_pin = p->m_state_pin;

            strcpy(this->m_hc05_params.m_mac_address, (const char *)p->m_mac_address);
        }

        /**
         * \brief   Obtiene una instancia del dispositivo HC05 construido.
         * \returns Puntero al dispositivo HC05.
         */
        Device *get_device() override
        {
            this->hc05_device.begin(&this->m_hc05_params);

            return &this->hc05_device;
        }
    private:
        hc05_param_struct m_hc05_params; ///< Parámetros del dispositivo HC05.
        HC05 hc05_device;                ///< Instancia del dispositivo HC05.
};

#endif