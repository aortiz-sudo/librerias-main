#include "LLS.h"

void LLS::set_device_parameters(const device_param_struct *_params)
{
  const lls_param_struct *p = static_cast<const lls_param_struct *>(_params);
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
  this->m_periodic = p->m_periodic;
  this->m_ascii = p->m_ascii;
}

void LLS::get_device_parameters(device_param_struct *_params)
{
  lls_param_struct *p = static_cast<lls_param_struct *>(_params);
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
  p->m_periodic = this->m_periodic;
  p->m_ascii = this->m_ascii;
}

uint16_t LLS::get_fuel_level(uint8_t p_slave_address)
{
  return this->m_lls_data[p_slave_address - 1].fuel_level;
}

int8_t LLS::get_temperature(uint8_t p_slave_address)
{
  return this->m_lls_data[p_slave_address - 1].temp;
}

uint16_t LLS::get_frequency_value(uint8_t p_slave_address)
{
  return this->m_lls_data[p_slave_address - 1].frequency_value;
}

status LLS::read_lls_data(uint8_t p_slave_address)
{
  if(p_slave_address > MAX_NUMBER_OF_SLAVES)
    return set_status(ADDRESS_ERROR);

  status l_status;

  command_struct command;

  if(!this->m_periodic)
  {
    command.data = nullptr;
    command.type = LLS_SINGLE_DATA_CMD;
    command.length = 3;
    command.response = m_lls_buffer;
    command.buffer_len = LLS_MAX_BUFFER_LEN;
    command.address = p_slave_address;

    l_status = send_command(&command);
  }
  else
  {
    if(get_port()->available())
    {
      read_data(m_lls_buffer, LLS_MAX_BUFFER_LEN);
      l_status = get_status();
    }
    else 
      return set_status(DATA_NOT_READY_ERROR);
  }

  if(l_status != NO_DEVICE_ERROR)
    return set_status(l_status);

  return set_status(parse_data(m_lls_buffer, p_slave_address));
}

status LLS::set_output_interval(uint8_t p_slave_address, uint8_t p_seconds)
{
  if(p_slave_address > MAX_NUMBER_OF_SLAVES)
    return set_status(ADDRESS_ERROR);

  if(this->m_periodic)
    return set_status(PERIODIC_ERROR);

  uint8_t data[1] = { p_seconds };

  command_struct command;
  command.type = LLS_INTERVAL_ADJUSTMENT_CMD;
  command.data = data;
  command.length = 4;
  command.response = this->m_lls_buffer;
  command.buffer_len = LLS_MAX_BUFFER_LEN;
  command.address = p_slave_address;

  status l_status = send_command(&command);

  if(l_status != NO_DEVICE_ERROR)
    return set_status(l_status);

  if(this->m_lls_buffer[3] == LLS_CMD_FAILED)
    return set_status(COMMAND_FAILED_ERROR);
    
  return set_status(COMMAND_SUCCESS);
}

status LLS::set_binary_output(uint8_t p_slave_address)
{
  this->m_ascii = false;
  return set_output_mode(p_slave_address, LLS_BINARY_OUTPUT);
}

status LLS::set_ascii_output(uint8_t p_slave_address)
{
  this->m_ascii = true;
  return set_output_mode(p_slave_address, LLS_ASCII_OUTPUT);
}

status LLS::set_output_mode(uint8_t p_slave_address, uint8_t p_mode)
{
  if(p_slave_address > MAX_NUMBER_OF_SLAVES)
    return set_status(ADDRESS_ERROR);

  uint8_t data[1] = { p_mode };
  
  command_struct command;
  command.type = LLS_OUTPUT_MODE_CMD;
  command.data = data;
  command.length = 4;
  command.response = this->m_lls_buffer;
  command.buffer_len = LLS_MAX_BUFFER_LEN;
  command.address = p_slave_address;

  status l_status = send_command(&command);

  if(l_status != NO_DEVICE_ERROR)
    return set_status(l_status);
  
  if(this->m_lls_buffer[3] == LLS_CMD_FAILED)
    return set_status(COMMAND_FAILED_ERROR);
  
  return set_status(COMMAND_SUCCESS);
}

status LLS::send_command(command_struct *p_command)
{
  set_status(NO_DEVICE_ERROR);
  size_t data_length = p_command->length + 1;

  if(data_length > LLS_MAX_BUFFER_LEN)
    data_length = LLS_MAX_BUFFER_LEN;

  uint8_t data[LLS_MAX_BUFFER_LEN] =
  {
    LLS_TX_MESSAGE_HEADER, p_command->address, p_command->type
  };

  if(p_command->data)
    memcpy(&data[3], p_command->data, data_length - 1);

  write_data(data, data_length);

  p_command->response_length = read_data(p_command->response, p_command->buffer_len);

  return get_status();
}

status LLS::parse_data(uint8_t *p_data, uint8_t p_slave_address)
{
  if(p_data[0] != LLS_RX_MESSAGE_HEADER && !this->m_ascii)
    return set_status(HEADER_ERROR);

  p_slave_address -= 1;
  
  if(!this->m_ascii)
  {
    m_lls_data[p_slave_address].address = p_data[1];
    m_lls_data[p_slave_address].temp = p_data[HEX_TEMP_POS];
    m_lls_data[p_slave_address].fuel_level = (p_data[HEX_FUEL_POS + 1] << 8) | p_data[HEX_FUEL_POS];
    m_lls_data[p_slave_address].frequency_value = (p_data[HEX_FREQ_POS + 1] << 8) | p_data[HEX_FREQ_POS];
  }
  else
  {
    const char temp[2] = 
    {
      p_data[ASCII_TEMP_POS], p_data[ASCII_TEMP_POS + 1]
    };

    const char fuel_level[4] = 
    { 
      p_data[ASCII_FUEL_POS], p_data[ASCII_FUEL_POS + 1], 
      p_data[ASCII_FUEL_POS + 2], p_data[ASCII_FUEL_POS + 3] 
    };

    const char frequency[4] = 
    {
      p_data[ASCII_FREQ_POS], p_data[ASCII_FREQ_POS + 1],
      p_data[ASCII_FREQ_POS + 2], p_data[ASCII_FREQ_POS + 3]
    };
    
    m_lls_data[p_slave_address].address = 0x01;
    m_lls_data[p_slave_address].temp = strtol(temp, 0, 16);
    m_lls_data[p_slave_address].fuel_level = strtol(fuel_level, 0, 16);
    m_lls_data[p_slave_address].frequency_value = strtol(frequency, 0, 16);
  }

  if(m_lls_data[p_slave_address].address != p_slave_address + 1 && !this->m_ascii)
    return ADDRESS_ERROR;

  if(m_lls_data[p_slave_address].fuel_level > 0x0FFF)
    return FUEL_OUT_OF_RANGE_ERROR;
  
  return NO_DEVICE_ERROR;
}

uint16_t LLS::crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check)
{
  uint8_t crc = 0x00;

  for(size_t i = 0; i < p_length; i++)
  {
    uint8_t byte = *p_data++;
    for(int i = 0; i < 8; i++)
    {
      if((crc ^ byte) & 0x01)
        crc = ((crc ^ 0x18) >> 1) | 0x80;
      else
        crc >>= 1;

      byte >>= 1;
    }
  }

  return crc & 0xFF;
}