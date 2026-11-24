#include "Ethernet_HTTP.h"

/// Tiempo máximo sin recibir datos mientras se lee el cuerpo de una respuesta.
static constexpr unsigned long HTTP_BODY_TIMEOUT_MS = 5000;

int Ethernet_HTTP::send_data_to_server(const uint8_t *p_data, size_t p_data_length)
{
    if(!this->m_server || !this->m_endpoint)
        return HTTP_ERROR_NO_DEFINED_SERVER;

    int http_code = post_request(p_data, p_data_length);

    logger.log("Codigo HTTP: ");
    logger.logln(http_code);

    return http_code;
}

int Ethernet_HTTP::get_data_from_server(uint8_t *p_data, size_t p_data_length)
{
    int content_length = -1;
    int http_code = get_request(&content_length);

    logger.log("Codigo HTTP: ");
    logger.logln(http_code);

    // En todas las salidas se cierra la conexión: antes los errores la dejaban
    // abierta y el W5500 solo tiene 8 sockets.
    if(http_code < 0)
    {
        if(this->m_global_client)
            this->m_global_client->stop();

        return http_code;
    }

    if(content_length < 1)
    {
        this->m_global_client->stop();
        return HTTP_ERROR_NO_STREAM;
    }

    if(content_length >= (int)p_data_length)
    {
        logger.logln("La respuesta no cabe en el buffer.");
        this->m_global_client->stop();
        return HTTP_ERROR_BUFFER_OVERFLOW;
    }

    if(http_code / 200 != 1)
    {
        this->m_global_client->stop();
        return -1;
    }

    // Cada bloque se agrega después de lo ya recibido. Antes se escribía siempre
    // al inicio del buffer y una respuesta que llegaba en varios segmentos TCP
    // (más de ~1460 bytes) quedaba corrupta.
    int byte_counter = 0;
    unsigned long last_data_millis = millis();

    while(byte_counter < content_length && (this->m_global_client->connected() || this->m_global_client->available()))
    {
        if(this->m_global_client->available())
        {
            int length = this->m_global_client->read(p_data + byte_counter, content_length - byte_counter);

            if(length > 0)
            {
                byte_counter += length;
                last_data_millis = millis();
                continue;
            }
        }

        if(millis() - last_data_millis > HTTP_BODY_TIMEOUT_MS)
            break;

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    this->m_global_client->stop();

    p_data[byte_counter] = '\0';

    if(byte_counter < content_length)
    {
        logger.log("Respuesta incompleta: ");
        logger.log(byte_counter);
        logger.log(" de ");
        logger.logln(content_length);
        return HTTP_ERROR_READ_TIMEOUT;
    }

    return byte_counter;
}

int Ethernet_HTTP::update_data(const uint8_t *p_data, size_t p_data_length)
{
    int http_code = patch_request(p_data, p_data_length);

    logger.log("Codigo HTTP: ");
    logger.logln(http_code);

    return http_code;
}

int Ethernet_HTTP::post_request(const uint8_t *p_data, size_t p_data_length)
{
    if(!connect_to_server())
        return HTTP_ERROR_NOT_CONNECTED;

    if(!send_http_request("POST", true, p_data, p_data_length))
        return HTTP_INVALID_REQUEST;

    return http_response();
}

int Ethernet_HTTP::patch_request(const uint8_t *p_data, size_t p_data_length)
{
    if(!connect_to_server())
        return HTTP_ERROR_NOT_CONNECTED;

    if(!send_http_request("PATCH", true, p_data, p_data_length))
        return HTTP_INVALID_REQUEST;

    return http_response();
}

int Ethernet_HTTP::get_request(int *content_length)
{  
    if(!connect_to_server())
        return HTTP_ERROR_NOT_CONNECTED;

    if(!send_http_request("GET"))
        return HTTP_INVALID_REQUEST;

    const char content_length_header[1][128] = { "Content-Length" };
    char content_length_value[1][64] = { "\0" };

    int http_code = http_response(content_length_header, content_length_value, 1);

    if(content_length_value[0][0] != '\0' && content_length != nullptr)
        *content_length = atoi((const char *)&content_length_value[0]);

    return http_code;
}  

int Ethernet_HTTP::download_content(Stream *p_stream)
{
    if(!connect_to_server())
        return -1;
        
    if(!send_http_request("GET"))
        return -1;

    const char content_length_header[1][128] = { "Content-Length" };
    char content_length_value[1][64] = { "\0" };
    
    int http_code = http_response(content_length_header, content_length_value, 1);

    logger.log("Descargando firmware nuevo: ");
    logger.log(content_length_value[0]);
    logger.logln(" bytes");

    if(http_code / 200 != 1)
    {
        this->m_global_client->stop();
        return -1;
    }

    if(!this->m_global_client->connected())
    {
        this->m_global_client->stop();
        return -2;
    }

    int byte_counter = 0;
    size_t file_size = atoi((const char *)&content_length_value[0]);
    uint8_t buffer[128] = { 0 };

    while(this->m_global_client->connected() && byte_counter < file_size)
    {
        while(this->m_global_client->available())
        {
            int len = this->m_global_client->read(buffer, sizeof(buffer));

            if(len > 0)
            {
                p_stream->write(buffer, len);
                byte_counter += len;
            }
            else
                vTaskDelay(1 / portTICK_PERIOD_MS);
        }
    }

    this->m_global_client->stop();

    return byte_counter;
}