/**
 * \file    Global_Client.h
 * 
 * \brief   Este archivo contiene la definición de la clase Global_Client y estructuras relacionadas para la gestión de comunicación con servidores (HTTP, WebSockets, Socket.IO).
 */

#ifndef _GLOBAL_CLIENT_H  
#define _GLOBAL_CLIENT_H

#include <Arduino.h>
#include <ESP_SSLClient.h>
#include <UIPEthernet.h>
//#include <Ethernet.h>
#include "string_handlers.h"

#if defined(ESP32)
    #include "Logger.h"
#endif

/**
 * \def     MAX_HEADERS
 * \brief   Máximo número de cabeceras HTTP.
 */
#define MAX_HTTP_HEADERS 8

/**
 * \def     HTTP_ERROR_NOT_CONNECTED
 * \brief   Código de error para conexión no establecida.
 */
#define HTTP_ERROR_NOT_CONNECTED     -1

/**
 * \def     HTTP_ERROR_READ_TIMEOUT
 * \brief   Código de error para tiempo de espera excedido en lectura.
 */
#define HTTP_ERROR_READ_TIMEOUT      -2

/**
 * \def     HTTP_ERROR_NO_STREAM
 * \brief   Código de error para ausencia de flujo de datos.
 */
#define HTTP_ERROR_NO_STREAM         -3

/**
 * \def     HTTP_ERROR_NO_DEFINED_SERVER
 * \brief   Código de error para servidor no definido.
 */
#define HTTP_ERROR_NO_DEFINED_SERVER -4

/**
 * \def     HTTP_INVALID_REQUEST
 * \brief   Código de error para peticiones invalidas.
 */
#define HTTP_INVALID_REQUEST         -5

/**
 * 
 */
#define HTTP_ERROR_BUFFER_OVERFLOW   -6

/**
 * \def CA_CERT_SIZE
 * \brief Tamaño máximo para el certificado SSL.
 */
#define CA_CERT_SIZE 2048

/**
 * \enum    client_type
 * \brief   Tipos de cliente soportados para comunicación.
 */
typedef enum : uint8_t
{
    HTTP,           ///< Cliente HTTP.
    WEBSOCKETS,     ///< Cliente WebSockets.
    SOCKET_IO,      ///< Cliente Socket.IO.
} client_type;

/**
 * \struct  client_params
 * \brief   Estructura para almacenar parámetros de configuración del cliente.
 */
struct client_params
{
    client_type m_type;                     ///< Tipo de cliente.
    char *m_server = nullptr;               ///< Dirección del servidor.
    uint16_t m_port;                        ///< Puerto de conexión.
};

/**
 * \class Global_Client
 * 
 * \brief Clase base para la gestión de comunicación con servidores mediante diferentes protocolos.
 * 
 * Proporciona métodos virtuales para enviar y recibir datos, así como métodos para configurar parámetros de conexión.
 */
class Global_Client
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~Global_Client() { /*delete this->m_ssl_client;*/ }

        /**
         * \brief               Envía datos al servidor.
         * \param p_data        Buffer con los datos a enviar.
         * \param p_data_length Longitud de los datos.
         * \returns             Código de estado o cantidad de datos enviados.
         */
        virtual int send_data_to_server(const uint8_t *p_data, size_t p_data_length) = 0;

        /**
         * \brief                   Obtiene datos del servidor.
         * \param p_buffer          Buffer donde se almacenarán los datos recibidos.
         * \param p_buffer_length   Longitud máxima del buffer.
         * \returns                 Código de estado o cantidad de datos recibidos.
         */
        virtual int get_data_from_server(uint8_t *p_buffer, size_t p_buffer_length) = 0;

        /**
         * \brief           Establece los parámetros de configuración del cliente.
         * \param p_params  Estructura con los parámetros de configuración.
         */
        void set_client_parameters(client_params p_params);

        /**
         * \brief   Conecta al servidor utilizando los parámetros configurados.
         * \returns true si la conexión fue exitosa, false en caso contrario.
         */
        bool connect_to_server();
        
        /**
         * \brief                       Establece encabezados personalizados para la conexión.
         * \param p_custom_header       Nombre del encabezado.
         * \param p_custom_header_value Valor del encabezado.
         * \param p_index               Indice en donde se guardara el encabezado.
         */
        void set_header(const char *p_header_name, const char *p_header_value, uint8_t p_index);

        void flush_headers();

        /**
         * \brief               Establece el endpoint para las peticiones.
         * \param p_endpoint    Cadena con el endpoint.
         */
        inline void set_endpoint(const char *p_endpoint)
        {
            this->m_endpoint = (char *)p_endpoint;
        }

        /**
         * \brief           Establece la dirección del servidor.
         * \param p_server  Cadena con la dirección del servidor.
         */
        inline void set_server(const char *p_server)
        {
            this->m_server = (char *)p_server;
        }

        /**
         * \brief           Establece el puerto de conexión.
         * \param p_port    Número de puerto.
         */
        inline void set_port(uint16_t p_port)
        {
            this->m_port = p_port;
        }

        /**
         * \brief           Establece si la conexión es segura (SSL).
         * \param p_secure  true para conexión segura, false para no segura.
         */
        inline void set_secure(bool p_secure)
        {
            this->m_secure = p_secure;
        }

        inline void set_certificate(const char *p_certificate)
        {
            this->m_certificate = (char *)p_certificate;
        }

        virtual inline void client_stop()
        {
            this->m_global_client->stop();
        }


    protected:
        Client *m_global_client = nullptr;          ///< Cliente base (puede ser Ethernet o SSL).
        EthernetClient m_client;                    ///< Cliente Ethernet para AVR.
        ESP_SSLClient m_ssl_client;                 ///< Cliente SSL (para conexiones seguras).
        //SSLClient *m_ssl_client = new SSLClient(m_client, TAs, (size_t)TAs_NUM, A0);
        char *m_server = nullptr;                   ///< Dirección del servidor.
        char *m_endpoint = nullptr;                 ///< Endpoint para peticiones.
        uint16_t m_port;                            ///< Puerto de conexión.
        client_type m_type;                         ///< Tipo de cliente.
        bool m_secure = false;                      ///< Indica si la conexión es segura (SSL).
        char *m_certificate = nullptr;              ///< Certificado SSL.

       /* static char *m_certificate[CA_CERT_SIZE] = R"EOF(
            -----BEGIN CERTIFICATE-----
            [TU CERTIFICADO ROOT CA AQUÍ] 
            -----END CERTIFICATE-----
        )EOF";*/

        char *m_header[MAX_HTTP_HEADERS] =          ///< Encabezados personalizados.
        { 
            nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr
        };

        char *m_header_value[MAX_HTTP_HEADERS] =    ///< Valor de los encabezados personalizados.
        { 
            nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr
        }; 

    protected:
        /**
         * \brief               Realizar peticiones HTTP al servidor.
         * \param p_type        Tipo de petición a realizar.
         * \param http_version  Indica si se debe usar HTTP/1.1 (true) o no (false).
         * \param p_data        Buffer de datos a enviar(opcional).
         * \returns             Código HTTP de la petición.
         */
        bool send_http_request(const char *p_type, bool http_version = true, const uint8_t *p_data = nullptr, size_t p_data_length = 0);

        /**
         * 
         * 
         */
        int http_response(const char p_headers[][128], char p_headers_values[][64], int p_headers_count, uint8_t *p_data = nullptr, size_t p_data_length = 0);

        int http_response()
        {
            return http_response(nullptr, nullptr, 0);
        }

    private:
        const char *m_requests[4] = { "POST", "GET", "PUT", "PATCH" };
};        

#endif