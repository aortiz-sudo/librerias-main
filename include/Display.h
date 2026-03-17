/**
 * \file    Display.h
 * 
 * \brief   Este archivo contiene el dirver utilizado para controlar el display DWIN.
 */
#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "UART_Device.h"
#include "Definitions.h"

#define DISPLAY_BUFFER_LEN 64 //Tamaño total del buffer de datos.

/**
 * \struct display_param_struct
 * 
 * \brief Estructura de parámetros para inicializar el display DWIN.
 * 
 * Hereda de uart_param_struct y establece valores por defecto para la velocidad de transmisión y el tipo de CRC.
 */
struct display_param_struct : uart_param_struct
{
    /**
     * \brief Constructor que inicializa los parámetros por defecto del display.
     * 
     * - m_data_transmission_rate: 115200 baudios.
     * - m_crc: Sin CRC.
     */
    display_param_struct() 
    {
        this->m_data_transmission_rate = 115200;
        this->m_crc = NO_CRC;
    }
};

/**
 * \class Display
 * 
 * \brief Esta clase es utilizada para controlar las pantallas DWIN, que utilizan
 *        comunicación UART.
 */
class Display : public UART_Device
{
    public:
        ~Display() { }

        /**
         * \brief   Función para reiniciar el sistema de la pantalla.
         */
        void system_reset();

        /**
         * \brief               Función para establecer la velocidad de comunicación de la pantalla.
         * \param p_BAUD_RATE   Velocidad de transmisión en baudios.
         * \returns             Estado del comando enviado.
         */
        status set_baudrate(uint32_t p_BAUD_RATE);

        /**
         * \brief           Función para activar o desactivar el CRC en la pantalla.
         * \param p_set_crc Variable que activa o desactiva el CRC.
         * \returns         Estado del comando enviado.
         */
        status set_CRC(bool p_set_crc);

        /**
         * \brief       Función para cambiar el reloj RTC de la pantalla.      
         * \param p_rtc Buffer que contiene los datos del RTC. 
         * \returns     Estado del comando enviado.
         */
        status set_RTC(const uint8_t *p_rtc);

        /**
         * \brief   Función para obtener los datos del reloj RTC de la pantalla.
         * \returns Buffer con los datos del reloj RTC.
         */
        uint8_t *get_RTC();

        /**
         * \brief   Función para obtener los datos recibidos de la pantalla cuando se envía algún comando.
         * \returns Buffer con los datos recibidos.
         */
        uint8_t *get_data();

        /**
         * \brief   Función para obtener la imagen que se encuentra mostrando la pantalla actualmente.
         * \returns Número de la imagen en la que se encuentra.
         */
        uint8_t get_current_pic();

        /**
         * \brief   Función para obtener la resolución horizontal actual.
         * \returns La resolución horizontal.
         */
        uint16_t get_horizontal_resolution();

        /**
         * \brief   Función para obtener la resolución vertical actual.
         * \returns La resolución vertical.
         */
        uint16_t get_vertical_resolution();

        /**
         * \brief               Función para establecer el brillo actual de la pantalla.
         * \param p_brightness  Valor del brillo al que se quiere establecer.
         * \returns             Estado del comando enviado.
         */
        status set_led_config(uint8_t p_brightness);

        /**
         * \brief           Función para cambiar la imagen que se encuentra mostrando la pantalla.
         * \param p_pic_id  Número de la imagen a la que se quiere cambiar.
         * \returns         Estado del comando enviado.
         */
        status set_pic(uint8_t p_pic_id);

        /**
         * \brief               Función utilizada para simular touch.
         * \param p_x           Coordena en X en donde se quiere simular.
         * \param p_y           Coordena en Y en donde se quiere simular.
         * \param p_press_mode  Modo de presión.
         * \returns             Estado del comando enviado
         */
        status simulate_touch(uint16_t p_x, uint16_t p_y, press_mode p_press_mode);

        /**
         * \brief                   Función para cambiar el valor de una variable personalizada.
         * \param p_address         Dirección de la variable.
         * \param p_vlaue           Buffer con el valor de la variable.
         * \param p_value_length    Tamaño del buffer.
         * \returns                 Estado del comando enviado.
         */
        status set_user_variable(uint16_t p_address, uint8_t *p_value, size_t p_value_length);

    public:
        static const status CRC_NO_ACTIVE = 5; //< Error que indica que el CRC no se encuentra activo
    
    private:
        /**
         * \brief               Función para el calcular el CRC de los datos enviados y recibidos.
         * 
         * \param p_data        Buffer de datos al cual se le hará el cálculo.
         * \param p_data_length Tamaño del buffer.
         * \param p_check       Indica si es para revisar y el CRC es correcto.
         * 
         * \returns             El CRC calculado.
         */
        uint16_t crc_calculation(const uint8_t *p_data, const size_t p_data_length, const bool p_check = false) override;

        /**
         * \brief           Función para enviar un comando.
         * \param p_command Estructura del comando a enviar.
         * \returns         Estado del comando enviado.
         */
        status send_command(command_struct *p_command) override;
    
    private:
        uint8_t m_rtc_array[7] = { 0 };                 //< Buffer para guardar el RTC.
        uint8_t m_buffer[DISPLAY_BUFFER_LEN] = { 0 };   //< Buffer para guardar los datos recibidos.
        uint16_t m_crc_cmd = SET_CRC_FALSE_CMD; //< Bandera de control para el CRC.
};

/**
 * \class Display_Builder
 * 
 * \brief Clase constructora para objetos Display.
 * 
 * Permite establecer los parámetros de configuración del display DGUS y obtener una instancia lista para su uso.
 * Hereda de Device_Builder y debe usarse para crear y configurar objetos Display de manera segura y estructurada.
 */
class Display_Builder : public Device_Builder
{
    public:
        /**
         * \brief Destructor virtual.
         */
        ~Display_Builder() { }

        /**
         * \brief           Establece los parámetros del display.
         * \param p_params  Parámetros de configuración del display.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            display_param_struct *p = static_cast<display_param_struct *>(p_params);
            this->m_display_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_display_params.m_rx_pin = p->m_rx_pin;
            this->m_display_params.m_tx_pin = p->m_tx_pin;
            this->m_display_params.m_enable_pin = p->m_enable_pin;
            this->m_display_params.m_timeout = p->m_timeout;
            this->m_display_params.m_enable_state = p->m_enable_state;
            this->m_display_params.m_type = p->m_type;
            this->m_display_params.m_crc = p->m_crc;
            this->m_display_params.m_port = p->m_port;
            this->m_display_params.m_config = p->m_config;
        }

        /**
         * \brief   Obtiene una instancia del objeto Display configurado.
         * \returns Puntero al objeto Display.
         */
        Device *get_device() override
        {
            this->m_display.begin(&this->m_display_params);

            return &this->m_display;
        }
    private:
        display_param_struct m_display_params; ///< Parámetros de configuración del display.
        Display m_display;  ///< Instancia del objeto Display.
};

#endif