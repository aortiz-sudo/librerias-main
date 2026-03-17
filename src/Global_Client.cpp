#include "Global_Client.h"

void Global_Client::set_client_parameters(client_params p_params)
{
    this->m_server = p_params.m_server;
    this->m_port = p_params.m_port;
    this->m_type = p_params.m_type;
}

bool Global_Client::connect_to_server()
{
    if(this->m_server == nullptr)
        return false;

    if(this->m_secure)
    {
        if(this->m_certificate != nullptr)
        {
            this->m_ssl_client.setCACert((const char *)this->m_certificate);
            logger.logln("Certificado SSL establecido.");
        }
        else
        {
            this->m_ssl_client.setInsecure();
            logger.logln("Conexion SSL insegura");
        }

        this->m_ssl_client.setClient(&this->m_client);
        this->m_global_client = &this->m_ssl_client;
    }
    else
        this->m_global_client = &this->m_client;
    
    logger.logln("Intentando conectar al servidor...");
    
    if(this->m_global_client->connect(this->m_server, this->m_port))
    {
        logger.logln("Conectado al servidor");
        return true;
    }
    else
    {
        logger.logln("No se pudo conectar al servidor");
        this->m_global_client->stop();
        return false;
    }
}

int Global_Client::http_response(const char p_headers[][128], char p_headers_values[][64], int p_headers_count, uint8_t *p_data, size_t p_data_length)
{
    if(!this->m_global_client->connected())
    {
        this->m_global_client->stop(); 
        return HTTP_ERROR_NOT_CONNECTED;
    }

    uint8_t header_buffer[512] = { 0 };
    int http_code = 0;
    size_t total_bytes = 0;
    bool end_of_headers = false;

    unsigned long time = millis();

    while(this->m_global_client->connected() || this->m_global_client->available())
    {
        size_t length = this->m_global_client->readBytesUntil('\n', header_buffer, sizeof(header_buffer) - 1);
        total_bytes += length;

        header_buffer[length] = '\0';

        if(string_contains((const char *)header_buffer, "HTTP"))
            http_code = atoi((const char *)&header_buffer[9]);

        for(int i = 0; i < p_headers_count; i++)
        {
            if(string_contains((const char *)header_buffer, p_headers[i]))
            {
                char *header_value = strchr((const char *)&header_buffer[0], ':') + 2;

                strncpy(p_headers_values[i], header_value, 63);
                p_headers_values[i][63] = '\0';
            }
        }

        if(strcmp((const char *)header_buffer, "\r") == 0)
            end_of_headers = true;

        if(end_of_headers && p_data && length < p_data_length && length > 1)
        {
            strncpy((char *)p_data, (const char *)header_buffer, length);
            break;
        } 
        else if(end_of_headers && !p_data && p_data_length == 0)
            break;

        if(length > 1)
            time = millis();
        
        if(millis() - time > 3500)
            break;

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    if(total_bytes < 1)
    {
        this->m_global_client->stop();
        return HTTP_ERROR_NO_STREAM;
    }

    return http_code;
}

bool Global_Client::send_http_request(const char *p_type, bool p_http_version, const uint8_t *p_data, size_t p_data_length)
{
    bool compare = false;

    for(int i = 0; i < 4; i++)
    {
        compare = strcmp(p_type, this->m_requests[i]) == 0 ? true : false;
        
        if(compare)
            break;
    }

    if(!compare)
        return false;

    if(!this->m_global_client->connected())
        return false;

    this->m_global_client->print(p_type);
    this->m_global_client->print(" ");
    this->m_global_client->print(this->m_endpoint);

    if(p_http_version)
        this->m_global_client->println(" HTTP/1.1");

    for(int i = 0; i < MAX_HTTP_HEADERS; i++)
    {
        if(this->m_header[i])
        {
            this->m_global_client->print(this->m_header[i]);
            this->m_global_client->print(": ");
            this->m_global_client->println(this->m_header_value[i]);
        }
    }

    this->m_global_client->println();

    if(p_data)
    {
        this->m_global_client->write(p_data, p_data_length);
        this->m_global_client->println("\r\n");
    }

    if(this->m_secure)
        this->m_global_client->flush();

    return true;
}

void Global_Client::set_header(const char *p_header_name, const char *p_header_value, uint8_t p_index)
{
    if(p_index >= MAX_HTTP_HEADERS)
    {
        logger.logln("El numero de encabezados supera el limite del buffer.");
        return;
    }

    this->m_header[p_index] = (char *)p_header_name;
    this->m_header_value[p_index] = (char *)p_header_value;
}

void Global_Client::flush_headers()
{
    for(int i = 0; i < MAX_HTTP_HEADERS; i++)
    {
        this->m_header[i] = nullptr;
        this->m_header_value[i] = nullptr;
    }   
}