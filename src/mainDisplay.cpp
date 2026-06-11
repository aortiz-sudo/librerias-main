/**
 * @file  mainDisplay.cpp
 * @brief Firmware del controlador de la pantalla DWIN (esclavo I2C).
 *
 * Recibe comandos por I2C desde el controlador principal mediante una cola circular
 * y los traduce en operaciones sobre la pantalla DWIN por UART.
 */

#include "mainDisplay.h"

Display *display;                   ///< Puntero al objeto de la pantalla.
Display_Builder display_builder;    ///< Constructor del objeto de la pantalla.

uint8_t current_state = 0;          ///< Estado actual de la pantalla.
uint8_t last_state = 0;             ///< Estado anterior de la pantalla.
uint8_t current_counter = 0;        ///< Contador actual del estado.
uint8_t last_counter = 0;           ///< Contador anterior del estado.
uint8_t rtc_data[7];                ///< Datos de fecha y hora.

/**
 * @def   CMD_QUEUE_SIZE
 * @brief Tamaño de la cola circular de comandos I2C para evitar pérdida de comandos.
 */
#define CMD_QUEUE_SIZE 4

/**
 * @struct i2c_cmd_t
 * @brief  Comando I2C almacenado en la cola circular.
 */
struct i2c_cmd_t {
    uint8_t data[I2C_BUFFER_LEN];   ///< Datos del comando.
    uint8_t length;                 ///< Longitud del comando.
};

volatile uint8_t cmd_queue_head = 0;    ///< Índice donde escribe el ISR.
volatile uint8_t cmd_queue_tail = 0;    ///< Índice donde lee el bucle principal.
i2c_cmd_t cmd_queue[CMD_QUEUE_SIZE];     ///< Cola circular de comandos I2C.

uint8_t state_pic_numbers[]
{
  INITIAL_SCREEN_PIC, LOG_IN_SCREEN_PIC, MAIN_MENU_NCAL_PIC, MAIN_MENU_WCAL_PIC, DISPATCH_MENU_PIC,
  PRINT_TICKET_MENU_PIC, CONFIGURATION_MENU_PIC, CALIBRATION_MENU_PIC, QUANTITY_DISPATCH_MENU_PIC,
  DISPATCHING_PRESS_TO_START_PIC, DATE_MENU_HOUR_PIC, PRODUCT_PRICE_PIC, TICKET_INFO_PIC, SCREEN_BRIGHTNESS_PIC
};

/** @brief Handler de recepción I2C. Encola el comando recibido. @param args Número de bytes recibidos. */
void I2C_RxHandler(int args);
/** @brief Ejecuta un comando I2C sobre la pantalla. @param p_data Buffer con el comando y sus datos. */
void execute_I2C_command(const uint8_t *p_data);
/** @brief Configura el watchdog timer del sistema. */
void configure_wdt();
/** @brief Habilita la interrupción de transmisión UART. */
void enable_tx_interrupt();
/** @brief Construye e inicializa el objeto de la pantalla. */
void build_display();

/**
 * @brief Función de configuración inicial de Arduino. Inicializa el I2C esclavo, la pantalla y el watchdog.
 */
void setup()
{
  configure_wdt();
  Wire.begin(I2C_SLAVE_ADDR);
  build_display();
  enable_tx_interrupt();
  Wire.onReceive(I2C_RxHandler);
}

/**
 * @brief Bucle principal: procesa los comandos I2C encolados y actualiza la pantalla.
 */
void loop()
{
  while(cmd_queue_tail != cmd_queue_head)
  {
    const uint8_t *cmd_data = cmd_queue[cmd_queue_tail].data;
    execute_I2C_command(cmd_data);
    cmd_queue_tail = (cmd_queue_tail + 1) % CMD_QUEUE_SIZE;
  }
}

void I2C_RxHandler(int p_length)
{
  uint8_t next_head = (cmd_queue_head + 1) % CMD_QUEUE_SIZE;

  if(next_head == cmd_queue_tail)
  {
    // Cola llena: descartar comando entrante
    while(Wire.available()) Wire.read();
    return;
  }

  uint8_t count = 0;
  while(Wire.available() && count < I2C_BUFFER_LEN)
  {
    cmd_queue[cmd_queue_head].data[count] = Wire.read();
    count++;
  }
  cmd_queue[cmd_queue_head].length = count;
  cmd_queue_head = next_head;
}

void execute_I2C_command(const uint8_t *p_data)
{
  switch(p_data[0])
  {
    case I2C_DISPLAY_CHANGE_PIC_CMD:
    {
      current_state = p_data[2];
      current_counter = p_data[3];

      if((last_counter != current_counter && last_state == current_state) || last_state != current_state)
      {
        last_state = current_state;
        last_counter = current_counter;
        display->set_pic(state_pic_numbers[current_state] + current_counter);
      }

      break;
    }

    case I2C_DISPLAY_CHANGE_BRIGHTNESS_CMD:
    {
      uint8_t brightness = p_data[2];

      display->set_led_config(brightness);
      break;
    }

    case I2C_DISPLAY_WRITE_TO_ADDRESS_CMD:
    {
      uint16_t address = (p_data[2] << 8) | p_data[3];
      uint8_t length = p_data[4];
      uint8_t data[length];

      for(int i = 0; i < length; i++)
        data[i] = p_data[i + 5];

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
      uint16_t x_coord = (p_data[2] << 8) | p_data[3];
      uint16_t y_coord = (p_data[4] << 8) | p_data[5];
      press_mode mode = (press_mode)p_data[6];
      display->simulate_touch(x_coord, y_coord, mode);
      break;
    }

    case I2C_WATCHDOG_RESET_CMD:
    {
      cli();
      MCUSR &= ~(1 << WDRF);
      WDTCSR |= (1 << WDCE) | (1 << WDE);
      WDTCSR = (1 << WDE);
      while(true) { }
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
  // WDP2|WDP1 = 1 s (interrupt) / 2 s (reset total).
  // Margen sobre los 500 ms de show_variables y operaciones puntuales.
  WDTCSR = (1 << WDE) | (1 << WDP2) | (1 << WDP1);
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