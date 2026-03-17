/**
 * \file    Ethernet_WebSocket.h
 * 
 * \brief   Este archivo contiene la definición de la clase Ethernet_WebSocket para el manejo de comunicación WebSocket sobre Ethernet.
 */

#ifndef _ETHERNET_WEBSOCKET
#define _ETHERNET_WEBSOCKET

#include <Global_Client.h>
#include <mbedtls/base64.h>
#include <mbedtls/sha1.h>

/**
 * \def     WEBSOCKET_TEXT_MESSAGE
 * \brief   Identificador para mensajes de texto WebSocket.
 */
#define WEBSOCKET_TEXT_MESSAGE           2

/**
 * \def     WEBSOCKET_BINARY_MESSAGE
 * \brief   Identificador para mensajes binarios WebSocket.
 */
#define WEBSOCKET_BINARY_MESSAGE         1

/**
 * \def     WEBSOCKET_ERROR_NOT_CONNECTED
 * \brief   Código de error para WebSocket no conectado.
 */
#define WEBSOCKET_ERROR_NOT_CONNECTED   -1

/**
 * \def     WEBSOCKET_ERROR_NO_STREAM
 * \brief   Código de error para ausencia de flujo en WebSocket.
 */
#define WEBSOCKET_ERROR_NO_STREAM       -2

/**
 * \def     WEBSOCKET_ERROR_MASKED_MESSAGE
 * \brief   Código de error para mensaje enmascarado.
 */
#define WEBSOCKET_ERROR_MASKED_MESSAGE  -3

/**
 * \def     WEBSOCKET_PING_MESSAGE
 * \brief   Identificador para mensaje ping WebSocket.
 */
#define WEBSOCKET_PING_MESSAGE          -4

/**
 * \def     WEBSOCKET_PONG_MESSAGE
 * \brief   Identificador para mensaje pong WebSocket.
 */
#define WEBSOCKET_PONG_MESSAGE          -5

/**
 * \def     WEBSOCKET_CLOSE_FRAME_MESSAGE
 * \brief   Identificador para mensaje de cierre WebSocket.
 */
#define WEBSOCKET_CLOSE_FRAME_MESSAGE   -6

/**
 * \def     WS_FIN
 * \brief   Bit FIN para frames WebSocket.
 */
#define WS_FIN          (uint8_t)0x80

/**
 * \def     WS_OPCODE_CON
 * \brief   Opcode para frame de continuación.
 */
#define WS_OPCODE_CON   (uint8_t)0x00

/**
 * \def     WS_OPCODE_TXT
 * \brief   Opcode para frame de texto.
 */
#define WS_OPCODE_TXT   (uint8_t)0x01

/**
 * \def     WS_OPCODE_BIN
 * \brief   Opcode para frame binario.
 */
#define WS_OPCODE_BIN   (uint8_t)0x02

/**
 * \def     WS_OPCODE_CLOSE
 * \brief   Opcode para frame de cierre.
 */
#define WS_OPCODE_CLOSE (uint8_t)0x08

/**
 * \def     WS_OPCODE_PING
 * \brief   Opcode para frame ping.
 */
#define WS_OPCODE_PING  (uint8_t)0x09

/**
 * \def     WS_OPCODE_PONG
 * \brief   Opcode para frame pong.
 */
#define WS_OPCODE_PONG  (uint8_t)0x0A

/**
 * \def     WS_MASK
 * \brief   Bit MASK para frames WebSocket.
 */
#define WS_MASK         (uint8_t)0x80

#define MAX_PONG_MESSAGE_SIZE 32
#define MAX_PING_MESSAGE_SIZE 32
#define MAX_MESSAGE_SIZE 512
#define MAX_CLOSE_MESSAGE_SIZE 64

/**
 * \enum    websocket_close_status_t
 * \brief   Enumeración de códigos de cierre para WebSocket.
 */
typedef enum : uint16_t
{
    NORMAL_CLOSURE = 1000,   ///< Cierre normal.
    GOING_AWAY,              ///< El servidor se va.
    PROTOCOL_ERROR,          ///< Error de protocolo.
    DATA_ERROR,              ///< Error de datos.
    TYPE_ERROR = 1007,       ///< Error de tipo de datos.
    POLICY_ERROR,            ///< Error de política.
    MESSAGE_TOO_BIG,         ///< Mensaje demasiado grande.
    CLIENT_ERROR,            ///< Error de cliente.
    UNEXPECTED_ERROR,        ///< Error inesperado.
} websocket_close_status_t;

/**
 * \class Ethernet_WebSocket
 * 
 * \brief Clase para gestionar la comunicación WebSocket sobre Ethernet.
 * 
 * Hereda de Global_Client y proporciona métodos para enviar y recibir mensajes, gestionar la conexión y manejar frames WebSocket.
 */
class Ethernet_WebSocket : public Global_Client
{
    public:
        /**
         * \brief Destructor.
         */
        ~Ethernet_WebSocket() { } 

        /**
         * \brief               Obtiene datos del servidor WebSocket.
         * \param p_data        Buffer donde se almacenarán los datos recibidos.
         * \param p_buffer_len  Longitud máxima del buffer.
         * \returns             Cantidad de datos recibidos.
         */
        int get_data_from_server(uint8_t *p_data, size_t p_buffer_len) override;

        /**
         * \brief           Envía datos al servidor WebSocket.
         * \param p_data    Buffer con los datos a enviar.
         * \param p_length  Longitud de los datos.
         * \returns         Cantidad de datos enviados.
         */
        int send_data_to_server(const uint8_t *p_data, size_t p_length) override;

        /**
         * \brief   Inicia el cliente WebSocket.
         * \returns true: La conexión fue exitosa.
         *          false: Caso contrario.
         */
        virtual bool start_websocket_client();

        /**
         * \brief       Establece la URL del servidor WebSocket.
         * \param p_url Cadena con la URL.
         */
        void set_ws_server(const char *p_url);

        /**
         * \brief               Establece el endpoint para la conexión WebSocket.
         * \param p_endpoint    Cadena con el endpoint.
         */
        void set_ws_endpoint(const char *p_endpoint);

        /**
         * \brief           Envía un mensaje pong al servidor.
         * \param p_payload Datos opcionales a enviar.
         * \param p_length  Longitud de los datos.
         */
        virtual void send_pong_message(uint8_t *p_payload = nullptr, size_t p_length = 0);

        /**
         * \brief           Envía un mensaje ping al servidor.
         * \param p_payload Datos a enviar.
         * \param p_length  Longitud de los datos.
         */
        virtual void send_ping_message(uint8_t *p_payload = nullptr, size_t p_length = 0);

        /**
         * \brief           Envía un mensaje de cierre al servidor.
         * \param p_message Mensaje de cierre.
         */
        void send_close_message(const char *p_message = nullptr);

        /**
         * \brief           Envía un mensaje binario al servidor.
         * \param p_data    Buffer con los datos binarios.
         * \param p_length  Longitud de los datos.
         */
        int send_binary_message(uint8_t *p_data, size_t p_length);

        /**
         * \brief           Envía un mensaje de texto al servidor.
         * \param p_string  Cadena de texto a enviar.
         */
        int send_text_message(const char *p_string);

        /**
         * \brief               Obtiene un mensaje recibido del servidor.
         * \param p_data        Buffer donde se almacenarán los datos recibidos.
         * \param p_length      Puntero para almacenar la longitud del mensaje recibido.
         * \param p_buffer_len  Longitud máxima del buffer.
         * \returns             Tipo de mensaje recibido o longitud de datos.
         */
        int get_message(uint8_t *p_data, int *p_length, size_t p_buffer_len);
        
        /**
         * \brief   Obtiene el código de estado de cierre WebSocket.
         * \returns Código de cierre actual.
         */
        inline websocket_close_status_t get_status_code() const
        {
            return this->m_status_code;
        }

        /**
         * 
         */
        inline void set_status_code(websocket_close_status_t p_status_code)
        {
            this->m_status_code = p_status_code;
        }

        /**
         * \brief   Indica si el cliente está conectado.
         * \returns true: Está conectado.
         *          false: Caso contrario.
         */
        inline bool connected()
        {
            return this->m_client.connected();
        }

    protected:
        websocket_close_status_t m_status_code = NORMAL_CLOSURE; ///< Código de estado de cierre WebSocket.
        
    protected:
        /**
         * \brief           Genera una clave para la conexión WebSocket.
         * \param p_output  Buffer donde se almacenará la clave generada.
         * \param p_length  Puntero para almacenar la longitud de la clave.
         * \returns         true: La clave fue generada correctamente
         *                  false: En caso contrario.
         */
        bool generate_websocket_key(char *p_output, size_t *p_length);

        /**
         * \brief   Realiza el handshake inicial con el servidor WebSocket.
         * \returns true: El handshake fue exitoso.
         *          false: En caso contrario.
         */
        virtual bool handshake();

        /**
         * \brief               Verifica que la clave esperada esté en la respuesta.
         * \param p_response    Cadena con la respuesta del servidor.
         * \param p_key         Clave a buscar.
         * \returns             true: La clave fue encontrada.
         *                      false: En caso contrario.
         */
        bool check_key(const char *p_response, const char *p_key);

        /**
         * \brief           Maneja los datos recibidos por WebSocket.
         * \param p_data    Buffer con los datos recibidos.
         * \param p_length  Longitud de los datos.
         * \returns         Código de estado o tipo de mensaje procesado.
         */
        int handle_websocket_data(uint8_t *p_data, size_t p_length);

    private:
        int send_message(uint8_t p_opcode, uint8_t *p_payload, size_t p_length);

};

#endif