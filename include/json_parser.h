#ifndef _JSON_PARSER_H
#define _JSON_PARSER_H

#include <Arduino.h>
#include "jsmn.h"
#include "string_handlers.h"

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
 * 
 */
typedef enum : int8_t
{
    JSON_NO_PRIMITIVE = -1,
    JSON_INT = 0,
    JSON_FLOAT = 1,
    JSON_BOOL = 2,
    JSON_NULL = 3,
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

struct minimal_json_struct 
{
    int id;
    char name[32];
};

struct product_json_struct : minimal_json_struct
{
    float price;
    minimal_json_struct client;
};

struct user_json_struct : minimal_json_struct
{
    char last_name[32];
    char email[64];
    char password[7];
    minimal_json_struct profile;
    minimal_json_struct branch;
    minimal_json_struct client;
};

struct tank_json_struct : minimal_json_struct
{
    int capacity;
    minimal_json_struct branch;
};

struct device_json_struct : minimal_json_struct
{
    minimal_json_struct client;
    tank_json_struct tank;
};

/**
 * \struct  json_update_struct
 * \brief   Estructura para almacenar información de actualizaciones JSON enviadas al servidor.
 */
struct json_update_struct
{
    int id;
    const char *product;      
    json_update update;
    char price[10];
};

template <typename T>
struct json_parse_value_result
{
    bool success;
    bool is_null;
    T value;
};

class JSON_Parser
{
    public:
        ~JSON_Parser() { }

        int parse_json(const char *p_json);
        int json_find_key_in_object(const char *p_json, int p_obj_index, const char *p_key);
        int json_skip(const jsmntok_t *tokens, int index);
        int json_find_object(const char *p_json, int p_obj_index, int p_current_object);
        bool jsoneq(const char *p_json, const jsmntok_t *p_token, const char *p_key);
        void parse_user(const char *p_json, int p_obj_index, user_json_struct *p_user);
        void parse_minimal(const char *p_json, int obj_inde, minimal_json_struct *p_minimal);
        jsmntok_t *get_token(int index);

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

        inline int get_token_count()
        {
            return this->m_json_token_count;
        }

        inline jsmntype_t get_token_type(int p_token_index)
        {
            return this->m_json_tokens[p_token_index].type;
        }

    private:
        jsmn_parser m_json_parser = { };
        int m_json_token_count = 0;
        jsmntok_t m_json_tokens[JSON_MAX_TOKENS] = { };

    private:
        int copy_token_value(const char *p_json, const jsmntok_t *p_token, char *p_out, size_t p_out_size);
        int get_token_length(const jsmntok_t *p_token);
        json_primitive_type_t get_token_primitive_type(const char *p_json, const jsmntok_t *p_token);
};

#endif