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

  // Último renglón sin salto de línea al final del archivo: antes se perdía.
  if(index > 0 && count < p_buffer_size)
  {
    p_lines[count][index] = '\0';
    count++;
  }

  file.close();

  return count;
}

int count_lines(const char *p_path)
{
  File file = SD.open(p_path, FILE_READ);

  if(!file)
    return -1;

  int count = 0;
  bool pending = false;

  while(file.available())
  {
    char c = file.read();

    if(c == '\n')
    {
      count++;
      pending = false;
      vTaskDelay(1 / portTICK_PERIOD_MS);
    }
    else if(c != '\r')
      pending = true;
  }

  file.close();

  return pending ? count + 1 : count;
}

bool write_lines(const char *p_path, char p_lines[][128], size_t p_count)
{
  File file = SD.open(p_path, FILE_WRITE);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo");
    return false;
  }

  for(size_t i = 0; i < p_count; i++)
  {
    file.print(p_lines[i]);
    file.print("\r\n");
  }

  file.close();

  return true;
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
  // count_lines() cuenta también un último renglón sin salto de línea. Con
  // get_number_of_lines() ese renglón no se contaba y se borraba al reescribir.
  int lines = count_lines(p_path);

  if(lines < 1)
    return false;

  char sd_parameters[lines][128];
  int read = read_file(p_path, sd_parameters, lines, '\n');

  // read_file() regresa -1 si no abre el archivo; antes eso pasaba como éxito y se
  // escribía el buffer sin inicializar.
  if(read <= 0)
  {
    logger.logln("Error al guardar los parametros.");
    return false;
  }

  size_t counter = 0;

  logger.logln("Guardando parametros...");

  for(int i = 0; i < read; i++)
  {
    if(counter < p_param_num && i == p_param_pos[counter])
    {
      strncpy(sd_parameters[i], p_params[counter], 127);
      sd_parameters[i][127] = '\0';
      counter++;
    }
  }

  // Se reescribe de una sola vez (antes eran un vaciado y dos escrituras por renglón):
  // el archivo queda a medias durante mucho menos tiempo si se va la luz.
  bool save = write_lines(p_path, sd_parameters, read);

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