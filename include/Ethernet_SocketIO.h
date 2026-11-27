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
#define MAX_NUMBER_OF_EVENTS 6

/**
 * \def     MAX_NUMBER_OF_NAMESPACES
 * \brief   Número máximo de namespaces a los que se puede conectar el cliente Socket.IO.
 */
#define MAX_NUMBER_OF_NAMPESPACES 4

/**
 * \def     SOCKETIO_RECONNECT_MIN_MS
 * \brief   Espera después del primer intento de reconexión fallido. Se duplica en cada
 *          fallo seguido hasta SOCKETIO_RECONNECT_MAX_MS.
 */
#define SOCKETIO_RECONNECT_MIN_MS 5000UL

/**
 * \def     SOCKETIO_RECONNECT_MAX_MS
 * \brief   Espera máxima entre intentos de reconexión con el servidor Socket.IO.
 */
#define SOCKETIO_RECONNECT_MAX_MS 60000UL

/**
 * \def     SOCKETIO_CONNECT_MESSAGE_SIZE
 * \brief   Tamaño del mensaje para conectarse a un namespace: "40", el namespace, el
 *          identificador y la llave (hasta 71 caracteres) caben con holgura. Un mensaje
 *          cortado sería JSON inválido y el servidor rechazaría la conexión.
 */
#define SOCKETIO_CONNECT_MESSAGE_SIZE 192

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
 * \typedef reconnect_callback
 * \brief   Función que se llama al restablecer la conexión con el servidor. Corre dentro de
 *          handle_message(), con http_mutex tomado: no debe hacer peticiones HTTP.
 */
typedef void(*reconnect_callback)();

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
         * \brief   Se une a todos los namespaces registrados.
         * \details Si alguno no responde o el servidor rechaza la conexión (por ejemplo, por
         *          la llave), cierra el socket: sin namespace no llega ningún evento aunque
         *          sigan los pings. handle_message() lo toma como desconexión y reintenta con
         *          espera (de SOCKETIO_RECONNECT_MIN_MS a SOCKETIO_RECONNECT_MAX_MS).
         * \returns true si se unió a todos.
         */
        bool connect_namespaces();

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
         * \brief                       Función a llamar cada vez que se restablece la conexión
         *                              (no en la primera conexión al arrancar).
         * \param p_callback_function   Función callback (ver reconnect_callback).
         */
        inline void set_reconnect_callback(reconnect_callback p_callback_function)
        {
            this->m_reconnect_callback = p_callback_function;
        }

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

        /**
         * \brief           Agrega la llave al objeto con el que se une al namespace, junto al
         *                  identificador: {"<identificador>":"<valor>","<p_name>":"<llave>"}.
         * \details         Se lee en cada conexión, así que una llave nueva se manda desde la
         *                  siguiente reconexión. Quien la cambie debe tener el mismo mutex que la
         *                  conexión (http_mutex). Un servidor que no la valida ignora el campo.
         * \param p_name    Nombre del campo.
         * \param p_value   Buffer con la llave terminada en nulo; vacío = no se manda.
         */
        void set_auth_key(const char *p_name, const char *p_value);

    private:
        char *m_namespace[MAX_NUMBER_OF_NAMPESPACES] =      ///< Arreglo de namespaces registrados.
        {
            nullptr, nullptr, nullptr, nullptr
        };
        char m_sid[25] = { 0 };                             ///< Session ID.
        char *m_identifier = nullptr;                       ///< Nombre del identificador.
        char *m_identifier_value = nullptr;                 ///< Valor del identificador.
        const char *m_auth_key = nullptr;                   ///< Nombre del campo de la llave.
        const char *m_auth_key_value = nullptr;             ///< Buffer con la llave (se lee en cada conexión).
        bool m_namespace_rejected = false;                  ///< El último intento fue rechazado (para registrarlo solo la primera vez).
        event_callback m_callback_functions[MAX_NUMBER_OF_EVENTS] = { 0 }; ///< Arreglo de funciones callback para eventos.
        char *m_events[MAX_NUMBER_OF_EVENTS] = { 0 };       ///< Arreglo de nombres de eventos.
        uint8_t m_event_counter = 0;                        ///< Contador de eventos registrados.
        unsigned long m_ping_timeout = 0;                   ///< Tiempo máximo de espera para ping.
        unsigned long m_ping_interval = 0;                  ///< Intervalo entre pings.
        unsigned long m_last_ping = 0;                      ///< Marca de tiempo del último ping.
        uint8_t m_namespace_counter = 0;                    ///< Contador de namespaces registrados.
        bool m_connected = true;                            ///< Último estado registrado en el log (para registrar solo los cambios).
        bool m_reconnecting = false;                        ///< Se perdió la conexión y se está reintentando.
        unsigned long m_reconnect_delay = 0;                ///< Espera antes del siguiente intento (0: inmediato).
        unsigned long m_last_reconnect_millis = 0;          ///< Fin del último intento de reconexión fallido.
        reconnect_callback m_reconnect_callback = nullptr;  ///< Se llama al restablecer la conexión.

    private:
        /**
         * \brief   Marca la conexión como perdida y prepara la reconexión: el primer intento
         *          es inmediato y los siguientes esperan cada vez más (ver reconnect()).
         */
        void begin_reconnect();

        /**
         * \brief   Intenta reconectar si ya pasó la espera desde el último intento fallido.
         *          Si falla, la espera se duplica de SOCKETIO_RECONNECT_MIN_MS hasta
         *          SOCKETIO_RECONNECT_MAX_MS.
         */
        void reconnect();

        /**
         * \brief               Registra en el log un cambio en la conexión con el servidor; si
         *                      el estado no cambió, el aviso va al nivel de detalle (debug).
         * \param p_connected   Estado nuevo de la conexión.
         * \param p_message     Aviso a registrar.
         */
        void report_connection(bool p_connected, const char *p_message);

        /**
         * \brief   Llave a mandar al unirse al namespace.
         * \returns La llave, o nullptr si no hay o trae caracteres que romperían el JSON.
         */
        const char *auth_key_to_send() const;

        /**
         * \brief                   Arma el mensaje para unirse a un namespace.
         * \param p_buffer          Buffer destino.
         * \param p_size            Tamaño del buffer.
         * \param p_namespace_index Índice del namespace.
         * \param p_key             Llave a incluir, o nullptr para mandar solo el identificador.
         * \returns                 true si el mensaje cupo completo.
         */
        bool build_connect_message(char *p_buffer, size_t p_size, int p_namespace_index, const char *p_key) const;

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