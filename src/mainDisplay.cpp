#include "mainDisplay.h"

Display *display;
Display_Builder display_builder;

uint8_t current_state = 0;
uint8_t last_state = 0;
uint8_t current_counter = 0;
uint8_t last_counter = 0;
uint8_t i2c_rx_counter = 0;
uint8_t i2c_rx_data[I2C_BUFFER_LEN];
uint8_t rtc_data[7];
bool i2c_data_received = false;

uint8_t state_pic_numbers[]
{
  INITIAL_SCREEN_PIC, LOG_IN_SCREEN_PIC, MAIN_MENU_NCAL_PIC, MAIN_MENU_WCAL_PIC, DISPATCH_MENU_PIC,
  PRINT_TICKET_MENU_PIC, CONFIGURATION_MENU_PIC, CALIBRATION_MENU_PIC, QUANTITY_DISPATCH_MENU_PIC,
  DISPATCHING_PRESS_TO_START_PIC, DATE_MENU_HOUR_PIC, PRODUCT_PRICE_PIC, TICKET_INFO_PIC, SCREEN_BRIGHTNESS_PIC
};

void I2C_RxHandler(int args);
void execute_I2C_command(uint8_t _command_byte);
void configure_wdt();
void enable_tx_interrupt();
void build_display();

void setup() 
{
  configure_wdt();
  Wire.begin(I2C_SLAVE_ADDR);
  build_display();
  enable_tx_interrupt();
  Wire.onReceive(I2C_RxHandler);
}

void loop() 
{ 
  if(i2c_data_received)
  {
    uint8_t command = i2c_rx_data[0];
    execute_I2C_command(command);
    i2c_rx_counter = 0;
    i2c_data_received = false;
  }
}

void I2C_RxHandler(int p_length)
{
  while(Wire.available() && i2c_rx_counter < I2C_BUFFER_LEN)
  {
    i2c_rx_data[i2c_rx_counter] = Wire.read();
    i2c_rx_counter++;
  }

  i2c_data_received = true;
}

void execute_I2C_command(uint8_t _command_byte)
{
  switch(_command_byte)
  {
    case I2C_DISPLAY_CHANGE_PIC_CMD:
    {
      current_state = i2c_rx_data[2];
      current_counter = i2c_rx_data[3];

      if((last_counter != current_counter && last_state == current_state) || last_state != current_state)
      {
        last_state = current_state;
        last_counter = current_counter;
      }

      display->set_pic(state_pic_numbers[current_state] + current_counter);

      break;
    }
    
    case I2C_DISPLAY_CHANGE_BRIGHTNESS_CMD:
    {
      uint8_t brightness = i2c_rx_data[2];

      display->set_led_config(brightness);
      break;
    }
    
    case I2C_DISPLAY_WRITE_TO_ADDRESS_CMD:
    {
      uint16_t address = (i2c_rx_data[2] << 8) | i2c_rx_data[3];
      uint8_t length = i2c_rx_data[4];
      uint8_t data[length];

      for(int i = 0; i < length; i++)
      {
        data[i] = i2c_rx_data[i + 5];
      } 
      
      display->set_user_variable(address, data, length);

      break;
    }
    
    case I2C_DISPLAY_SYSTEM_RESET_CMD:
    {
      display->system_reset();
      break;
    }

    case I2C_DISPLAY_TOUCH_CMD:
    {
      uint16_t x_coord = (i2c_rx_data[2] << 8) | i2c_rx_data[3];
      uint16_t y_coord = (i2c_rx_data[4] << 8) | i2c_rx_data[5];
      press_mode mode = (press_mode)i2c_rx_data[6];
      display->simulate_touch(x_coord, y_coord, mode);
      break;
    }

    case I2C_WATCHDOG_RESET_CMD:
    {
      cli();  // Deshabilitar interrupciones
      MCUSR &= ~(1 << WDRF);  // Limpiar bandera de watchdog
      WDTCSR |= (1 << WDCE) | (1 << WDE);
      WDTCSR = (1 << WDE);  // Habilitar watchdog con timeout mínimo
      while(true) { }  // Esperar reset
    }
  }
}

void build_display()
{
    display_param_struct params;
    params.m_port = (Stream *)&Serial;
    params.m_timeout = 10;
    params.m_config = SERIAL_8N1;

    display_builder.set_device_parameters(&params);

    display = (Display *)display_builder.get_device();
}

void configure_wdt()
{
  cli();
  MCUSR &= ~(1 << WDRF);
  WDTCSR = 0x00;
  WDTCSR |= (1 << WDCE) | (1 << WDE);
  WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP0);
  sei();
}

void enable_tx_interrupt()
{
  UCSR0B |= (1 << TXCIE0);
}

ISR(WDT_vect)
{

}

#if defined(__AVR_ATMEGA328P__)
  ISR(USART_TX_vect)
#else
  ISR(USART0_TX_vect)
#endif
{
  WDTCSR |= (1 << WDIE);
}