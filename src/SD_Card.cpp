#include "SD_Card.h"

bool write_to_file(const char *p_path, const char *p_text)
{
  File file = SD.open(p_path, FILE_WRITE);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return false;
  }

  file.print(p_text);
  file.close();

  return true;
}

bool append_to_file(const char *p_path, const char *p_text)
{
  File file = SD.open(p_path, FILE_APPEND);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return false;
  }

  file.print(p_text);
  file.close();

  return true;
}

int read_file(const char *p_path, char p_lines[][128], size_t p_buffer_size, char p_terminator)
{
  File file = SD.open(p_path, FILE_READ);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return -1;
  }

  size_t index = 0;
  size_t count = 0;
  
  while(file.available())
  {
    char c = file.read();

    if(c != '\r' && c != '\n')
    {
      p_lines[count][index] = c;
      index++;
    }

    if(c == p_terminator || index >= 126)
    {
      p_lines[count][index] = '\0';
      count++;
      index = 0;
    }

    if(count >= p_buffer_size)
      break;
  }

  file.close();

  return count;
}

int get_number_of_lines(const char *p_path)
{
  int count = 0;

  File file = SD.open(p_path, FILE_READ);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return -1;
  }

  while(file.available())
  {
    if(file.read() == '\n')
    {
      count++;
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
  }
  
  file.close();
    
  return count;
}

bool save_event(const char *p_event)
{
  bool file_exists = SD.exists(EVENTS_PATH);
  bool save = true;

  File file;

  if(!file_exists)
  {
    file = SD.open(EVENTS_PATH, FILE_WRITE, true);
    file.close();

    if(!file)
    {
      logger.logln("No se pudo guardar el evento.");
      return false;
    }

    save = write_to_file(EVENTS_PATH, p_event);
  }
  else
    save = append_to_file(EVENTS_PATH, p_event);

  if(save)
  {
    append_to_file(EVENTS_PATH, "\r\n");
    logger.logln("Evento guardado con exito.");
  }
  else
    logger.logln("No se pudo guardar el evento.");
  
  return save;
}

int get_events(char p_event[][256], size_t *n)
{
  int lines = get_number_of_lines(EVENTS_PATH);

  if(lines < 1)
    return lines;

  size_t events = lines > 10 ? 10 : lines;
  *n = events;

  File file = SD.open(EVENTS_PATH, FILE_READ);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return -1;
  }

  size_t event_postion[events];
  size_t line_counter = 0;
  size_t event_counter = 0;
  size_t char_counter = 0;
  int total_length = 0;

  while(file.available())
  {
    char c = file.read();

    if(c == '[')
    {
      if(line_counter == (lines - (events - event_counter)))
        event_postion[event_counter++] = char_counter;

      line_counter++;
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    char_counter++;
  }
  
  for(int i = 0; i < event_counter; i++)
  {
    file.seek(i == 0 ? event_postion[i] : event_postion[i] + 1);
    int length = file.readBytesUntil(']', p_event[i], 256);
    p_event[i][length++] = i == (event_counter - 1) ? ']' : ',';
    p_event[i][length] = '\0';
    total_length += length;
  }

  logger.logln(" ");

  file.close();

  return total_length;
}

bool erase_events(size_t n)
{
  int total_lines = get_number_of_lines(EVENTS_PATH);
  logger.logln(total_lines);

  if(total_lines <= 10)
    return write_to_file(EVENTS_PATH, "");

  if(SD.exists(EVENTS_TMP_PATH))
    SD.remove(EVENTS_TMP_PATH);

  File tmp = SD.open(EVENTS_TMP_PATH, FILE_WRITE, true);
  File file = SD.open(EVENTS_PATH, FILE_READ);

  if(!file || !tmp)
    return false;

  size_t line_counter = 0;

  while(file.available())
  {
    char c = file.read();
    tmp.print(c);

    if(c == '\n')
    {
      line_counter++;
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    if(line_counter >= total_lines - n)
    {
      file.close();
      tmp.close();

      break;
    }
  }

  if(!write_to_file(EVENTS_PATH, ""))
    return false;

  file = SD.open(EVENTS_PATH, FILE_WRITE);
  tmp = SD.open(EVENTS_TMP_PATH, FILE_READ);

  while(tmp.available())
  {
    char c = tmp.read();
    file.print(c);
    
    if(c == '\n')
      vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  file.close();
  tmp.close();

  if(!SD.remove(EVENTS_TMP_PATH))
    logger.logln("No se pudo borrar el archivo temporal");

  return true;
}

bool save_parameter(const char *p_path, const char **p_params, uint8_t *p_param_pos, size_t p_param_num)
{ 
  int lines =  get_number_of_lines(p_path);

  if(lines < 1)
    return false;

  char sd_parameters[lines][128];

  if(!read_file(p_path, sd_parameters, lines, '\n'))
  {
    logger.logln("Error al guardar los parametros.");
    return false;
  }

  size_t counter = 0;

  logger.logln("Guardando parametros...");

  for(int i = 0; i < lines; i++)
  {
    if(i == p_param_pos[counter])
    {
      strncpy(sd_parameters[i], p_params[counter], 127);
      sd_parameters[i][127] = '\0';
      counter++;
    }
  }

  bool save = true;
  int line_counter = lines;
  counter = 0;

  if(!write_to_file(p_path, ""))
    save = false;

  while(save && line_counter--)
  {
    save = append_to_file(p_path, sd_parameters[counter++]);
    append_to_file(p_path, "\r\n");
  }
  
  logger.logln(save ? "Parametros guardados exitosamente." : "Error al guardar los parametros.");

  return save;
}

bool save_dispatch(const char *p_dispatch, bool p_delete)
{
  bool file_exists = SD.exists(DISPATCHES_PATH);

  File file;

  bool save;

  if(!file_exists || p_delete)
  {
    file = SD.open(DISPATCHES_PATH, FILE_WRITE, true);

    if(!file)
    {
      logger.logln("No se pudo abrir el archivo");
      return false;
    }

    file.close();

    save = write_to_file(DISPATCHES_PATH, p_dispatch);
  }
  else if(file_exists && !p_delete)
    save = append_to_file(DISPATCHES_PATH, p_dispatch);

  if(save)
    append_to_file(DISPATCHES_PATH, "\r\n");

  return save;
}

int get_last_dispatch(char p_last_dispatch[128])
{
  size_t lines = get_number_of_lines(DISPATCHES_PATH);

  if(lines < 1)
    return 0;

  File file = SD.open(DISPATCHES_PATH, FILE_READ);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return -1;
  }

  size_t char_counter = 0;
  size_t line_counter = 0;

  while(file.available())
  {
    char c = file.read();

    if(line_counter == lines - 1)
    {
      file.seek(char_counter);
      size_t length = file.readBytesUntil('!', p_last_dispatch, 128);
      p_last_dispatch[length] = '!';
      p_last_dispatch[length + 1] = '\0';
      
      return 1;
    }

    if(c)
      char_counter++;

    if(c == '\n')
      line_counter++;
  }

  return -2;
}

bool get_dispatches(char p_dispatches[][128], uint8_t p_number_of_dispatches)
{
  File file = SD.open(DISPATCHES_PATH, FILE_READ);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return false;
  }

  uint8_t dispatch_counter = 0;
  uint8_t buffer = 0;

  while(file.available())
  {
    char data = file.read();
    
    if(data == '.' || isDigit(data) || data == ';')
      p_dispatches[dispatch_counter][buffer++] = data;
    
    if(data == '!')
    {
      p_dispatches[dispatch_counter][buffer++] = '!';
      p_dispatches[dispatch_counter][buffer++] = '\0';
      dispatch_counter++;
      buffer = 0;
    }

    if(dispatch_counter >= p_number_of_dispatches)
      break;
  }

  return true;
}