#include "json_parser.h"

int JSON_Parser::parse_json(const char *p_json)
{
    jsmn_init(&this->m_json_parser);
    int tokens = jsmn_parse(&this->m_json_parser, p_json, strlen(p_json), this->m_json_tokens, JSON_MAX_TOKENS);

    this->m_json_token_count = tokens;

    return tokens;
}

bool JSON_Parser::jsoneq(const char *p_json, const jsmntok_t *p_token, const char *p_key)
{
    int len = p_token->end - p_token->start;
    return (p_token->type == JSMN_STRING && (int)strlen(p_key) == len && strncmp(p_json + p_token->start, p_key, len) == 0);
}

int JSON_Parser::copy_token_value(const char *p_json, const jsmntok_t *p_token, char *p_out, size_t p_out_size)
{   
    if(!p_out || p_out_size == 0) 
        return -1;

    int len = get_token_length(p_token);

    if(len >= (int)p_out_size)
        len = p_out_size - 1;

    mempcpy(p_out, p_json + p_token->start, len);
    p_out[len] = '\0';

    return len;
}

int JSON_Parser::get_token_length(const jsmntok_t *p_token)
{
    return p_token->end - p_token->start;
}

json_primitive_type_t JSON_Parser::get_token_primitive_type(const char *p_json, const jsmntok_t *p_token)
{
    if(p_token->type != JSMN_PRIMITIVE)
        return JSON_NO_PRIMITIVE;

    char str[32];
    int length = get_token_length(p_token);

    if(length >= (int)sizeof(str))
        length = sizeof(str) - 1;

    memcpy(str, p_json + p_token->start, length);

    str[length] = '\0';

    if(strcmp(str, "null") == 0)
        return JSON_NULL;
    
    if(strcmp(str, "true") == 0 || strcmp(str, "false") == 0)
        return JSON_BOOL;

    if(strchr(str, '.') != nullptr || strchr(str, 'e') != nullptr || strchr(str, 'E') != nullptr)
        return JSON_FLOAT;

    return JSON_INT;
}

int JSON_Parser::json_find_key_in_object(const char *p_json, int p_obj_index, const char *p_key)
{
    if(p_obj_index < 0 || p_obj_index >= this->m_json_token_count)
        return -1;

    jsmntok_t *token = &this->m_json_tokens[p_obj_index];

    if(token->type != JSMN_OBJECT)
        return -1;

    int n_keys = token->size;
    int token_index = p_obj_index + 1;

    for(int i = 0; i < n_keys && token_index < this->m_json_token_count; i++)
    {
        int value_index = token_index + 1;

        if(value_index >= this->m_json_token_count)
            return -1;

        jsmntok_t *key_token = &this->m_json_tokens[token_index];
        
        if(jsoneq(p_json, key_token, p_key))
            return value_index;

        token_index = value_index + json_skip(this->m_json_tokens, value_index);

    }
    return -1;
}

void JSON_Parser::parse_user(const char *p_json, int p_obj_index, user_json_struct *p_user)
{
    p_user->id = 0;
    p_user->name[0] = p_user->last_name[0] = p_user->email[0] = p_user->password[0] = '\0';
    p_user->profile.id = p_user->branch.id = p_user->client.id = 0;
    p_user->profile.name[0] = p_user->branch.name[0] = p_user->client.name[0] = '\0';

    int id_index = json_find_key_in_object(p_json, p_obj_index, "id");
    int name_index = json_find_key_in_object(p_json, p_obj_index, "nombre");
    int last_name_index = json_find_key_in_object(p_json, p_obj_index, "apellido");
    int email_index = json_find_key_in_object(p_json, p_obj_index, "correo");
    int password_index = json_find_key_in_object(p_json, p_obj_index, "clave");
    int branch_index = json_find_key_in_object(p_json, p_obj_index, "sucursal");
    int profile_index = json_find_key_in_object(p_json, p_obj_index, "perfil");
    int client_index = json_find_key_in_object(p_json, p_obj_index, "cliente");

    if(id_index > 0)
    {
        json_parse_value_result<int> result = token_to_value<int>(p_json, &this->m_json_tokens[id_index]);

        if(result.success && !result.is_null)
            p_user->id = result.value;
        else
            p_user->id = 0;
    }
    else
        p_user->id = 0;

    if(name_index > 0)
    {
        char name[32];
        json_parse_value_result<const char *> result  = token_to_string<const char *>(p_json, &this->m_json_tokens[name_index], name, sizeof(name));

        if(result.success && !result.is_null)
            strncpy(p_user->name, name, sizeof(p_user->name));
        else
            p_user->name[0] = '\0';
    }
    else
        p_user->name[0] = '\0';

    if(last_name_index > 0)
    {
        char last_name[32];
        json_parse_value_result<const char *> result = token_to_string<const char *>(p_json, &this->m_json_tokens[last_name_index], last_name, sizeof(last_name));

        if(result.success && !result.is_null)
            strncpy(p_user->last_name, last_name, sizeof(p_user->last_name));
        else
            p_user->last_name[0] = '\0';
    }
    else
        p_user->last_name[0] = '\0';

    if(email_index > 0)
    {
        char email[32];
        json_parse_value_result<const char *> result = token_to_string<const char *>(p_json, &this->m_json_tokens[email_index], email, sizeof(email));

        if(result.success && !result.is_null)
            strncpy(p_user->email, email, sizeof(p_user->email));
        else
            p_user->email[0] = '\0';
    }
    else
        p_user->email[0] = '\0';

    if(password_index > 0)
    {
        char password[32];
        json_parse_value_result<const char *> result  = token_to_string<const char *>(p_json, &this->m_json_tokens[password_index], password, sizeof(password));

        if(result.success && !result.is_null)
            strncpy(p_user->password, password, sizeof(p_user->password));
        else
            p_user->password[0] = '\0';
    }
    else
        p_user->password[0] = '\0';

    if(profile_index > 0)
        parse_minimal(p_json, profile_index, &p_user->profile);

    if(branch_index > 0)
        parse_minimal(p_json, branch_index, &p_user->branch);

    if(client_index > 0)
        parse_minimal(p_json, client_index, &p_user->client);
}

void JSON_Parser::parse_minimal(const char *p_json, int p_obj_index, minimal_json_struct *p_minimal)
{
    int id_index = json_find_key_in_object(p_json, p_obj_index, "id");
    int name_index = json_find_key_in_object(p_json, p_obj_index, "nombre");

    if(id_index > 0)
    {
        json_parse_value_result<int> result = token_to_value<int>(p_json, &this->m_json_tokens[id_index]);
        
        if(result.success && !result.is_null)
            p_minimal->id = result.value;
        else
            p_minimal->id = 0;
    }
    else
        p_minimal->id = 0;

    if(name_index > 0)
    {
        char name[32];
        json_parse_value_result<const char *> result = token_to_string<const char *>(p_json, &this->m_json_tokens[name_index], name, sizeof(name));

        if(result.success && !result.is_null)
            strncpy(p_minimal->name, name, sizeof(p_minimal->name));
        else
            p_minimal->name[0] = '\0';
    }
    else
        p_minimal->name[0] = '\0';
}

int JSON_Parser::json_find_object(const char *p_json, int p_obj_index, int p_current_object)
{
    if(p_obj_index < 0 || p_obj_index >= this->m_json_token_count)
        return -1;

    jsmntok_t *token = &this->m_json_tokens[p_obj_index];

    if(token->type != JSMN_ARRAY)
        return -1;
    
    int j = 0;
    for(int i = p_obj_index; i < this->m_json_token_count && j < token->size;)
    {
        jsmntok_t *obj_token = &this->m_json_tokens[i];

        if(obj_token->type != JSMN_OBJECT)
        {
            i++;
            continue;
        }
        
        if(j == p_current_object)
            return i;
        
        i += json_skip(this->m_json_tokens, i);
        j++;
    }

    return -1;
}

int JSON_Parser::json_skip(const jsmntok_t *tokens, int index)
{
    int count = 1;
    int end = tokens[index].end;

    for(int i = index + 1; tokens[i].start < end; i++)
        count++;
    
    return count;
}

jsmntok_t *JSON_Parser::get_token(int index)
{
    if(index >= this->m_json_token_count || index < 0)
        return nullptr;
    
    return &this->m_json_tokens[index];
}     