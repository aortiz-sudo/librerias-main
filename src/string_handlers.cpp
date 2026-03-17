#include "string_handlers.h"

bool string_contains(const char *p_string, const char *p_string_contained)
{
  return strstr(p_string, p_string_contained) != nullptr;
}

size_t split_string(const char *p_src, char p_dest[][STRING_BUFFER], size_t p_dest_size, char p_delim)
{
  size_t counter = 0;
  int j = 0;

  size_t len = strlen(p_src);

  for(size_t i = 0; i < len + 1; i++)
  {
    char c = p_src[i];

    if(counter >= p_dest_size)
      break;

    if(c != p_delim && c != '\0' && j < STRING_BUFFER - 1)
        p_dest[counter][j++] = c;
    else
    {
      p_dest[counter][j] = '\0';
      counter++;
      j = 0;
    }
  }

  return counter;
}

size_t get_substring(const char *p_string, char *p_substring, char p_terminator, size_t p_buffer_size)
{
  size_t count = 0;

  while(count < p_buffer_size - 1)
  {
    if(p_string[count] == p_terminator)
      break;

    *p_substring++ = p_string[count++];
  }

  *p_substring = '\0';

  return count;
}

size_t get_lines(const char *p_string)
{
  size_t counter = 0;
  while(*p_string)
  {
    if(*p_string++ == '\n')
      counter++;
  }

  return counter;
}

int get_index(const char *p_string, char c)
{
  int counter = 0;

  do
  {
    if(*p_string == c)
      return counter;

    counter++;
  } 
  while (*p_string++);

  return -1;
}

bool parse_json(const char *p_json, char p_keys[][32], char p_values[][32], size_t n)
{
  size_t counter = 0;

  char *p = strchr(p_json, '"') + 1;

  if(!p)
    return false;

  size_t data_counter = 0;
  size_t pos = 0;
  bool reading_key = true;
  
  while(*p && data_counter < n)
  {
    char c = *p;

    bool char_is_valid = (c != '"' && c != ',' && c != '}' &&  c != '{' && c != ':' && c != '[' && c != ']');

    if(char_is_valid)
    {
      if(pos < 31)
      {
        if(reading_key)
          p_keys[data_counter][pos] = c;
        else
          p_values[data_counter][pos] = c;
      }
      pos++;
    }
    else if(c == ':')
    {
      p_keys[data_counter][pos] = '\0';
      pos = 0;
      reading_key = false;
    }
    else if(c == ',')
    {
      p_values[data_counter][pos] = '\0';
      pos = 0;
      data_counter++;
      reading_key = true;
    }

    p++;
  }

  if(data_counter < n)
    p_values[data_counter][pos] = '\0';

  return true;
}

bool get_json_key_value(char p_keys[][32], char p_values[][32], const char *p_target, char *p_out, size_t p_max_pairs)
{
  for(int i = 0; i < p_max_pairs; i++)
  {
    if(strcmp(p_target, p_keys[i]) == 0)
    {
      strncpy(p_out, p_values[i], 31);
      p_out[31] = '\0';
      return true;
    }
  }
  p_out[0] = '\0';
  return false;
}