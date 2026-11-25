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

/*------ Log anterior: /data.log se renombra aquí al rotar -----*/
#define LOG_OLD_PATH "/data.old.log"

/**
 * \def   LOG_MAX_SIZE
 * \brief Tamaño en bytes a partir del cual /data.log se rota a /data.old.log. Entre los
 *        dos archivos el log de la SD ocupa como máximo unas 2 × LOG_MAX_SIZE.
 */
#define LOG_MAX_SIZE (256UL * 1024UL)

/**
 * \class   Logger
 * \brief   Envia logs al puerto serie o al archivo data.log en la tarjeta SD,
 *          dependiendo si esta en modo debug o producción.
 *
 * Dos niveles:
 * - log()/logln(): lo que sirve para diagnosticar en campo (errores, cambios de estado,
 *   resumen del arranque, despachos, actualizaciones). Va a la SD en release.
 * - debug()/debugln(): detalle de desarrollo (tramas, JSON, pantallas, cada ping...).
 *   Solo sale por el puerto serie en depuración; nunca se guarda en la SD.
 *
 * En la SD cada renglón lleva la fecha una sola vez, aunque se arme con varias llamadas
 * (log("Fuel: "); logln(nivel);). Si otra tarea escribe mientras un renglón está a
 * medias, ese renglón se cierra primero, así cada renglón es de una sola tarea.
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
         * \brief           Mensajes de detalle: mismos argumentos que log(), pero solo salen
         *                  por el puerto serie (depuración). En release no se guardan en la SD.
         */
        template<typename... Args>
        void debug(const Args&... p_args)
        {
            if(!this->m_use_file)
                log(p_args...);
        }

        /**
         * \brief           Igual que debug(), con salto de línea (mismos argumentos que logln()).
         */
        template<typename... Args>
        void debugln(const Args&... p_args)
        {
            if(!this->m_use_file)
                logln(p_args...);
        }

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
         * \brief           Silencia o restaura la salida del log al puerto serie (depuración).
         *                  El log en la SD no se silencia: es el historial del equipo.
         * \param p_muted   true: silenciar, false: restaurar.
         */
        void set_serial_muted(bool p_muted);

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
        /**
         * \brief           Escribe un fragmento del log: abre el archivo, cierra el renglón que
         *                  otra tarea haya dejado a medias, pone la fecha al inicio del renglón
         *                  y escribe los datos con print()/println().
         * \param p_newline true: termina el renglón (println).
         * \param p_args    Argumentos para print()/println().
         */
        template<typename... Args>
        void entry(bool p_newline, const Args&... p_args);

        /**
         * \brief   Abre /data.log para agregar y lo rota a /data.old.log si ya llegó a
         *          LOG_MAX_SIZE.
         * \returns true si el archivo quedó abierto.
         */
        bool open_file();

        Print *m_logger = nullptr; ///< Puntero al objeto Print utilizado para mostrar los datos.
        Print *m_unmuted_logger = nullptr; ///< Salida que se restaura al quitar el silencio.
        uint8_t *m_rtc[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr }; ///< Array de punteros para fecha y hora (año, mes, día, hora, minuto, segundo).
        File m_file_logger; ///< Archivo para guardar los logs en la SD.
        bool m_use_file = false; ///< Indica si se utiliza el archivo para guardar logs.
        bool m_at_line_start = true; ///< El siguiente fragmento empieza renglón (lleva fecha).
        TaskHandle_t m_line_owner = nullptr; ///< Tarea que escribió el último fragmento.
};

/**
 * \brief Objeto global Logger para registrar mensajes en consola o archivo.
 */
extern Logger logger;
#endif