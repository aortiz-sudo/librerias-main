#include "Ethernet_SocketIO.h"

bool Ethernet_SocketIO::start_websocket_client()
{
    if(!connect_to_server())
        return false;

    logger.log("Enviando Hansdshake a ");
    logger.log(this->m_server);
    logger.logln("...");

    if(!handshake())
    {
        logger.logln("Handshake rechazado");
        this->m_global_client->stop();
        return false;
    }

    const char *text = "probe";
    send_ping_message((uint8_t *)text, strlen(text));

    unsigned long time = millis();
    while(!this->m_global_client->available() && millis() - time < 5000)
        vTaskDelay(1 / portTICK_PERIOD_MS);

    char response[128] = "";
    int length = 0;
    int type = get_message((uint8_t *)response, &length, 128);

    if(type != WEBSOCKET_TEXT_MESSAGE)
    {
        this->m_global_client->stop();
        return false;
    }

    logger.logln(response);
    if(!string_contains(response, "3probe"))
    {
        this->m_global_client->stop();
        return false;
    }

    send_upgrade_message();

    this->m_last_ping = millis();
    return true;
}

bool Ethernet_SocketIO::connect_to_namespace(int p_namespace_index)
{
    if(p_namespace_index >= this->m_namespace_counter)
        return false;

    char string[48] = "";

    string[0] = (char)NORMAL_MESSAGE;
    string[1] = (char)CONNECT_MESSAGE;
    snprintf(&string[2], sizeof(string) - 3, "%s,{\"%s\":\"%s\"}", this->m_namespace[p_namespace_index], this->m_identifier, this->m_identifier_value);
    string[sizeof(string) - 1] = '\0';

    logger.logln(string);
    send_text_message(string);
    
    bool client_available = false;
    unsigned long time = millis();
    while(!client_available && millis() - time < 5000)
    {
        client_available = this->m_global_client->available();
        vTaskDelay(1 / portTICK_PERIOD_MS);(1);
    }

    if(!client_available)
        return false;

    char response[256] = "";
    int length = 0;
    int type = get_message((uint8_t *)response, &length, 256);

    logger.logln(response);

    if(type != WEBSOCKET_TEXT_MESSAGE || length == 0)
    {
        logger.logln("Error en el mensaje");
        return false;
    }

    if(response[1] != (char)CONNECT_MESSAGE)
        return false;

    char *json = strchr(response, '{');

    char key[32] = "";
    char value[32] = "";
    char sid[25] = "";

    parse_json(json, &key, &value, 1);
    get_json_key_value(&key, &value, "sid", &sid[0], 1);

    logger.log("Session ID del namespace: ");
    logger.logln(sid);
    return true;
}

void Ethernet_SocketIO::send_pong_message(uint8_t *p_payload, size_t p_length)
{
    size_t length = 2 + p_length;

    if(length > MAX_PONG_MESSAGE_SIZE)
        length = MAX_PONG_MESSAGE_SIZE;

    char data[MAX_PONG_MESSAGE_SIZE];

    if(p_length > 0)
    {
        data[0] = (char)PONG_MESSAGE;

        for(int i = 1; i < length; i++)
            data[i] = (char)p_payload[i - 1];

        data[length - 1] = '\0';
    }
    else
    {
        data[0] = (char)PONG_MESSAGE;
        data[1] = '\0';
    }

    send_text_message(data);
}

void Ethernet_SocketIO::send_ping_message(uint8_t *p_payload, size_t p_length)
{
    size_t length = 2 + p_length;

    if(length > MAX_PING_MESSAGE_SIZE)
        length = MAX_PING_MESSAGE_SIZE;

    char data[MAX_PING_MESSAGE_SIZE];

    if(p_length > 0)
    {
        data[0] = (char)PING_MESSAGE;

        for(int i = 1; i < length; i++)
            data[i] = (char)p_payload[i - 1];

        data[length - 1] = '\0';
    }
    else
    {
        data[0] = (char)PING_MESSAGE;
        data[1] = '\0';
    }

    send_text_message(data);
}

void Ethernet_SocketIO::send_upgrade_message()
{
    char data[] = { (char)UPGRADE_MESSAGE, '\0' };
    send_text_message(data);
}

bool Ethernet_SocketIO::handshake()
{
    size_t length = 0;
    char websocket_key[64] = "";

    if(!generate_websocket_key(websocket_key, &length))
        return false;
    
    websocket_key[length] = '\0';

    char digits[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    char id[8] = "";

    for(int i = 0; i < 7; i++)
        id[i] = digits[random(0, 64)];

    char endpoint[90] = "";

    snprintf(endpoint, sizeof(endpoint), "/socket.io/?EIO=4&transport=polling&t=%s\r\n", id);
    endpoint[sizeof(endpoint) - 1] = '\0';

    set_endpoint(endpoint);
    
    if(!send_http_request("GET", false))
        return false;

    const char content_length_header[128] = { "Content-Length" };
    char content_length_value[64] = "";
    char string[256] = "";
    int http_code = http_response(&content_length_header, &content_length_value, 1, (uint8_t *)string, 256);
    
    if(http_code != 200)
        return false;

    int content_length = atoi(content_length_value);
    if(content_length > 0)
        string[content_length] = '\0';

    if(string_contains(string, "sid"))
        get_session_parameters(string);

    bool client_connect = false;
    unsigned long time = millis();
    
    while(!client_connect && millis() - time < 5000)
    {
        client_connect = connect_to_server();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    if(!client_connect)
        return false;

    snprintf(endpoint, sizeof(endpoint) - 1,"/socket.io/?EIO=4&transport=websocket&sid=%s\r\n", this->m_sid);
    endpoint[sizeof(endpoint) - 1] = '\0';

    set_endpoint(endpoint);

    char host_header[64]  = "";
    snprintf(host_header, sizeof(host_header) - 1, "%s:%d", this->m_server, this->m_port);
    host_header[sizeof(host_header) - 1] = '\0';

    set_header("Host", host_header, 0);
    set_header("Upgrade", "websocket", 1);
    set_header("Connection", "Upgrade", 2);
    set_header("Sec-WebSocket-key", websocket_key, 3);
    set_header("Sec-WebSocket-Version", "13", 4);

    if(!send_http_request("GET", false))
    {
        flush_headers();
        return false;
    }

    const char websocket_key_header[128] = { "Sec-WebSocket-Accept" };
    char websocket_key_value[64] = { "\0" };

    http_code = http_response(&websocket_key_header, &websocket_key_value, 1, (uint8_t *)string, 256);

    logger.logln(string);

    flush_headers();

    return http_code == 101 ? check_key(websocket_key_value, websocket_key) : false;
}

void Ethernet_SocketIO::get_session_parameters(const char *p_string)
{
    char values[5][32] = { 0 };
    char keys[5][32] = { 0 };

    parse_json(strchr(p_string, '{'), keys, values, 5);
    
    char sid[32] = "";
    char str_ping_interval[32] = "";
    char str_ping_timeout[32] = "";

    get_json_key_value(keys, values, "sid", sid, 5);
    get_json_key_value(keys, values, "pingInterval", str_ping_interval, 5);
    get_json_key_value(keys, values, "pingTimeout", str_ping_timeout, 5);

    strncpy(this->m_sid, sid, sizeof(this->m_sid) - 1);
    this->m_sid[sizeof(this->m_sid) - 1] = '\0';
    this->m_ping_interval = atoi(str_ping_interval);
    this->m_ping_timeout = atoi(str_ping_timeout);

    logger.logln(this->m_sid);
    logger.logln(this->m_ping_interval);
    logger.logln(this->m_ping_timeout);
}

void Ethernet_SocketIO::set_identifer(const char *p_identifier, const char *p_value)
{
    this->m_identifier = (char *)p_identifier;
    this->m_identifier_value = (char *)p_value;
}

void Ethernet_SocketIO::handle_message()
{
    unsigned long current_millis = millis();

    if(current_millis - this->m_last_ping > this->m_ping_interval + 10000)
    {
        logger.logln("Se perdió la conexion al servidor.");
        this->m_global_client->flush();
        this->m_global_client->stop();

        if(!start_websocket_client())
            return;
        
        for(int i = 0; i < this->m_namespace_counter; i++)
            connect_to_namespace(i);

        return;
    }

    uint8_t data[256] = { 0};
    int length = 0;
    int message_type = get_message(data, &length, 256);

    if(message_type == WEBSOCKET_ERROR_NOT_CONNECTED)
    {
        logger.logln("No hay conexion al servidor.");

        if(!start_websocket_client())
        {
            logger.logln("No se pudo reconectar al servidor.");
            return;
        }

        for(int i = 0; i < this->m_namespace_counter; i++)
            connect_to_namespace(i);
        
        return;
    }

    if(message_type == WEBSOCKET_ERROR_NO_STREAM)
        return;

    if(message_type == WEBSOCKET_CLOSE_FRAME_MESSAGE)
    {
        logger.logln("Se cerro la conexion.");
        return;
    }

    if(message_type != WEBSOCKET_TEXT_MESSAGE)
    {
        logger.logln("Mensaje no valido.");
        return;
    }

    engine_io_msg_t engine_io_type = (engine_io_msg_t)data[0];

    switch(engine_io_type)
    {
        case PING_MESSAGE:
        {
            logger.logln("Mensaje ping");

            this->m_last_ping = current_millis;

            if(length > 1)
                send_pong_message(&data[1], length - 1);
            else
                send_pong_message();

            return;
        }

        case PONG_MESSAGE:
        {
            logger.logln("Mensaje pong");
            return;
        }

        case NORMAL_MESSAGE:
        {
            logger.logln((const char *)&data[0]);
            event_handler((const char *)&data[1], length - 1);
            return;
        }
    }
}

void Ethernet_SocketIO::event_handler(const char *p_data, size_t p_length)
{
    if(p_data[0] == (char)EVENT_MESSAGE && this->m_event_counter > 0)
    {
        char event_name[32] = "";
        get_event_name((const char *)p_data, event_name, 32);

        logger.log("Evento: ");
        logger.logln(event_name);

        for(int i = 0; i < this->m_event_counter; i++)
        {
            if(strcmp((const char *)event_name, this->m_events[i]) == 0)
                this->m_callback_functions[i](strchr(p_data, '{'));
        }
    }
    else if(p_data[0] == (char)CONNECT_MESSAGE)
        logger.logln("Conectado al servidor Socket.IO.");
    else if(p_data[0] == (char)DISCONNECT_MESSAGE)
        logger.logln("Error al conectar al servidor Socket.IO.");
}

void Ethernet_SocketIO::get_event_name(const char *p_data, char *p_string, size_t p_buffer_length)
{
    char *temp = strchr(p_data, '"') + 1;

    size_t counter = 0;
    do
    {
        p_string[counter] = *temp++;

        counter++;

        if(counter >= p_buffer_length)
            break;

    } while (*temp != '"');

    p_string[counter] = '\0';
}
        
void Ethernet_SocketIO::set_event(const char *p_event, event_callback p_callback_function)
{
    if(this->m_event_counter < MAX_NUMBER_OF_EVENTS)
    {
        this->m_events[m_event_counter] = (char *)p_event;
        this->m_callback_functions[m_event_counter] = p_callback_function;
        m_event_counter++;
    }
    else
        logger.logln("El numero de eventos llegó al máximo");
}

bool Ethernet_SocketIO::send_event(const char *p_namespace, const char *p_event_name, const char *p_payload)
{
    if(!this->m_global_client->connected())
        return false;

    char buffer[200] = "";

    snprintf(buffer, sizeof(buffer) - 1, "%c%c%s,[\"%s\",%s]", (char)NORMAL_MESSAGE, (char)EVENT_MESSAGE,
                                                                p_namespace, p_event_name, p_payload);

    buffer[sizeof(buffer) - 1] = '\0';

    logger.log("Enviando evento: ");
    logger.logln(buffer);
    bool ret = send_text_message(buffer) >= 1 ? true : false;
    return ret;
}