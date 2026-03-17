/**
 * \file    Satellite.h
 * 
 * \brief   Este archivo contiene la definición de la clase Satellite para el manejo de dispositivos satelitales por UART.
 */

#ifndef _SATELLITE_H
#define _SATELLITE_H

#include <Arduino.h>
#include "UART_Device.h"
#include "Global_Client.h"

/**
 * \class Satellite
 * 
 * \brief Clase base abstracta para dispositivos satelitales que combinan comunicación UART.
 * 
 * Hereda de UART_Device y Global_Client, y define la interfaz para enviar y recibir datos, obtener el ID y reiniciar el dispositivo.
 */
class Satellite : public UART_Device, Global_Client
{
    public:
        /**
         * \brief Destructor virtual.
         */
        ~Satellite() { }

        /**
         * \brief               Envía datos al servidor.
         * 
         * \param p_data        Buffer con los datos a enviar.
         * \param p_data_length Longitud de los datos.
         * \returns             Código de estado o cantidad de datos enviados.
         */
        virtual int send_data_to_server(const uint8_t *p_data, size_t p_data_length) = 0;

        /**
         * \brief                   Obtiene datos del servidor.
         * 
         * \param p_data            Buffer donde se almacenarán los datos recibidos.
         * \param p_buffer_length   Longitud máxima del buffer.
         * \returns                 Código de estado o cantidad de datos recibidos.
         */
        virtual int get_data_from_server(uint8_t *p_data, size_t p_buffer_length) = 0;

        /**
         * \brief               Obtiene el ID del dispositivo satelital.
         * 
         * \param p_id_buffer   Buffer donde se almacenará el ID.
         * \returns             Estado de la operación.
         */
        virtual status get_id(uint8_t *p_id_buffer) = 0;

        /**
         * \brief Reinicia el dispositivo satelital.
         */
        virtual void reset_device() = 0;
    
    protected:
        /**
         * \brief           Envía un comando al dispositivo satelital.
         * 
         * \param p_command Estructura del comando.
         * \returns         Estado de la operación.
         */
        virtual status send_command(command_struct *p_command) = 0;

        /**
         * \brief           Calcula el CRC de los datos enviados y recibidos.
         * 
         * \param p_data    Buffer de datos.
         * \param p_length  Longitud del buffer.
         * \param p_check   Indica si se revisa el CRC.
         * \returns         CRC calculado.
         */
        virtual uint16_t crc_calculation(const uint8_t *p_data, const size_t p_length, const bool p_check = false) = 0;
};

#endif