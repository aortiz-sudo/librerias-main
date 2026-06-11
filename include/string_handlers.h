/**
 * \file    string_handlers.h
 * 
 * \brief   Funciones utilizadas para la manipulación de cadenas de caracteres.
 */

#ifndef _STRING_HANDLERS_H
#define _STRING_HANDLERS_H

#include <Arduino.h>

/*-------Tamaño máximo de una cadena de caracteres-------*/
#define STRING_BUFFER 64

/** 
 * \brief                       Función para verificar que una cadena de caracteres contenga otra cadena dentro de ella.
 * 
 * \param p_string              Cadena de caracteres original.
 * \param p_string_contained    Cadena de caracteres a verificar.
 * 
 * \returns                     false: La cadena a verificar no se encuentra en la cadena origial.
 *                              true: La cadena a verificar si se encuentra en la cadena original.
*/
bool string_contains(const char *p_string, const char *p_string_contained);

/**
 * \brief                   Función para leer una cadena de caracteres hasta un carácter específico.
 * \param p_string          Cadena de caracteres de origen.
 * \param p_substring       Buffer donde se guardará la subcadena obtenida.
 * \param p_terminator      Carácter que delimita el final de la subcadena.
 * \param p_buffer_size     Tamaño máximo del buffer de destino.
 *
 * \returns                 Número de caracteres copiados en el buffer.
 */
size_t get_substring(const char *p_string, char *p_substring, char p_terminator, size_t p_buffer_size);

/** 
 * \brief                       Función para separar una cadena de caracteres en varias con un carácter de referencia.
 * 
 * \param p_src         Cadena de caracteres a separar.
 * \param p_dest        Buffer para guardar las cadenas de caracteres individuales.
 * \param p_dest_size   Tamaño del buffer.
 * \param p_delim       Carácter de referencia.
 * 
 * \returns                     Numero de cadena de caracteres obtenidos.
*/
size_t split_string(const char *p_src, char p_dest[][STRING_BUFFER], size_t p_dest_size, char p_delim);

/** 
 * \brief           Función para obtener el número de líneas en una cadena de caracteres.
 * 
 * \param p_string  Cadena de caracteres de la cual se quiere obtener el numero de lineas. 
 * 
 * \returns         Numero de líneas en la cadena de caracteres.
*/
size_t get_lines(const char *p_string);

/**
 * \brief           Función para obtener la primera posición en la que se encuentra un carácter en una cadena de caracteres.
 * 
 * \param p_string  Cadena de caracteres en donde se quiere buscar el carácter.
 * \param c         carácter a buscar.
 * 
 * \returns          -1: No se encontró el carácter.
 *                  >-1: Primera posicion en la que se encontro el carácter.
 */
int get_index(const char *p_string, char c);

/**
 * \brief           Función para procesar una cadena de caracteres con formato tipo JSON.
 * 
 * \param p_json    Cadena tipo JSON a procesar.
 * \param p_keys    Buffer en donde guardar las claves encontradas.
 * \param p_values  Buffer en donde guardar los valores de las claves.
 * \param n         Número de claves y llaves a buscar.
 */
bool parse_json(const char *p_json, char p_keys[][32], char p_values[][32], size_t n);

/**
 * \brief                   Función para obtener el valor de una clave de un JSON.
 * 
 * \param p_keys            Buffer con las claves.
 * \param p_values          Buffer con los valores.
 * \param p_key             Nombre de la clave.
 * \param p_value           Buffer en donde guardar el valor.
 * \param p_length          Número de claves que se encuentran en el buffer.
 * 
 * \returns                 true: Se guardo algún valor en el buffer.
 *                          false: No se guardo ningún valor.
 */
bool get_json_key_value(char p_keys[][32], char p_values[][32], const char *p_target, char *p_out, size_t p_max_pairs);

#endif