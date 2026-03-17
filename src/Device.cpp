#include "Device.h"

size_t Device::write(uint8_t p_data)
{
    return write_data(&p_data, 0x01);
}

size_t Device::write(uint8_t *p_data, size_t p_data_length)
{
    return write_data(p_data, p_data_length);
}