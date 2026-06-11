/**
 * \file    Ethernet_SocketIO.h
 * 
 * \brief   Este archivo contiene la definición de la clase Ethernet_SocketIO para el manejo de comunicación Socket.IO sobre Ethernet.
 */

#ifndef _ETHERNET_SOCKET_IO
#define _ETHERNET_SOCKET_IO

#include "Ethernet_WebSocket.h"

/**
 * \def     MAX_NUMBER_OF_EVENTS
 * \brief   Número máximo de eventos que puede manejar el cliente Socket.IO.
 */
#define MAX_NUMBER_OF_EVENTS 4

/**
 * \def     MAX_NUMBER_OF_NAMESPACES
 * \brief   Número máximo de namespaces a los que se puede conectar el cliente Socket.IO.
 */
#define MAX_NUMBER_OF_NAMPESPACES 4
/**
 * \enum    engine_io_msg_t
 * \brief   Tipos de mensajes de Engine.IO.
 */


typedef enum : char
{
    OPEN_MESSAGE = '0',      ///< Mensaje de apertura de conexión.
    CLOSE_MESSAGE,           ///< Mensaje de cierre de conexión.
    PING_MESSAGE,            ///< Mensaje de ping.
    PONG_MESSAGE,            ///< Mensaje de pong.
    NORMAL_MESSAGE,          ///< Mensaje normal.
    UPGRADE_MESSAGE,         ///< Mensaje de actualización.
    NOOP_MESSAGE,            ///< Mensaje sin operación.
} engine_io_msg_t;

/**
 * \enum    socket_io_msg_t
 * \brief   Tipos de mensajes de Socket.IO.
 */
typedef enum : char 
{
    CONNECT_MESSAGE = '0',       ///< Mensaje de conexión.
    DISCONNECT_MESSAGE,          ///< Mensaje de desconexión.
    EVENT_MESSAGE,               ///< Mensaje de evento.
    ACK_MESSAGE,                 ///< Mensaje de confirmación.
    CONENCT_ERROR_MESSAGE,       ///< Mensaje de error de conexión.
    BIN_EVENT_MESSAGE,           ///< Mensaje de evento binario.
    BIN_ACK_MESSAGE,             ///< Mensaje de confirmación binaria.
} socket_io_msg_t;

/**
 * \typedef event_callback
 * \brief   Definición de tipo para funciones de callback de eventos.
 */
typedef void(*event_callback)(const char *);

/**
 * \class Ethernet_SocketIO
 * 
 * \brief Clase para gestionar la comunicación Socket.IO sobre Ethernet.
 * 
 * Hereda de Ethernet_WebSocket y proporciona métodos para enviar mensajes, manejar eventos y gestionar la conexión con namespaces.
 */
class Ethernet_SocketIO : public Ethernet_WebSocket
{
    public:
        /**
         * \brief Destructor.
         */
        ~Ethernet_SocketIO() { }

        /**
         * \brief   Inicia el cliente WebSocket para Socket.IO.
         * \returns true si la conexión fue exitosa, false en caso contrario.
         */
        bool start_websocket_client() override;

        /**
         * \brief           Envía un mensaje pong al servidor.
         * \param p_payload Datos opcionales a enviar.
         * \param p_length  Longitud de los datos.
         */
        void send_pong_message(uint8_t *p_payload = nullptr, size_t p_length = 0) override;

        /**
         * \brief           Envía un mensaje ping al servidor.
         * \param p_payload Datos a enviar.
         * \param p_length  Longitud de los datos.
         */
        void send_ping_message(uint8_t *p_payload, size_t p_length) override;

        /**
         * \brief           Establece un encabezado personalizado para la conexión.
         * \param p_header  Nombre del encabezado.
         * \param p_value   Valor del encabezado.
         */
        void set_custom_header(const char *p_header, const char *p_value);

        /**
         * \brief Envía un mensaje de actualización (upgrade) al servidor.
         */
        void send_upgrade_message();

        /**
         * \brief                   Conecta al namespace especificado.
         * \param p_namespace_index Indice del namespace (opcional).
         * \returns                 true si la conexión fue exitosa, false en caso contrario.
         */
        bool connect_to_namespace(int p_namespace_index = 0);

        /**
         * \brief Maneja los mensajes recibidos del servidor.
         */
        void handle_message();

        /**
         * \brief                       Asocia un evento con una función callback.
         * \param p_event               Nombre del evento.
         * \param p_callback_function   Función callback a ejecutar cuando se recibe el evento.
         */
        void set_event(const char *p_event, event_callback p_callback_function);

        /**
         * \brief               Envía un evento al servidor dentro del namespace indicado.
         * \param p_namespace    Namespace por el que se enviará el evento.
         * \param p_event_name   Nombre del evento a emitir.
         * \param p_payload      Datos (payload) asociados al evento.
         * \returns             true si el evento se envió correctamente, false en caso contrario.
         */
        bool send_event(const char *p_namespace, const char *p_event_name, const char *p_payload);

        /**
         * \brief               Establece el namespace para la conexión.
         * \param p_namespace   Cadena con el namespace.
         */
        inline int set_namespace(const char *p_namespace)
        {
            if(this->m_namespace_counter >= MAX_NUMBER_OF_NAMPESPACES)
                return -1;
                
            this->m_namespace[this->m_namespace_counter] = (char *)p_namespace;
            return this->m_namespace_counter++;
        }

        /**
         * \brief               Obtiene el índice asociado a un namespace registrado.
         * \param p_namespace    Cadena con el namespace a buscar.
         * \returns             Índice del namespace si existe, -1 si no se encuentra.
         */
        inline int get_namespace_index(const char *p_namespace)
        {
            for(int i = 0; i < MAX_NUMBER_OF_NAMPESPACES; i++)
            {
                if(this->m_namespace)
                    if(strcmp(p_namespace, this->m_namespace[i]) == 0)
                        return i;
            }

            return -1;
        }

        /**
         * \brief               Establece el identificador para unirse al namespace.
         * \param p_identifier  Nombre del identificador.
         * \param p_value       Valor del identificador.
         */
        void set_identifer(const char *p_identifier, const char *p_value);

    private:
        char *m_namespace[MAX_NUMBER_OF_NAMPESPACES] =      ///< Arreglo de namespaces registrados.
        {
            nullptr, nullptr, nullptr, nullptr
        };
        char m_sid[25] = { 0 };                             ///< Session ID.
        char *m_identifier = nullptr;                       ///< Nombre del identificador.
        char *m_identifier_value = nullptr;                 ///< Valor del identificador.
        event_callback m_callback_functions[MAX_NUMBER_OF_EVENTS] = { 0 }; ///< Arreglo de funciones callback para eventos.
        char *m_events[MAX_NUMBER_OF_EVENTS] = { 0 };       ///< Arreglo de nombres de eventos.
        uint8_t m_event_counter = 0;                        ///< Contador de eventos registrados.
        unsigned long m_ping_timeout = 0;                   ///< Tiempo máximo de espera para ping.
        unsigned long m_ping_interval = 0;                  ///< Intervalo entre pings.
        unsigned long m_last_ping = 0;                      ///< Marca de tiempo del último ping.
        uint8_t m_namespace_counter = 0;                    ///< Contador de namespaces registrados.

    private:
        /**
         * \brief   Realiza el handshake inicial con el servidor.
         * \returns true si el handshake fue exitoso, false en caso contrario.
         */
        bool handshake() override;

        /**
         * \brief               Obtiene los parámetros de sesión a partir de la respuesta del servidor.
         * \param p_response    Cadena con la respuesta del servidor.
         */
        void get_session_parameters(const char *p_response);

        /**
         * \brief           Maneja los datos recibidos de eventos.
         * \param p_data    Datos recibidos.
         * \param p_length  Longitud de los datos.
         */
        void event_handler(const char *p_data, size_t p_length);

        /**
         * \brief                   Extrae el nombre del evento de los datos recibidos.
         * \param p_data            Datos recibidos.
         * \param p_string          Buffer donde se almacenará el nombre del evento.
         * \param p_buffer_length   Longitud máxima del buffer.
         */
        void get_event_name(const char *p_data , char *p_string, size_t p_buffer_length);
};

#endif