#include "Display.h"

status Display::send_command(command_struct *p_command)
{
    uint8_t data_length = ((get_crc_type() != NO_CRC) ? (p_command->length + 2) : p_command->length) + 4;

    uint8_t data[DISPLAY_BUFFER_LEN] = 
    { 
        FRAME_HEADER_H, FRAME_HEADER_L, (uint8_t)(data_length - 3), p_command->type
    };

    for(int i = 4; i < data_length; i++)
    {
        data[i] = p_command->data[i - 4];

        if(i > DISPLAY_BUFFER_LEN)
            break;
    }

    write_data(data, data_length);

    p_command->response_length = read_data(p_command->response, p_command->buffer_len);

    return get_status();
}

void Display::system_reset()
{
    uint8_t data[] = 
    {
        (uint8_t)(SYSTEM_RESET_REG >> 8), 
        (uint8_t)(SYSTEM_RESET_REG & 0xFF), (uint8_t)(SYSTEM_RESET_CMD_H >> 8), 
        (uint8_t)(SYSTEM_RESET_CMD_H & 0xFF), (uint8_t)(SYSTEM_RESET_CMD_L >> 8), 
        (uint8_t)(SYSTEM_RESET_CMD_L & 0xFF)
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    send_command(&command);
}

status Display::set_baudrate(uint32_t p_BAUD_RATE)
{
    uint16_t value = (uint16_t)(206438400/(64 * p_BAUD_RATE));

    uint8_t data[] = 
    {
        (uint8_t)(UART_CONFIG_REG >> 8),
        (uint8_t)(UART_CONFIG_REG & 0xFF), (uint8_t)(m_crc_cmd >> 8),
        (uint8_t)(m_crc_cmd & 0xFF), (uint8_t)(value >> 8),
        (uint8_t)(value & 0xFF)
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    status l_status = send_command(&command);

    reset_serial_port(p_BAUD_RATE);
    
    return l_status;
}

status Display::set_CRC(bool p_set_crc)
{
    set_crc_type(p_set_crc ? CRC_16 : NO_CRC);
    m_crc_cmd = p_set_crc ? SET_CRC_TRUE_CMD : SET_CRC_FALSE_CMD;

    uint8_t data[] = 
    {
        (uint8_t)(UART_CONFIG_REG >> 8),
        (uint8_t)(UART_CONFIG_REG & 0xFF), (uint8_t)(m_crc_cmd >> 8),
        (uint8_t)(m_crc_cmd & 0xFF)
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    return send_command(&command);
}

status Display::set_RTC(const uint8_t *p_rtc)
{
    uint8_t data[] =
    {
        (uint8_t)(RTC_CONFIG_REG >> 8),
        (uint8_t)(RTC_CONFIG_REG & 0xFF), 
        p_rtc[0], p_rtc[1], p_rtc[2], p_rtc[3],
        p_rtc[4], p_rtc[5], p_rtc[6], 0x00
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    return send_command(&command);
}

uint8_t *Display::get_RTC()
{
    uint8_t data[] =
    {
        (uint8_t)(RTC_CONFIG_REG >> 8),
        (uint8_t)(RTC_CONFIG_REG & 0xFF), 0x04
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = READ_COMMAND;

    if(send_command(&command) != NO_DEVICE_ERROR)
        return nullptr;

    for(int i = 0; i < 7; i++)
        this->m_rtc_array[i] = this->m_buffer[7+i];

    return &this->m_rtc_array[0];
}

uint8_t Display::get_current_pic()
{
    uint8_t data[] = 
    {
        (uint8_t)(PIC_NOW_REG >> 8),
        (PIC_NOW_REG & 0xFF), 0x01
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = READ_COMMAND;

    if(send_command(&command) != NO_DEVICE_ERROR)
        return 0xFF;
    
    return this->m_buffer[8];
}

uint16_t Display::get_horizontal_resolution()
{
    uint8_t data[] = 
    {
        (uint8_t)(LCD_HOR_REG >> 8),
        (uint8_t)(LCD_HOR_REG & 0xFF), 0x01
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = READ_COMMAND;

    if(send_command(&command) != NO_DEVICE_ERROR)
        return 0x00;
    
    return (uint16_t)(this->m_buffer[9] << 8 | this->m_buffer[8]);
}

uint16_t Display::get_vertical_resolution()
{
    uint8_t data[] = 
    {
        (uint8_t)(LCD_VER_REG >> 8),
        (uint8_t)(LCD_VER_REG & 0xFF), 0x01
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = READ_COMMAND;

    if(send_command(&command) != NO_DEVICE_ERROR)
        return 0x00;
    
    return (uint16_t)(this->m_buffer[9] << 8 | this->m_buffer[8]);
}

status Display::set_led_config(uint8_t p_brightness)
{
    uint8_t data[] = 
    {
        (uint8_t)(LED_CONFIG_REG >> 8),
        (uint8_t)(LED_CONFIG_REG & 0xFF), p_brightness
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    return send_command(&command);
}

status Display::set_pic(uint8_t p_pic_id)
{
    uint8_t data[] = 
    {
        (uint8_t)(PIC_SET_REG >> 8),
        (uint8_t)(PIC_SET_REG & 0xFF),(uint8_t)(ENABLE_PAGE_SWITCH_CMD >> 8),
        (uint8_t)(ENABLE_PAGE_SWITCH_CMD & 0xFF), 0x00, p_pic_id
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    return send_command(&command);
}

status Display::simulate_touch(uint16_t p_x, uint16_t p_y, press_mode p_press_mode)
{
    uint8_t data[] = 
    {
        (uint8_t)(TP_SIMULATION_REG >> 8),
        (uint8_t)(TP_SIMULATION_REG & 0xFF), (uint8_t)(ENABLE_TOUCH_SIM_CMD >> 8),
        (uint8_t)(ENABLE_TOUCH_SIM_CMD & 0xFF), (uint8_t)(p_press_mode >> 8),
        (uint8_t)(p_press_mode & 0xFF), (uint8_t)(p_x >> 8), (uint8_t)(p_x & 0xFF),
        (uint8_t)(p_y >> 8), (uint8_t)(p_y & 0xFF)
    };

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    return send_command(&command);
}

status Display::set_user_variable(uint16_t p_address, uint8_t *p_value, size_t p_value_length)
{
    uint8_t data[DISPLAY_BUFFER_LEN] = 
    {
        (uint8_t)(p_address >> 8), (uint8_t)(p_address & 0xFF)
    };

    for(size_t i = 2; i < p_value_length + 2; i++)
    {
        data[i] = p_value[i - 2];

        if(i >= DISPLAY_BUFFER_LEN - 4)
            break;
    }

    command_struct command;
    command.data = data;
    command.length = sizeof(data);
    command.response = this->m_buffer;
    command.buffer_len = DISPLAY_BUFFER_LEN;
    command.type = WRITE_COMMAND;

    return send_command(&command);
}

uint16_t Display::crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check)
{
  uint16_t crc = 0xFFFF;

  for(size_t i = 3; i < p_length; i++)
  {   
    crc ^= p_data[i];
    for(int j = 0; j < 8; j++)
    {
      if(crc & 0x01)
        crc  = (crc >> 1) ^ 0xA001;
      else
        crc >>= 1; 
    }
  }

  return crc;
}

uint8_t* Display::get_data()
{
    return &this->m_buffer[0];
}