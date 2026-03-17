#include "MT124.h"

void MT124::set_device_parameters(const device_param_struct *p_params)
{
    const mt124_param_struct *p = static_cast<const mt124_param_struct *>(p_params);
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
}

void MT124::get_device_parameters(device_param_struct *p_params)
{
    mt124_param_struct *p = static_cast<mt124_param_struct *>(p_params);
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
}

reader_status MT124::get_reader_status(uint8_t p_reader_address)
{
    if(p_reader_address < 1 || p_reader_address > 16)
    {
        set_status(READER_ADDRESS_ERROR);
        return READER_ADDRESS_DOES_NOT_EXISTS;
    }
    
    command_struct command;
    command.address = p_reader_address;
    command.type = MT124_CHECK_STATUS_CMD;
    command.response = this->m_buffer;
    command.buffer_len = MT124_BUFFER_LEN;
    command.length = 7;
    command.data = nullptr;

    status l_status = send_command(&command);

    if(l_status != NO_DEVICE_ERROR)
    {
        set_status(l_status);
        this->m_reader_status[p_reader_address - 1] = READER_NO_STATUS;
    }
    else
        this->m_reader_status[p_reader_address - 1] = (reader_status)m_buffer[READER_STATUS_POS];
    
    return this->m_reader_status[p_reader_address - 1];
}

status MT124::get_tag_data(uint8_t **p_buffer, uint8_t p_reader_address)
{
    reader_status l_reader_status = get_reader_status(p_reader_address);

    status l_status = get_status();

    if(l_status != NO_DEVICE_ERROR)
        return l_status;
    
    if(l_reader_status != READER_READING_TAG_SUCCESFULLY)
        return set_status(NO_READING_ERROR);
    
    command_struct command;
    command.type = MT124_GET_TAG_DATA_CMD;
    command.address = p_reader_address;
    command.response = this->m_buffer;
    command.buffer_len = MT124_BUFFER_LEN;
    command.length = 7;
    command.data = nullptr;

    l_status = send_command(&command);

    if(l_status != NO_DEVICE_ERROR)
        return set_status(l_status);
    
    for(int i = TAG_DATA_POS; i < TAG_DATA_POS + TAG_DATA_SIZE; i++)
        this->m_tag_data[i - TAG_DATA_POS] = this->m_buffer[i];

    *p_buffer = &this->m_tag_data[0];

    return set_status(READ_TAG_SUCCESSFULLY);
}

status MT124::send_command(command_struct *p_command)
{
    set_status(NO_DEVICE_ERROR);
    uint8_t data[MT124_BUFFER_LEN] =
    {
        MT124_TX_MESSAGE_HEADER, (uint8_t)(p_command->length - 4), p_command->type, READER_ADDR_H, p_command->address
    };

    if(p_command->data != nullptr)
    {
        for(int i = 5; i < p_command->length; i++)
        {
            data[i] = p_command->data[i - 5];

            if(i >= MT124_BUFFER_LEN)
                break;
        }
    }

    data[p_command->length - 2] = MT124_END_OF_MESSAGE;

    write_data(data, p_command->length);

    p_command->response_length = read_data(p_command->response, p_command->buffer_len);

    return get_status();
}

uint16_t MT124::crc_calculation(const uint8_t *p_data, const size_t p_data_length, const bool p_check)
{
    uint8_t crc = 0;

    for(int i = 0; i < p_data_length; i++)
        crc ^= *p_data++;

    return crc & 0xFF;
}