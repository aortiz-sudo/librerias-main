#include "Smartone.h"

status Smartone::get_id(uint8_t *p_id_buffer)
{
    set_status(NO_DEVICE_ERROR);

    command_struct command;
    command.type = SMARTONE_ID_MESSAGE;
    command.data = nullptr;
    command.length = 5;
    command.response = this->m_response;
    command.buffer_len = SMARTONE_RESPONSE_BUFFER_LEN;

    status l_status = send_command(&command);

    if(l_status != NO_DEVICE_ERROR)
        return l_status;
    
    for(int i = 3; i < command.response_length - 2; i++)
        this->m_id[i - 3]  = this->m_response[i];
    
    p_id_buffer = this->m_id;

    return NO_DEVICE_ERROR;
}

void Smartone::reset_device()
{
    command_struct command;
    command.type = SMARTONE_RESET_MESSAGE;
    command.data = nullptr;
    command.length = 5;
    command.response = this->m_response;
    command.buffer_len = SMARTONE_RESPONSE_BUFFER_LEN;

    send_command(&command);
}

status Smartone::write_trunc_message(const uint8_t *p_data, size_t p_data_length)
{
    set_status(NO_DEVICE_ERROR);

    if(p_data_length > SMARTONE_TRUNC_BUFFER_LEN)
        return set_status(DATA_OUT_OF_RANGE_ERROR);

    command_struct command;
    command.type = SMARTONE_TRUNC_MESSAGE;
    command.data = (uint8_t *)p_data;
    command.length = p_data_length + 6;
    command.response = this->m_response;
    command.buffer_len = SMARTONE_RESPONSE_BUFFER_LEN;

    return send_command(&command);
}

int Smartone::send_data_to_server(const uint8_t *p_data, size_t p_data_length)
{
    uint8_t temp_id[4];
    get_id(temp_id);
    delay(10);
    
    status l_status = write_trunc_message(p_data, p_data_length);

    if(l_status != NO_DEVICE_ERROR)
        return -1;
    
    return 1;
}

int Smartone::get_data_from_server(uint8_t *p_buffer, size_t p_buffer_length)
{    
    return -1;
}

status Smartone::write_raw_message(const uint8_t *p_data, size_t p_data_length)
{
    set_status(NO_DEVICE_ERROR);

    if(p_data_length > SMARTONE_RAW_BUFFER_LEN)
        return set_status(DATA_OUT_OF_RANGE_ERROR);

    command_struct command;
    command.type = SMARTONE_RAW_MESSAGE;
    command.data = (uint8_t *)p_data;
    command.length = p_data_length + 6;
    command.response = this->m_response;
    command.buffer_len = SMARTONE_RESPONSE_BUFFER_LEN;

    return send_command(&command);
}

status Smartone::send_command(command_struct *p_command)
{
    uint8_t data[p_command->length]
    {
        SMARTONE_MESSAGE_HEADER, (uint8_t)p_command->length, p_command->type, 0x00
    };

    if(p_command->data)
        for(int i = 4; i < p_command->length; i++)
            data[i] = p_command->data[i - 4];
    
    activate_enable_pin();
    write_data(data, p_command->length);
    
    if(p_command->type != SMARTONE_RESET_MESSAGE)
    {
        p_command->response_length = read_data(p_command->response, p_command->buffer_len);
        deactivate_enable_pin();
    }
    else
    {
        deactivate_enable_pin();
        return set_status(NO_DEVICE_ERROR);
    }

    if(p_command->response_length == 0)
        return get_status();

    if(p_command->response[2] == 0xFF)
        return set_status(NACK_MESSAGE);
    
    return set_status(NO_DEVICE_ERROR);
}

uint16_t Smartone::crc_calculation(const uint8_t *p_data, const size_t p_data_length, const bool p_check)
{
    uint16_t crc = 0xFFFF;

    for(size_t i = 0; i < p_data_length; i++)
    {
        uint8_t byte = p_data[i];

        if(p_check && i > p_data_length - 3)
            byte = ~p_data[i];
        
        crc ^= byte;

        for(int j = 0; j < 8; i++)
        {
            if(crc & 0x01)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc >>= 1;
        }
    }

    if(!p_check)
        crc = ~crc;
    
    return crc;
}
