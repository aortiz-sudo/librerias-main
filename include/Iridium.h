#ifndef _IRIDIUM_H
#define _IRIDIUM_H

#include "Satellite.h"
#include "string_handlers.h"

#define REPEAT_LAST_CMD             "A/"
#define ECHO_CMD                    "ATEn"
#define QUIET_MODE_CMD              "ATQn"
#define VERBOSE_MODE_CMD            "ATVn"
#define SOFT_RESET_CMD              "ATZn"
#define DTR_OPTION_CMD              "AT&Dn"
#define RESTORE_SETTINGS_CMD        "AT&Fn"
#define FLOW_CONTROL_CMD            "AT&Kn"
#define GET_CONFIG_CMD              "AT&V"
#define SET_CONFIG_CMD              "AT&Wn"
#define SET_PROFILE_CMD             "AT&Yn"
#define DISPLAY_REGISTERS_CMD       "AT%R"
#define FLUSH_EEPROM_CMD            "AT*F"
#define RADIO_ACTIVITY_CMD          "AT*Rn"
#define GET_RTC_CMD                 "AT+CCLK"
#define GET_SERIAL_NUMBER_CMD       "AT+CGSN"
#define INDICATOR_EVENT_CMD         "AT+CIER"
#define RING_INDICATION_STATUS_CMD  "AT+CRIS"
#define SIGNAL_QUALITY_STATUS_CMD   "AT+CSQ"
#define SET_BAUDRATE_CMD            "AT+IPR"
#define WRITE_BIN_DATA_CMD          "AT+SBDWB"
#define READ_BIN_DATA_CMD           "AT+SBDRB"
#define WRITE_TEXT_MSG_CMD          "AT+SBDWT"
#define READ_TEXT_MSG_CMD           "AT+SDBRT"
#define INITIATE_SBD_SESSION_CMD    "AT+SBDI"
#define INITIATE_SBD_E_SESSION_CMD  "AT+SBDIX"
#define CLEAR_SBD_MSG_BUFFER_CMD    "AT+SBDD"

#define MAX_TX_BIN_DATA_LEN     340
#define MAX_RX_BIN_DATA_LEN     270
#define MAX_TX_TEXT_MESSAGE_LEN 120
#define MAX_RX_TEXT_MESSAGE_LEN 270

#define CLEAR_MOBILE_ORIGINATED_BUFFER (uint8_t)0
#define CLEAR_MOBILE_TERMINATED_BUFFER (uint8_t)1
#define CLEAR_BOTH_BUFFERS             (uint8_t)2

#define ENABLE  (uint8_t)1
#define DISABLE (uint8_t)0

#define IRIDIUM_BINARY_MESSAGE (uint8_t)0x00
#define IRIDIUM_TEXT_MESSAGE   (uint8_t)0x01

struct iridium_param_struct : uart_param_struct
{
    iridium_param_struct()
    {
        this->m_data_transmission_rate = 19200;
        this->m_crc = CRC_16;
    }
};

typedef enum : uint8_t
{
    _0,
    _600_BPS,
    _1200_BPS,
    _4800_BPS,
    _9600_BPS,
    _19200_BPS,
    _38400_BPS,
    _57600_BPS,
    _115200_BPS,
} iridium_baudrate;

class Iridium : public Satellite
{
    public:
        int send_data_to_server(const uint8_t *p_data, size_t p_data_length) override;
        int get_data_from_server(uint8_t *p_data, size_t p_buffer_length) override;
        status get_id(uint8_t *p_id_buffer);
        void reset_device() override;
    
    protected:
        status send_command(command_struct *p_command) override;
        uint16_t crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check = false) override;
};

class Iridium_Builder : public UART_Device_Builder
{
    public:
        void set_device_parameters(device_param_struct *p_params) override
        {
            iridium_param_struct *p = static_cast<iridium_param_struct*>(p_params);
            this->m_iridium_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_iridium_params.m_rx_pin = p->m_rx_pin;
            this->m_iridium_params.m_tx_pin = p->m_tx_pin;
            this->m_iridium_params.m_enable_pin = p->m_enable_pin;
            this->m_iridium_params.m_timeout = p->m_timeout;
            this->m_iridium_params.m_enable_state = p->m_enable_state;
            this->m_iridium_params.m_type = p->m_type;
            this->m_iridium_params.m_crc = p->m_crc;
            this->m_iridium_params.m_port = p->m_port;
            this->m_iridium_params.m_config = p->m_config;
        }

        Device *get_device() override
        {
            this->iridium_device.begin(&this->m_iridium_params);

            return &this->iridium_device;
        }

    private:
        iridium_param_struct m_iridium_params;
        Iridium iridium_device;
};
#endif