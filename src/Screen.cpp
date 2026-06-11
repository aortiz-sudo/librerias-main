#include "Screen.h"

status Screen::change_pic()
{
    command_struct command;

    uint8_t counter = (uint8_t)this->m_current_state->get_counter();
    uint8_t state = (uint8_t)this->m_current_screen_state;
    
    uint8_t data[] = { 0x00, state, counter };
    command.address = this->m_address;
    command.type = I2C_DISPLAY_CHANGE_PIC_CMD;
    command.data = data;
    command.length = sizeof(data);

    return send_command(&command);
}

status Screen::set_brightness(uint8_t p_brightness, bool p_save)
{
    command_struct command;

    if(p_save)
        this->m_brightness = p_brightness;

    logger.log("Brillo: ");
    logger.logln(p_brightness);

    uint8_t data[] = { 0x00, p_brightness };

    command.address = this->m_address;
    command.type = I2C_DISPLAY_CHANGE_BRIGHTNESS_CMD;
    command.data = data;
    command.length = sizeof(data);

    return send_command(&command);
}

status Screen::write_to_address(const uint16_t p_address, const uint8_t *p_data, const size_t p_length, bool p_little_endian)
{
    command_struct command;

    size_t length = 4 + p_length;
    uint8_t data[length] = { 0x00, (uint8_t)((p_address >> 8) & 0xFF), (uint8_t)(p_address & 0xFF), (uint8_t)p_length };

    int counter = p_little_endian ? (p_length - 1) : 0;
    for(int i = 4; i < length; i++)
    {
        data[i] = p_data[counter];
        counter = p_little_endian ? (counter - 1) : (counter + 1);
    }

    command.address = this->m_address;
    command.type = I2C_DISPLAY_WRITE_TO_ADDRESS_CMD;
    command.data = data;
    command.length = length;

    return send_command(&command);
}

status Screen::display_touch(uint16_t p_x, uint16_t p_y, press_mode p_mode)
{
    command_struct command;
    uint8_t data[] = 
    { 
        0x00, (uint8_t)((p_x >> 8) & 0xFF), (uint8_t)(p_x & 0xFF), 
        (uint8_t)((p_y >> 8) & 0xFF), (uint8_t)(p_y & 0xFF), (uint8_t)p_mode 
    };

    command.address = this->m_address;
    command.type = I2C_DISPLAY_TOUCH_CMD;
    command.data = data;
    command.length = sizeof(data);

    return send_command(&command);
}

status Screen::system_reset()
{
    command_struct command;

    uint8_t data[] = { 0x00 };
    command.address = this->m_address;
    command.type = I2C_DISPLAY_SYSTEM_RESET_CMD;
    command.data = data;
    command.length = sizeof(data);

    return send_command(&command);
}

status Screen::update(char p_key)
{
    if(p_key)
        display_touch(10, 10, TOUCH);

    this->m_current_state->screen(p_key);
    transition();

    // Solo enviar change_pic si hubo una transición real
    if(this->m_pic_changed)
    {
        this->m_pic_changed = false;
        return change_pic();
    }

    return NO_DEVICE_ERROR;
}

void Screen::init(uint8_t p_brightness)
{    
    system_reset();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    set_brightness(p_brightness, true);
    transition();
}

void Screen::set_transition(bool p_transition, screen_state p_screen_state)
{
    this->m_active_transition = p_transition;
    this->m_next_screen_state = p_screen_state;
}

void Screen::transition()
{
    if(!this->m_initialized)
    {
        this->m_current_screen_state = INITIAL_SCREEN_STATE;
        this->m_current_state = this->m_state[INITIAL_SCREEN_STATE];
        this->m_current_state->set_device(this);
        this->m_current_state->draw();
        this->m_initialized = true;
        this->m_pic_changed = true;
    }

    if(this->m_active_transition)
    {
        this->m_last_screen_state = this->m_current_screen_state;
        this->m_current_screen_state = this->m_next_screen_state;

        this->m_current_state->reset();
        this->m_current_state = this->m_state[this->m_current_screen_state];
        this->m_current_state->set_device(this);
        this->m_current_state->draw();

        this->m_active_transition = false;
        this->m_pic_changed = true;
    }
}