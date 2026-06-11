#include "Ethernet_WebSocket.h"

int Ethernet_WebSocket::get_data_from_server(uint8_t *p_data, size_t p_buffer_len)
{
    if(!this->m_global_client->connected())
        return WEBSOCKET_ERROR_NOT_CONNECTED;

    int client_available = this->m_global_client->available();

    if(!client_available)
        return WEBSOCKET_ERROR_NO_STREAM;

    size_t counter = 0;
    while(this->m_global_client->available() && counter < p_buffer_len)
    {
        p_data[counter++] = this->m_global_client->read();
    }

    return handle_websocket_data(p_data, counter);
}

int Ethernet_WebSocket::send_data_to_server(const uint8_t *p_data, size_t p_length)
{   
    if(!this->m_global_client->connected())
        return WEBSOCKET_ERROR_NOT_CONNECTED;

    return this->m_global_client->write(p_data, p_length);
}

bool Ethernet_WebSocket::start_websocket_client()
{
    logger.log("Enviando Hansdshake a ");
    logger.log(this->m_server);
    logger.logln("...");

    if(connect_to_server())
        logger.logln("Conectado al servidor websocket");
    else
    {
        logger.logln("No se pudo conectar al servidor websocket");
        return false;
    }

    if(!handshake())
    {
        logger.logln("Handshake rechazado");
        this->m_global_client->stop();
        return false;
    }

    return true;
}

bool Ethernet_WebSocket::generate_websocket_key(char *p_output, size_t *p_length)
{
    uint8_t random_key[16];

    esp_fill_random(random_key, 16);

    if(mbedtls_base64_encode((unsigned char *)p_output, 64, p_length, random_key, sizeof(random_key)) != 0)
        return false;

    return true;
}

bool Ethernet_WebSocket::handshake()
{
    size_t length = 0;
    char websocket_key[64]  = "";

    if(!generate_websocket_key(websocket_key, &length))
        return false;
    
    websocket_key[length] = '\0';

    set_endpoint("/");
    char host_header[64] = "";
    snprintf(host_header, sizeof(host_header) - 1, "%s:%d", this->m_server, this->m_port);
    host_header[sizeof(host_header) - 1] = '\0';
    set_header("Host", host_header, 0);
    set_header("Upgrade", "websocket", 1);
    set_header("Connection", "Upgrade", 2);
    set_header("Sec-Websocket-key", websocket_key, 3);
    set_header("Sec-WebSocket-Version", "13", 4);

    if(!send_http_request("GET", 5))
    {
        flush_headers();
        return false;
    }

    const char websocket_key_header[128] = { "Sec-Websocket-Accept" };
    char websocket_key_value[64] = "";

    int http_code = http_response(&websocket_key_header, &websocket_key_value, 1);

    flush_headers();

    if(http_code != 101)
        return false;
    
    return check_key(websocket_key_value, websocket_key);
}

bool Ethernet_WebSocket::check_key(const char *p_response_key, const char *p_key)
{
    char *key = (char *)p_response_key;

    size_t length = 0;
    
    mbedtls_base64_decode(nullptr, 0, &length, (unsigned char *)key, strlen(key) - 1);
    uint8_t decode_data[length + 1];
    int ret = mbedtls_base64_decode(decode_data, length, &length, (unsigned char *)key, strlen(key) - 1);
    if(ret != 0)
    {
        logger.logln("No se pudo decodificar\r\nError: ");
        logger.logln(ret, HEX);
        return false;
    }

    decode_data[length] = '\0';

    const char *uuid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    char check[96] = "";

    strncat(check, p_key, sizeof(check) - 1);
    strncat(check, uuid, sizeof(check) - 1);

    check[sizeof(check) - 1] = '\0';
    
    uint8_t output[21];
    mbedtls_sha1_context context;

    mbedtls_sha1_init(&context);
    mbedtls_sha1_starts_ret(&context);
    mbedtls_sha1_update_ret(&context, (unsigned char *)check, strlen(check));
    mbedtls_sha1_finish_ret(&context, output);
    mbedtls_sha1_free(&context);

    output[20] = '\0';

    return strcmp((const char *)output, (const char *)decode_data) == 0 ? true : false;
}

int Ethernet_WebSocket::get_message(uint8_t *p_data, int *p_length, size_t p_buffer_len)
{
    uint8_t data[MAX_MESSAGE_SIZE];
    int type = 0;
    *p_length = get_data_from_server(data, p_buffer_len);
    
    if(*p_length < 0)
    {
        type = *p_length;
        return type;
    }
    
    uint8_t payload_byte = data[1] & ~WS_MASK;

    if((data[0] & ~WS_FIN) == WS_OPCODE_BIN)
        type = WEBSOCKET_BINARY_MESSAGE;
    else if((data[0] & ~WS_FIN) == WS_OPCODE_TXT)
        type = WEBSOCKET_TEXT_MESSAGE;
    
    int index = payload_byte <= 125 ? 2 : (payload_byte == 126 ? 4 : 10);

    size_t counter = 0;
    for(int i = index; i < *p_length; i++)
    {
        p_data[i - index] = data[i];
        counter++;
    }

    if(type == WEBSOCKET_TEXT_MESSAGE)
        p_data[counter] = '\0';
    
    *p_length = counter;

    return type;
}

int Ethernet_WebSocket::handle_websocket_data(uint8_t *p_data, size_t p_length)
{
    uint8_t opcode = p_data[0] & ~WS_FIN;

    if((p_data[1] & WS_MASK))
    {
        this->m_status_code = UNEXPECTED_ERROR;
        send_close_message("masked message");
        return WEBSOCKET_ERROR_MASKED_MESSAGE;
    }

    switch(opcode)
    {
        case WS_OPCODE_CLOSE:
        {
            if(p_length >= 4)
                this->m_status_code = (websocket_close_status_t)((p_data[2] << 8) | p_data[3]);
            else
                this->m_status_code = NORMAL_CLOSURE;

            for(int i = 0; i < p_length; i++)
            {
                if(p_data[i] < 0x10)
                    logger.log("0");

                logger.log(p_data[i], HEX);
                logger.log(" ");
            }

            logger.logln();
            send_close_message("ok"); 
            this->m_global_client->stop();
            return WEBSOCKET_CLOSE_FRAME_MESSAGE;
        }
        
        case WS_OPCODE_PING:
        {
            size_t length = p_data[1] & ~WS_MASK;
            uint8_t *app_data = &p_data[2];
            send_pong_message(app_data, length);
            return WEBSOCKET_PING_MESSAGE;
        }

        case WS_OPCODE_BIN:
        case WS_OPCODE_TXT:
            return p_length;
    }

    return 0;
}

void Ethernet_WebSocket::send_pong_message(uint8_t *p_payload, size_t p_length)
{   
    size_t length = p_length > MAX_PONG_MESSAGE_SIZE ? MAX_PONG_MESSAGE_SIZE : p_length;
    
    send_message(WS_OPCODE_PONG, p_payload, length);
}

void Ethernet_WebSocket::send_ping_message(uint8_t *p_payload, size_t p_length)
{
    size_t length = p_length > MAX_PING_MESSAGE_SIZE ? MAX_PING_MESSAGE_SIZE : p_length;
    
    send_message(WS_OPCODE_PING, p_payload, length);
}

void Ethernet_WebSocket::send_close_message(const char *p_message)
{
    if(p_message == nullptr)
        p_message = "close";

    size_t length = strlen(p_message) + 2;

    if(length > MAX_CLOSE_MESSAGE_SIZE)
        length = MAX_CLOSE_MESSAGE_SIZE;

    uint8_t data[MAX_CLOSE_MESSAGE_SIZE];
    data[0] = (uint8_t)((this->m_status_code >> 8) & 0xFF);
    data[1] = (uint8_t)(this->m_status_code & 0xFF);

    mempcpy(&data[2], p_message, length - 2);

    send_message(WS_OPCODE_CLOSE, data, length);

    this->m_global_client->stop();
}

int Ethernet_WebSocket::send_binary_message(uint8_t *p_data, size_t p_length)
{
    return send_message(WS_OPCODE_BIN, p_data, p_length);
}

int Ethernet_WebSocket::send_text_message(const char *p_string)
{
    return send_message(WS_OPCODE_TXT, (uint8_t *)p_string, strlen(p_string));
}

int Ethernet_WebSocket::send_message(uint8_t p_opcode, uint8_t *p_payload, size_t p_length)
{
    size_t length = 6 + p_length + (p_length > 125 ? 2 : 0);

    if(length > MAX_MESSAGE_SIZE)
        length = MAX_MESSAGE_SIZE;

    uint8_t payload_byte = p_length > 125 ? 126 : (uint8_t)p_length;
    uint8_t data[MAX_MESSAGE_SIZE];
    uint8_t mask[4] = { 0 };
    esp_fill_random(mask, 4);
    int index = 2;

    data[0] = WS_FIN | p_opcode;
    data[1] = WS_MASK | payload_byte;
    
    if(payload_byte == 126)
    {
        data[2] = (p_length >> 8) & 0xFF;
        data[3] = (p_length & 0xFF);
        index = 4;
    }

    for(int i = 0; i < 4; i++)
        data[index++] = mask[i];
    
    for(int i = 0; i < p_length; i++)
    {
        data[index++] = p_payload[i] ^ mask[i % 4];

        if(index >= length)
            break;
    }

    return send_data_to_server(data, length);
}