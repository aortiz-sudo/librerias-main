#include "State.h"
#include "Screen.h"

float State::m_volume = 0.0f;
float State::m_price = 0.0f;
float State::m_dispatched_volume = 0.0f;
float State::m_dispatched_cost = 0.0f;
bool State::m_quantity_dispatch = false;

//template <typename T>
//State::state_callback<T> State::callback_holder<T>::m_fn = nullptr;

uint8_t State::increment_counter(uint8_t p_current_counter, uint8_t p_max_counter)
{
    screen_state current_state = this->m_device->get_screen_state();
    uint8_t reset_val = (current_state != DATE_AND_HOUR_SCREEN_STATE) ? 1 : 0;
    reset_val = (current_state == BRIGHTNESS_SCREEN_STATE) ? p_max_counter : reset_val;

    uint8_t counter = (p_current_counter >= p_max_counter) ? reset_val : (p_current_counter + 1);

    return counter;
}

uint8_t State::decrement_counter(uint8_t p_current_counter, uint8_t p_max_counter)
{
    screen_state current_state = this->m_device->get_screen_state();
    uint8_t compare_val = (current_state != DATE_AND_HOUR_SCREEN_STATE) ? 1  : 0;
    uint8_t reset_val = (current_state == BRIGHTNESS_SCREEN_STATE) ? 1 : p_max_counter;

    uint8_t counter = (p_current_counter > compare_val) ? (p_current_counter - 1) : reset_val;

    return counter;
}

void State::handle_special_key(char p_key, uint8_t p_counter)
{
    switch(p_key)
    {
        case ENTER:
            if(this->m_counter > 0 && this->m_menu && p_counter > 0)
                this->m_device->set_transition(true, this->m_next_states[p_counter - 1]);
            return;

        case UP:
            if(this->m_menu)
            {
                this->m_counter = decrement_counter(this->m_counter, this->m_max_counter);
                logger.logln(this->m_counter);
            }
            return;

        case DOWN:
            if(this->m_menu)
            {
                this->m_counter = increment_counter(this->m_counter, this->m_max_counter);
                logger.logln(this->m_counter);
            }
            return;

        case LOCK:
            this->m_device->set_transition(true, INITIAL_SCREEN_STATE);
            return;

        case BACK:
            this->m_device->set_transition(true, this->m_last_state);   
        return;

        case CLEAR:
            if(p_counter > 0)
                this->m_counter = 0;
        return;
    }
}

void initial_state::draw()
{
    logger.logln("PANTALLA INICIAL\r\nPresione cualquier tecla para iniciar");
    this->m_device->change_pic();

    this->m_state_callback_function(nullptr, 0);
}

void initial_state::screen(char p_key)
{
    if(p_key)
        this->m_device->set_transition(true, LOG_IN_SCREEN_STATE);
}

void initial_state::reset()
{
    this->m_counter = 0;
}

void login_state::draw()
{
    logger.logln("PANTALLA LOGIN\r\nIntroduzca la contraseña");
    this->m_device->change_pic();
    this->m_device->write_to_address(DISPLAY_PASSWORD_ADDRESS, (uint8_t *)this->m_password_text, sizeof(this->m_password_text) - 1);
}

void login_state::screen(char p_key)
{
    this->m_next_state = this->m_device->get_calibration_pin_state() ? MENU_CAL_SCREEN_STATE : MENU_SCREEN_STATE;
    
    if(p_key)
    {
        const char *text = "                      ";
        this->m_device->write_to_address(DISPLAY_WRONG_PASS_ADDRESS, (uint8_t *)text, strlen(text));
    }

    if(isDigit(p_key) && m_digit_counter < m_max_counter)
    {
        this->m_keypad_buffer[this->m_digit_counter] = p_key;
        m_password_text[this->m_digit_counter] = '*';
        logger.log(this->m_password_text);
        m_digit_counter++;
        this->m_device->write_to_address(DISPLAY_PASSWORD_ADDRESS, (uint8_t *)this->m_password_text, sizeof(this->m_password_text) - 1); 
        return;
    }
    
    handle_special_key(p_key);

    if(p_key == CLEAR)
    {
        this->m_password_text[this->m_digit_counter - 1] = ' ';
        this->m_digit_counter = (this->m_digit_counter != 0) ? (this->m_digit_counter - 1) : 0;
        logger.log(this->m_password_text);
        this->m_device->write_to_address(DISPLAY_PASSWORD_ADDRESS, (uint8_t *)this->m_password_text, sizeof(this->m_password_text) - 1);

        return;
    }
    
    if(p_key == ENTER)
    {
        this->m_state_callback_function((uint8_t *)this->m_keypad_buffer, sizeof(m_keypad_buffer));
        reset();
    }
        
}

void login_state::reset()
{
    memset(this->m_keypad_buffer, '\0', sizeof(this->m_keypad_buffer));
    memset(this->m_password_text, ' ', 4);
    this->m_password_text[4] = '\0';

    this->m_digit_counter = 0;
    this->m_counter = 0;

    this->m_device->write_to_address(DISPLAY_PASSWORD_ADDRESS, (uint8_t *)this->m_password_text, sizeof(this->m_password_text) - 1);
}

void menu_state::draw()
{
    logger.logln("PANTALLA MENU\r\nBienvenido\r\n1. Despacho\r\n2. Re-impresion\r\n3. Configuracion");
    this->m_device->change_pic();
}

void menu_state::screen(char p_key)
{
    calibration = this->m_device->get_calibration_pin_state();

    if(calibration)
    {
        this->m_device->set_transition(true, MENU_CAL_SCREEN_STATE);
        return;
    }

    if(!p_key)
        return;

    if(!isDigit(p_key))
    {
        handle_special_key(p_key, this->m_counter);
        return;
    }
    
    if(p_key >= '1' && p_key <= this->m_max_counter + (uint8_t)('0'))
    {
        this->m_counter = (uint8_t)(p_key - '0');
        logger.logln(this->m_counter);
        return;
    }
}

void menu_state::reset()
{
    this->m_counter = 0;
}

void menu_cal_state::draw()
{
    logger.logln("PANTALLA MENU\r\nBienvenido\r\n1. Despacho\r\n2. Re-impresion\r\n3. Configuracion\r\n4. Calibracion");

    this->m_device->change_pic();
}

void menu_cal_state::screen(char p_key)
{
    calibration =this->m_device->get_calibration_pin_state();

    if(!calibration)
    {
        this->m_device->set_transition(true, MENU_SCREEN_STATE);
        return;
    }

    if(!isDigit(p_key))
    {
        handle_special_key(p_key, this->m_counter);
        return;
    }
    
    if(p_key >= '1' && p_key <= this->m_max_counter + (uint8_t)('0'))
    {
        this->m_counter = (uint8_t)(p_key - '0');
        logger.logln(this->m_counter);
        return;
    }
}

void menu_cal_state::reset()
{
    this->m_counter = 0;
}

void dispatch_menu_state::draw()
{
    logger.logln("PANTALLA ELECCION DE DESPACHO\r\n1. Despacho por cantidad\r\n2. Despacho libre\r\n");
    this->m_device->change_pic();
}

void dispatch_menu_state::screen(char p_key)
{
    this->m_last_state = this->m_device->get_calibration_pin_state() ? MENU_CAL_SCREEN_STATE : MENU_SCREEN_STATE;

    if(!isDigit(p_key))
    {
        handle_special_key(p_key, this->m_counter);
        return;
    }
    
    if(p_key == '1' || p_key =='2')
    {
        this->m_counter = (uint8_t)(p_key - '0');
        logger.logln(this->m_counter);
        return;
    }
}

void dispatch_menu_state::reset()
{
    this->m_counter = 0;
}

void reprint_menu_state::draw()
{
    logger.logln("PANTALLA RE-IMPRESION\r\n1. Reimprimir ultimo ticket\r\n2. Reimprimir ticket de todo el dia");
    this->m_device->change_pic();
}

void reprint_menu_state::screen(char p_key)
{
    this->m_last_state = this->m_device->get_calibration_pin_state() ? MENU_CAL_SCREEN_STATE : MENU_SCREEN_STATE;

    if(!isDigit(p_key))
    {
        handle_special_key(p_key, this->m_counter);

        if(p_key == ENTER && this->m_counter > 0)
        {
            this->m_state_callback_function(nullptr, (int)this->m_counter);
            this->m_device->set_transition(false);
            return;
        }

        return;
    }

    if(p_key == '1' || p_key == '2')
    {
        this->m_counter = (uint8_t)(p_key - '0');
        logger.logln(this->m_counter);
        return;
    }
}

void reprint_menu_state::reset()
{
    this->m_counter = 0;
}

void config_menu_state::draw()
{
    logger.logln("PANTALLA CONFIGURACION\r\n1. Fecha y hora\r\n2. Precio\r\n3. Datos del ticket\r\n4. Brillo de la pantalla");
    this->m_device->change_pic();
}

void config_menu_state::screen(char p_key)
{
    this->m_last_state = this->m_device->get_calibration_pin_state() ? MENU_CAL_SCREEN_STATE : MENU_SCREEN_STATE;

    if(!isDigit(p_key))
    {
        handle_special_key(p_key, this->m_counter);

        return;
    }

    if(p_key >= '1' && p_key <= '4')
    {
        this->m_counter = (uint8_t)(p_key - '0');
        logger.logln(this->m_counter);
        return;
    }
}

void config_menu_state::reset()
{
    this->m_counter = 0;
}

void calibration_state::draw()
{
    logger.logln("PANTALLA CALIBRACION\r\n1. Flujo medido: \r\n2. Fujo patron:");
    this->m_device->change_pic();
}

void calibration_state::screen(char p_key)
{
    this->m_last_state = this->m_device->get_calibration_pin_state() ? MENU_CAL_SCREEN_STATE : MENU_SCREEN_STATE;

    if((isDigit(p_key) && this->m_digit_counter < this->m_max_digit_counter & this->m_counter > 1) || p_key == CLEAR)
    {
        this->m_int_standard_liters = this->m_device->write_int_value<uint32_t>(DISPLAY_STANDARD_FLOW_ADDRESS, m_int_standard_liters, p_key); 
        logger.logln(this->m_int_standard_liters);
        return;
    }
    else
    if(!isDigit(p_key))
    {
        //handle_special_key(p_key, this->m_counter);
        
        if(p_key == ENTER)
        {
            this->m_counter = 1;
            this->m_state_callback_function((uint8_t *)&this->m_float_measured_liters, 0);
            this->m_device->set_calibrating(true);
        }
        else if(p_key == BACK)
        {
            if(this->m_counter < this->m_max_counter)
            {
                this->m_state_callback_function((uint8_t *)&this->m_float_measured_liters, 3);
                this->m_counter++;
            }
            
            if(this->m_counter == 3)
            {
                this->m_float_standard_liters = (float)(this->m_int_standard_liters) / 100.0f;
                this->m_state_callback_function((uint8_t *)&this->m_float_standard_liters, 2);
                this->m_device->set_transition(true, this->m_last_state);
            }

           this->m_device->set_calibrating(false);
        }
    }

    if(this->m_device->get_calibrating())
    {
        this->m_state_callback_function((uint8_t *)&this->m_float_measured_liters, 1);
        this->m_int_measured_liters = (uint32_t)(this->m_float_measured_liters * 100.0f);
        this->m_device->write_to_address(DISPLAY_MEASURED_FLOW_ADDRESS, (uint8_t *)&this->m_int_measured_liters, sizeof(this->m_int_measured_liters), true);
    }
}

void calibration_state::reset()
{
    this->m_counter = 0;
    this->m_int_measured_liters = 0;
    this->m_int_standard_liters = 0;
    this->m_float_measured_liters = 0.0f;
    this->m_float_standard_liters = 0.0f;

    this->m_device->write_to_address(DISPLAY_STANDARD_FLOW_ADDRESS, (uint8_t *)&this->m_int_standard_liters, sizeof(this->m_int_standard_liters), true);
    this->m_device->write_to_address(DISPLAY_MEASURED_FLOW_ADDRESS, (uint8_t *)&this->m_int_measured_liters, sizeof(this->m_int_measured_liters), true);
}

void quantity_dispatch_state::draw()
{
    logger.logln("PANTALLA DESPACHO POR CANTIDAD\r\n1. Cantidad en litros:\r\n2. Cantidad en precio:");
    this->m_device->write_int_value<uint32_t>(DISPLAY_VOLUME_DISPATCH_ADDRESS, this->m_int_volume);
    this->m_device->write_int_value<uint32_t>(DISPLAY_PRICE_DISPATCH_ADDRESS, this->m_int_cost);
    this->m_device->change_pic();
    this->m_price = this->m_device->get_price();
}

void quantity_dispatch_state::screen(char p_key)
{
    if(this->m_counter > 0)
    {
        this->m_current_index = this->m_counter - 1;
        this->m_other_index = (this->m_current_index == 0) ? 1 : 0; 
    }

    if(p_key)
    {
        handle_special_key(p_key, 0);

        if(p_key == UP || p_key == DOWN)
        {
            this->m_digit_counter[0] = 0;
            this->m_digit_counter[1] = 0;   
            this->m_dispatch_values[0] = 0;
            this->m_dispatch_values[1] = 0;

            return;
        }

        if(p_key == ENTER && this->m_int_volume > 0 && this->m_int_cost > 0)
        {
            m_quantity_dispatch = true;
            m_volume = m_int_volume / 100.00;
            this->m_device->set_transition(true, this->m_next_states[0]);
            return;
        }
        
        if(isDigit(p_key) && this->m_digit_counter[this->m_current_index] < this->m_max_digit_counter && this->m_counter > 0 || p_key == CLEAR)
        {
            this->m_dispatch_values[this->m_current_index] = this->m_device->write_int_value<uint32_t>(this->m_addresses[this->m_current_index], this->m_dispatch_values[this->m_current_index], p_key);
            this->m_dispatch_values[this->m_other_index] = (this->m_current_index == 1) ? (uint32_t)(round(this->m_dispatch_values[this->m_current_index] / this->m_price)) : (uint32_t)(round(this->m_price * this->m_dispatch_values[this->m_current_index]));
            this->m_device->write_to_address(this->m_addresses[this->m_other_index], (uint8_t *)&this->m_dispatch_values[this->m_other_index], sizeof(uint32_t), true);
            this->m_int_volume = this->m_dispatch_values[0];
            this->m_int_cost = this->m_dispatch_values[1];
            logger.log("INT VOLUME: ");
            logger.logln(this->m_int_volume);
            logger.log("FLOAT VOLUME: ");
            logger.logln(this->m_int_volume / 100.0);
            logger.log("INT COST: ");
            logger.logln(this->m_int_cost);
            logger.log("FLOAT COST: ");
            logger.logln(this->m_int_cost / 100.0);

            if(!(p_key == '0' && this->m_dispatch_values[this->m_current_index] == 0))
                this->m_digit_counter[this->m_current_index] = (p_key == CLEAR && this->m_digit_counter[this->m_current_index] > 0) ? this->m_digit_counter[this->m_current_index]  - 1 : this->m_digit_counter[this->m_current_index]  + 1;
        }
    }
}

void quantity_dispatch_state::reset()
{
    this->m_counter = 0;
    this->m_digit_counter[0] = 0;
    this->m_digit_counter[1] = 0;
    this->m_dispatch_values[0] = 0;
    this->m_dispatch_values[1] = 0;
    this->m_int_volume = 0;
    this->m_int_cost = 0;
    this->m_device->write_int_value<uint32_t>(DISPLAY_VOLUME_DISPATCH_ADDRESS, this->m_int_volume);
    this->m_device->write_int_value<uint32_t>(DISPLAY_PRICE_DISPATCH_ADDRESS, this->m_int_cost);
}

void dispatch_state::draw()
{
    logger.logln("PANTALLA DESPACHO");
    this->m_device->change_pic();
    m_price = this->m_device->get_price();
    uint32_t zero = 0;
    this->m_device->write_to_address(DISPLAY_DISPATCHED_VOLUME_ADDRESS, (uint8_t *)&zero, sizeof(zero));
    this->m_device->write_to_address(DISPLAY_DISPATCHED_COST_ADDRESS, (uint8_t *)&zero, sizeof(zero));

    const char *product = this->m_device->get_product();
    this->m_device->write_to_address(DISPLAY_PRODUCT_ADDRESS, (uint8_t *)product, strlen(product));

    if(!this->m_device->get_tag_detected())
    {  
        this->m_device->set_dispatching(false);
        this->m_counter = 0;
        this->m_device->write_to_address(DISPLAY_TAG_MESSAGE_ADDRESS, (uint8_t *)this->m_no_tag_message, strlen(this->m_no_tag_message));
    }
}

void dispatch_state::screen(char p_key)
{
    if(!isDigit(p_key))
    {
        //handle_special_key(p_key);

        if(p_key == BACK)
        {
            logger.logln(this->m_counter);
            if(this->m_counter == 0)
                this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 2);

            if(this->m_counter == 1 && m_dispatched_volume > 0.0)
                this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 3);

            if(this->m_counter == 2)
            {
                this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 2);
                this->m_device->set_transition(true, DISPATCH_MENU_SCREEN_STATE);
            }
            
            if(this->m_counter < this->m_max_counter)
                this->m_counter++;

            this->m_device->set_dispatching(false);

            return;
        }

        if((p_key == ENTER || (this->m_device->get_tag_detected() && !dispatch_initialized)) && this->m_counter < 2 && !this->m_device->get_dispatching())
        {
            dispatch_initialized = true;
            this->m_counter = 0;
            this->m_device->set_dispatching(true);
            this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 0);
            const char *text = "                 ";
            this->m_device->write_to_address(DISPLAY_TAG_MESSAGE_ADDRESS, (uint8_t *)text, strlen(text));
        }
    }

    if(this->m_counter == 0 && !this->m_device->get_calibration_pin_state())
    {
        if(OPENED_CABINET)
        {
            this->m_counter = 1;
            this->m_device->set_dispatching(false);
            const char *text1 = "                 ";
            this->m_device->write_to_address(DISPLAY_TAG_MESSAGE_ADDRESS, (uint8_t *)text1, strlen(text1));
            const char *text2 = "GABINETE ABIERTO";
            this->m_device->write_to_address(DISPLAY_TAG_MESSAGE_ADDRESS, (uint8_t *)text2, strlen(text2));
        }
        else if(CLOSED_CABINET)
        {
            this->m_counter = 0;
            this->m_device->set_dispatching(true);
            const char *text = "                   ";
            this->m_device->write_to_address(DISPLAY_TAG_MESSAGE_ADDRESS, (uint8_t *)text, strlen(text));
        }
    }
    
    if(this->m_device->get_dispatching())
    {
        this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 1);
        m_dispatched_cost = m_price * m_dispatched_volume;
        uint32_t int_dispatched_liters = (uint32_t)(m_dispatched_volume * 100);
        uint32_t int_dispatched_cost = (uint32_t)(m_dispatched_cost * 100);
        this->m_device->write_to_address(DISPLAY_DISPATCHED_VOLUME_ADDRESS, (uint8_t *)&int_dispatched_liters, sizeof(uint32_t), true);
        this->m_device->write_to_address(DISPLAY_DISPATCHED_COST_ADDRESS, (uint8_t *)&int_dispatched_cost, sizeof(uint32_t), true);
    }

    if(m_dispatched_volume >= m_volume * 0.80 && m_quantity_dispatch && !already_stop)
    {   
        already_stop = true;
        this->m_state_callback_function((uint8_t*)&m_dispatched_volume, 4);
    }

    if(m_dispatched_volume >= m_volume && m_quantity_dispatch && this->m_counter < 2)
    {
        this->m_counter = 2;
        this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 1);
        m_dispatched_cost = m_price * m_dispatched_volume;
        uint32_t int_dispatched_liters = (uint32_t)(m_dispatched_volume * 100);
        uint32_t int_dispatched_cost = (uint32_t)(m_dispatched_cost * 100);
        this->m_device->write_to_address(DISPLAY_DISPATCHED_VOLUME_ADDRESS, (uint8_t *)&int_dispatched_liters, sizeof(uint32_t), true);
        this->m_device->write_to_address(DISPLAY_DISPATCHED_COST_ADDRESS, (uint8_t *)&int_dispatched_cost, sizeof(uint32_t), true);
        this->m_device->set_dispatching(false);
        this->m_state_callback_function((uint8_t *)&m_dispatched_volume, 3);
    }
}

void dispatch_state::reset()
{
    dispatch_initialized = false;
    already_stop = false;
    this->m_counter = 0;
    m_dispatched_volume = 0.0;
    m_dispatched_cost = 0.0;
    this->m_device->set_dispatching(false);
    this->m_device->set_tag_detected(false);
    m_volume = 0;
    m_quantity_dispatch = false;

    uint32_t zero = 0;

    this->m_device->write_to_address(DISPLAY_DISPATCHED_VOLUME_ADDRESS, (uint8_t *)&zero, sizeof(uint32_t), true);
    this->m_device->write_to_address(DISPLAY_DISPATCHED_COST_ADDRESS, (uint8_t *)&zero, sizeof(uint32_t), true);
}

void date_and_hour_state::draw()
{
    logger.logln("PANTALLA FECHA Y HORA");
    this->m_device->change_pic();

    for(int i = 0; i < 5; i++)
        this->m_rtc_info[i] = this->m_device->write_int_value<uint16_t>(this->m_rtc_info_addresses[i],  this->m_rtc_info[this->m_counter]);
}

void date_and_hour_state::screen(char p_key)
{
    if(this->m_last_counter != this->m_counter)
    {
        this->m_digit_counter = 0;
        this->m_last_counter = this->m_counter;
    }

    if(!isDigit(p_key) && p_key != CLEAR)
    {
        handle_special_key(p_key, 0);

        if(p_key == ENTER)
            this->m_state_callback_function(this->m_rtc_info, sizeof(this->m_rtc_info));

        return;
    }
    
    if((isDigit(p_key) && this->m_digit_counter < this->m_max_digit_counter) || p_key == CLEAR)
    {
        this->m_rtc_info[this->m_counter] = this->m_device->write_int_value<uint16_t>(this->m_rtc_info_addresses[this->m_counter],  (uint16_t)this->m_rtc_info[this->m_counter], p_key);
        logger.logln(this->m_rtc_info[this->m_counter]);

        if(!(p_key == '0' && this->m_rtc_info[this->m_counter] == 0))
        {
            this->m_digit_counter = (p_key == CLEAR && this->m_digit_counter > 0) ? this->m_digit_counter - 1 : this->m_digit_counter + 1;
            this->m_digit_counter = (this->m_digit_counter > this->m_max_counter) ? this->m_max_digit_counter : this->m_digit_counter; 
        }
    }
}

void date_and_hour_state::reset()
{
    this->m_counter = 0;
    this->m_digit_counter = 0;

    for(int i = 0; i < 5; i++)
        this->m_rtc_info[i] = this->m_device->write_int_value<uint16_t>(this->m_rtc_info_addresses[i],  this->m_rtc_info[this->m_counter]);
}

void price_state::draw()
{
    logger.logln("PANTALLA PRECIO");
    m_price = this->m_device->get_price();
    this->m_device->change_pic();
    this->m_int_price = (uint16_t)(this->m_price * 100);
    this->m_int_new_price = 0;
    this->m_device->write_to_address(DISPLAY_CURRENT_PRICE_ADDRESS, (uint8_t *)&this->m_int_price, sizeof(uint16_t), true);
    this->m_device->write_to_address(DISPLAY_NEW_PRICE_ADDRESS, (uint8_t *)&this->m_int_new_price, sizeof(uint16_t), true);
    logger.logln(this->m_price);
}

void price_state::screen(char p_key)
{
    if(!isDigit(p_key) && p_key != CLEAR)
    {
        handle_special_key(p_key);

        if(p_key == ENTER && this->m_int_new_price > 0)
        {
            m_price = this->m_int_new_price / 100.0;
            this->m_device->write_to_address(DISPLAY_CURRENT_PRICE_ADDRESS, (uint8_t *)&this->m_int_new_price, sizeof(uint16_t), true);
            this->m_int_new_price = 0;
            this->m_device->write_to_address(DISPLAY_NEW_PRICE_ADDRESS, (uint8_t *)&this->m_int_new_price, sizeof(uint16_t), true);
            this->m_device->set_price(m_price);
            this->m_state_callback_function((uint8_t *)&m_price, 1);
            this->m_digit_counter = 0;
        }
        else if(p_key == BACK)
            this->m_int_new_price = 0;

        return;
    }
    
    if(((isDigit(p_key)) && this->m_digit_counter < this->m_max_digit_counter) || p_key == CLEAR)
    {
        this->m_int_new_price = this->m_device->write_int_value<uint16_t>(DISPLAY_NEW_PRICE_ADDRESS, this->m_int_new_price, p_key);
        
        if(!(p_key == 0 && this->m_int_new_price == 0))
        {
            this->m_digit_counter = (p_key == CLEAR && this->m_digit_counter > 0) ? this->m_digit_counter - 1 : this->m_digit_counter + 1;
            this->m_digit_counter = (this->m_digit_counter > this->m_max_digit_counter) ? this->m_max_digit_counter : this->m_digit_counter;
        }

        logger.logln(this->m_int_new_price);
    }
}

void price_state::reset()
{
    this->m_counter = 0;
    this->m_int_new_price = 0;
    this->m_int_price = 0;
    this->m_digit_counter = 0;
}

void ticket_info_state::draw()
{
    logger.logln("PANTALLA DATOS DEL TICKET");
    this->m_device->change_pic();
    this->m_state_callback_function(nullptr, 1);
}

void ticket_info_state::screen(char p_key)
{  
    if(!isDigit(p_key))
    {
        handle_special_key(p_key);

        if(p_key == CLEAR && this->m_show)
            this->m_show = false;

        return;
    }
}

void ticket_info_state::reset()
{
    this->m_counter = 0;
    this->m_show = false;

    this->m_state_callback_function(nullptr, 2);
}

void brightness_state::draw()
{
    logger.logln("PANTALLA BRILLO");
    this->m_device->change_pic();
    this->m_brightness = this->m_device->get_brightness();
    this->m_bar_value = this->m_brightness;
    logger.logln(this->m_bar_value);
    logger.logln(this->m_brightness);
    this->m_device->write_to_address(DISPLAY_BRIGHTNESS_BAR_ADDRESS, (uint8_t *)&this->m_bar_value, sizeof(uint16_t), true);
}

void brightness_state::screen(char p_key)
{
    if(!isDigit(p_key))
    {
        handle_special_key(p_key);

        if(p_key == ENTER)
        {
            this->m_state_callback_function((uint8_t *)&this->m_bar_value, 1);
            this->m_brightness = this->m_bar_value;
            this->m_save = true;
            this->m_device->set_transition(true, this->m_last_state);

            return;
        }
        
        if(p_key == UP || p_key == DOWN)
        {
            this->m_bar_value = (p_key == DOWN ) ? this->m_bar_value - 10 : this->m_bar_value + 10;
            this->m_bar_value = (this->m_bar_value <= 10) ? 10 : this->m_bar_value;
            this->m_bar_value = (this->m_bar_value >= 100) ? 100 : this->m_bar_value;
            this->m_device->write_to_address(DISPLAY_BRIGHTNESS_BAR_ADDRESS, (uint8_t *)&this->m_bar_value, sizeof(uint16_t), true);
            this->m_device->set_brightness(this->m_bar_value);
            logger.logln(this->m_bar_value);

            return;
        }

        if(p_key == BACK && !this->m_save)
        {
            this->m_device->set_brightness(this->m_brightness);
            return;
        }
    }
}

void brightness_state::reset()
{
    this->m_counter = 0;
    this->m_bar_value = 0;
    this->m_save = false;
    this->m_brightness = 0;
}