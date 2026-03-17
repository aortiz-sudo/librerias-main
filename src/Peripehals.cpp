#include "Peripehals.h"

void Peripehals::set_device_parameters(const device_param_struct *p_params)
{
    const peripehals_param_struct *p = static_cast<const peripehals_param_struct *>(p_params);
    set_transmission_rate(p->m_data_transmission_rate);
    this->m_scl_pin = p->m_scl_pin;
    this->m_sda_pin = p->m_sda_pin;
    set_type(p->m_type);
    set_crc_type(p->m_crc);
    set_port(p->m_port);
    this->m_address = p->m_address;

    strcpy(this->m_mac_address, p->m_mac_address);
}

void Peripehals::get_device_parameters(device_param_struct *p_params)
{
    peripehals_param_struct *p = static_cast<peripehals_param_struct *>(p_params);
    p->m_data_transmission_rate = get_transmission_rate();
    p->m_scl_pin = this->m_scl_pin;
    p->m_sda_pin = this->m_sda_pin;
    p->m_type = get_type();
    p->m_crc = get_crc_type();
    p->m_port = get_port();
    p->m_address = this->m_address;

    strcpy(p->m_mac_address, this->m_mac_address);
}

status Peripehals::write_mac_address()
{
    command_struct command;

    const size_t length = 19;
    uint8_t data[length] = { 0x00 };

    for(int i = 1; i < length; i++)
        data[i] = (uint8_t)this->m_mac_address[i - 1];
    
    command.address = this->m_address;
    command.type = I2C_PRINTER_MAC_ADDRESS_CMD;
    command.data = data;
    command.length = length;

    status command_status;

    while(!this->m_data_received)
    {
        command_status = send_command(&command);
        read_peripehal_data(&this->m_voltage, &this->m_sat_out, &this->m_printer_connected);
    }

    this->m_data_received = false;

    return command_status;
}

status Peripehals::read_peripehal_data(float *p_voltage, uint8_t *p_sat_out, uint8_t *p_printer_connected)
{
    uint8_t data[7];
    
    if(!(read_data(data, 7) > 0))
        return get_status();

    *p_voltage = *((float *)&data[0]);
    *p_sat_out = data[4];
    *p_printer_connected = 1;
    this->m_data_received = data[6];

    return get_status();
}

status Peripehals::printer_write(uint8_t *p_data, size_t p_length)
{
    command_struct command;

    size_t length = p_length + 1;

    if(length > I2C_BUFFER_LENGTH)
        length = I2C_BUFFER_LENGTH;

    uint8_t data[I2C_BUFFER_LENGTH] = { (uint8_t)p_length }; 

    for(size_t i = 1; i < length; i++)
        data[i] = p_data[i - 1];
    
    command.address = this->m_address;
    command.type = I2C_PRINTER_PRINTER_PRINT_CMD;
    command.data = data;
    command.length = length;

    status command_status;

    while(!this->m_data_received)
    {
        command_status = send_command(&command);
        read_peripehal_data(&this->m_voltage, &this->m_sat_out, &this->m_printer_connected);
    }

    this->m_data_received = false;

    return command_status;
}

status Peripehals::printer_write(const char *p_str)
{
    return printer_write((uint8_t *)p_str, strlen(p_str));
}

status Peripehals::print_float(float p_float)
{
    uint32_t int_val = (uint32_t)(p_float);
    uint32_t dec_val = (uint32_t)((p_float - int_val) * 100);

    char str_float[24];
    size_t length = snprintf(str_float, sizeof(str_float), "%u.%02u\0", int_val, dec_val);

    return printer_write((uint8_t *)str_float, length);
}

status Peripehals::print_int(int p_int)
{
    char str_int[24];
    size_t length = snprintf(str_int, sizeof(str_int), "%d\0", p_int);

    return printer_write((uint8_t *)str_int, length);
}

status Peripehals::print_uint(uint32_t p_uint)
{
    char str_uint[24];
    size_t length = snprintf(str_uint, sizeof(str_uint), "%d\0", p_uint);

    return printer_write((uint8_t *)str_uint, length);
}

status Peripehals::reset_printer()
{
    command_struct command;
    command.address = this->m_address;
    command.type = I2C_PRINTER_RESET_CMD;
    command.data = nullptr;
    command.length = 0;

    status command_status;

    while(!this->m_data_received)
    {
        command_status = send_command(&command);
        read_peripehal_data(&this->m_voltage, &this->m_sat_out, &this->m_printer_connected);
    }

    this->m_data_received = false;

    return command_status;
}