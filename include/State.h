/**
 * \file    State.h
 * 
 * \brief   Este archivo contiene la definición de la clase base State y sus clases derivadas para la gestión de estados de pantalla en el sistema.
 */

#ifndef _STATE_H
#define _STATE_H

#include <Arduino.h>
#include "Definitions.h"

class Screen;

/*------------KEYPAD KEYS-------------*/
/**
 * \def     BACK
 * \brief   Tecla para regresar.
 */
#define BACK   'A'
/**
 * \def     CLEAR
 * \brief   Tecla para limpiar.
 */
#define CLEAR  'B'
/**
 * \def     ENTER
 * \brief   Tecla para aceptar.
 */
#define ENTER  'C'
/**
 * \def     DOWN
 * \brief   Tecla para bajar.
 */
#define DOWN   'D'
/**
 * \def     LOCK
 * \brief   Tecla para bloquear.
 */
#define LOCK   '*'
/**
 * \def     UP
 * \brief   Tecla para subir.
 */
#define UP     '#'

#ifndef CALIBRATION_PIN
    /**
     * \def     CALIBRATION_PIN
     * \brief   Pin utilizado para calibración.
     */
    #define CALIBRATION_PIN (int8_t)34
#endif

/**
 * \typedef state_callback_function
 * \brief Definición de tipo para funciones de estado.
 */
typedef void(*state_callback_function)(uint8_t *, int);

/**
 * \enum screen_state
 * \brief Enumeración de los diferentes estados de pantalla.
 */
typedef enum : uint8_t
{
    INITIAL_SCREEN_STATE,              ///< Estado inicial.
    LOG_IN_SCREEN_STATE,               ///< Estado de inicio de sesión.
    MENU_SCREEN_STATE,                 ///< Estado de menú principal.
    MENU_CAL_SCREEN_STATE,             ///< Estado de menú con calibración.
    DISPATCH_MENU_SCREEN_STATE,        ///< Estado de menú de despacho.
    REPRINT_MENU_SCREEN_STATE,         ///< Estado de menú de reimpresión.
    CONFIGURATION_MENU_SCREEN_STATE,   ///< Estado de menú de configuración.
    CALIBRATION_SCREEN_STATE,          ///< Estado de calibración.
    QUANTITY_DISPATCH_SCREEN_STATE,    ///< Estado de despacho por cantidad.
    DISPATCH_SCREEN_STATE,             ///< Estado de despacho.
    DATE_AND_HOUR_SCREEN_STATE,        ///< Estado de fecha y hora.
    PRICE_SCREEN_STATE,                ///< Estado de precio.
    TICKET_INFO_SCREEN_STATE,          ///< Estado de información de ticket.
    BRIGHTNESS_SCREEN_STATE,           ///< Estado de brillo.
} screen_state;

/**
 * \class State
 * 
 * \brief Clase base abstracta para la gestión de estados de pantalla.
 * 
 * Proporciona la interfaz y atributos comunes para todos los estados de pantalla.
 */
class State
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~State() { }

        /**
         * \brief Dibuja el estado en pantalla.
         */
        virtual void draw() = 0;

        /**
         * \brief       Maneja la interacción con la pantalla según la tecla presionada.
         * \param p_key Tecla presionada.
         */
        virtual void screen(char p_key) = 0;

        /**
         * \brief Reinicia el estado.
         */
        virtual void reset() = 0;

        /**
         * \brief           Asocia el dispositivo Screen al estado.
         * \param p_device  Puntero al objeto Screen.
         */
        inline void set_device(const Screen *p_device)
        {
            this->m_device = (Screen *)p_device;
        }

        /**
         * \brief           Establece el contador del estado.
         * \param p_counter Valor del contador.
         */
        inline void set_counter(const int p_counter)
        {
            this->m_counter = p_counter;
        }

        /**
         * \brief   Obtiene el valor actual del contador.
         * \returns Valor del contador.
         */
        inline int get_counter() const
        {
            return this->m_counter;
        }

        /**
         * \brief               Asocia una función de estado personalizada.
         * \param p_function    Función de estado.
         */
        inline void set_state_callback_function(const state_callback_function p_function)
        {
            this->m_state_callback_function = p_function;
        }

        /*
        template<typename T>
        using state_callback = void (*)(T, int);

        template<typename T>
        void set_callback(state_callback<T> p_fn)
        {
            callback_holder<T>::m_fn = p_fn;
        }

        template<typename T>
        void call_function(T p_data, int p_args)
        {
           auto fn = callback_holder<T>::m_fn;

           if(fn)
            fn(p_data, p_args);
        }
        */

        /**
         * \brief           Establece si el estado corresponde a un menú.
         * \param p_menu    true si es menú, false en caso contrario.
         */
        inline void set_menu(const bool p_menu)
        {
            this->m_menu = p_menu;
        }

        /**
         * \brief   Indica si el estado corresponde a un menú.
         * \returns true si es menú, false en caso contrario.
         */
        inline bool get_menu() const
        {
            return this->m_menu;
        }

        /**
         * \brief           Establece el precio actual.
         * \param p_price   Precio a establecer.
         */
        inline void set_price(const float p_price)
        {
            m_price = p_price;
        }

    protected:
        Screen *m_device = nullptr;        ///< Puntero al dispositivo Screen.
        uint8_t m_counter = 0;            ///< Contador actual.
        uint8_t m_max_counter = 0;        ///< Valor máximo del contador.

        /*template <typename T>
        struct callback_holder
        {
            static state_callback<T> m_fn;
        };*/

        state_callback_function m_state_callback_function = nullptr;   ///< Función de estado personalizada.
        bool m_menu = false;               ///< Indica si el estado es un menú.
        bool calibration = false;          ///< Indica si está en modo calibración.

        screen_state m_last_state = INITIAL_SCREEN_STATE;         ///< Estado anterior.
        screen_state m_next_state = INITIAL_SCREEN_STATE;         ///< Siguiente estado.
        static float m_price;              ///< Precio actual.
        static float m_volume;             ///< Volumen actual.
        static float m_dispatched_volume;  ///< Volumen despachado.
        static float m_dispatched_cost;    ///< Costo despachado.
        static bool m_quantity_dispatch;   ///< Indica si se despacha por cantidad.

        screen_state m_next_states[4] = { (screen_state)0 };     ///< Arreglo de posibles siguientes estados.
        const char *m_no_tag_message = "No se detecta tag"; ///< Mensaje de tag no detectado.
    
    protected:
        /**
         * \brief                   Incrementa el contador de estado.
         * \param p_current_counter Valor actual del contador.
         * \param p_max_counter     Valor máximo permitido.
         * \returns                 Nuevo valor del contador.
         */
        uint8_t increment_counter(uint8_t p_current_counter, uint8_t p_max_counter);

        /**
         * \brief                   Decrementa el contador de estado.
         * \param p_current_counter Valor actual del contador.
         * \param p_max_counter     Valor máximo permitido.
         * \returns                 Nuevo valor del contador.
         */
        uint8_t decrement_counter(uint8_t p_current_counter, uint8_t p_max_counter);

        /**
         * \brief Maneja teclas especiales en el estado.
         * \param p_key Tecla presionada.
         * \param p_counter Valor del contador.
         */
        void handle_special_key(char p_key, uint8_t p_counter = 1);
};

/**
 * \class initial_state
 * 
 * \brief Estado inicial de la pantalla.
 */
class initial_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado inicial y sus transiciones.
         */
        initial_state() 
        { 
            set_menu(false);
            this->m_max_counter = 0;
            this->m_last_state = INITIAL_SCREEN_STATE;
            this->m_next_states[0] = LOG_IN_SCREEN_STATE;
        }

        ~initial_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;
};
/**
 * \class login_state
 * 
 * \brief Estado de inicio de sesión.
 */
class login_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de login y sus transiciones.
         */
        login_state() 
        { 
            set_menu(false);
            this->m_max_counter = 4;
            this->m_last_state = INITIAL_SCREEN_STATE;
            this->m_next_states[0] = MENU_SCREEN_STATE;
        }

        ~login_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;
    
    private:
        char m_keypad_buffer[5] = {'\0', '\0', '\0', '\0', '\0'}; ///< Buffer para entrada de teclado.
        char m_password_text[5] = { ' ', ' ', ' ', ' ', '\0' };   ///< Buffer para mostrar contraseña.
        uint8_t m_digit_counter = 0;                              ///< Contador de dígitos ingresados.
};

/**
 * \class menu_state
 * 
 * \brief Estado de menú principal.
 */
class menu_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de menú principal y sus transiciones.
         */
        menu_state() 
        {  
            set_menu(true);
            this->m_max_counter = 3;
            this->m_last_state = LOG_IN_SCREEN_STATE;
            this->m_next_states[0] = DISPATCH_MENU_SCREEN_STATE;
            this->m_next_states[1] = REPRINT_MENU_SCREEN_STATE;
            this->m_next_states[2] = CONFIGURATION_MENU_SCREEN_STATE;
        }

        ~menu_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override; 
};

/**
 * \class menu_cal_state
 * 
 * \brief Estado de menú con opción de calibración.
 */
class menu_cal_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de menú con calibración y sus transiciones.
         */
        menu_cal_state() 
        {  
            set_menu(true);
            this->m_max_counter = 4;
            this->m_last_state = LOG_IN_SCREEN_STATE;
            this->m_next_states[0] = DISPATCH_MENU_SCREEN_STATE;
            this->m_next_states[1] = REPRINT_MENU_SCREEN_STATE;
            this->m_next_states[2] = CONFIGURATION_MENU_SCREEN_STATE;
            this->m_next_states[3] = CALIBRATION_SCREEN_STATE;
        }

        ~menu_cal_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override; 
};

/**
 * \class dispatch_menu_state
 * 
 * \brief Estado de menú de despacho.
 */
class dispatch_menu_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de menú de despacho y sus transiciones.
         */
        dispatch_menu_state() 
        { 
            set_menu(true);
            this->m_max_counter = 2;
            this->m_last_state = MENU_SCREEN_STATE;
            this->m_next_states[0] = QUANTITY_DISPATCH_SCREEN_STATE;
            this->m_next_states[1] = DISPATCH_SCREEN_STATE;
        }

        ~dispatch_menu_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;
        
};

/**
 * \class reprint_menu_state
 * 
 * \brief Estado de menú para reimpresión de tickets.
 */
class reprint_menu_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de menú de reimpresión.
         */
        reprint_menu_state() 
        { 
            set_menu(true);
            this->m_max_counter = 2;
            this->m_last_state = MENU_SCREEN_STATE;
        }

        ~reprint_menu_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;

    private:
        
};

/**
 * \class config_menu_state
 * 
 * \brief Estado de menú de configuración.
 */
class config_menu_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de menú de configuración y sus transiciones.
         */
        config_menu_state() 
        { 
            set_menu(true);
            this->m_max_counter = 4;
            this->m_last_state = MENU_SCREEN_STATE;
            this->m_next_states[0] = DATE_AND_HOUR_SCREEN_STATE;
            this->m_next_states[1] = PRICE_SCREEN_STATE;
            this->m_next_states[2] = TICKET_INFO_SCREEN_STATE;
            this->m_next_states[3] = BRIGHTNESS_SCREEN_STATE;
        }

        ~config_menu_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;
};

/**
 * \class calibration_state
 * 
 * \brief Estado de calibración.
 */
class calibration_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de calibración.
         */
        calibration_state() 
        { 
            set_menu(false);
            this->m_max_counter = 3;
            this->m_last_state = MENU_SCREEN_STATE;
        }

        ~calibration_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;

    private:
        uint8_t m_max_digit_counter = 8;      ///< Máximo de dígitos permitidos.
        uint8_t m_digit_counter = 0;          ///< Contador de dígitos ingresados.
        uint32_t m_int_standard_liters = 0;   ///< Litros estándar (entero).
        uint32_t m_int_measured_liters = 0;   ///< Litros medidos (entero).
        float m_float_standard_liters = 0.0f;  ///< Litros estándar (flotante).
        float m_float_measured_liters = 0.0f;  ///< Litros medidos (flotante).
};

/**
 * \class quantity_dispatch_state
 * 
 * \brief Estado para despacho por cantidad.
 */
class quantity_dispatch_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de despacho por cantidad y sus transiciones.
         */
        quantity_dispatch_state() 
        { 
            set_menu(true);
            this->m_max_counter = 2;
            this->m_last_state = DISPATCH_MENU_SCREEN_STATE;
            this->m_next_states[0] = DISPATCH_SCREEN_STATE;
        }

        ~quantity_dispatch_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;
    
    private:
        uint8_t m_max_digit_counter = 8;          ///< Máximo de dígitos permitidos.
        uint8_t m_digit_counter[2] = { 0 };    ///< Contadores de dígitos para volumen y costo.
        uint8_t m_current_index = 0;              ///< Índice actual.
        uint8_t m_other_index = 1;                ///< Índice alternativo.
        uint32_t m_dispatch_values[2] = { 0 }; ///< Valores de despacho (volumen y costo).
        uint32_t m_int_volume = 0;                ///< Volumen (entero).
        uint32_t m_int_cost = 0;                  ///< Costo (entero).
        uint16_t m_addresses[2] = { DISPLAY_VOLUME_DISPATCH_ADDRESS, DISPLAY_PRICE_DISPATCH_ADDRESS }; ///< Direcciones de memoria para volumen y precio.
};

/**
 * \class dispatch_state
 * 
 * \brief Estado de despacho.
 */
class dispatch_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de despacho.
         */
        dispatch_state() 
        { 
            set_menu(false);
            this->m_max_counter = 3;
            this->m_last_state = DISPATCH_MENU_SCREEN_STATE;
        }

        ~dispatch_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;

    private:
        bool dispatch_initialized = false;
        bool already_stop = false;
};

/**
 * \class date_and_hour_state
 * 
 * \brief Estado para configuración de fecha y hora.
 */
class date_and_hour_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de fecha y hora.
         */
        date_and_hour_state() 
        { 
            set_menu(true);
            this->m_max_counter = 4;
            this->m_last_state = CONFIGURATION_MENU_SCREEN_STATE;
        }

        ~date_and_hour_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;

    private:
        uint8_t m_max_digit_counter = 2;      ///< Máximo de dígitos permitidos.
        uint8_t m_digit_counter = 0;          ///< Contador de dígitos ingresados.
        uint8_t m_last_counter = 0;           ///< Último valor del contador.
        uint8_t m_rtc_info[5] = { 0} ;        ///< Información de fecha y hora.
        uint16_t m_rtc_info_addresses[5] = 
        { 
            DISPLAY_NEW_HOUR_ADDRESS, DISPLAY_NEW_MINUTE_ADDRESS, DISPLAY_NEW_DAY_ADDRESS,
            DISPLAY_NEW_MONTH_ADDRESS, DISPLAY_NEW_YEAR_ADDRESS 
        }; ///< Direcciones de memoria para fecha y hora.
};

/**
 * \class price_state
 * 
 * \brief Estado para configuración de precio.
 */
class price_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de precio.
         */
        price_state() 
        { 
            set_menu(false);
            this->m_max_counter = 0;
            this->m_last_state = CONFIGURATION_MENU_SCREEN_STATE;
        }

        ~price_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override; 
    
    private:
        uint8_t m_max_digit_counter = 4;  ///< Máximo de dígitos permitidos.
        uint8_t m_digit_counter = 0;      ///< Contador de dígitos ingresados.
        float m_new_price = 0.0f;          ///< Nuevo precio (flotante).
        uint16_t m_int_price = 0;         ///< Precio actual (entero).
        uint16_t m_int_new_price = 0;     ///< Nuevo precio (entero).
};

/**
 * \class ticket_info_state
 * 
 * \brief Estado para mostrar información del ticket.
 */
class ticket_info_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de información de ticket.
         */
        ticket_info_state() 
        { 
            set_menu(false);
            this->m_last_state = CONFIGURATION_MENU_SCREEN_STATE;
        }

        ~ticket_info_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override; 

    private:
        bool m_show = false; ///< Indica si se muestra la información.
};

/**
 * \class brightness_state
 * 
 * \brief Estado para configuración de brillo.
 */
class brightness_state : public State
{
    public:
        /**
         * \brief Constructor. Inicializa el estado de brillo.
         */
        brightness_state() 
        { 
            set_menu(false);
            this->m_max_counter = 10;
            this->m_last_state = CONFIGURATION_MENU_SCREEN_STATE;
        }

        ~brightness_state() { }
        void draw() override;
        void screen(char p_key) override;
        void reset() override;
    
    private:
        uint8_t m_brightness = 100; ///< Valor de brillo.
        uint16_t m_bar_value = 0;   ///< Valor de la barra de brillo.
        bool m_save = false;        ///< Indica si se guarda el valor.
};

#endif