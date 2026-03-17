/**
 * \file    Printer.h
 * 
 * \brief   Este archivo contiene la definición de la clase Printer para el manejo de impresoras térmicas conectadas por I2C.
 */

#ifndef _PRINTER_H
#define _PRINTER_H

#include "Peripehals.h"
#include "Definitions.h"

/**
 * \def     PRINTER_BUFFER_LEN
 * \brief   Tamaño del buffer para datos de la impresora.
 */
#define PRINTER_BUFFER_LEN 16

/**
 * \def     LF
 * \brief   Código ASCII para salto de línea (Line Feed).
 */
#define LF  (uint8_t)0x0A
/**
 * \def     CR
 * \brief   Código ASCII para retorno de carro (Carriage Return).
 */
#define CR  (uint8_t)0x0D
/**
 * \def     HT
 * \brief   Código ASCII para tabulación horizontal (Horizontal Tab).
 */
#define HT  (uint8_t)0x09
/**
 * \def     GS
 * \brief   Código ASCII para Group Separator.
 */
#define GS  (uint8_t)0x1D
/**
 * \def     DLE
 * \brief   Código ASCII para Data Link Escape.
 */
#define DLE (uint8_t)0x10
/**
 * \def     EOT
 * \brief   Código ASCII para End of Transmission.
 */
#define EOT (uint8_t)0x04
/**
 * \def     ESC
 * \brief   Código ASCII para Escape.
 */
#define ESC (uint8_t)0x1B
/**
 * 
 */
#define FS  (uint8_t)0x1C

/**
 * \enum    printer_charset
 * \brief   Conjuntos de caracteres soportados por la impresora.
 */
typedef enum : uint8_t
{
    USA,
    FRANCE,
    GERMANY,
    UK,
    DENMARK_I,
    SWEDEN,
    ITALY,
    SPAIN_I,
    JAPAN,
    NORWAY,
    DENMARK_II,
    SPAIN_II,
    LATIN_AMERICA,
    KOREA,
    SLOVENIA_CROATIA,
    CHINA,
    VIETNAM,
    ARABIA
} printer_charset;

/**
 * \enum    justification
 * \brief   Tipos de justificación de texto soportados por la impresora.
 */
typedef enum : uint8_t
{
    LEFT,    ///< Justificación a la izquierda.
    CENTER,  ///< Justificación centrada.
    RIGHT    ///< Justificación a la derecha.
} justification;

/**
 * \enum    char_size
 * \brief   Tamaños de caracteres soportados por la impresora.
 */
typedef enum : uint8_t
{
    NORMAL,
    _1X = 0,
    DOUBLE,
    _2X = 1,
    _3X,
    _4X,
    _5X,
    _6X,
    _7X,
    _8X
} char_size;

/**
 * \class Printer
 * 
 * \brief Clase para el manejo de impresoras térmicas conectadas por I2C.
 * 
 * Permite imprimir tickets, configurar el formato de impresión, cortar papel, ajustar márgenes y otros efectos de texto.
 */
class Printer : public Peripehals
{
    public:
        /**
         * \brief Destructor.
         */
        ~Printer() { }

        /**
         * \brief           Imprime el último ticket almacenado.
         * \param p_ticket  Estructura con los datos del ticket.
         */
        void print_last_ticket(ticket_struct *p_ticket);

        /**
         * \brief               Imprime el ticket del dia.
         * \param p_tickets     Arreglo de tickets.
         * \param p_services    Cantidad de servicios/tickets.
         */
        void print_day_ticket(ticket_struct *p_tickets, size_t p_services);

        /**
         * \brief Inicializa la impresora.
         */
        void init_printer();

        /**
         * \brief           Establece el conjunto de caracteres de la impresora.
         * \param p_charset Conjunto de caracteres.
         */
        void set_charset(printer_charset p_charset);

        /**
         * \brief Imprime un salto de línea.
         */
        void print_line_feed();

        /**
         * \brief   Imprime varios saltos de línea.
         * \param n Número de saltos de línea.
         */
        void print_line_feed(uint8_t n);

        /**
         * \brief Imprime una tabulación horizontal.
         */
        void print_horizontal_tab();

        /**
         * \brief Realiza un corte parcial del papel.
         */
        void partial_cut();

        /**
         * \brief Realiza un corte total del papel.
         */
        void full_cut();

        /**
         * \brief   Establece la justificación del texto.
         * \param n Tipo de justificación.
         */
        void set_justification(justification n);

        /**
         * \brief Establece el modo de impresión para papel de 80mm.
         */
        void set_80mm();

        /**
         * \brief Establece el modo de impresión para papel de 54mm.
         */
        void set_54mm();

        /**
         * \brief   Establece el ancho del área de impresión.
         * \param n Ancho en puntos.
         */
        void set_area_width(uint16_t n);

        /**
         * \brief Establece el espaciado de línea por defecto.
         */
        void set_default_line_spacing();

        /**
         * \brief   Establece el espaciado de línea personalizado.
         * \param n Espaciado en puntos.
         */
        void set_line_spacing(uint8_t n);

        /**
         * \brief           Establece el ancho de los caracteres.
         * \param p_width   Tamaño de ancho.
         */
        void set_char_width(char_size p_width);

        /**
         * \brief           Establece la altura de los caracteres.
         * \param p_height  Tamaño de altura.
         */
        void set_char_height(char_size p_height);

        /**
         * \brief Activa el modo de doble ancho de caracteres.
         */
        void set_double_width();

        /**
         * \brief Activa el modo de doble altura de caracteres.
         */
        void set_double_height();

        /**
         * \brief Activa el modo negrita.
         */
        void set_bold();

        /**
         * \brief Activa el subrayado.
         */
        void set_underline();

        /**
         * \brief Desactiva todos los efectos de texto.
         */
        void no_effects();

        /**
         * \brief   Establece el margen izquierdo.
         * \param n Margen en puntos.
         */
        void set_left_margin(uint16_t n);

        /**
         * 
         */
        void cancel_chinese_mode(); 
    public:
        static const status PRINTER_DISCONNECTED = 6; ///< Estado para impresora desconectada.
};

/**
 * \class Printer_Builder
 * 
 * \brief Clase constructora para dispositivos Printer.
 * 
 * Permite obtener una instancia lista para su uso y configurada con los parámetros de Peripehals.
 */
class Printer_Builder : public Peripehals_Builder
{
    public:
        /**
         * \brief Destructor.
         */
        ~Printer_Builder() { }

        /**
         * \brief   Obtiene una instancia del dispositivo Printer construido.
         * \returns Puntero al dispositivo Printer.
         */
        Device *get_device() override
        {
            this->m_printer.begin(&this->m_peripehals_params);

            return &this->m_printer;
        }
    private:
        Printer m_printer; ///< Instancia del dispositivo Printer.
};

#endif