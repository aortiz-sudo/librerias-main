#include "UART_Device.h"

void UART_Device::set_device_parameters(const device_param_struct *p_params)
{
    const uart_param_struct *p = static_cast<const uart_param_struct *>(p_params);
    set_transmission_rate(p_params->m_data_transmission_rate);
    this->m_rx_pin = p->m_rx_pin;
    this->m_tx_pin = p->m_tx_pin;
    this->m_enable_pin = p->m_enable_pin;
    this->m_timeout = p->m_timeout;
    this->m_enable_state = p->m_enable_state;
    this->m_enable_delay = p->m_enable_delay;
    set_type(p->m_type);
    set_crc_type(p->m_crc);
    set_port(p_params->m_port);
    this->m_config = p->m_config;
}

void UART_Device::get_device_parameters(device_param_struct *p_params)
{
    uart_param_struct *p = static_cast<uart_param_struct *>(p_params);
    p->m_data_transmission_rate = get_transmission_rate();
    p->m_rx_pin = this->m_rx_pin;
    p->m_tx_pin = this->m_tx_pin;
    p->m_enable_pin = this->m_enable_pin;
    p->m_timeout = this->m_timeout;
    p->m_enable_state = this->m_enable_state;
    p->m_enable_delay = this->m_enable_delay;
    p->m_type = get_type();
    p->m_crc = get_crc_type();
    p->m_port = get_port();
    p->m_config = this->m_config;
}

void UART_Device::begin(const device_param_struct *p_params)
{
    set_device_parameters(p_params);
    #if defined(ESP32)
        static_cast<HardwareSerial *>(get_port())->begin(get_transmission_rate(), this->m_config, this->m_rx_pin, this->m_tx_pin);
    #else
       static_cast<HardwareSerial *>(get_port())->begin(get_transmission_rate(), this->m_config);
    #endif

    if(this->m_enable_pin > -1)
    {
        pinMode(this->m_enable_pin, OUTPUT);
        digitalWrite(this->m_enable_pin, this->m_enable_state);
    }
}

size_t UART_Device::write_data(uint8_t *p_data, size_t p_data_length)
{    
    crc_type crc = get_crc_type();
    switch(crc)
    {
        case NO_CRC:
        break;

        case CRC_8:
            p_data[p_data_length - 1] = crc_calculation(p_data, p_data_length - 1) & 0xFF;
        break;

        case CRC_16:
            uint16_t data_crc = crc_calculation(p_data, p_data_length - 2);
            p_data[p_data_length - 1] = data_crc >> 8;
            p_data[p_data_length - 2] = data_crc & 0xFF;
        break;
    }

    return get_port()->write(p_data, p_data_length);
}

size_t UART_Device::read_data(uint8_t *p_buffer, size_t p_buffer_length)
{
    size_t count = 0;
    set_status(NO_DEVICE_ERROR);

    unsigned long time = millis();
    
    while(!get_port()->available() && this->m_timeout > 0)
    {
        if(millis() - time >= this->m_timeout)
        {
            set_status(TIMEOUT_ERROR);
            return 0;
        }
    }

    count = get_port()->readBytes(p_buffer, p_buffer_length);

    if(get_crc_type() != NO_CRC && crc_calculation(p_buffer, count, true) != 0)
    {
        set_status(CRC_ERROR);
        return 0;
    }
    
    return count;
}

void UART_Device::reset_serial_port(unsigned long p_BAUD_RATE, int p_config)
{
    static_cast<HardwareSerial *>(get_port())->flush();
    #if defined(ESP32)
        vTaskDelay(2 / portTICK_PERIOD_MS);
    #else
        delay(2);
    #endif
    static_cast<HardwareSerial *>(get_port())->end();
    #if defined(ESP32)
      static_cast<HardwareSerial *>(get_port())->begin(p_BAUD_RATE, p_config, this->m_rx_pin, this->m_tx_pin);
    #else
      static_cast<HardwareSerial *>(get_port())->begin(p_BAUD_RATE, p_config);
    #endif
}

void UART_Device::reset_serial_port()
{
    static_cast<HardwareSerial *>(get_port())->flush();
    #if defined(ESP32)
        vTaskDelay(2 / portTICK_PERIOD_MS);
    #else
        delay(2);
    #endif
    static_cast<HardwareSerial *>(get_port())->end();
    #if defined(ESP32)
      static_cast<HardwareSerial *>(get_port())->begin(get_transmission_rate(), this->m_config, this->m_rx_pin, this->m_tx_pin);
    #else
      static_cast<HardwareSerial *>(get_port())->begin(get_transmission_rate(), this->m_config);
    #endif
}

void UART_Device::activate_enable_pin()
{
    if(this->m_enable_pin > -1)
    {
        digitalWrite(this->m_enable_pin, this->m_enable_state);
        #if defined(ESP32)
            vTaskDelay(this->m_enable_delay / portTICK_PERIOD_MS);
        #else
            delay(2);
        #endif
    }
}

void UART_Device::deactivate_enable_pin()
{
    if(this->m_enable_pin > -1)
    {
        digitalWrite(this->m_enable_pin, !this->m_enable_state);
        #if defined(ESP32)
            vTaskDelay(this->m_enable_delay / portTICK_PERIOD_MS);
        #else
            delay(2);
        #endif
    }
} 