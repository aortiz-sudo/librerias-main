/**
 * \file    json_parser.h
 *
 * \brief   Este archivo contiene la definición de la clase JSON_Parser y las estructuras
 *          asociadas para el procesamiento de datos JSON utilizando la librería jsmn.
 */

#ifndef _JSON_PARSER_H
#define _JSON_PARSER_H

#include <Arduino.h>
#include "jsmn.h"
#include "string_handlers.h"

/**
 * \def     JSON_MAX_TOKENS
 * \brief   Número máximo de tokens que puede manejar el parser JSON.
 */
#ifndef JSON_MAX_TOKENS
    #define JSON_MAX_TOKENS 512
#endif

/**
 * \enum    json_event
 * \brief   Enumeración de eventos JSON que pueden ser generados por el sistema.
 */
typedef enum : uint8_t
{
    FUEL_LEVEL = 1,             ///< Evento de nivel de combustible.
    LOG_IN,                     ///< Evento de inicio de sesión.
    FUEL_DISPATCHED,            ///< Evento de despacho de combustible.
    MAIN_POWER_DISCONNECTED,    ///< Evento de desconexión de energía principal.
    BATTERY_DISCONNECTED,       ///< Evento de desconexión de batería.
    LOW_BATTERY,                ///< Evento de batería baja.
    FUEL_SENSOR_DISCONNECTED,   ///< Evento de desconexión de sensor de combustible.
    PRINTER_DISCONNECTED,       ///< Evento de desconexión de impresora.
    RFID_READER_DISCONNECTED,   ///< Evento de desconexión de lector RFID.
    OPEN_CABINET,               ///< Evento de gabinete abierto.
    UNEXPECTED_RESET            ///< Evento de reinicio inesperado.
} json_event;

/**
 * \enum    json_update
 * \brief   Enumeración de actualizaciones JSON que pueden ser enviadas al servidor.
 */
typedef enum : uint8_t
{
    PRICE_UPDATE    ///< Actualización de precio.
} json_update;

/**
 * \enum    json_primitive_type_t
 * \brief   Tipos primitivos que puede tener un valor JSON.
 */
typedef enum : int8_t
{
    JSON_NO_PRIMITIVE = -1, ///< El valor no es un primitivo válido.
    JSON_INT = 0,           ///< Valor entero.
    JSON_FLOAT = 1,         ///< Valor flotante.
    JSON_BOOL = 2,          ///< Valor booleano.
    JSON_NULL = 3,          ///< Valor nulo.
} json_primitive_type_t;

/**
 * \struct  json_event_struct
 * \brief   Estructura para almacenar información de eventos JSON generados por el sistema.
 */
typedef struct 
{
    uint16_t fuel_level;            ///< Nivel de combustible.
    int8_t fuel_temp;               ///< Temperatura del combustible.
    uint8_t int_battery_level;      ///< Nivel entero de batería.
    uint8_t dec_battery_level;      ///< Nivel decimal de batería.
    uint8_t day;                    ///< Día.
    uint8_t month;                  ///< Mes.
    uint8_t year;                   ///< Año.
    uint8_t hour;                   ///< Hora.
    uint8_t minute;                 ///< Minuto.
    uint8_t second;                 ///< Segundo.
    json_event event;               ///< Evento.
    bool open;                      ///< Estado de apertura de gabinete.
    char ticket_number[15];         ///< Folio del ticket.
    char quantity_dispatched[15];   ///< Cantidad despachada.
    char user_id[10];               ///< ID del usuario.
    char *password;                 ///< Contraseña.
    char *tag;                      ///< Tag RFID.
} json_event_struct;

/**
 * \struct  minimal_json_struct
 * \brief   Estructura base con la información mínima de una entidad JSON (id y nombre).
 */
struct minimal_json_struct
{
    int id;             ///< Identificador de la entidad.
    char name[32];      ///< Nombre de la entidad.
};

/**
 * \struct  product_json_struct
 * \brief   Estructura con la información de un producto.
 *
 * Hereda de minimal_json_struct y añade el precio y el cliente asociado.
 */
struct product_json_struct : minimal_json_struct
{
    float price;                    ///< Precio del producto.
    minimal_json_struct client;     ///< Cliente asociado al producto.
};

/**
 * \struct  user_json_struct
 * \brief   Estructura con la información de un usuario.
 *
 * Hereda de minimal_json_struct y añade datos de contacto, credenciales y las entidades asociadas.
 */
struct user_json_struct : minimal_json_struct
{
    char last_name[32];             ///< Apellido del usuario.
    char email[64];                 ///< Correo electrónico del usuario.
    char password[7];               ///< Contraseña del usuario.
    minimal_json_struct profile;    ///< Perfil asociado al usuario.
    minimal_json_struct branch;     ///< Sucursal asociada al usuario.
    minimal_json_struct client;     ///< Cliente asociado al usuario.
};

/**
 * \struct  tank_json_struct
 * \brief   Estructura con la información de un tanque.
 *
 * Hereda de minimal_json_struct y añade la capacidad y la sucursal asociada.
 */
struct tank_json_struct : minimal_json_struct
{
    int capacity;                   ///< Capacidad del tanque.
    minimal_json_struct branch;     ///< Sucursal asociada al tanque.
};

/**
 * \struct  device_json_struct
 * \brief   Estructura con la información de un dispositivo.
 *
 * Hereda de minimal_json_struct y añade el cliente y el tanque asociados.
 */
struct device_json_struct : minimal_json_struct
{
    minimal_json_struct client;     ///< Cliente asociado al dispositivo.
    tank_json_struct tank;          ///< Tanque asociado al dispositivo.
};

/**
 * \struct  json_update_struct
 * \brief   Estructura para almacenar información de actualizaciones JSON enviadas al servidor.
 */
struct json_update_struct
{
    int id;                 ///< Identificador de la entidad a actualizar.
    const char *product;    ///< Producto asociado a la actualización.
    json_update update;     ///< Tipo de actualización a realizar.
    char price[10];         ///< Nuevo precio en formato cadena.
};

/**
 * \struct  json_parse_value_result
 * \brief   Resultado de la conversión de un token JSON a un valor tipado.
 * \tparam  T   Tipo del valor convertido.
 */
template <typename T>
struct json_parse_value_result
{
    bool success;   ///< Indica si la conversión fue exitosa.
    bool is_null;   ///< Indica si el valor del token era nulo.
    T value;        ///< Valor convertido.
};

/**
 * \class JSON_Parser
 *
 * \brief Clase para el procesamiento de cadenas JSON utilizando la librería jsmn.
 *
 * Permite tokenizar un JSON, buscar claves y objetos, comparar tokens y convertir valores a tipos específicos.
 */
class JSON_Parser
{
    public:
        /**
         * \brief Destructor.
         */
        ~JSON_Parser() { }

        /**
         * \brief           Tokeniza una cadena JSON.
         * \param p_json    Cadena JSON a procesar.
         * \returns         Número de tokens encontrados o código de error.
         */
        int parse_json(const char *p_json);

        /**
         * \brief               Busca una clave dentro de un objeto JSON.
         * \param p_json        Cadena JSON.
         * \param p_obj_index   Índice del token del objeto donde buscar.
         * \param p_key         Clave a buscar.
         * \returns             Índice del token del valor asociado a la clave, o -1 si no se encuentra.
         */
        int json_find_key_in_object(const char *p_json, int p_obj_index, const char *p_key);

        /**
         * \brief           Salta un token y todos sus tokens hijos.
         * \param tokens    Arreglo de tokens.
         * \param index     Índice del token a saltar.
         * \returns         Índice del siguiente token después del bloque saltado.
         */
        int json_skip(const jsmntok_t *tokens, int index);

        /**
         * \brief                   Busca el n-ésimo objeto dentro de la cadena JSON.
         * \param p_json            Cadena JSON.
         * \param p_obj_index       Índice del objeto buscado.
         * \param p_current_object  Índice del token desde donde iniciar la búsqueda.
         * \returns                 Índice del token del objeto encontrado, o -1 si no existe.
         */
        int json_find_object(const char *p_json, int p_obj_index, int p_current_object);

        /**
         * \brief           Compara el contenido de un token con una clave dada.
         * \param p_json    Cadena JSON.
         * \param p_token   Token a comparar.
         * \param p_key     Clave con la que se compara.
         * \returns         true si el token coincide con la clave, false en caso contrario.
         */
        bool jsoneq(const char *p_json, const jsmntok_t *p_token, const char *p_key);

        /**
         * \brief               Procesa un objeto JSON y llena una estructura de usuario.
         * \param p_json        Cadena JSON.
         * \param p_obj_index   Índice del token del objeto usuario.
         * \param p_user        Estructura donde se almacenará la información del usuario.
         */
        void parse_user(const char *p_json, int p_obj_index, user_json_struct *p_user);

        /**
         * \brief               Procesa un objeto JSON y llena una estructura mínima (id y nombre).
         * \param p_json        Cadena JSON.
         * \param obj_inde      Índice del token del objeto.
         * \param p_minimal     Estructura donde se almacenará la información mínima.
         */
        void parse_minimal(const char *p_json, int obj_inde, minimal_json_struct *p_minimal);

        /**
         * \brief           Obtiene un token por su índice.
         * \param index     Índice del token.
         * \returns         Puntero al token solicitado.
         */
        jsmntok_t *get_token(int index);

        /**
         * \brief           Convierte un token JSON a un valor numérico o booleano del tipo indicado.
         * \tparam T         Tipo de destino (int, float o bool).
         * \param p_json    Cadena JSON.
         * \param p_token   Token a convertir.
         * \returns         Resultado de la conversión, incluyendo el valor y su validez.
         */
        template<typename T>
        json_parse_value_result<T> token_to_value(const char *p_json, const jsmntok_t *p_token)
        {
            json_parse_value_result<T> result{};
            result.success = false;
            result.is_null = false;
            
            json_primitive_type_t type = get_token_primitive_type(p_json, p_token);

            if(type == JSON_NO_PRIMITIVE)
                return result;
            
            if(type == JSON_NULL)
            {
                result.is_null = true;
                result.success = true;
                return result;
            }
                
            char buffer[32];
            if(copy_token_value(p_json, p_token, buffer, sizeof(buffer)) == -1)
                return result;

            if constexpr (std::is_same<T, float>::value)
            {   
                if(type != JSON_FLOAT)
                    return result;

                result.value = atof(buffer);
            }
            else if constexpr (std::is_same<T, int>::value)
            {
                if(type != JSON_INT)
                    return result;

                result.value = atoi(buffer);
            }
            else if constexpr (std::is_same<T, bool>::value)
            {  
                if(type != JSON_BOOL)
                    return result;

               result.value = strcmp(buffer, "true") == 0 ? true : false;
            }
            else
                return result;
            
            result.success = true;
            return result;
        }

        /**
         * \brief               Copia el valor de un token de tipo cadena a un buffer.
         * \tparam T             Tipo asociado al resultado (no afecta la copia de la cadena).
         * \param p_json        Cadena JSON.
         * \param p_token       Token de tipo cadena a copiar.
         * \param p_output      Buffer donde se almacenará la cadena.
         * \param p_output_len  Tamaño máximo del buffer.
         * \returns             Resultado de la operación, indicando éxito o valor nulo.
         */
        template <typename T>
        json_parse_value_result<T> token_to_string(const char *p_json, const jsmntok_t *p_token, char *p_output, size_t p_output_len)
        {
            json_parse_value_result<T> result{};
            result.success = false;

            if(get_token_primitive_type(p_json, p_token) == JSON_NULL)
            {
                result.is_null = true;
                return result;
            }

            if(p_token->type != JSMN_STRING)
                return result;

            if(copy_token_value(p_json, p_token, p_output, p_output_len) == -1)
                return result;

            result.success = true;
            return result;
        }

        /**
         * \brief   Obtiene el número de tokens generados en el último parseo.
         * \returns Cantidad de tokens.
         */
        inline int get_token_count()
        {
            return this->m_json_token_count;
        }

        /**
         * \brief                   Obtiene el tipo de un token por su índice.
         * \param p_token_index     Índice del token.
         * \returns                 Tipo del token (jsmntype_t).
         */
        inline jsmntype_t get_token_type(int p_token_index)
        {
            return this->m_json_tokens[p_token_index].type;
        }

    private:
        jsmn_parser m_json_parser = { };                    ///< Instancia del parser jsmn.
        int m_json_token_count = 0;                         ///< Número de tokens del último parseo.
        jsmntok_t m_json_tokens[JSON_MAX_TOKENS] = { };     ///< Arreglo de tokens generados.

    private:
        /**
         * \brief           Copia el valor de un token a un buffer.
         * \param p_json    Cadena JSON.
         * \param p_token   Token a copiar.
         * \param p_out     Buffer de destino.
         * \param p_out_size Tamaño máximo del buffer.
         * \returns         Número de bytes copiados, o -1 en caso de error.
         */
        int copy_token_value(const char *p_json, const jsmntok_t *p_token, char *p_out, size_t p_out_size);

        /**
         * \brief           Obtiene la longitud de un token.
         * \param p_token   Token del cual obtener la longitud.
         * \returns         Longitud del token en bytes.
         */
        int get_token_length(const jsmntok_t *p_token);

        /**
         * \brief           Determina el tipo primitivo de un token JSON.
         * \param p_json    Cadena JSON.
         * \param p_token   Token a evaluar.
         * \returns         Tipo primitivo del token (entero, flotante, booleano, nulo o no primitivo).
         */
        json_primitive_type_t get_token_primitive_type(const char *p_json, const jsmntok_t *p_token);
};

#endif