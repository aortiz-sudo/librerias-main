/**
 * \file    Ethernet_HTTP.h
 * 
 * \brief   Este archivo contiene la definición de la clase Ethernet_HTTP para el manejo de comunicación HTTP sobre Ethernet.
 */

#ifndef _ETHERNET_HTTP
#define _ETHERNET_HTTP

#include <Global_Client.h>

/**
 * \class Ethernet_HTTP
 * 
 * \brief Clase para gestionar la comunicación HTTP sobre Ethernet.
 * 
 * Hereda de Global_Client y proporciona métodos para enviar, recibir y actualizar datos mediante peticiones HTTP.
 */
class Ethernet_HTTP : public Global_Client
{
    public:
        /**
         * \brief Destructor.
         */
        ~Ethernet_HTTP() { }

        /**
         * \brief                   Obtiene datos del servidor mediante una petición HTTP GET.
         * \param p_data            Buffer donde se almacenarán los datos recibidos.
         * \param p_data_length     Longitud máxima del buffer.
         * \returns                 Código de estado o cantidad de datos recibidos.
         */
        int get_data_from_server(uint8_t *p_data = nullptr, size_t p_data_length = 0) override;

        /**
         * \brief                   Envía datos al servidor mediante una petición HTTP POST.
         * \param p_data            Buffer con los datos a enviar.
         * \param p_data_length     Longitud de los datos a enviar.
         * \returns                 Código de estado o cantidad de datos enviados.
         */
        int send_data_to_server(const uint8_t *p_data, size_t p_data_length) override;

        /**
         * \brief                   Actualiza datos en el servidor mediante una petición HTTP PATCH.
         * \param p_json            Buffer con los datos en formato JSON.
         * \param p_data_length     Longitud de los datos JSON.
         * \returns                 Código de estado de la operación.
         */
        int update_data(const uint8_t *p_data, size_t p_data_length);

        /**
         * \brief           Descarga contenido desde alguna URL.
         * \param p_stream  Puntero a donde se quiere guardar el contenido.
         * \returns         Código de estado o cantidad de bytes descargados.
         */
        int download_content(Stream *p_stream);

    private:
        /**
         * \brief               Realiza una petición HTTP POST al servidor.
         * \param p_data        Buffer de datos.
         * \param p_data_length Tamaño del buffer de datos.
         * \returns             Código de estado de la operación.
         */
        int post_request(const uint8_t *p_data, size_t p_data_length);

        /**
         * \brief               Realiza una petición HTTP PATCH al servidor.
         * \param p_data        Buffer de datos.
         * \param p_data_length Tamaño del buffer de datos.
         * \returns             Código de estado de la operación.
         */
        int patch_request(const uint8_t *p_data, size_t p_data_length);

        /**
         * \brief               Realiza una petición HTTP GET al servidor.
         * \returns             Código de estado de la operación.
         */
        int get_request(int *p_content_length = nullptr);

        /**
         * \brief                       Obtiene el código HTTP de la respuesta del servidor.
         * \returns                     Respuesta HTTP.
         */
        int handle_response();
    
    private:

        char m_response[2048] = { 0 }; ///< Buffer para la respuesta HTTP del servidor.

};

#endif