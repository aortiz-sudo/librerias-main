#include "Ethernet_HTTP.h"

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

    if(content_length < 1)
        return HTTP_ERROR_NO_STREAM;

    if(content_length >= p_data_length)
        return HTTP_ERROR_BUFFER_OVERFLOW;

    if(http_code / 200 != 1)
        return -1;

    int byte_counter = 0;

    while(this->m_global_client->connected() && byte_counter < content_length)
    {
        while(this->m_global_client->available())
        {
            int length = this->m_global_client->read(p_data, p_data_length);

            if(length > 0)
                byte_counter += length;
            else
                vTaskDelay(1 / portTICK_PERIOD_MS);
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    this->m_global_client->stop();

    p_data[byte_counter] = '\0';

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