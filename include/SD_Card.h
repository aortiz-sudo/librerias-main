/**
 * \file    SD_Card.h
 * 
 * \brief   Archivo que contiene las funciones que se utilizan para manipular los archivos
 *          guardados en la tarjeta SD del proyecto como el de los parámetros, eventos,
 *          despachos, etc.
 */
#ifndef _SD_CARD_H
#define _SD_CARD_H

#include <Arduino.h>
#include <SD.h>
#include "Logger.h"

#define EVENTS_PATH                     "/Eventos.txt"             ///< Ruta de los eventos.
#define PARAMETERS_PATH                 "/Parametros.txt"          ///< Ruta de los parámetros.
#define DISPATCHES_PATH                 "/Despachos.txt"           ///< Ruta de los despachos.
#define GITHUB_PARAMETERS_PATH          "/github.txt"              ///< Ruta para los parametros de github.
#define API_KEY_PATH                    "/api-key.txt"             ///< Ruta para obtener la api key.
#define CA_CERT_PATH                    "/ca-cert.pem"             ///< Ruta para el certificado SSL raíz.
#define USERS_PATH                      "/usuarios.txt"            ///< Ruta para los usuarios.
#define EVENTS_TMP_PATH                 "/Eventos_tmp.txt"         ///< Ruta temporal de los eventos.
#define PARAMETERS_BACKUP_PATH          "/backups/Parametros.txt"  ///< Ruta para respaldo de los parametros.
#define DISPATCHES_BACKUP_PATH          "/backups/Despachos.txt"   ///< Ruta para respaldo de los despachos.
#define GITHUB_PARAMETERS_BACKUP_PATH   "/backups/github.txt"      ///< Ruta para respaldo de los parametros de github.
#define API_KEY_BACKUP_PATH             "/backups/api-key.txt"     ///< Ruta para respaldo de la api key;
#define USERS_BACKUP_PATH               "/backups/usuarios.txt"    ///< Ruta para respaldo de los usuarios.
#define EVENTS_BACKUP_PATH              "/backups/Eventos.txt"     ///< Ruta para respaldo de los eventos.
#define CORE_DUMP_LOG_PATH              "/coredump.elf"            ///< Ruta para el log del core dump.
#define ESP32_FW_PATH                   "/firmwares/firmware.bin"  ///< Ruta para el firmware de la ESP32. 
#define DISPLAY_FW_PATH                 "/firmwares/display.bin"   ///< Ruta para el firmware del controlador de la pantalla.
#define PRINTER_FW_PATH                 "/firmwares/printer.bin"   ///< Ruta para el firmware del controlador de la impresora.

/**
 * \enum    save_type_t
 * \brief   Tipos de datos que se pueden guardar en la tarjeta SD.
 */
typedef enum : uint8_t
{
    SAVE_EVENT,             ///< Guardar un evento.
    SAVE_DISPATCH,          ///< Guardar un despacho.
    SAVE_GENERAL_PARAMETER, ///< Guardar un parámetro general.
    SAVE_GITHUB_PARAMETER,  ///< Guardar un parámetro de GitHub.
    SAVE_API_KEY,           ///< Guardar la API key.
    SAVE_USER,              ///< Guardar un usuario.
} save_type_t;

/**
 * \struct  sd_info_struct
 * \brief   Estructura con la información necesaria para una operación de guardado en la SD.
 */
struct sd_info_struct
{
    save_type_t save_type;  ///< Tipo de dato a guardar.
    uint8_t position;       ///< Posición del dato dentro del archivo.
    bool delete_info;       ///< Indica si se debe eliminar el contenido previo.
    char path[32];          ///< Ruta del archivo destino.
    char data[256];         ///< Datos a guardar.
};

/**
 * \brief           Función para guardar datos en un archivo eliminando los datos anteriores.
 * \param p_path    Ruta del archivo en dónde se guardarán los datos.
 * \param p_text    Datos que se quieren guardar.
 * \returns         true: Los datos fueron guardados de manera exitosa.
 *                  false: No se pudieron guardar los datos.
 */
bool write_to_file(const char *p_path, const char *p_text);

/**
 * \brief           Función para agregar datos a un archivo.
 * \param p_path    Ruta del archivo en dónde se agregarán los datos.
 * \param p_text    Datos que se quieren agregar.
 * \returns         true: Los datos se agregaron de manera exitosa.
 *                  false: Los datos no se pudieron agregar.
 */
bool append_to_file(const char *p_path, const char *p_text);

/**
 * \brief               Función para leer los datos guardados en un archivo.
 * \param p_path        Ruta del archivo que se quiere leer.
 * \param p_lines       Buffer en donde se quieren guardar los datos.
 * \param p_buffer      Tamaño del buffer.
 * \param p_terminator  Delimitador para separar los datos guardados.
 * \returns             -1: No se pudo abrir el archivo.
 *                     >-1: Número de datos guardados.
 */
int read_file(const char *p_path, char p_lines[][128], size_t p_buffer_size, char p_terminator = '\n');

/**
 * \brief           Función para obtener el número de líneas totales de un archivo.
 * \param p_path    Ruta del archivo del que se quiere obtener el número de líneas.
 * \returns         -1: No se pudo leer el archivo.
 *                 >-1: Número de líneas.
 */
int get_number_of_lines(const char *p_path);

/**
 * \brief           Función para guardar un evento que no se pudo enviar al servidor.
 * \param p_event   Evento que se quiere guardar.
 * \returns         true: El evento se pudo guardar de manera exitosa.
 *                  false: El evento no se pudo guardar.
 */
bool save_event(const char *p_event);

/**
 * \brief           Función para obtener eventos guardados.
 * \param p_event   Buffer en donde se guardarán los eventos.
 * \param n         Variable que guardará la cantidad de eventos guardados.
 * \returns         -1: No se pudieron obtener los eventos guardados.
 *                 >-1: Longitud total del buffer.
 */
int get_events(char p_event[][256], size_t *n);

/**
 * \brief           Función para eliminar eventos ya enviados al servidor.
 * \param n         Número de eventos que se quieren eliminar.
 * \returns         true: Los eventos pudieron ser eliminados.
 *                  false: No se pudieron eliminar los eventos.
 */
bool erase_events(size_t n);

/**
 * \brief               Función para guardar parámetros.
 * \param p_params      Buffer que guarda la información de los parámetros.
 * \param p_params_pos  Buffer que guarda la posición de los parámetros en el archivo.
 * \param p_params_num  Número de parámetros a guardar.
 * \returns             true: El parámetro se guardo de manera exitosa.
 *                      false: No se pudo guardar el parámetro.
 */
bool save_parameter(const char *p_path, const char **p_params, uint8_t *p_param_pos, size_t p_params_num);

/**
 * \brief               Función para guardar la información de un despacho, que se utilizará para la impresión de los tickets.
 * \param p_dispatch    Información del despacho.
 * \param p_delete      Variable utilizada para saber si se quiere eliminar el resto del contenido del archivo.
 * \returns             true: El despacho fue guardado correctamente.
 *                      false: No se pudo guardar el despacho.
 */
bool save_dispatch(const char *p_dispatch, bool p_delete = false);

/**
 * \brief                   Función para obtener el último despacho guardado.
 * \param p_last_dispatch   Buffer en donde se guardara el despacho.
 * \returns                 -1: No se pudo leer el último despacho.
 *                         >-1: Tamaño del buffer.
 */
int get_last_dispatch(char p_last_dispatch[128]);

/**
 * \brief                           Función para obtener una cantidad de determinada de despachos guardados.
 * \param p_dispatches              Buffer en donde se guaradarán los despachos.
 * \param p_number_of_dispatches    Cantidad de despachos que se quieren leer.
 * \returns                         true: Los despachos fueron guardados en el buffer con éxito.
 *                                  false: No se pudieron guardar los despachos.
 */
bool get_dispatches(char p_dispatches[][128], uint8_t p_number_of_dispatches);

#endif