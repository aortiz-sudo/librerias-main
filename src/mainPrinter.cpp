#include "mainprinter.h"

//HC05 hc05;

//char printer_mac_address[18] = "";
//bool mac_address = false;

volatile uint8_t i2c_rx_data[I2C_BUFFER_LEN];
volatile bool i2c_data_received = false;
volatile uint8_t i2c_rx_counter = 0;
float voltage = 0.0;
uint8_t sat_out = 0;
uint8_t printer_connected = 0;
bool executed = false;

void I2C_RxHandler(int length);
void I2C_TxHandler();
float get_voltage();
float get_vcc();
float get_avg_analog_read(const uint8_t p_pin, int p_times);
//void build_hc05();
void execute_i2c_command(uint8_t p_command);
void assign_data(uint8_t *p_data, size_t p_length);
void received();
//void print_ticket(bool p_day_ticket);
void configure_wdt();

void setup() 
{
  configure_wdt();

  Serial1.begin(9600);
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onReceive(I2C_RxHandler);
  Wire.onRequest(I2C_TxHandler);

  //build_hc05();

  pinMode(sat_out_pin, INPUT);
}

void loop() 
{
  received();

  voltage = get_voltage();
  sat_out = digitalRead(sat_out_pin);
  printer_connected = true;
}

void received()
{
  if(i2c_data_received)
  {
    uint8_t command = i2c_rx_data[0];
    execute_i2c_command(command);
    i2c_data_received = false;
    i2c_rx_counter = 0;
  }
}

void I2C_RxHandler(int length)
{
  while(Wire.available())
  {
    i2c_rx_data[i2c_rx_counter] = Wire.read();
    i2c_rx_counter++;
  }

  WDTCSR |= (1 << WDIE);
  i2c_data_received = true;
}

void I2C_TxHandler()
{
  uint8_t *voltage_ptr = (uint8_t *)&voltage;

  for(size_t i = 0; i < 4; i++)
    Wire.write(*voltage_ptr++);

  Wire.write(sat_out);
  Wire.write(printer_connected);
  Wire.write(executed);

  WDTCSR |= (1 << WDIE);
  executed = false;
}

/*void build_hc05()
{
  hc05_param_struct params;
  params.m_data_transmission_rate = 38400;
  params.m_port = (Stream *)&Serial;
  params.m_state_pin = state_pin;
  params.m_enable_pin = enable_at_pin;
  params.m_enable_state = LOW;
  params.m_timeout = 0;
  params.m_enable_delay = 1000;

  hc05.begin(&params);
}*/

void execute_i2c_command(uint8_t p_command)
{
  switch (p_command)
  {
    case I2C_PRINTER_MAC_ADDRESS_CMD:
    {
      /*assign_data((uint8_t *)printer_mac_address, i2c_rx_counter - 2);

      if(!printer_connected)
        hc05.pair(printer_mac_address, 20);

      mac_address = true;*/
      executed = true;
      break;
    }

    case I2C_PRINTER_PRINTER_PRINT_CMD:
    {
      size_t length = i2c_rx_data[1];
      uint8_t data[length];

      for(size_t i = 0; i < length; i++)
        data[i] = i2c_rx_data[i + 2];

      Serial1.write(data, length);
      executed = true;
      break;
    }

    case I2C_PRINTER_RESET_CMD:
    {
      //hc05.reset();

      executed = true;
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

void assign_data(uint8_t *p_parameter, size_t p_length)
{
  for(size_t i = 0; i < p_length; i++)
    p_parameter[i] = i2c_rx_data[i + 2];
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

float get_voltage()
{
  float vcc = 0.0;
  float adc = 0.0;

  for(int i = 0; i < 5; i++)
    vcc += get_vcc();

  vcc /= 5.0;

  adc = get_avg_analog_read(battery_pin, 5);

  float factor = (R1 + R2) / R2;

  WDTCSR |= (1 << WDIE);

  return ((vcc / 1023.0) * adc) * factor;
}

float get_avg_analog_read(const uint8_t p_pin, int p_times)
{
  float analog_read = 0.0;
  analogReference(DEFAULT);

  for(int i = 0; i < p_times; i++)
    analog_read += analogRead(p_pin);

  return analog_read/(float)p_times;
}

float get_vcc()
{
  //Seleccionar canal interno de 1.1V y la referencia de voltaje a la externa
  ADMUX = ((0x01 << MUX3) | (0x01 << MUX2) | (0x01 << MUX1)) & ~(0x01 << MUX0) & ~(0x01 << REFS1) | (0x01 << REFS0);

  delayMicroseconds(350);

  //Empezar la conversión del canal interno
  ADCSRA |= (1 << ADSC);

  //Eperar a que la conversión este lista
  while((ADCSRA >> ADSC) & 0x01);

  //Obtener el valor del voltaje de alimentación
  float vcc = (1.1 * 1023.0)/ADC;

  //Retornar el valor del voltaje con corrección
  return vcc;
}

ISR(WDT_vect)
{

}