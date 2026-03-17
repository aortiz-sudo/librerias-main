/**
 * \file    Peripehals.h
 * 
 * \brief   Este archivo contiene la definición de la clase Peripehals para el manejo de periféricos conectados por I2C, como impresoras y dispositivos satelitales.
 */

#ifndef _PERIPEHALS_H
#define _PERIPEHALS_H

#include "I2C_Master_Device.h"
#include "Definitions.h"

/**
 * \struct  peripehals_param_struct
 * \brief   Estructura de parámetros para inicializar el dispositivo Peripehals.
 * 
 * Hereda de i2c_param_struct y añade la dirección MAC de la impresora.
 */
struct peripehals_param_struct : i2c_param_struct
{
    /**
     * \brief Constructor que inicializa el tipo de dispositivo como I2C.
     */
    peripehals_param_struct()
    {
        this->m_type = I2C_DEVICE;
    }

    char m_mac_address[18]; ///< Dirección MAC del periférico.
};

/**
 * \class Peripehals
 * 
 * \brief Clase para el manejo de periféricos conectados por I2C.
 * 
 * Permite configurar el dispositivo, escribir la dirección MAC, leer datos de periféricos, y controlar la impresora.
 */
class Peripehals : public I2C_Master_Device
{
    public:
        /**
         * \brief Destructor.
         */
        ~Peripehals() { }

        /**
         * \brief           Establece los parámetros del dispositivo Peripehals.
         * \param p_params   Parámetros del dispositivo.
         */
        void set_device_parameters(const device_param_struct *p_params) override;

        /**
         * \brief           Obtiene los parámetros actuales del dispositivo Peripehals.
         * \param p_params  Estructura donde se guardarán los parámetros.
         */
        void get_device_parameters(device_param_struct *p_params) override;

        /**
         * \brief   Escribe la dirección MAC en el periférico.
         * \returns Estado de la operación.
         */
        status write_mac_address();

        /**
         * \brief                       Lee datos del periférico, como voltaje, estado satelital y conexión de impresora.
         * \param p_voltage             Puntero para guardar el voltaje leído.
         * \param p_sat_out             Puntero para guardar el estado satelital.
         * \param p_printer_connected   Puntero para guardar el estado de conexión de la impresora.
         * 
         * \returns                     Estado de la operación.
         */
        status read_peripehal_data(float *p_voltage, uint8_t *p_sat_out, uint8_t *p_printer_connected);

        /**
         * \brief           Escribe datos en la impresora conectada.
         * \param p_data    Buffer de datos a imprimir.
         * \param p_length  Longitud del buffer.
         * \returns         Estado de la operación.
         */
        status printer_write(uint8_t *p_data, size_t p_length);

        /**
         * \brief       Escribe una cadena de texto en la impresora conectada.
         * \param p_str Cadena de texto a imprimir.
         * \returns     Estado de la operación.
         */
        status printer_write(const char *p_str);

        /**
         * \brief           Imprime un número flotante en la impresora.
         * \param p_float   Número flotante a imprimir.
         * \returns         Estado de la operación.
         */
        status print_float(float p_float);

        /**
         * \brief       Imprime un número entero en la impresora.
         * \param p_int Número entero a imprimir.
         * \returns     Estado de la operación.
         */
        status print_int(int p_int);

        /**
         * \brief           Imprime un número entero sin signo en la impresora.
         * \param p_uint    Número entero sin signo a imprimir.
         * \returns         Estado de la operación.
         */
        status print_uint(uint32_t p_uint);

        /**
         * \brief   Reinicia la impresora conectada.
         * \returns Estado de la operación.
         */
        status reset_printer();

    private:
        char m_mac_address[18] = "";        ///< Dirección MAC del periférico.
        bool m_data_received = false;       ///< Indica si se recibieron datos del periférico.
        float m_voltage = 0.0;              ///< Voltaje leído del periférico.
        uint8_t m_sat_out = 1;              ///< Estado satelital del periférico.
        uint8_t m_printer_connected = 0;    ///< Estado de conexión de la impresora.
};

/**
 * \class Peripehals_Builder
 * 
 * \brief Clase constructora para dispositivos Peripehals.
 * 
 * Permite establecer los parámetros de configuración y obtener una instancia lista para su uso.
 */
class Peripehals_Builder : public I2C_Master_Device_Builder
{
    public:
        /**
         * \brief Destructor.
         */
        ~Peripehals_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo Peripehals.
         * \param p_params  Parámetros del dispositivo.
         */
        void set_device_parameters(device_param_struct *p_params) override
        {
            peripehals_param_struct *p = static_cast<peripehals_param_struct *>(p_params);
            
            this->m_peripehals_params.m_data_transmission_rate = p->m_data_transmission_rate;
            this->m_peripehals_params.m_scl_pin = p->m_scl_pin;
            this->m_peripehals_params.m_sda_pin = p->m_sda_pin;
            this->m_peripehals_params.m_type = p->m_type;
            this->m_peripehals_params.m_crc = p->m_crc;
            this->m_peripehals_params.m_port = p->m_port;
            this->m_peripehals_params.m_address = p->m_address;

            strcpy(this->m_peripehals_params.m_mac_address, p->m_mac_address);
        }

        /**
         * \brief Obtiene una instancia del dispositivo Peripehals construido.
         * \returns Puntero al dispositivo Peripehals.
         */
        virtual Device *get_device() override
        {
            this->m_peripehals_device.begin(&this->m_peripehals_params);

            return &this->m_peripehals_device;
        }

    protected:
        peripehals_param_struct m_peripehals_params; ///< Parámetros del dispositivo Peripehals.
        Peripehals m_peripehals_device;              ///< Instancia del dispositivo Peripehals.
};

#endif