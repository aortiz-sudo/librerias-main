/**
 * \file    Screen.h
 * 
 * \brief   Este archivo contiene la definición de la clase Screen para el manejo de la pantalla principal del sistema por I2C.
 */

#ifndef _SCREEN_H
#define _SCREEN_H

#include "I2C_Master_Device.h"
#include "State.h"

/**
 * \def     OPENED_CABINET
 * \brief   Macro para obtener el estado de gabinete abierto.
 */
#define OPENED_CABINET this->m_device->get_cabinet_state()
/**
 * \def     CLOSED_CABINET
 * \brief   Macro para obtener el estado de gabinete cerrado.
 */
#define CLOSED_CABINET !this->m_device->get_cabinet_state()

/**
 * \struct  screen_param_struct
 * \brief   Estructura de parámetros para inicializar la pantalla principal.
 * 
 * Hereda de i2c_param_struct y añade el estado de pantalla.
 */
struct screen_param_struct : i2c_param_struct
{
    /**
     * \brief Constructor que inicializa el tipo de dispositivo como I2C.
     */
    screen_param_struct() 
    {
        this->m_type = I2C_DEVICE;
    }

    screen_state m_screen_state; ///< Estado actual de la pantalla.
};

/**
 * \class Screen
 * 
 * \brief Clase para el manejo de la pantalla principal del sistema por I2C.
 * 
 * Permite gestionar los estados de la pantalla, la transición entre ellos, el brillo, el producto, el precio y el estado del gabinete.
 */
class Screen : public I2C_Master_Device
{
    public:
        /**
         * \brief Destructor.
         */
        ~Screen() { }

        /**
         * \brief   Obtiene el último estado de pantalla.
         * \returns Estado de pantalla anterior.
         */
        inline screen_state get_last_screen_state() const
        {
            return this->m_last_screen_state;
        }

        /**
         * \brief   Obtiene el estado actual de la pantalla.
         * \returns Puntero al estado actual.
         */
        inline State *get_state() const
        {
            return this->m_current_state;
        }

        /**
         * \brief           Establece el estado actual de la pantalla.
         * \param p_state   Puntero al nuevo estado.
         */
        inline void set_state(const State *p_state)
        {
            this->m_current_state = (State *)p_state;
        }

        /**
         * \brief   Obtiene el estado de pantalla actual.
         * \returns Estado de pantalla actual.
         */
        inline screen_state get_screen_state() const
        {
            return this->m_current_screen_state;
        }

        /**
         * 
         * 
         */
        inline void set_sate_callback(state_callback_function p_function, screen_state p_state)
        {
            this->m_state[p_state]->set_state_callback_function(p_function);
        }
        /**
         * \brief   Indica si la pantalla está en modo despacho.
         * \returns true: Está despachando
         *          false: Caso contrario.
         */
        inline bool get_dispatching() const
        {
            return this->m_dispatching;
        }

        /**
         * \brief               Establece el modo despacho.
         * \param p_dispatching true: Para activar
         *                      false: Para desactivar.
         */
        inline void set_dispatching(const bool p_dispatching)
        {
            this->m_dispatching = p_dispatching;
        }

        /**
         * \brief   Indica si la pantalla está en modo calibración.
         * \returns true: Está calibrando
         *          false: Caso contrario.
         */
        inline bool get_calibrating() const
        {
            return this->m_calibrating;
        }

        /**
         * \brief               Establece el modo calibración.
         * \param p_calibrating true: Para activar,
         *                      false: Para desactivar.
         */
        inline void set_calibrating(const bool p_calibrating)
        {
            this->m_calibrating = p_calibrating;
        }

        /**
         * \brief       Establece si se ha detectado un tag RFID.
         * \param p_tag true: Se detectó.
         *              false: Caso contrario.
         */
        inline void set_tag_detected(const bool p_tag)
        {
            this->m_tag_detected = p_tag;
        }

        /**
         * \brief   Indica si se ha detectado un tag RFID.
         * \returns true: Si se detectó.
         *          false: En caso contrario.
         */
        inline bool get_tag_detected() const
        {
            return this->m_tag_detected;
        }

        /**
         * \brief           Establece el precio actual.
         * \param p_price   Precio a establecer.
         */
        inline void set_price(float p_price)
        {
            this->m_price = p_price;
        }

        /**
         * \brief   Obtiene el precio actual.
         * \returns Precio actual.
         */
        inline float get_price() const
        {
            return this->m_price;
        }

        /**
         * \brief   Obtiene el brillo actual de la pantalla.
         * \returns Valor de brillo.
         */
        inline uint8_t get_brightness() const
        {
            return this->m_brightness;
        }

        /**
         * \brief           Establece el producto actual.
         * \param p_product Producto a establecer.
         */
        inline void set_product(const char* p_product)
        {
            this->m_product = (char *)p_product;
        }

        /**
         * \brief   Obtiene el producto actual.
         * \returns Producto actual.
         */
        inline const char *get_product() const
        {
            return (const char *)this->m_product;
        }

        /**
         * \brief   Obtiene el contador del estado actual.
         * \returns Valor del contador.
         */
        inline uint8_t get_state_counter() const
        {
            return this->m_current_state->get_counter();
        }

        /**
         * \brief         Establece el estado del gabinete.
         * \param p_state true: Está abierto.
         *                false: Está cerrado.
         */
        inline void set_cabinet_state(bool p_state)
        {
            this->m_cabinet_state = p_state;
        }

        /**
         * \brief   Obtiene el estado del gabinete.
         * \returns true: Está abierto.
         *                false: Está cerrado.
         */
        inline bool get_cabinet_state() const
        {
            return this->m_cabinet_state;
        }

        /**
         * \brief   Obtiene el estado del pin de calibración.
         * \returns true: Está activo. 
         *          false: Caso contrario.
         */
        inline bool get_calibration_pin_state()
        {
            return digitalRead(CALIBRATION_PIN);
        }

        /**
         * \brief   Cambia la imagen mostrada en la pantalla.
         * \returns Estado de la operación.
         */
        status change_pic();

        /**
         * \brief               Establece el brillo de la pantalla.
         * \param p_brightness  Valor de brillo.
         * \param p_save        true: Guarda el valor. false: No lo guarda.
         * \returns             Estado de la operación.
         */
        status set_brightness(uint8_t p_brightness, bool p_save = false);

        /**
         * \brief                   Escribe datos en una dirección específica de la pantalla.
         * \param p_address         Dirección de memoria.
         * \param p_data            Buffer de datos.
         * \param p_length          Longitud del buffer.
         * \param p_little_endian   true si los datos están en little endian.
         * \returns                 Estado de la operación.
         */
        status write_to_address(const uint16_t p_address, const uint8_t *p_data, const size_t p_length, bool p_little_endian = false);

        /**
         * \brief   Reinicia el sistema de la pantalla.
         * \returns Estado de la operación.
         */
        status system_reset();

        /**
         * \brief           Simula un toque en la pantalla.
         * \param p_x       Coordenada X.
         * \param p_y       Coordenada Y.
         * \param p_mode    Modo de presión.
         * \returns         Estado de la operación.
         */
        status display_touch(uint16_t p_x, uint16_t p_y, press_mode p_mode);

        /**
         * \brief Realiza la transición entre estados de pantalla.
         */
        void transition();

        /**
         * \brief                   Establece la transición entre estados.
         * \param p_transition      true para activar la transición.
         * \param p_screen_state    Estado de pantalla destino.
         */
        void set_transition(bool p_transition, screen_state p_screen_state = INITIAL_SCREEN_STATE);

        /**
         * \brief       Actualiza el estado de la pantalla según la tecla presionada.
         * \param p_key Tecla presionada.
         * \returns     Estado de la operación.
         */
        status update(char p_key);

        /**
         * \brief               Inicializa la pantalla con el brillo especificado.
         * \param p_brightness  Valor de brillo inicial.
         */
        void init(uint8_t p_brightness);

        /**
         * \brief           Escribe un valor entero en una dirección específica de la pantalla.
         * \tparam          INT Tipo de dato entero.
         * \param p_address Dirección de memoria.
         * \param p_data    Valor a escribir.
         * \param p_key     Tecla presionada (opcional).
         * \returns         Valor escrito.
         */
        template <typename INT>
        INT write_int_value(uint16_t p_address, INT p_data, char p_key  = '\0')
        {
            if(p_key)
            {
                if(p_key != CLEAR)
                    p_data = (10 * p_data) + (p_key - '0');
                else if(p_key == CLEAR)
                    p_data = (INT)(p_data / 10);
            }
            else
                p_data = 0;

            write_to_address(p_address, (uint8_t *)&p_data, sizeof(p_data), true);

            return p_data;
        }
    
    private:
        initial_state m_initial_state;                      ///< Estado inicial.
        login_state m_login_state;                          ///< Estado de inicio de sesión.
        menu_state m_menu_state;                            ///< Estado de menú principal.
        menu_cal_state m_menu_cal_state;                    ///< Estado de menú con calibración.
        dispatch_menu_state m_dispatch_menu_state;          ///< Estado de menú de despacho.
        reprint_menu_state m_reprint_menu_state;            ///< Estado de menú de reimpresión.
        config_menu_state m_config_menu_state;              ///< Estado de menú de configuración.
        calibration_state m_calibration_state;              ///< Estado de calibración.
        quantity_dispatch_state m_quantity_dispatch_state;  ///< Estado de despacho por cantidad.
        dispatch_state m_dispatch_state;                    ///< Estado de despacho.
        date_and_hour_state m_date_and_hour_state;          ///< Estado de fecha y hora.
        price_state m_price_state;                          ///< Estado de precio.
        ticket_info_state m_ticket_info_state;              ///< Estado de información de ticket.
        brightness_state m_brightness_state;                ///< Estado de brillo.

        State *m_current_state = nullptr;             ///< Puntero al estado actual.

        State *m_state[14] = 
        { 
            &this->m_initial_state, &this->m_login_state, &this->m_menu_state, &this->m_menu_cal_state,
            &this->m_dispatch_menu_state, &this->m_reprint_menu_state, &this->m_config_menu_state,
            &this->m_calibration_state, &this->m_quantity_dispatch_state, &this->m_dispatch_state, 
            &this->m_date_and_hour_state, &this->m_price_state, &this->m_ticket_info_state,
            &this->m_brightness_state
        }; ///< Arreglo de punteros a los estados de pantalla.

        screen_state m_last_screen_state = INITIAL_SCREEN_STATE;             ///< Estado de pantalla anterior.
        screen_state m_current_screen_state = INITIAL_SCREEN_STATE;;          ///< Estado de pantalla actual.
        screen_state m_next_screen_state = INITIAL_SCREEN_STATE;;             ///< Siguiente estado de pantalla.

        bool m_active_transition = false;             ///< Indica si hay una transición activa.
        uint8_t m_transition_counter = 0;             ///< Contador de transición.

        bool m_initialized = false;                   ///< Indica si la pantalla está inicializada.
        bool m_calibrating = false;                   ///< Indica si está en modo calibración.
        bool m_dispatching = false;                   ///< Indica si está en modo despacho.
        bool m_tag_detected = false;                  ///< Indica si se detectó un tag RFID.
        bool m_cabinet_state = false;                 ///< Estado del gabinete.
        float m_price = 0.0;                          ///< Precio actual.
        uint8_t m_brightness = 100;                   ///< Brillo actual.
        char* m_product = nullptr;                    ///< Producto actual.
};

/**
 * \class Screen_Builder
 * 
 * \brief Clase constructora para dispositivos Screen.
 * 
 * Permite establecer los parámetros de configuración y obtener una instancia lista para su uso.
 */
class Screen_Builder : public I2C_Master_Device_Builder
{
    public:
        /**
         * \brief Destructor.
         */
        ~Screen_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo Screen.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            screen_param_struct *p = static_cast<screen_param_struct *>(p_params);
            this->m_screen_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_screen_params.m_sda_pin = p->m_sda_pin;
            this->m_screen_params.m_scl_pin = p->m_scl_pin;
            this->m_screen_params.m_type = p->m_type;
            this->m_screen_params.m_crc = p->m_crc;
            this->m_screen_params.m_port = p->m_port;
            this->m_screen_params.m_address = p->m_address;
        }

        /**
         * \brief   Obtiene una instancia del dispositivo Screen construido.
         * \returns Puntero al dispositivo Screen.
         */
        Device *get_device() override
        {
            this->m_screen_device.begin(&this->m_screen_params);

            return &this->m_screen_device;
        }
    private:
        screen_param_struct m_screen_params; ///< Parámetros del dispositivo Screen.
        Screen m_screen_device;              ///< Instancia del dispositivo Screen.
};

#endif