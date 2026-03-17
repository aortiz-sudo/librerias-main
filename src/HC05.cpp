#include "HC05.h"

void HC05::set_device_parameters(const device_param_struct *p_params)
{
    const hc05_param_struct *p = static_cast<const hc05_param_struct *>(p_params);
    set_transmission_rate(p->m_data_transmission_rate);
    this->m_rx_pin = p->m_rx_pin;
    this->m_tx_pin = p->m_tx_pin;
    this->m_enable_pin = p->m_enable_pin;
    this->m_timeout = p->m_timeout;
    this->m_enable_state = p->m_enable_state;
    set_type(p->m_type);
    set_crc_type(p->m_crc);
    set_port(p->m_port);
    this->m_config = p->m_config;
    this->m_state_pin = p->m_state_pin;
    this->m_enable_delay = p->m_enable_delay;
}

void HC05::get_device_parameters(device_param_struct *p_params)
{
    hc05_param_struct *p = static_cast<hc05_param_struct *>(p_params);
    p->m_data_transmission_rate = get_transmission_rate();
    p->m_rx_pin = this->m_rx_pin;
    p->m_tx_pin = this->m_tx_pin;
    p->m_enable_pin = this->m_enable_pin;
    p->m_timeout = this->m_timeout;
    p->m_enable_state = this->m_enable_state;
    p->m_type = get_type();
    p->m_crc = get_crc_type();
    p->m_port = get_port();
    p->m_config = this->m_config;
    p->m_state_pin = this->m_state_pin;

    strncpy(p->m_mac_address, (const char *)this->m_mac_address, sizeof(p->m_mac_address));
}

void HC05::begin(const device_param_struct *p_params)
{
    set_device_parameters(p_params);
    #if defined(ESP32)
        static_cast<HardwareSerial *>(get_port())->begin(get_transmission_rate(), this->m_config, 
                                 this->m_rx_pin, this->m_tx_pin);
    #else
        static_cast<HardwareSerial *>(get_port())->begin(get_transmission_rate(), this->m_config);
    #endif

    if(this->m_state_pin > -1)
        pinMode(this->m_state_pin, INPUT);

    if(this->m_enable_pin > -1)
    {
        pinMode(this->m_enable_pin, OUTPUT);
        deactivate_enable_pin();
    }
}


void HC05::format_mac_address(const char *p_mac_address)
{ 
  uint8_t counter = 0;

  do
  {
    if(*p_mac_address != ':' && (counter != 4 && counter != 7))
    {
      this->m_mac_address[counter] = *p_mac_address;
      counter++;
    }
    else if(counter == 4 || counter == 7)
    {
      this->m_mac_address[counter] = ',';
      counter++;
    }

    if(counter >= 15)
      break;

  } while(*p_mac_address++);

  this->m_mac_address[14] = '\0';
}

status HC05::send_command(command_struct *p_command)
{
    bool reset = string_contains((const char *)p_command->data, "RESET");

    set_status(NO_DEVICE_ERROR);
    activate_enable_pin();
    write_data((uint8_t *)p_command->data, p_command->length);
    if(reset)
        deactivate_enable_pin();
    else
    {
        p_command->response_length = read_data((uint8_t *)p_command->response, p_command->buffer_len);
        deactivate_enable_pin();
    }

    if(p_command->response_length == 0)
        return get_status();

    if(p_command->response_length > 0 && string_contains((const char *)p_command->response, FAIL_RESPONSE))
        return set_status(COMMAND_FAIL);
    
    return set_status(NO_DEVICE_ERROR);
}

bool HC05::config_uart(uint32_t p_BAUD_RATE, bool p_stop_bit, uint8_t p_parity)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%u,%u,%u\r\n", UART_CMD, p_BAUD_RATE, p_stop_bit, p_parity);

    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

bool HC05::set_role(uint8_t p_role)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%d\r\n", ROLE_CMD, p_role);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) == NO_DEVICE_ERROR ? true : false;
}

int8_t HC05::get_role()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;
    
    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", ROLE_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    if(send_command(&command) == NO_DEVICE_ERROR)
    {
        return m_response[6] - '0';
    }

    return -1;
}

bool HC05::connected()
{
    if(this->m_state_pin > -1)
        return digitalRead(this->m_state_pin);
    
    if(check_state() == CONNECTED)
        return true;
    
    return false;
}

bool HC05::connect(const char *p_mac_address, const char *p_password)
{
    //if(!delete_saved_connections()) return false;
    if(!set_role(MASTER)) return false;
    if(!config_connection_mode(_SPECIFIC)) return false;

    inquire_mode inq_m;
    inq_m.access_mode = false;
    inq_m.max_bt_devices = 5;
    inq_m.max_inquiring_time = 20;
    
    if(!config_inquire(inq_m)) return false;
    if(!set_password(p_password)) return false;

    unsigned long time = millis();

    while(check_state() != INQUIRING)
    {
        if(millis() - time > 30000)
        {
            set_status(CONNECT_ERROR);
            return false;
        }
    }

    if(!pair(this->m_mac_address, 20)) return false;
    if(!bind(this->m_mac_address)) return false;

    time = millis();

    while(!connected())
    {
        if(millis() - time > 15000)
        {
            set_status(CONNECT_ERROR);
            return false;
        }
    }

    return true;
}

bool HC05::connect(const char*p_mac_address)
{
  if(!pair(this->m_mac_address, 20)) return false;
  if(!seek_device(this->m_mac_address)) return false;
  if(!link(this->m_mac_address)) return false;

  return true;
}

bool HC05::pair(const char *p_mac_address, uint8_t p_time)
{
    format_mac_address(p_mac_address);
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%s,%d\r\n", PAIR_CMD, p_mac_address, p_time);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;
    
    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

bool HC05::pair(uint8_t p_time)
{
    return pair(this->m_mac_address, p_time);
}

bool HC05::bind(const char *p_mac_address)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;
  
    str_len = snprintf(str, sizeof(str) - 1, "%s=%s\r\n", BIND_CMD, p_mac_address);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

bool HC05::get_binded_device(char *p_mac_address)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", BIND_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    if(send_command(&command) == NO_DEVICE_ERROR)
    {
        for(int i = 0; i < 15; i++)
            p_mac_address[i] = m_response[6 + i];

        return true;
    }

    return false;
}

bool HC05::set_password(const char *p_password)
{
    if(strlen(p_password) > 4)
    {
        set_status(LONG_PASSWORD);
        return false;
    }

    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=\"%s\"\r\n", PSWD_CMD, p_password);
    str[sizeof(str) - 1] = '\0';
    
    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

bool HC05::get_password(char *p_password)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", PSWD_CMD);
    str[sizeof(str) - 1] = '\0';
    
    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    if(send_command(&command) == NO_DEVICE_ERROR)
    {
        strncpy(p_password, (const char *)&m_response[6], 5);

        return true;
    }

    return false;
}

bool HC05::delete_saved_connections()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", CLEAR_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

bool HC05::config_inquire(inquire_mode p_inqm)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%u,%u,%u\r\n", INQM_CMD, p_inqm.access_mode, p_inqm.max_bt_devices, p_inqm.max_inquiring_time);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

inquire_mode HC05::get_inquire_mode()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;
    char string_array_1[2][STRING_BUFFER];
    char string_array_2[3][STRING_BUFFER];

    inquire_mode inq_m;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", INQM_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    if(send_command(&command) == NO_DEVICE_ERROR)
    {
        size_t length_1 = split_string(m_response, string_array_1, 2,'=');
        size_t length_2;

        if(length_1 > 0)
        length_2 = split_string((const char *)string_array_1[1], string_array_2, 3, ',');

        if(length_2 > 0)
        {
            inq_m.access_mode = (bool)atoi(string_array_2[0]);
            inq_m.max_bt_devices = (uint8_t)atoi(string_array_2[1]);
            inq_m.max_inquiring_time = (uint8_t)atoi(string_array_2[2]);
        }
    }

    return inq_m;
}

void HC05::reset()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", RESET_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;
    send_command(&command);
}

void HC05::default_config()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", RESTORE_CMD);
    str[sizeof(str) - 1] = '\0';

    uint32_t temp_timeout = get_timeout();
    set_timeout(0);
    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;
    send_command(&command);
    set_timeout(temp_timeout);
}

state HC05::check_state()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", STATE_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    if(send_command(&command) == NO_DEVICE_ERROR)
    {
        for(uint8_t i = 0; i < 9; i++)
        {
        if(string_contains((const char *)m_response, m_state_str[i]))
            return (state)i;
        }
    }

    return UNKNOWN;
}

bool HC05::config_connection_mode(connection_mode p_mode)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%u\r\n", CMODE_CMD, (uint8_t)p_mode);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

connection_mode HC05::get_connection_mode()
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s\r\n", CMODE_CMD);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    if(send_command(&command) == NO_DEVICE_ERROR)
    {
        return connection_mode(m_response[7] - '0');
    }

    return _SPECIFIC;
}

bool HC05::link(const char *_mac_address)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%s\r\n", LINK_CMD, _mac_address);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}

bool HC05::seek_device(const char *p_mac_address)
{
    char str[32];
    size_t str_len = 0;
    command_struct command;

    str_len = snprintf(str, sizeof(str) - 1, "%s=%s\r\n", SEEK_DEVICE_CMD, p_mac_address);
    str[sizeof(str) - 1] = '\0';

    command.data = (uint8_t *)str;
    command.length = str_len;
    command.response = (uint8_t *)m_response;
    command.buffer_len = HC05_BUFFER_LEN;

    return send_command(&command) != NO_DEVICE_ERROR ? false : true;
}