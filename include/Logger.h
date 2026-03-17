/**
 * \file    Logger.h
 * 
 * \brief   Contiene la clase Logger.
 */
#ifndef _LOGGER_H
#define _LOGGER_H

#include <Arduino.h>
#include "SD.h"

/*------ Direccion del archivo data.log -----*/
#define LOG_PATH "/data.log"

/**
 * \class   Logger
 * \brief   Envia logs al puerto serie o al archivo data.log en la tarjeta SD, 
 *          dependiendo si esta en modo debug o producción.
 */
class Logger : public Print
{
    public:
        ~Logger() { }

        /** 
         * \brief           Función para comenzar un objeto logger.
         * \param p_debug   Indica si el programa se encuentra en modo debug o producción.
        */
        void begin(bool p_debug = true);
        
        /**
         * \brief           Implementación de las función heredada de la clase Print para enviar datos.
         * \param p_data    Dato que se quiere enviar.
         * \returns         Cantidad de datos enviados.
         */
        size_t write(uint8_t p_data) override;

        /**
         * \brief           Implementación de las función heredada de la clase Print para enviar datos.
         * \param p_data    Buffer de datos que se quieren enviar.
         * \param p_length  Tamaño del buffer.
         * \returns         Cantidad de datos enviados.
         */
        size_t write(const uint8_t *p_data, size_t p_length) override;

        /**
         * \brief           Funciones utilizadas para la escritura de los datos que se quieren mostrar.
         * \param p_data    Datos que se quieren enviar, ya sean números enteros, flotantes, caracteres
         *                  o una cadena de ello.
         */
        void log(const __FlashStringHelper *ifsh); 
        void log(const char *p_data);
        void log(char p_data);
        void log(unsigned char p_data, int base = DEC);
        void log(int p_data, int base = DEC);
        void log(unsigned int p_data, int base = DEC);
        void log(long p_data, int base = DEC);
        void log(unsigned long p_data, int base = DEC);
        void log(long long p_data, int base = DEC);
        void log(unsigned long long p_data, int base = DEC);
        void log(double p_data, int dec = 2);
        void log(const Printable& p_data);

        /**
         * \brief           Funciones utilizadas para la escritura de los datos que se quieren mostrar con salto de línea.
         * \param p_data    Datos que se quieren enviar, ya sean números enteros, flotantes, caracteres
         *                  o una cadena de ellos.
         */
        void logln(const __FlashStringHelper *ifsh);
        void logln(const char *p_data);
        void logln(char p_data);
        void logln(unsigned char p_data, int base = DEC);
        void logln(int p_data, int base = DEC);
        void logln(unsigned int p_data, int base = DEC);
        void logln(long p_data, int base = DEC);
        void logln(unsigned long p_data, int base = DEC);
        void logln(long long p_data, int base = DEC);
        void logln(unsigned long long p_data, int base = DEC);
        void logln(double p_data, int dec = 2);
        void logln(const Printable& p_data);
        void logln(void);

        /**
         * \brief Función utilizada para escribir la fecha y hora en la que se enviaron los datos.
         */
        void log_date();

        /**
         * \brief           Función utilizada para establecer que se quiere utilizar para mostrar los datos.
         * \param p_logger  Objeto que se quiere utilizar para mostrar los datos.
         */
        inline void set_logger(Print *p_logger)
        {
            this->m_logger = p_logger;
        }

        /**
         * \brief       Función para establecer los punteros en donde se guardan los datos de la fecha y hora.
         * \param p_rtc Array de punteros en donde se guardan la fecha y hora.
         */
        inline void set_rtc(uint8_t *p_rtc[6])
        {
            for(int i = 0; i < 6; i++)
                this->m_rtc[i] = p_rtc[i];
        }

    private:
        Print *m_logger = nullptr; ///< Puntero al objeto Print utilizado para mostrar los datos.
        uint8_t *m_rtc[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }; ///< Array de punteros para fecha y hora (año, mes, día, hora, minuto, segundo).
        File m_file_logger; ///< Archivo para guardar los logs en la SD.
        bool m_use_file = false; ///< Indica si se utiliza el archivo para guardar logs.
};

/**
 * \brief Objeto global Logger para registrar mensajes en consola o archivo.
 */
extern Logger logger;
#endif