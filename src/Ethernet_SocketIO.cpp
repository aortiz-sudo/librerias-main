#include "Ethernet_SocketIO.h"

bool Ethernet_SocketIO::start_websocket_client()
{
    // Lo que quedara guardado de la conexión anterior no es de esta.
    reset_rx_buffer();

    if(!connect_to_server())
        return false;

    logger.debug("Enviando Hansdshake a ");
    logger.debug(this->m_server);
    logger.debugln("...");

    if(!handshake())
    {
        // Mientras se reintenta la reconexión solo se registró la pérdida (report_connection()).
        if(this->m_connected)
            logger.logln("Handshake rechazado");
        else
            logger.debugln("Handshake rechazado");

        this->m_global_client->stop();
        return false;
    }

    const char *text = "probe";
    send_ping_message((uint8_t *)text, strlen(text));

    char response[128] = "";
    int length = 0;
    int type = wait_message(response, &length, sizeof(response), 5000);

    if(type != WEBSOCKET_TEXT_MESSAGE)
    {
        this->m_global_client->stop();
        return false;
    }

    logger.debugln(response);
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

    // Se une con el identificador y, si hay, la llave: un servidor que no la valida ignora
    // el campo. En el log la llave va como ****. Si el mensaje con la llave no cupiera, se
    // manda el de antes, sin ella, en lugar de un JSON cortado que el servidor rechazaría.
    const char *auth_key = auth_key_to_send();
    char string[SOCKETIO_CONNECT_MESSAGE_SIZE] = "";
    char log_string[SOCKETIO_CONNECT_MESSAGE_SIZE] = "";

    if(auth_key && !build_connect_message(string, sizeof(string), p_namespace_index, auth_key))
    {
        logger.logln("La llave no cabe en el mensaje de conexion al namespace; se conecta sin ella.");
        auth_key = nullptr;
    }

    if(!auth_key)
        build_connect_message(string, sizeof(string), p_namespace_index, nullptr);

    build_connect_message(log_string, sizeof(log_string), p_namespace_index, auth_key ? "****" : nullptr);

    logger.debugln(log_string);
    send_text_message(string);
    
    char response[256] = "";
    int length = 0;
    int type = wait_message(response, &length, sizeof(response), 5000);

    if(type == WEBSOCKET_ERROR_NO_STREAM)
    {
        logger.debugln("Sin respuesta al unirse al namespace.");
        return false;
    }

    logger.debugln(response);

    if(type != WEBSOCKET_TEXT_MESSAGE || length == 0)
    {
        logger.logln("Error en el mensaje");
        return false;
    }

    // 44<namespace>,{"message":"..."}: el servidor rechazó la conexión (por ejemplo, la llave).
    // Se registra la primera vez; mientras siga rechazándola, los reintentos van al debug.
    if(response[0] == (char)NORMAL_MESSAGE && response[1] == (char)CONENCT_ERROR_MESSAGE)
    {
        char reason[64] = "";
        const char *message = strstr(response, "\"message\":\"");

        if(message)
        {
            message += strlen("\"message\":\"");
            size_t reason_length = strcspn(message, "\"");

            if(reason_length >= sizeof(reason))
                reason_length = sizeof(reason) - 1;

            memcpy(reason, message, reason_length);
            reason[reason_length] = '\0';
        }

        if(!this->m_namespace_rejected)
        {
            logger.log("El servidor rechazo la conexion al namespace ");
            logger.log(this->m_namespace[p_namespace_index]);
            logger.log(": ");
            logger.logln(reason);
        }
        else
        {
            logger.debugln("El servidor sigue rechazando la conexion al namespace.");
        }

        this->m_namespace_rejected = true;
        return false;
    }

    if(response[1] != (char)CONNECT_MESSAGE)
    {
        logger.debugln("Respuesta inesperada al unirse al namespace.");
        return false;
    }

    if(this->m_namespace_rejected)
        logger.logln("El servidor volvio a aceptar la conexion al namespace.");

    this->m_namespace_rejected = false;

    char *json = strchr(response, '{');

    char key[32] = "";
    char value[32] = "";
    char sid[25] = "";

    parse_json(json, &key, &value, 1);
    get_json_key_value(&key, &value, "sid", &sid[0], 1);

    logger.debug("Session ID del namespace: ");
    logger.debugln(sid);
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

    logger.debugln(string);

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

    logger.debugln(this->m_sid);
    logger.debugln(this->m_ping_interval);
    logger.debugln(this->m_ping_timeout);
}

void Ethernet_SocketIO::report_connection(bool p_connected, const char *p_message)
{
    // Solo los cambios: con el servidor caído handle_message() reintenta en cada ciclo de
    // la tarea y cada intento repetía el mismo aviso en la SD.
    if(p_connected != this->m_connected)
        logger.logln(p_message);
    else
        logger.debugln(p_message);

    this->m_connected = p_connected;
}

void Ethernet_SocketIO::set_identifer(const char *p_identifier, const char *p_value)
{
    this->m_identifier = (char *)p_identifier;
    this->m_identifier_value = (char *)p_value;
}

int Ethernet_SocketIO::wait_message(char *p_buffer, int *p_length, size_t p_size, unsigned long p_timeout_ms)
{
    unsigned long start = millis();

    while(millis() - start < p_timeout_ms)
    {
        int type = get_message((uint8_t *)p_buffer, p_length, p_size);

        // Ping de Engine.IO mientras se espera otra respuesta: se contesta y se sigue esperando.
        if(type == WEBSOCKET_TEXT_MESSAGE && *p_length == 1 && p_buffer[0] == (char)PING_MESSAGE)
        {
            this->m_last_ping = millis();
            send_pong_message();
            continue;
        }

        if(type == WEBSOCKET_TEXT_MESSAGE || type == WEBSOCKET_BINARY_MESSAGE || type == WEBSOCKET_ERROR_NOT_CONNECTED ||
           type == WEBSOCKET_CLOSE_FRAME_MESSAGE || type == WEBSOCKET_ERROR_MASKED_MESSAGE)
            return type;

        // Todavía no llega el mensaje completo, o fue un ping o pong del websocket ya atendido.
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    return WEBSOCKET_ERROR_NO_STREAM;
}

bool Ethernet_SocketIO::connect_namespaces()
{
    for(int i = 0; i < this->m_namespace_counter; i++)
    {
        if(!connect_to_namespace(i))
        {
            this->m_global_client->flush();
            this->m_global_client->stop();
            return false;
        }
    }

    return true;
}

void Ethernet_SocketIO::set_auth_key(const char *p_name, const char *p_value)
{
    this->m_auth_key = p_name;
    this->m_auth_key_value = p_value;
}

const char *Ethernet_SocketIO::auth_key_to_send() const
{
    if(!this->m_auth_key || !this->m_auth_key_value || this->m_auth_key_value[0] == '\0')
        return nullptr;

    // Va sin escapar dentro del JSON: comillas, diagonales o caracteres de control lo romperían.
    for(const char *c = this->m_auth_key_value; *c; c++)
    {
        if(*c < 0x20 || *c > 0x7E || *c == '"' || *c == '\\')
            return nullptr;
    }

    return this->m_auth_key_value;
}

bool Ethernet_SocketIO::build_connect_message(char *p_buffer, size_t p_size, int p_namespace_index, const char *p_key) const
{
    int length;

    if(p_key)
        length = snprintf(p_buffer, p_size, "%c%c%s,{\"%s\":\"%s\",\"%s\":\"%s\"}", (char)NORMAL_MESSAGE, (char)CONNECT_MESSAGE,
                          this->m_namespace[p_namespace_index], this->m_identifier, this->m_identifier_value, this->m_auth_key, p_key);
    else
        length = snprintf(p_buffer, p_size, "%c%c%s,{\"%s\":\"%s\"}", (char)NORMAL_MESSAGE, (char)CONNECT_MESSAGE,
                          this->m_namespace[p_namespace_index], this->m_identifier, this->m_identifier_value);

    return length > 0 && (size_t)length < p_size;
}

void Ethernet_SocketIO::begin_reconnect()
{
    this->m_reconnecting = true;
    this->m_reconnect_delay = 0;
}

void Ethernet_SocketIO::reconnect()
{
    // Sin esta espera se reintentaba en cada ciclo de la tarea (100 ms más lo que tarda la
    // conexión) con http_mutex tomado: el W5500 no descansaba y el HTTP no podía enviar.
    if(millis() - this->m_last_reconnect_millis < this->m_reconnect_delay)
        return;

    // Si no se pudo unir a un namespace, connect_namespaces() ya cerró el socket: se sigue
    // reintentando con espera, igual que si no hubiera conexión.
    if(start_websocket_client() && connect_namespaces())
    {
        this->m_reconnecting = false;
        this->m_reconnect_delay = 0;
        report_connection(true, "Conexion al servidor Socket.IO restablecida.");

        // Los eventos que el servidor emitió mientras no había conexión se perdieron.
        if(this->m_reconnect_callback)
            this->m_reconnect_callback();

        return;
    }

    // La espera cuenta desde que terminó el intento: uno con timeout de TLS tarda segundos.
    this->m_last_reconnect_millis = millis();

    if(this->m_reconnect_delay == 0)
        this->m_reconnect_delay = SOCKETIO_RECONNECT_MIN_MS;
    else if(this->m_reconnect_delay < SOCKETIO_RECONNECT_MAX_MS)
        this->m_reconnect_delay = min(this->m_reconnect_delay * 2, SOCKETIO_RECONNECT_MAX_MS);

    logger.debug("No se pudo reconectar al servidor; siguiente intento en ");
    logger.debug(this->m_reconnect_delay / 1000);
    logger.debugln(" s.");
}

void Ethernet_SocketIO::handle_message()
{
    // Mientras se reintenta no se lee el socket: no hay conexión.
    if(this->m_reconnecting)
    {
        reconnect();
        return;
    }

    unsigned long current_millis = millis();

    if(current_millis - this->m_last_ping > this->m_ping_interval + 10000)
    {
        report_connection(false, "Se perdió la conexion al servidor.");
        this->m_global_client->flush();
        this->m_global_client->stop();

        begin_reconnect();
        reconnect();
        return;
    }

    uint8_t data[256] = { 0};
    int length = 0;
    int message_type = get_message(data, &length, 256);

    if(message_type == WEBSOCKET_ERROR_NOT_CONNECTED)
    {
        report_connection(false, "No hay conexion al servidor.");

        begin_reconnect();
        reconnect();
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
            logger.debugln("Mensaje ping");

            this->m_last_ping = current_millis;

            if(length > 1)
                send_pong_message(&data[1], length - 1);
            else
                send_pong_message();

            return;
        }

        case PONG_MESSAGE:
        {
            logger.debugln("Mensaje pong");
            return;
        }

        case NORMAL_MESSAGE:
        {
            // El evento apiKey trae la llave: no va al log (en depuración sale por el puerto serial).
            if(strstr((const char *)&data[0], "\"apikey\""))
                logger.debugln("Mensaje con la API key (no se muestra).");
            else
                logger.debugln((const char *)&data[0]);

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

    logger.debug("Enviando evento: ");
    logger.debugln(buffer);
    bool ret = send_text_message(buffer) >= 1 ? true : false;
    return ret;
}