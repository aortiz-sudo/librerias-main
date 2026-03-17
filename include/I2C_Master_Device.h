/**
 * \file    I2C_Master_Device.h
 * 
 * \brief   Este archivo contiene la definición de la clase I2C_Master_Device para el manejo de dispositivos I2C, así como su clase constructora.
 */

#ifndef _I2C_MASTER_DEVICE_H
#define _I2C_MASTER_DEVICE_H

#include "Device.h"
#include <SD.h>

/**
 * \brief   Longitud máxima del buffer para operaciones I2C.
 */
#define MAX_BUFFER_LEN 48

/**
 * \struct i2c_param_struct
 * 
 * \brief  Estructura de parámetros para inicializar dispositivos I2C.
 * 
 * Hereda de device_param_struct y añade dirección y pines SCL/SDA.
 */
struct i2c_param_struct : device_param_struct
{
    i2c_param_struct() { this->m_type = I2C_DEVICE; }
    uint8_t m_address;      ///< Dirección I2C del dispositivo.
    int8_t m_scl_pin = -1;  ///< Pin SCL (opcional).
    int8_t m_sda_pin = -1;  ///< Pin SDA (opcional).
};

/**
 * \class   I2C_Master_Device
 * 
 * \brief   Clase base para dispositivos maestros I2C.
 * 
 * Proporciona métodos para inicializar, leer y escribir datos, y enviar comandos por I2C.
 */
class I2C_Master_Device : public Device
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~I2C_Master_Device() { }

        /**
         * \brief           Inicializa el dispositivo con los parámetros especificados.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void begin(const device_param_struct *p_params) override;

        /**
         * \brief           Establece los parámetros del dispositivo.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void set_device_parameters(const device_param_struct *p_params) override;

        /**
         * \brief           Obtiene los parámetros actuales del dispositivo.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void get_device_parameters(device_param_struct *p_params) override;

        /**
         * \brief               Escribe datos en el dispositivo I2C.
         * \param p_data        Puntero a los datos.
         * \param p_data_length Longitud de los datos.
         * \returns             Número de bytes escritos.
         */
        virtual size_t write_data(uint8_t *p_data, size_t p_data_length) override;

        /**
         * \brief                   Lee datos del dispositivo I2C.
         * \param p_buffer          Buffer de destino.
         * \param p_buffer_length   Longitud máxima a leer.
         * \returns                 Número de bytes leídos.
         */
        virtual size_t read_data(uint8_t *p_buffer, size_t p_buffer_length) override;

        /**
         * \brief               Quemar un nuevo firmware al dispositivo I2C.
         * \param p_file        Puntero al archivo del firmware.
         * \param p_file_size   Tamaño del archivo.
         */
        status burn_firmware(Stream *p_file, size_t p_file_size);

        /**
         * \brief   Verificiar si el dispositivo I2C esta listo para recibir datos.
         * 
         * \return  true: El dispositivo esta listo.
         *          false: El dispositivo no esta listo.
         */
        bool device_ready();

        /**
         * \brief   Indicarle al dispositivo I2C que inicialice la aplicación principal.
         * 
         * \return  El estado de la operación.
         */
        status start_device();

        /**
         * \brief   Reinicia el dispositivo.
         * \returns El estado de la operación.
         */
        status reset_device();

        /**
         * \brief   Obtener la dirección I2C del dispositivo.
         * \return  La dirección I2C.
         */
        inline uint8_t get_address() const
        {
            return this->m_address;
        }

        /**
         * \brief           Establecer la direccion I2C del dispositivo.
         * \param p_address La direccion ha utilizar.
         */
        inline void set_address(uint8_t p_address)
        {
            this->m_address = p_address;
        }
    
        /**
         * \brief   Calcular el CRC del firmware.
         * \param p_file        Puntero al archivo del firmware.
         * \param p_file_size   Tamaño del archivo.
         * \return              El valor CRC calculado.
         */
        uint16_t calculate_firmware_crc(File *p_file, size_t p_file_size);
        
    public:
        static const status NACK_ERROR      = 6; ///< Error de NACK en comunicación I2C.
        static const status NOT_SLAVE_ERROR = 7; ///< Error si el dispositivo no responde como esclavo.

    protected:
        static TwoWire *m_i2c_port;   ///< Puerto I2C utilizado.
        static bool m_port_ready;       ///< Indica si el puerto está listo.
        static int8_t m_scl_pin;           ///< Pin SCL configurado.
        static int8_t m_sda_pin;           ///< Pin SDA configurado.
        uint8_t m_address = 0;                  ///< Dirección I2C del dispositivo.

    protected:
        /**
         * \brief           Envía un comando al dispositivo I2C.
         * \param p_command Comando a enviar.
         * \returns         Estado de la operación.
         */
        virtual status send_command(command_struct *p_command) override;
};

/**
 * \class   I2C_Master_Device_Builder
 * 
 * \brief   Clase abstracta para construir dispositivos I2C.
 * 
 * Proporciona una interfaz para establecer los parámetros y obtener una instancia del dispositivo.
 */
class I2C_Master_Device_Builder : public Device_Builder
{
    public:
        /**
         * \brief Destructor virtual.
         */
        virtual ~I2C_Master_Device_Builder() { }

        /**
         * \brief           Establece los parámetros del dispositivo I2C.
         * \param p_params  Parámetros del dispositivo.
         */
        virtual void set_device_parameters(device_param_struct *p_params) = 0;

        /**
         * \brief   Obtiene una instancia del dispositivo construido.
         * \returns Puntero al objeto Device.
         */
        virtual Device *get_device() = 0;
};

#endif