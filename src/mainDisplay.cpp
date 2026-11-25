/**
 * @file  mainDisplay.cpp
 * @brief Firmware del controlador de la pantalla DWIN (esclavo I2C).
 *
 * Recibe comandos por I2C desde el controlador principal mediante una cola circular
 * y los traduce en operaciones sobre la pantalla DWIN por UART.
 */

#include "mainDisplay.h"
#include <avr/wdt.h>

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
 *
 * La deteccion de cola llena es next_head == tail, asi que la capacidad real es
 * CMD_QUEUE_SIZE - 1. Debe cubrir la rafaga que el ESP32 manda sin pausa en una
 * sola iteracion de Screen_Task: 1 escritura de RTC (solo en pantallas sin RTC
 * propio) + 3 de show_variables + 1 de display_touch + 1 de change_pic, mas una
 * de set_brightness al salir del atenuado = 7 comandos.
 *
 * Con 4 (capacidad 3) la rafaga desbordaba y I2C_RxHandler descartaba los
 * ultimos comandos. El mas perjudicado era change_pic, que va al final y no se
 * reintenta (Screen::update limpia m_pic_changed antes de enviarlo), de modo que
 * el cambio de pagina se perdia para siempre.
 */
#define CMD_QUEUE_SIZE 8

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
  Wire.onReceive(I2C_RxHandler);
}

/**
 * @brief Bucle principal: procesa los comandos I2C encolados y actualiza la pantalla.
 */
void loop()
{
  // El watchdog solo debe reiniciar si este ciclo se cuelga (ver configure_wdt()).
  // También se alimenta por comando: si el ESP32 llena la cola al ritmo al que se
  // vacía, el while no termina y la app sigue sana.
  wdt_reset();

  while(cmd_queue_tail != cmd_queue_head)
  {
    wdt_reset();
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

  // Una escritura sin datos (un sondeo de dirección) no es un comando. Antes se encolaba
  // igual y se ejecutaba con lo que quedara en ese lugar de la cola; 0x00 es el reinicio.
  if(count == 0)
    return;

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

// Watchdog de 1 s alimentado en loop(): reinicia solo si la app se cuelga.
// Antes no se llamaba nunca a wdt_reset(); lo "alimentaba" la interrupción de fin de
// transmisión del UART, así que la app se reiniciaba cada vez que pasaba ~1 s sin mandar
// nada a la pantalla (mientras el ESP32 arrancaba, flasheaba la impresora o la página no
// cambiaba). Con el bootloader, cada reinicio deja la pantalla ~1 s sin responder.
void configure_wdt()
{
  MCUSR &= ~(1 << WDRF);
  wdt_enable(WDTO_1S);
}