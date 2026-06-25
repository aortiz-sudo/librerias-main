/**
 * @file  main.cpp
 * @brief Archivo principal del sistema de despacho de combustible.
 * 
 * Este archivo contiene la lógica principal para la gestión de hardware, tareas, 
 * comunicación, eventos y control de periféricos en el sistema.
 */

#include "main.h"

//========================= Variables Globales =========================//

/// Puntero a la pantalla principal.
static Screen *screen;
/// Puntero a la impresora.
static Printer *printer;
/// Puntero al sensor LLS.
static LLS *lls_device;
/// Puntero al dispositivo RFID.
static MT124 *rfid_device;
/// Cliente HTTP para envio de eventos sobre Ethernet.
static Ethernet_HTTP *http_client;
/// Cliente WebSocket sobre Ethernet.
static Ethernet_SocketIO *websocket_client;
/// Objeto para el control del RTC DS3231.
static DS3231 rtc;
/// Objeto para parsear JSONs
static JSON_Parser parser;

//========================= Handles de Tareas =========================//

/// Handle para la tarea de la pantalla.
static TaskHandle_t Screen_Task_Handle;
/// Handle para la tarea general.
static TaskHandle_t General_Task_Handle;
/// Handle para conexion HTTP.
static TaskHandle_t HTTP_Task_Handle;
/// Handle para conexcion websocket.
static TaskHandle_t Websocket_Task_Handle;
/// Handle para la queue de los relays
static QueueHandle_t Relay_Queue;
/// Handle para el queue de eventos
static QueueHandle_t Event_Queue;
/// Handle para el eque de actualizacon de producto.
static QueueHandle_t Update_Queue;
/// Handle para guardar datos en la SD.
static QueueHandle_t SD_Write_Queue;
/// Handle para leer datos de la SD.
static QueueHandle_t SD_Read_Queue;
/// Handle para enviar flag de lectura de SD.
static QueueHandle_t SD_Read_Flag_Queue;
/// Mutex para acceso concurrente a la pantalla.
static SemaphoreHandle_t screen_mutex;
/// Mutex para acceso concurrente al cliente HTTP.
static SemaphoreHandle_t http_mutex;
/// Mutex para acceso concurrente al bus I2C (Wire).
SemaphoreHandle_t i2c_mutex;
/// Grupo de eventos para suspender tareas.
static EventGroupHandle_t suspend_task_group;

//========================= Variables de Configuración =========================//

/// Factor de calibración del caudalímetro.
static float k_factor = 0.250f;
/// Factor de corrección
static float offset = 1.017915f;
/// Precio del producto
static float price = 20.0f;
/// Brillo de la pantalla.
static uint8_t brightness = 100;
/// Valor ASCII del sensor LLS.
static uint8_t lls_ascii = 0;
/// Nivel máximo de combustible.
static uint16_t max_fuel_level = 4095;
/// Número de ticket actual.
static uint32_t ticket_number = 0;
/// Numero de usuarios registrados
static int number_of_users = 0;
/// ID del cliente.
static uint32_t client_id = 0;
/// ID del producto.
static uint32_t product_id = 0;

/// Estado del controlador de la pantalla.
static status display_status = Device::NO_DEVICE_ERROR;

/// Estado del controlador de la impresora.
static status printer_status = Device::NO_DEVICE_ERROR;

/// Contador de pulsos del caudalímetro.
static volatile uint64_t flow_counter = 0;

static bool trigger_fw_update[3] = { false, false, false };

//========================= Configuración de Red =========================//

/// Dirección MAC para Ethernet.
static uint8_t ethernet_mac_address[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

/// Dirección IP estática.
static IPAddress ip(192, 168, 137, 55);

/// Servidor DNS.
static IPAddress dns_server(8, 8, 8, 8);

/// Puerta de enlace.
static IPAddress gateway(192, 168, 137, 1);

/// Máscara de subred.
static IPAddress subnet(255, 255, 255, 0);

/// Puerto de comunicación.
static uint16_t port = 443; // Puerto para comunicación segura (HTTPS).

//========================= Variables de Estado de Periféricos =========================//

/// Estado de salida satelital.
static uint8_t sat_out = 0;
/// Estado de conexión de la impresora.
static uint8_t printer_connected = 1;
/// Nivel de combustible leído por LLS.
static uint16_t lls_fuel_level = 0;
/// Temperatura leída por LLS.
static int8_t lls_temp = 0;
/// Datos del RTC.
static uint8_t rtc_data[7] = { 0 };
/// Último día leído del RTC.
static uint8_t last_rtc_day;
/// Voltaje de la batería.
static float voltage = 12.0;
/// Estado de gabinete abierto.
static bool open_cabinet = false;
/// Cadena para almacenar el tag RFID leído.
static char str_tag[25] = "";
/// Bandera de despacho en curso.
static bool dispatching = false;
/// Bandera de calibración en curso.
static bool calibrating = false;
/// Bandera de lectura RFID realizada.
static bool rfid_read   = false;
/// Bandera de RTC listo.
static bool rtc_ready   = false;
/// Estado de la conexión del módulo Ethernet
static bool ethernet_begin = false;
/// Bandera para enviar evento de batería baja.
static bool low_battery_event_flag = false;
/// Bandera para envar evento de gabinete abierto.
static bool open_cabinet_event_flag = false;
/// Bandera para enviar evento de impresora desconectada.
static bool printer_disconnected_event_flag = false;
/// Bandera para enviar evento de lector RFID desconectado.
static bool rfid_disconnected_event_flag = false;
///
bool printing = false;
/// Bandera para diferir la impresión fuera del callback I2C.
static bool pending_print = false;
/// Datos del despacho pendiente de imprimir.
static char pending_print_data[160] = "";
/// Tipo de impresión pendiente (0 = último ticket, 1 = reimpresión, 2 = ticket del día).
static int pending_print_args = 0;

#ifndef debug_mode  
  /// Punteros a los datos del RTC para el logger.
  uint8_t *log_rtc[6] = { &rtc_data[0], &rtc_data[1],  &rtc_data[2], &rtc_data[4], &rtc_data[5], &rtc_data[6] }; 
#endif

//========================= Buffers y Parámetros de Usuario/Negocio =========================//

static char mac_address[13] = "";
static char server[SERVER_SIZE] = "";
static char printer_mac_address[PRINTER_MAC_SIZE] = "";
static char business_name[BUSINESS_NAME_SIZE] = "";
static char business_address[BUSINESS_ADDR_SIZE] = "";
static char business_area[BUSINESS_AREA_SIZE] = "";
static char business_city[BUSINESS_CITY_SIZE] = "";
static char business_state[BUSINESS_STATE_SIZE] = "";
static char business_postal_code[BUSINESS_POST_SIZE] = "";
static char business_RFC[BUSINESS_RFC_SIZE] = "";
static char product[PRODUCT_SIZE] = "";

/// Arreglo de punteros a los parámetros principales.
static char *parameters[] = 
{
  server, printer_mac_address, business_name, business_address, 
  business_area, business_city, business_state, 
  business_postal_code, business_RFC, product
};

static const uint8_t parameters_sizes[] =
{
  SERVER_SIZE, PRINTER_MAC_SIZE, BUSINESS_NAME_SIZE, BUSINESS_ADDR_SIZE, BUSINESS_AREA_SIZE, 
  BUSINESS_CITY_SIZE, BUSINESS_STATE_SIZE, BUSINESS_POST_SIZE, BUSINESS_RFC_SIZE, PRODUCT_SIZE
};

static char user_names[MAX_NUMBER_OF_USERS][USER_NAME_SIZE] = { 0 };
static char passwords[MAX_NUMBER_OF_USERS][USER_PSWD_SIZE] = { 0 };
static int user_ids[MAX_NUMBER_OF_USERS];
static char *active_user = nullptr;
static char *active_password = nullptr;
static int user_index = 0;

/// Parámetros que se imprimen en el ticket.
static char *ticket_string_parameters[] =
{
  business_name, business_address, business_area, business_city, 
  business_state, business_postal_code, business_RFC, active_user
};

/// Tamaños de los strings para el ticket.
static const uint8_t ticket_string_sizes[] = 
{
  BUSINESS_NAME_SIZE, BUSINESS_ADDR_SIZE, BUSINESS_AREA_SIZE, BUSINESS_CITY_SIZE, 
  BUSINESS_STATE_SIZE, BUSINESS_POST_SIZE, BUSINESS_RFC_SIZE, USER_NAME_SIZE
};

/// Direcciones de memoria para mostrar información del ticket en pantalla.
static const uint16_t display_ticket_info_addresses[] =
{
  DISPLAY_BUSINESS_NAME_ADDRESS, DISPLAY_BUSINESS_ADDR_ADDRESS, DISPLAY_BUSINESS_CITY_ADDRESS, 
  DISPLAY_BUSINESS_STATE_ADDRESS, DISPLAY_BUSINESS_POSTAL_ADDRESS, DISPLAY_BUSINESS_RFC_ADDRESS
};

/// Información del ticket a mostrar en pantalla.
static char *display_ticket_info[] =
{
  business_name, business_address, business_city, 
  business_state, business_postal_code, business_RFC
};

/// Header personalizado para WebSocket.
static const char *websocket_identifier = "identificador";
/// Namespace del dispositivo.
static const char *socket_io_device_namespace = "/device";
/// Namespace de actualizaciones.
static const char *socket_io_update_namespace = "/actualizacion";
/// Valor del header personalizado.
static const char *device_name = (const char *)&mac_address[0];
/// Evento de actualización de producto.
static const char *product_update_event = "productoActualizado";
/// Evento de actualización de usuario.
static const char *user_update_event = "usuarioActualizado";
/// Evento de actualización de firmware.
static const char *firmware_update_event = "actualizacionFirmware";
/// Evento de estado de actualización de fimrware.
static const char *firmware_update_state_event = "resultadoActualizacionFirmware";
/// Evento de conexión de dispositivo.
static const char *device_connected_event = "deviceConnected";

/// Token de github
static char github_token[GITHUB_TOKEN_SIZE] = "";

/// Versión actual del firmware ESP32.
static char current_esp32_fw_v[10] = "";

/// Versión actual del firmware del controlador del display.
static char current_display_fw_v[10] = "";

/// Versión actual del firmware del controlador de la impresora.
static char current_printer_fw_v[10] = "";

/// Arrays para guardar los parametros de Github y las posiciones en el archivo.
static char *github_parameters[] = { github_token, current_esp32_fw_v, current_display_fw_v, current_printer_fw_v };
static const uint8_t github_param_pos[] = { GITHUB_TOKEN_POS, ESP32_FW_VERSION_POS, DISPLAY_FW_VERSION_POS, PRINTER_FW_VERSION_POS };
static const uint8_t github_param_sizes[] = { GITHUB_TOKEN_SIZE, sizeof(current_esp32_fw_v), sizeof(current_display_fw_v), sizeof(current_printer_fw_v)};

/// Ruta a los archivos de firmware.
static const char *firmware_paths[3] = { ESP32_FW_PATH, DISPLAY_FW_PATH, PRINTER_FW_PATH };

/// Endpoint para descargar los archivos de firmware desde Github.
static const char *branches_endpoint = "/repos/aortiz-sudo/firmware_repository/contents";
static const char *firmwares_endpoints[3] = { "/.pio/build/esp32_release/firmware.bin", 
                                              "/.pio/build/display_release/display.bin", 
                                              "/.pio/build/printer_release/printer.bin" };
static char api_key[72];
static char ca_cert[CA_CERT_SIZE] = { 0 };
static char data_endpoint[32] = "/api/data/";
static char product_endpoint[32] = "/api/productos";
static char tag_endpoint[32] = "/api/tag";
static char device_endpoint[32] = "/api/dispositivos";
static char users_endpoint[32] = "/api/usuarios?cliente=";



static char dispatches[MAX_NUMBER_OF_DISPATCHES][128] = { 0 };
static ticket_struct tickets[MAX_NUMBER_OF_DISPATCHES] = { 0 };
static user_json_struct users[MAX_NUMBER_OF_USERS] = {};
 
//========================= Prototipos de Funciones =========================//

/** @brief Callback del estado inicial de la pantalla. @param p_info Datos del estado. @param args Argumentos adicionales. */
void initial(uint8_t *p_info, int args);
/** @brief Valida la contraseña ingresada en el estado de inicio de sesión. @param p_password Datos con la contraseña. @param args Argumentos adicionales. */
void check_password(uint8_t *p_password, int args);
/** @brief Imprime el último ticket (callback del menú de reimpresión). @param p_last_ticket Datos del ticket. @param args Argumentos adicionales. */
void print_ticket(uint8_t *p_last_ticket, int args);
/** @brief Ejecuta la impresión del ticket actual. */
void execute_print_ticket();
/** @brief Procesa los datos de calibración del caudalímetro. @param p_calibration_data Datos de calibración. @param args Argumentos adicionales. */
void calibrate(uint8_t *p_calibration_data, int args);
/** @brief Maneja el despacho de combustible. @param p_disptached_volume Volumen despachado. @param args Argumentos adicionales. */
void dispatch(uint8_t *p_disptached_volume, int args);
/** @brief Actualiza la fecha y hora a partir de la entrada del usuario. @param p_rtc_info Datos de fecha y hora. @param args Argumentos adicionales. */
void rtc_info(uint8_t *p_rtc_info, int args);
/** @brief Actualiza el precio del producto. @param p_price_info Datos del precio. @param args Argumentos adicionales. */
void price_info(uint8_t *p_price_info, int args);
/** @brief Configura/muestra la información del negocio en el ticket. @param p_business_info Datos del negocio. @param args Argumentos adicionales. */
void show_ticket_info(uint8_t *p_business_info, int args);
/** @brief Ajusta el brillo de la pantalla. @param p_brightness Valor de brillo. @param args Argumentos adicionales. */
void brightness_info(uint8_t *p_brightness, int args);

/** @brief Obtiene y formatea la dirección MAC del módulo Bluetooth de la impresora. @param p_mac_address Cadena con la dirección MAC. */
void get_mac_address(const char *p_mac_address);
/** @brief Procesa la tecla presionada y actualiza el estado de la pantalla. @param p_key Tecla presionada. */
void manage_screen(char p_key);
/** @brief Muestra en pantalla las variables actuales del sistema. */
void show_variables();
/** @brief Actualiza la fecha y hora mostradas en la pantalla. @param p_display_write true para escribir en la pantalla. */
void update_display_rtc(bool p_display_write = true);
/** @brief Carga los parámetros generales desde la tarjeta SD. */
void config_parameters();
/** @brief Carga los usuarios registrados desde la tarjeta SD. */
void config_users();
/** @brief Lee la API key desde la tarjeta SD. */
void read_api_key();
/** @brief Lee el certificado SSL raíz (CA) desde la tarjeta SD. */
void read_ca_cert();
/** @brief Carga los parámetros de GitHub (token y versiones de firmware) desde la SD. */
void config_github();
/** @brief Lee y escribe el estado de las entradas y salidas del sistema. */
void read_write_ios();
/** @brief Controla el estado de los relevadores. @param p_relay_states Arreglo con el estado de cada relevador. */
void relay_control(uint8_t p_relay_states[3]);
/** @brief Verifica si se solicitó un reinicio del sistema. */
void check_reset();
/** @brief Verifica el estado de conexión de la impresora. */
void check_printer();
/** @brief Obtiene los datos del sensor de nivel de combustible (LLS). */
void get_lls_data();
/** @brief Obtiene los datos del lector RFID. */
void get_rfid_data();
/** @brief Construye la estructura de información para guardar datos en la SD. @param p_path Ruta del archivo. @param p_data Datos a guardar. @param p_save_type Tipo de guardado. @param p_pos Posición del dato. @param p_delete Indica si se elimina el contenido previo. */
void build_sd_info(const char *p_path, const char *p_data, save_type_t p_save_type, uint8_t p_pos = 0, bool p_delete = false);
/** @brief Guarda en la tarjeta SD los datos pendientes en la cola. */
void save_to_sd();
/** @brief Envía un evento al servidor. @param p_json Estructura con la información del evento. @return true si se envió correctamente. */
bool send_data(json_event_struct *p_json);
/** @brief Envía una cadena JSON de evento al servidor. @param p_json Cadena JSON a enviar. @return true si se envió correctamente. */
bool send_data(const char *p_json);
/** @brief Envía una actualización al servidor. @param p_json Estructura con la información de la actualización. @return true si se envió correctamente. */
bool update_data(json_update_struct *p_json);
/** @brief Envía una actualización JSON a un endpoint específico. @param p_json Cadena JSON a enviar. @param p_endpoint Endpoint destino. @return true si se envió correctamente. */
bool update_data(const char *p_json, const char *p_endpoint);
/** @brief Genera y encola un evento del sistema. @param p_event Tipo de evento. @param p_use_password Indica si se incluye la contraseña. @param p_quantity Cantidad asociada al evento. @param p_use_tag Indica si se incluye el tag RFID. */
void trigger_event(json_event p_event, bool p_use_password = false, float p_quantity = 0, bool p_use_tag = false);
/** @brief Genera y encola una actualización del sistema. @param p_update Tipo de actualización. @param p_price Nuevo precio. @param p_product Producto asociado. @param p_id Identificador de la entidad. */
void trigger_update(json_update p_update, const char *p_price, const char *p_product, uint32_t p_id);
/** @brief Revisa y procesa la cola de eventos. */
void check_events();
/** @brief Revisa y procesa la cola de actualizaciones. */
void check_updates();
/** @brief Envía los eventos pendientes guardados en la tarjeta SD. */
void send_last_events();
/** @brief Gestiona la generación y envío de eventos del sistema. */
void events();
/** @brief Obtiene la lista de usuarios desde el servidor. */
void get_users();
/** @brief Procesa el JSON de usuarios recibido del servidor. @param p_json Cadena JSON. @param p_token Tokens del JSON. @param p_token_size Número de tokens. @param p_index Índice de inicio. */
void parse_users(const char *p_json, jsmntok_t *p_token, int p_token_size, int p_index);
/** @brief Construye la cadena con la información de un despacho. @param p_dispatch_str Buffer destino. @param p_liters Litros despachados. */
void build_dispatch_string(char p_dispatch_str[160], float p_liters);
/** @brief Revisa los despachos guardados en la tarjeta SD. @param init Indica si es la inicialización. */
void check_dispatches(bool init = false);
/** @brief Copia un archivo a su ubicación de respaldo. @param p_main_path Ruta del archivo original. @param p_backup_path Ruta del respaldo. */
void copy_backup(const char *p_main_path, const char *p_backup_path);
/** @brief Construye la estructura de un ticket a partir de la cadena de despacho. @param p_ticket Estructura de ticket destino. @param p_dispatch_str Cadena con los datos del despacho. @param p_rtc Datos de fecha y hora. */
void build_ticket(ticket_struct *p_ticket, const char *p_dispatch_str, uint8_t *p_rtc);
/** @brief Configura los pines de entrada/salida del sistema. */
void config_io_pins();
/** @brief Configura el logger del sistema. */
void config_logger();
/** @brief Configura la conexión Ethernet. */
void config_ethernet();
/** @brief Verifica el enlace físico de Ethernet. @return true si el enlace está activo. */
bool check_ethernet_link();
/** @brief Crea e inicializa las tareas de FreeRTOS. */
void config_tasks();
/** @brief Lee y procesa el core dump almacenado tras un reinicio inesperado. */
void read_core_dump();
/** @brief Verifica si hay actualizaciones de firmware disponibles. */
void check_firmware_updates();
/** @brief Revisa los disparadores de actualización de firmware. */
void check_firmware_triggers();
/** @brief Descarga un firmware desde un endpoint y lo guarda en un archivo. @param p_file Archivo destino. @param p_endpoint Endpoint de descarga. @return true si la descarga fue exitosa. */
bool download_firmware(File *p_file, const char *p_endpoint);
/** @brief Actualiza el firmware de la ESP32. @param p_file Archivo del firmware. @param p_file_size Tamaño del archivo. @return true si la actualización fue exitosa. */
bool update_esp32(File *p_file, size_t p_file_size);
/** @brief Quema un firmware a un dispositivo I2C (pantalla o impresora). @param device Dispositivo I2C destino. @param p_path Ruta del archivo de firmware. @return true si la operación fue exitosa. */
bool burn_firmware(I2C_Master_Device *device, const char *p_path);
/** @brief Informa al servidor el estado de la actualización de firmware. @param p_state Estado de la actualización. */
void send_update_state(bool p_state);

/* ========================= Funciones callback para eventos de Socket IO  =========================*/

/** @brief Callback de Socket.IO para actualizar un usuario. @param p_payload Carga útil con los datos del usuario. */
void update_user(const char *p_payload);
/** @brief Callback de Socket.IO para actualizar un producto. @param p_payload Carga útil con los datos del producto. */
void update_product(const char *p_payload);
/** @brief Callback de Socket.IO para actualizar el firmware. @param p_payload Carga útil con la información del firmware. */
void update_firmware(const char *p_payload);
/** @brief Callback de Socket.IO ejecutado cuando el dispositivo se conecta. @param payload Carga útil del evento de conexión. */
void device_connected(const char *payload);

/* ========================= Funciones constructoras  =========================*/

/** @brief Construye y configura el dispositivo LLS. @param p_builder Constructor del dispositivo. @return Puntero al dispositivo construido. */
Device *build_lls_device(Device_Builder *p_builder);
/** @brief Construye y configura el dispositivo de pantalla. @param p_builder Constructor del dispositivo. @return Puntero al dispositivo construido. */
Device *build_screen_device(Device_Builder *p_builder);
/** @brief Construye y configura el lector RFID MT124. @param p_builder Constructor del dispositivo. @return Puntero al dispositivo construido. */
Device *build_mt124_device(Device_Builder *p_builder);
/** @brief Construye y configura el dispositivo de periféricos (impresora). @param p_builder Constructor del dispositivo. @return Puntero al dispositivo construido. */
Device *build_peripehals_device(Device_Builder *p_builder);
/** @brief Construye y configura el cliente HTTP. @param p_builder Constructor del cliente. @return Puntero al cliente construido. */
Global_Client *build_http_client(Global_Client_Builder *p_builder);
/** @brief Construye y configura el cliente Socket.IO. @param p_builder Constructor del cliente. @return Puntero al cliente construido. */
Global_Client *build_socketio_client(Global_Client_Builder *p_builder);

//========================= Arreglo de Funciones de Estado =========================//

state_callback_function state_callback_functions[14]
{
  initial, check_password, nullptr, nullptr, nullptr, print_ticket,
  nullptr, calibrate, nullptr, dispatch, rtc_info, price_info,
  show_ticket_info, brightness_info
};

//========================= Implementación de Funciones =========================//

/**
 * @brief ISR para el caudalímetro 1.
 */
void IRAM_ATTR isr_flowmeter_pin_1()
{
  //portENTER_CRITICAL_ISR(&flow_counter_spinlock);
  if(dispatching || calibrating)
    flow_counter++;
  //portEXIT_CRITICAL_ISR(&flow_counter_spinlock);
}

/**
 * @brief ISR para el caudalímetro 2.
 */
void IRAM_ATTR isr_flowmeter_pin_2()
{
  //portENTER_CRITICAL_ISR(&flow_counter_spinlock);
  if(dispatching || calibrating)
    flow_counter++;
  //portEXIT_CRITICAL_ISR(&flow_counter_spinlock);
}

/**
 * @brief ISR para el caudalímetro 3.
 */
/*void IRAM_ATTR isr_flowmeter_pin_3()
{
  if(dispatching || calibrating)
    flow_counter++;
}*/

/**
 * @brief Tarea general del sistema: gestiona sensores, periféricos, eventos y E/S.
 * @param pvParameters Parámetros de la tarea (no utilizados).
 */
void General_Task(void *pvParameters)
{
  pinMode(FLOWMETER_PIN_1, INPUT);
  pinMode(FLOWMETER_PIN_2, INPUT);
  //pinMode(FLOWMETER_PIN_3, INPUT);

  attachInterrupt(digitalPinToInterrupt(FLOWMETER_PIN_1), isr_flowmeter_pin_1, FALLING);
  attachInterrupt(digitalPinToInterrupt(FLOWMETER_PIN_2), isr_flowmeter_pin_2, FALLING);
  //attachInterrupt(digitalPinToInterrupt(FLOWMETER_PIN_3), isr_flowmeter_pin_3, FALLING);

  MT124_Builder rfid_builder;
  LLS_Builder lls_builder;

  rfid_device = (MT124 *)build_mt124_device(&rfid_builder);
  lls_device = (LLS *)build_lls_device(&lls_builder);

  while(!rtc_ready) { vTaskDelay(1 / portTICK_PERIOD_MS); }

  check_dispatches(true);

  while(true)
  {
    EventBits_t trigger_bit = xEventGroupGetBits(suspend_task_group);

    if(trigger_bit & TRIGGER_BIT)
    {
      logger.log("Suspendiendo tarea: ");
      logger.logln(pcTaskGetName(NULL));
      xEventGroupSetBits(suspend_task_group, GENERAL_TASK_BIT);
      vTaskSuspend(NULL);
    }

    read_write_ios();
    events();
    get_lls_data();
    get_rfid_data();
    check_dispatches();
    check_printer();
    check_reset();
    save_to_sd();

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarea encargada de la comunicación HTTP con el servidor.
 * @param pvParameters Parámetros de la tarea (no utilizados).
 */
void HTTP_Task(void *pvParameters)
{
  Global_Client_Builder http_builder;
  get_mac_address(WiFi.macAddress().c_str());
  
  size_t current_len = strlen(data_endpoint);
  size_t space_left = sizeof(data_endpoint) - current_len - 1; // -1 para el caracter nulo

  strncat(data_endpoint, device_name, space_left);
  data_endpoint[sizeof(data_endpoint) - 1] = '\0';

  logger.logln(data_endpoint);

  http_client = (Ethernet_HTTP *)build_http_client(&http_builder);

  if(ca_cert[0] != '\0')
    http_client->set_certificate(ca_cert);

  unsigned long last_millis = 0;
  unsigned long last_request_user_millis = 0;
  
  while(true)
  {
    EventBits_t trigger_bit = xEventGroupGetBits(suspend_task_group);

    if(trigger_bit & TRIGGER_BIT)
    {
      logger.log("Suspendiendo tarea: ");
      logger.logln(pcTaskGetName(NULL));
      xEventGroupSetBits(suspend_task_group, HTTP_TASK_BIT);
      vTaskSuspend(NULL);
    }

    unsigned long current_millis = millis();

    check_events();
    check_updates();

    if(current_millis - last_request_user_millis > 90000)
    {
      get_users();

      last_request_user_millis = current_millis;
    }

    if(current_millis - last_millis > 60000)
    {
      xSemaphoreTake(http_mutex, portMAX_DELAY);
      check_ethernet_link();
      xSemaphoreGive(http_mutex);

      send_last_events();

      last_millis = current_millis;
    }

    /*if(rtc_data[4] == 4 && rtc_data[5] == 0)
      check_firmware_updates();*/

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarea encargada de la comunicación WebSocket/Socket.IO con el servidor.
 * @param pvParameters Parámetros de la tarea (no utilizados).
 */
void Websocket_Task(void *pvParameters)
{
  Global_Client_Builder websocket_builder;
  websocket_client = (Ethernet_SocketIO *)build_socketio_client(&websocket_builder);

  websocket_client->set_event(user_update_event, update_user);
  websocket_client->set_event(product_update_event, update_product);
  websocket_client->set_event(firmware_update_event, update_firmware);
  websocket_client->set_identifer(websocket_identifier, device_name);
  int device_namespace_index = websocket_client->set_namespace(socket_io_device_namespace);
  
  while(!ethernet_begin) { vTaskDelay(1000 / portTICK_PERIOD_MS); }

  xSemaphoreTake(http_mutex, portMAX_DELAY);
  if(websocket_client->start_websocket_client())
    websocket_client->connect_to_namespace(device_namespace_index);
  xSemaphoreGive(http_mutex);

  unsigned long last_reconnect_millis = 0;

  while(true)
  {
    EventBits_t trigger_bit = xEventGroupGetBits(suspend_task_group);

    if(trigger_bit & TRIGGER_BIT)
    {
      logger.log("Suspendiendo tarea: ");
      logger.logln(pcTaskGetName(NULL));
      xEventGroupSetBits(suspend_task_group, WS_TASK_BIT);
      vTaskSuspend(NULL);
    }

    if(!ethernet_begin)
    {
      unsigned long current_millis = millis();
      if(current_millis - last_reconnect_millis > 60000)
      {
        xSemaphoreTake(http_mutex, portMAX_DELAY);
        check_ethernet_link();
        xSemaphoreGive(http_mutex);
        last_reconnect_millis = current_millis;
      }
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    xSemaphoreTake(http_mutex, portMAX_DELAY);
    websocket_client->handle_message();
    xSemaphoreGive(http_mutex);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Tarea encargada de gestionar la pantalla y la interacción con el teclado.
 * @param pvParameters Parámetros de la tarea (no utilizados).
 */
void Screen_Task(void *pvParameters)
{
  xSemaphoreTake(screen_mutex, portMAX_DELAY);
  Screen_Builder screen_builder;
  Printer_Builder printer_builder;

  screen = (Screen *)build_screen_device(&screen_builder);
  printer = (Printer *)build_peripehals_device(&printer_builder);

  const int rows = 4;
  const int cols = 4;

  char keys[rows][cols]
  {
    {   '1', '2' ,'3',     UP  },
    {   '4', '5' ,'6',    DOWN },
    {   '7', '8' ,'9',   CLEAR },
    { LOCK , '0' , BACK, ENTER }
  };

  uint8_t row_pins[rows] = {7, 6, 5, 4};
  uint8_t col_pins[cols] = {0, 1, 2, 3};

  Keypad_I2C keypad = Keypad_I2C(makeKeymap(keys), row_pins, col_pins, rows, cols, KEYPAD_ADDR);
  keypad.begin();

  for(int i = 0; i < 14; i++)
    screen->set_sate_callback(state_callback_functions[i], (screen_state)i);

  screen->init(brightness);
  screen->set_price(price);
  screen->set_product((const char *)product);

  update_display_rtc();

  last_rtc_day = rtc_data[RTC_DAY_INDEX];

  xSemaphoreGive(screen_mutex);

  rtc_ready = true;

  while(true)
  {
    char key = keypad.getKey();

    static unsigned long last_rtc_update = 0;
    unsigned long now = millis();
    if(now - last_rtc_update >= 500)
    {
      update_display_rtc();
      last_rtc_update = now;
    }

    manage_screen(key);

    if(pending_print)
    {
      pending_print = false;
      execute_print_ticket();
    }

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Función de configuración inicial de Arduino. Inicializa hardware, parámetros y tareas.
 */
void setup()
{
  config_io_pins();
  //Wire.setBufferSize(256);
  Wire.begin(SDA_PIN, SCL_PIN);
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  vTaskDelay(1000 / portTICK_PERIOD_MS);

  update_display_rtc(false);
  config_logger();
  config_parameters();
  config_users();
  read_api_key();
  read_ca_cert();
  config_github();

  for(int i = 0; i < 128; i++)
  {
    Wire.beginTransmission(i);

    if(Wire.endTransmission() == 0)
    {
      logger.log("0x");

      if(i < 0x10)
        logger.log("0");
      
      logger.logln(i, HEX);
    }
  }

  config_ethernet();



  read_core_dump();
  config_tasks();

  vTaskDelete(NULL);
}

/**
 * @brief Bucle principal de Arduino. La lógica se ejecuta en las tareas de FreeRTOS.
 */
void loop()
{
  vTaskDelay(10 / portTICK_PERIOD_MS);
}

/**
 * @brief Estado inicial del sistema.
 * 
 * Esta función se encarga de configurar el estado inicial del sistema, incluyendo
 * la limpieza de la contraseña ingresada y la configuración del usuario por defecto.
 * 
 * @param p_info  Puntero a la información del estado.
 * @param args    Argumentos adicionales.
 */
void initial(uint8_t *p_info, int args)
{
  active_user = nullptr;
  active_password = nullptr;
}

/**
 * @brief Verifica la contraseña ingresada por el usuario.
 * 
 * Compara la contraseña ingresada con las contraseñas almacenadas en los parámetros
 * del sistema. Si la contraseña es correcta, se cambia el estado de la pantalla al menú
 * correspondiente. Si es incorrecta, se muestra un mensaje de error en la pantalla.
 * 
 * @param p_password  Puntero a la contraseña ingresada.
 * @param args        Argumentos adicionales.
 */
void check_password(uint8_t *p_password, int args)
{
  bool correct_password = false;

  for(int i = 0; i < number_of_users; i++)
  {
    if(strcmp((const char *)p_password, passwords[i]) == 0)
    {
      screen_state next_state = screen->get_calibration_pin_state() ? MENU_CAL_SCREEN_STATE : MENU_SCREEN_STATE;
      screen->set_transition(true, next_state);
      correct_password = true;

      active_user = user_names[i];
      active_password = passwords[i];

      user_index = i;
      
      trigger_event(LOG_IN, true);

      logger.logln(active_user);
    }
  }

  if(!correct_password)
  {
    const char *text = "Clave invalida";
    logger.logln(text);
    screen->write_to_address(DISPLAY_WRONG_PASS_ADDRESS, (uint8_t *)text, strlen(text));
  }
}

/**
 * @brief Imprime el ticket de despacho.
 * 
 * Esta función se encarga de imprimir el ticket del día o el correspondiente al último despacho
 * realizado. Si no hay despachos registrados, se muestra un mensaje indicando que no
 * se han realizado despachos.
 * 
 * @param p_data Puntero a los datos del ticket.
 * @param args   Argumentos adicionales que determinan el tipo de impresión.
 *               0: Imprimir último ticket (datos en p_data).
 *               1: Imprimir último ticket (obtener datos del sistema).
 *               2: Imprimir ticket del día (obtener todos los despachos del día).
 */
void print_ticket(uint8_t *p_data, int args)
{
  if(args == 0 && p_data)
  {
    strncpy(pending_print_data, (const char *)p_data, sizeof(pending_print_data) - 1);
    pending_print_data[sizeof(pending_print_data) - 1] = '\0';
  }

  pending_print_args = args;
  pending_print = true;
}

void execute_print_ticket()
{
  logger.log("printer_connected: ");
  logger.logln(printer_connected);

  if(printer_connected == 0)
  {
    logger.logln("Impresora no conectada, omitiendo impresion.");
    return;
  }

  int args = pending_print_args;

  if(args == 0 || args == 1)
  {
    char last_dispatch[128] = "";

    bool print_continue = true;
    if(args == 0)
      strncpy(last_dispatch, pending_print_data, sizeof(last_dispatch));
    else if(args == 1)
      print_continue = get_last_dispatch(last_dispatch);

    last_dispatch[sizeof(last_dispatch) - 1] = '\0';

    if(print_continue == 1)
    {
      logger.logln("Imprimiendo ultimo ticket...");

      ticket_struct ticket;
      uint8_t rtc_array[7];
      logger.logln(last_dispatch);

      build_ticket(&ticket, last_dispatch, rtc_array);

      printing = true;
      printer->print_last_ticket(&ticket);
      printing = false;
    }
    else if(print_continue == 0)
      logger.logln("No se han realizado despachos");
    else if(print_continue < 0)
      return;
  }
  else if(args == 2)
  {
    int lines = get_number_of_lines(DISPATCHES_PATH);
    logger.logln(lines);

    if(lines < 0)
    {
      logger.logln("No se pudieron leer los despachos.");
      return;
    }

    logger.logln("Imprimiendo ticket del dia...");

    if(lines > 0)
    {
      char date[16];
      snprintf(date, sizeof(date), "%u;%u;%u", rtc_data[RTC_YEAR_INDEX], rtc_data[RTC_MONTH_INDEX], rtc_data[RTC_DAY_INDEX]);

      date[sizeof(date) - 1] = '\0';

      if(!get_dispatches(dispatches, lines))
      {
        logger.logln("No se pudieron leer los despachos.");
        return;
      }

      size_t counter = 0;
      for(int i = 0; i < lines; i++)
      {
        if(string_contains(dispatches[i], date))
          counter++;
      }

      if(counter > 0)
      {
        int ticket_counter = 0;

        for(int i = 0; i < lines; i++)
        {
          if(string_contains(dispatches[i], date))
            build_ticket(&tickets[ticket_counter++], dispatches[i], rtc_data);
        }

        printing = true;
        printer->print_day_ticket(tickets, counter);
        printing = false;
      }
      else
        logger.logln("No se han hecho despachos este día.");
    }
  }
}

/**
 * @brief Función de calibración del caudalímetro.
 * 
 * Esta función permite calibrar el caudalímetro del sistema mediante el volúmen
 * de combustible medido.
 * 
 * @param p_calibration_data  Puntero a los datos de calibración.
 * @param args                Que accion realizar. 
 *                            0: Obtener volumen. 
 *                            1: Calibrar con volumen dado.
 */
void calibrate(uint8_t *p_calibration_data, int args)
{
  float *volume = (float *)p_calibration_data;

  switch(args)
  {
    case 0:
    {  
      uint8_t current_relay_states[3] = { RELAY_ON, RELAY_ON, RELAY_ON };
      xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10));
      break;
    }

    case 1:
    {
      *volume = flow_counter * k_factor;
      break;
    }

    case 2:
    {
      uint8_t current_relay_states[3] = { RELAY_OFF, RELAY_OFF, RELAY_OFF };
      xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10));

      if(*volume <= 0.0f)
        return;

      k_factor = (*volume) / (float)(flow_counter);
      logger.logln(k_factor, 6);

      char k_factor_str[15];
      uint32_t k_factor_int = (uint32_t)k_factor;
      uint32_t k_factor_dec = (uint32_t)((k_factor - k_factor_int) * 1000000.0f);
      snprintf(k_factor_str, sizeof(k_factor_str), "%u.%06u", k_factor_int, k_factor_dec);
      k_factor_str[sizeof(k_factor_str) - 1] = '\0';

      build_sd_info(PARAMETERS_PATH, k_factor_str, SAVE_GENERAL_PARAMETER, K_FACTOR_POS);

      flow_counter = 0;
      break;
    }

    case 3:
    {
      uint8_t current_relay_states[3] = { RELAY_OFF, RELAY_OFF, RELAY_OFF };
      xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10));
      break;
    }
  }
}

/**
 * @brief Despacha combustible y genera el ticket correspondiente.
 * 
 * Esta función se encarga de realizar el despacho de combustible, ya sea solo leyendo
 * el volumen despachado o realizando todo el proceso de despacho y generación de ticket.
 * 
 * @param p_dispatch_volume Puntero al volumen despachado.
 * @param args              Acción a realizar:
 *                          0: Iniciar despacho (enciende los relés) y leer el volumen inicial.
 *                          1: Leer el volumen despachado.
 *                          2: Detener el despacho (apaga los relés).
 *                          3: Finalizar el despacho: incrementa el folio, genera el evento,
 *                             guarda en SD, imprime el ticket, persiste el folio y reinicia el contador de pulsos.
 *                          4: Reducir el caudal (patrón de relés intermedio) y leer el volumen.
 */
void dispatch(uint8_t *p_dispatch_volume, int args)
{
  switch(args)
  {
    case 0:
    {
      uint8_t current_relay_states[3] = { RELAY_ON, RELAY_ON, RELAY_ON };
      if(xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10)) == pdTRUE)
      {
        logger.logln("Despacho inicializado");
        float *dispatched_volume = (float *)p_dispatch_volume;
        *dispatched_volume = flow_counter * k_factor * offset;
      }
      break;
    }

    case 1:
    {
      float *dispatched_volume = (float *)p_dispatch_volume;
      *dispatched_volume = flow_counter * k_factor * offset;
      break;
    }

    case 2:
    {
      uint8_t current_relay_states[3] = { RELAY_OFF, RELAY_OFF, RELAY_OFF };
      if(xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10)) == pdTRUE)
          logger.logln("Despacho detenido");
      break;
    }

    case 3:
    {
      uint8_t current_relay_states[3] = { RELAY_OFF, RELAY_OFF, RELAY_OFF };
      float *dispatched_volume = (float *)p_dispatch_volume;
      float liters = *dispatched_volume;

      if(xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10)) == pdTRUE)
        logger.logln("Despacho detenido");

      ticket_number++;

      trigger_event(FUEL_DISPATCHED, true, liters, screen->get_tag_detected());
      char str_dispatch[160];
      build_dispatch_string(str_dispatch, liters);

      build_sd_info(DISPATCHES_PATH, str_dispatch, SAVE_DISPATCH);

      print_ticket((uint8_t *)str_dispatch, 0);
  
      char str_ticket_number[10];
      snprintf(str_ticket_number, sizeof(str_ticket_number), "%u", ticket_number);
      str_ticket_number[sizeof(str_ticket_number) - 1] = '\0';
      build_sd_info(PARAMETERS_PATH, str_ticket_number, SAVE_GENERAL_PARAMETER, TICKET_NUMBER_POS);

      flow_counter = 0;
      break;
    }

    case 4:
    {
      float *dispatched_volume = (float *)p_dispatch_volume;
      *dispatched_volume = flow_counter * k_factor * offset;

      uint8_t current_relay_states[3] = { RELAY_ON, RELAY_OFF, RELAY_ON };
      xQueueSend(Relay_Queue, (void *)&current_relay_states[0], pdMS_TO_TICKS(10));
      break;
    }
  }
}

/**
 * @brief Actualiza la información del RTC en el sistema.
 * 
 * Esta función se encarga de leer los datos del RTC y actualizar la hora, fecha y
 * demás parámetros en el sistema.
 * 
 * @param p_rtc_info  Puntero a la información del RTC.
 * @param args        Argumentos adicionales.
 */
void rtc_info(uint8_t *p_rtc_info, int args)
{
  rtc.setSecond(0);
  rtc.setHour(p_rtc_info[0] & 0xFF);
  rtc.setMinute(p_rtc_info[1] & 0xFF);
  rtc.setDate(p_rtc_info[2] & 0xFF);
  rtc.setMonth(p_rtc_info[3] & 0xFF);
  rtc.setYear(p_rtc_info[4] & 0xFF);

  update_display_rtc();
}

/**
 * @brief Actualiza el precio del producto en el sistema.
 * 
 * Esta función permite modificar el precio del producto actual y guarda el cambio
 * en los parámetros del sistema.
 * 
 * @param p_price_info  Puntero a la información del precio.
 * @param args          Argumentos adicionales.
 */
void price_info(uint8_t *p_price_info, int args)
{
  if(args == 1)
  {
    float new_price = *(float *)p_price_info;
    price = new_price;
    char price_str[10];
    uint8_t price_int = (uint8_t)new_price;
    uint8_t price_dec = (uint8_t)((new_price - price_int) * 100);

    snprintf(price_str, sizeof(price_str), "%u.%u", price_int, price_dec);
    price_str[sizeof(price_str) - 1] = '\0';

    build_sd_info(PARAMETERS_PATH, price_str, SAVE_GENERAL_PARAMETER, PRICE_POS);

    trigger_update(PRICE_UPDATE, price_str, product, product_id);
  }
}

/**
 * @brief Muestra la información del ticket en la pantalla.
 * 
 * Esta función se encarga de mostrar en la pantalla los datos relevantes del ticket,
 * como el nombre del negocio, dirección, RFC, entre otros.
 * 
 * @param p_business_info Puntero a la información del negocio.
 * @param args            Argumentos adicionales.
 */
void show_ticket_info(uint8_t *p_business_info, int args)
{
  if(args == 1)
  {
    for(int i = 0; i < 6; i++)
    {
      display_status = screen->write_to_address(display_ticket_info_addresses[i], (uint8_t *)display_ticket_info[i], strlen(display_ticket_info[i]));
      logger.log(i + 1);
      logger.log(". ");
      logger.logln(display_ticket_info[i]);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
  else if(args == 2)
  {
    const char *text = "                                       ";
    for(int i = 0; i < 6; i++)
    {
      display_status = screen->write_to_address(display_ticket_info_addresses[i], (uint8_t *)text, strlen(text));
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
  }
}

/**
 * @brief Actualiza el brillo de la pantalla.
 * 
 * Esta función permite modificar el brillo de la pantalla y guarda el cambio en los
 * parámetros del sistema.
 * 
 * @param p_brightness  Puntero al valor de brillo.
 * @param args          Argumentos adicionales.
 */
void brightness_info(uint8_t *p_brightness, int args)
{
  brightness = *p_brightness;

  display_status = screen->set_brightness(brightness, true);

  char brightness_str[5];
  snprintf(brightness_str, sizeof(brightness_str), "%u\0", brightness);

  brightness_str[sizeof(brightness_str) - 1] = '\0';

  build_sd_info(PARAMETERS_PATH, brightness_str, SAVE_GENERAL_PARAMETER, BRIGHTNESS_POS);
}

/**
 * @brief Obtiene la dirección MAC del dispositivo.
 * 
 * Esta función se encarga de leer y almacenar la dirección MAC del dispositivo en
 * el formato adecuado para su uso en el sistema.
 * 
 * @param p_mac_address Puntero a la dirección MAC en formato de texto.
 */
void get_mac_address(const char *p_mac_address)
{
  char *ptr = &mac_address[0];
  do
  {
    if(*p_mac_address != ':')
      *ptr++ = *p_mac_address;
  } 
  while (*p_mac_address++);

  *ptr = '\0';

  logger.logln(mac_address);
}

/**
 * @brief Maneja la transición entre estados de la pantalla.
 * 
 * Esta función se encarga de gestionar el cambio de estados en la pantalla principal,
 * así como de reiniciar el temporizador de inactividad si se ha presionado una tecla.
 * 
 * @param p_key Carácter de la tecla presionada.
 */
void manage_screen(char p_key)
{
  unsigned long current_millis = millis();
  static unsigned long last_millis = 0;
  static uint8_t lock_dim_level = 0;
  static bool busy_dimmed = false;

  screen_state current_screen_state = screen->get_screen_state();
  bool in_lock = (current_screen_state == INITIAL_SCREEN_STATE);
  bool in_busy = (current_screen_state == DISPATCH_SCREEN_STATE
                  || current_screen_state == CALIBRATION_SCREEN_STATE);

  if(p_key)
  {
    last_millis = current_millis;

    if(in_lock && lock_dim_level != 0)
    {
      screen->set_brightness(screen->get_brightness(), false);
      lock_dim_level = 0;
    }

    if(in_busy && busy_dimmed)
    {
      screen->set_brightness(screen->get_brightness(), false);
      busy_dimmed = false;
    }
  }

  if(!in_lock
     && current_screen_state != DISPATCH_SCREEN_STATE
     && current_screen_state != CALIBRATION_SCREEN_STATE
     && current_millis - last_millis > 300000UL)
  {
    screen->set_transition(true, INITIAL_SCREEN_STATE);
    last_millis = current_millis;
    lock_dim_level = 0;
  }

  if(in_lock)
  {
    unsigned long lock_elapsed = current_millis - last_millis;

    if(lock_dim_level == 0 && lock_elapsed > 60000UL)
    {
      screen->set_brightness(10, false);
      lock_dim_level = 1;
    }
    else if(lock_dim_level == 1 && lock_elapsed > 300000UL)
    {
      screen->set_brightness(0, false);
      lock_dim_level = 2;
    }
  }
  else if(lock_dim_level != 0)
  {
    screen->set_brightness(screen->get_brightness(), false);
    lock_dim_level = 0;
  }

  if(in_busy)
  {
    if(!busy_dimmed
       && current_millis - last_millis > 120000UL
       && screen->get_brightness() > 20)
    {
      screen->set_brightness(20, false);
      busy_dimmed = true;
    }
  }
  else if(busy_dimmed)
  {
    screen->set_brightness(screen->get_brightness(), false);
    busy_dimmed = false;
  }

  printer_status = printer->read_peripehal_data(&voltage, &sat_out, &printer_connected);

  static unsigned long last_variables_update = 0;
  unsigned long vars_now = millis();
  if(vars_now - last_variables_update >= 500)
  {
    show_variables();
    last_variables_update = vars_now;
  }

  if(current_screen_state == INITIAL_SCREEN_STATE)
    check_firmware_triggers();

  display_status = screen->update(p_key);

  /* if(printer_status != Printer::NO_DEVICE_ERROR)
    printer->start_device();

  if(display_status != Screen::NO_DEVICE_ERROR)
    screen->start_device(); */
}

/**
 * @brief Muestra el voltaje de la batería en la pantalla.
 * 
 * Esta función se encarga de mostrar el voltaje actual de la batería en la dirección
 * de memoria correspondiente en la pantalla.
 * 
 * @param p_voltage Voltaje a mostrar.
 */
void show_variables()
{
  int16_t int_voltage = (int16_t)(voltage * 10);
  int16_t fuel_temp = (int16_t)lls_temp;
  int16_t fuel_level = map(lls_fuel_level, 0, max_fuel_level, 0, 100);

  screen->write_to_address(DISPLAY_VOLTAGE_VALUE_ADDRESS, (uint8_t *)&int_voltage, sizeof(int_voltage), true);
  screen->write_to_address(DISPLAY_FUEL_TEMP_ADDRESS, (uint8_t *)&fuel_temp, sizeof(fuel_temp), true);
  screen->write_to_address(DISPLAY_FUEL_LEVEL_ADDRESS, (uint8_t *)&fuel_level, sizeof(fuel_level), true);
}

/**
 * @brief Actualiza la información del RTC en la pantalla.
 * 
 * Esta función lee los datos del RTC y los muestra en la pantalla en el formato
 * correspondiente.
 * 
 * @param p_display_write Indica si se debe escribir en la pantalla.
 */
void update_display_rtc(bool p_display_write)
{
  bool century = false;
  bool h12 = false;
  bool pm = false;
  
  rtc_data[RTC_YEAR_INDEX] = rtc.getYear();
  rtc_data[RTC_MONTH_INDEX] = rtc.getMonth(century);
  rtc_data[RTC_DAY_INDEX] = rtc.getDate();
  rtc_data[RTC_HOUR_INDEX] = rtc.getHour(h12, pm);
  rtc_data[RTC_MINUTE_INDEX] = rtc.getMinute();
  rtc_data[RTC_SECOND_INDEX] = rtc.getSecond();

  if(p_display_write)
    display_status = screen->write_to_address(DISPLAY_RTC_ADDRESS, rtc_data, sizeof(rtc_data));
}

/**
 * @brief Configura los parámetros del sistema desde la tarjeta SD.
 * 
 * Esta función se encarga de leer los parámetros almacenados en la tarjeta SD y
 * configurarlos en el sistema. Si no se pueden leer los parámetros, se configuran
 * valores por defecto.
 */
void config_parameters()
{
  int lines = get_number_of_lines(PARAMETERS_PATH);

  if(lines != 21)
  {
    copy_backup(PARAMETERS_PATH, PARAMETERS_BACKUP_PATH);
    lines = get_number_of_lines(PARAMETERS_PATH);
  }

  char sd_parameters[21][128];

  if(read_file(PARAMETERS_PATH, sd_parameters, lines, '\n'))
  {
    for(int i = 0; i < 10; i++)
    {
      memset(parameters[i], '\0', parameters_sizes[i]);
      strncpy((char *)parameters[i], (const char *)sd_parameters[i], parameters_sizes[i] - 1);
      parameters[i][parameters_sizes[i] - 1] = '\0';
    }

    number_of_users = atoi((const char *)sd_parameters[NUMBER_OF_USERS_POS]);
    ticket_number = atoi((const char *)sd_parameters[TICKET_NUMBER_POS]);
    max_fuel_level = atoi((const char *)sd_parameters[MAX_FUEL_LEVEL_POS]);
    price = atof((const char *)sd_parameters[PRICE_POS]);
    brightness = atoi((const char *)sd_parameters[BRIGHTNESS_POS]);
    k_factor = atof((const char *)sd_parameters[K_FACTOR_POS]);
    lls_ascii = atoi((const char *)sd_parameters[LLS_ASCII_POS]);
    port = atoi((const char *)sd_parameters[PORT_POS]);
    offset = atof((const char *)sd_parameters[OFFSET_POS]);
    client_id = atoi((const char *)sd_parameters[CLIENT_ID_POS]);
    product_id = atoi((const char *)sd_parameters[PRODUCT_ID_POS]);
  }

  for(int i = 0; i < 10; i++)
    logger.logln(parameters[i]);

  logger.logln(number_of_users);
  logger.logln(ticket_number);
  logger.logln(max_fuel_level);
  logger.logln(price);
  logger.logln(brightness);
  logger.logln(k_factor, 6);
  logger.logln(lls_ascii);
  logger.logln(port);
  logger.logln(offset, 6);
  logger.logln(client_id);
  logger.logln(product_id);
}

void config_users()
{
  int lines = get_number_of_lines(USERS_PATH);

  if(lines != number_of_users && !SD.exists(USERS_PATH))
  { 
    strncpy(user_names[0], "Usuario temporal", USER_NAME_SIZE);
    strncpy(passwords[0], "1234", USER_PSWD_SIZE);
    user_ids[0] = -1;

    return;
  }
  else if(SD.exists(USERS_PATH) && lines != number_of_users)
  {
    copy_backup(USERS_PATH, USERS_BACKUP_PATH);
    lines = get_number_of_lines(USERS_PATH);
  }

  char sd_user_lines[MAX_NUMBER_OF_USERS][128];

  if(read_file(USERS_PATH, sd_user_lines, lines, '\n'))
  {
    for(int i = 0; i < lines; i++)
    {
      if(i > MAX_NUMBER_OF_USERS - 1)
        break;

      char user_line[3][64];
      split_string(sd_user_lines[i], user_line, 3, ',');
      user_ids[i] = atoi(user_line[0]);
      strncpy(passwords[i], user_line[1], USER_PSWD_SIZE);
      strncpy(user_names[i], user_line[2], USER_NAME_SIZE);

      logger.log(user_ids[i]);
      logger.log(": ");
      logger.log(passwords[i]);
      logger.log(", ");
      logger.logln(user_names[i]);
    }
  }
}

/**
 * 
 * 
 */
void read_api_key()
{
  int lines = get_number_of_lines(API_KEY_PATH);

  if(lines != 1)
  {
    copy_backup(API_KEY_PATH, API_KEY_BACKUP_PATH);
    lines = get_number_of_lines(API_KEY_PATH);
  }

  char sd_line[1][128];

  if(read_file(API_KEY_PATH, sd_line, 1, '\n'))
  {
    strncpy(api_key, sd_line[0], sizeof(api_key));
    logger.logln(api_key);
  }
}

/**
 * @brief Lee el certificado SSL raíz desde la tarjeta SD.
 */
void read_ca_cert()
{
  File cert_file = SD.open(CA_CERT_PATH, FILE_READ);

  if(!cert_file)
  {
    logger.logln("No se encontro el certificado SSL en la SD.");
    return;
  }

  size_t file_size = cert_file.size();

  if(file_size == 0 || file_size >= CA_CERT_SIZE)
  {
    logger.logln("Certificado SSL invalido o excede el buffer.");
    cert_file.close();
    return;
  }

  cert_file.readBytes(ca_cert, file_size);
  ca_cert[file_size] = '\0';
  cert_file.close();

  logger.logln("Certificado SSL cargado desde SD.");
}

/**
 *
 *
 */
void config_github()
{
  int lines = get_number_of_lines(GITHUB_PARAMETERS_PATH);

  if(lines != 4)
  {
    copy_backup(GITHUB_PARAMETERS_PATH, GITHUB_PARAMETERS_BACKUP_PATH);
    lines = get_number_of_lines(GITHUB_PARAMETERS_PATH);
  }

  char gh_parameters[4][128];

  if(read_file(GITHUB_PARAMETERS_PATH, gh_parameters, lines, '\n'))
  {
    for(int i = 0; i < 4; i++)
    {
      strncpy((char *)github_parameters[i], (const char *)gh_parameters[i], github_param_sizes[i] - 1);
      github_parameters[i][github_param_sizes[i] - 1] = '\0';
      logger.logln(github_parameters[i]);
    }
  }
}

/**
 * @brief Lee y escribe en los pines de entrada/salida del sistema.
 * 
 * Esta función se encarga de leer el estado de los pines configurados como entrada y
 * escribir en aquellos configurados como salida. Controla el estado del relé según
 * el modo de operación (despacho o calibración).
 */
void read_write_ios()
{
  open_cabinet = false;
  screen->set_cabinet_state(open_cabinet);
  dispatching = screen->get_dispatching();
  calibrating = screen->get_calibrating();

  uint8_t current_relay_states[3]; 
  if(xQueueReceive(Relay_Queue, (void *)&current_relay_states[0], 0) == pdTRUE)
    relay_control(current_relay_states);
}

/**
 * @brief Controla el estado de los relés del sistema.
 * 
 * Esta función permite activar o desactivar los relés del sistema, controlando así
 * el flujo de combustible y otras funciones de los periféricos.
 * 
 * @param p_relay_states Estado deseado para los relés (activo/inactivo).
 */
void relay_control(uint8_t p_relay_states[3])
{
  digitalWrite(RELAY_PIN_1, p_relay_states[0]);
  digitalWrite(RELAY_PIN_2, p_relay_states[1]);
  digitalWrite(RELAY_PIN_3, p_relay_states[2]);
}

/**
 * 
 * 
 */
void check_reset()
{
  if(rtc_data[RTC_HOUR_INDEX] == 3 && rtc_data[RTC_MINUTE_INDEX] == 0 && rtc_data[RTC_SECOND_INDEX] > 0 && rtc_data[RTC_SECOND_INDEX] < 5)
    ESP.restart();
}

/**
 * 
 * 
 */
void check_printer()
{
  unsigned long current_millis = millis();
  static unsigned long last_millis = 0;
  if(printer_connected == 0 && current_millis - last_millis > 30000 && !dispatching && !calibrating)
  {
    logger.logln("Conectando a impresora...");
    printer->reset_printer();
    last_millis = current_millis;
  }
}

/**
 * 
 * 
 */
void get_lls_data()
{
  unsigned long current_millis = millis();
  static unsigned long last_millis = 0;
  static bool get_first = false;

  if(current_millis - last_millis > 3600000 || !get_first)
  {
    status l_status = lls_device->read_lls_data(0x01);
    json_event event;
    
    if(l_status == LLS::NO_DEVICE_ERROR)
    {
      lls_fuel_level = lls_device->get_fuel_level(0x01);
      lls_temp = lls_device->get_temperature(0x01);

      logger.log("Fuel: ");
      logger.logln(lls_fuel_level);

      logger.log("Temp: ");
      logger.logln(lls_temp);
      event = FUEL_LEVEL;
    }
    else if(l_status == LLS::TIMEOUT_ERROR)
    {
      lls_fuel_level = 0;
      lls_temp = 0;

      event = FUEL_SENSOR_DISCONNECTED;
    }

    trigger_event(event);

    get_first = true;
    last_millis = current_millis;
  }
}

/**
 * 
 * 
 */
void get_rfid_data()
{
  unsigned long current_millis = millis();
  static unsigned long last_millis = 0;

  screen_state current_state = screen->get_screen_state();
  uint8_t counter = screen->get_state() ? screen->get_state_counter() : 0;

  unsigned long rfid_request_time = (!screen->get_dispatching() && current_state == DISPATCH_SCREEN_STATE && counter < 2) ? 1000 : 600000;

  if(current_state != DISPATCH_SCREEN_STATE || (current_state == DISPATCH_SCREEN_STATE && counter == 2))
    rfid_read = false;

  if(current_millis - last_millis > rfid_request_time && !screen->get_dispatching() && !rfid_read)
  {
    uint8_t *tag_data;
    status rfid_status = rfid_device->get_tag_data(&tag_data, RFID_READER_ADDR);
    
    if(rfid_status == MT124::READ_TAG_SUCCESSFULLY && (current_state == DISPATCH_SCREEN_STATE && counter != 2))
    {
      screen->set_tag_detected(true);

      rfid_read = true;
      rfid_disconnected_event_flag = false;  // Reiniciar bandera de desconexión al leer una etiqueta exitosamente

      uint32_t int_tag = (tag_data[0] << 16) | (tag_data[1] << 8) | tag_data[2];
      snprintf(str_tag, sizeof(str_tag), "\"%010u\"", int_tag);

      str_tag[sizeof(str_tag) - 1] = '\0';

      logger.log("Tag: ");
      logger.logln(str_tag);
    }
    else if(rfid_status == MT124::TIMEOUT_ERROR && !rfid_disconnected_event_flag)
    {
      trigger_event(RFID_READER_DISCONNECTED);
      rfid_disconnected_event_flag = true;
    }

    last_millis = current_millis;
  }
}

void build_sd_info(const char *p_path, const char *p_data, save_type_t p_type, uint8_t p_pos, bool p_delete)
{
  sd_info_struct sd_info{};
  memset(&sd_info, 0, sizeof(sd_info));
  strncpy(sd_info.data, p_data, sizeof(sd_info.data));
  sd_info.data[sizeof(sd_info.data) - 1] = '\0';
  strncpy(sd_info.path, p_path, sizeof(sd_info.path));
  sd_info.path[sizeof(sd_info.path) - 1] = '\0';
  sd_info.save_type = p_type;
  sd_info.position = p_pos;
  sd_info.delete_info = p_delete;

  if(xQueueSend(SD_Write_Queue, &sd_info, pdMS_TO_TICKS(10)) != pdTRUE)
    logger.logln("No se pudo guardar la información en la tarjeta SD.");
}

void save_to_sd()
{
  sd_info_struct sd_info;
  if(xQueueReceive(SD_Write_Queue, &sd_info, 0) == pdTRUE)
  {
    switch(sd_info.save_type)
    {
      case SAVE_EVENT:
        save_event(sd_info.data);
      break;

      case SAVE_DISPATCH:
        save_dispatch(sd_info.data, sd_info.delete_info);
      break;

      case SAVE_GENERAL_PARAMETER:
      case SAVE_GITHUB_PARAMETER:
      case SAVE_API_KEY:
      case SAVE_USER:
        const char *ptr = sd_info.data;
        save_parameter(sd_info.path, &ptr, &sd_info.position, 1);
      break;
    }
  }
}

/**
 * @brief Envía datos de evento al servidor.
 * 
 * Esta función se encarga de enviar los datos de un evento al servidor a través de
 * HTTP. Si el envío falla, intenta guardar el evento para un envío posterior.
 * 
 * @param p_json  Puntero a la estructura de evento JSON a enviar.
 * @return        true: El envío fue exitoso.
 * @return        false: El envío falló y se guardó el evento localmente.
 */
bool send_data(json_event_struct *p_json)
{
  char json[MAX_EVENT_JSON_LENGTH]  = "";
  int length = snprintf(json, sizeof(json), "[{\"fecha\":\"20%02u-%02u-%02uT%02u:%02u:%02u\",\"idEvento\":%u,\"folio\":%s,\"nivel\":%u,\"temperatura\":%d,\"energia\":%u.%u,\"abierto\":%u,\"clave\":%s,\"idUsuario\":%s,\"cantidad\":%s,\"tag\":%s}]", 
                        p_json->year, p_json->month, p_json->day, p_json->hour, p_json->minute, p_json->second, p_json->event, p_json->ticket_number, p_json->fuel_level, p_json->fuel_temp, p_json->int_battery_level, p_json->dec_battery_level, p_json->open, 
                        p_json->password, p_json->user_id, p_json->quantity_dispatched, p_json->tag);
  
  json[sizeof(json) - 1] = '\0';

  logger.logln(json);
  
  xSemaphoreTake(http_mutex, portMAX_DELAY);
  bool send = send_data(json);
  xSemaphoreGive(http_mutex);

  http_client->flush_headers();

  if(!send)
    build_sd_info(EVENTS_PATH, json, SAVE_EVENT);

  return send;
}

/**
 * @brief Envía datos JSON al servidor a través de HTTP.
 * 
 * Esta función se encarga de realizar el envío de datos JSON al servidor utilizando
 * una petición HTTP. Retorna el estado del envío.
 * 
 * @param p_json  Puntero a la cadena de datos JSON a enviar.
 * @return        true: Si el envío fue exitoso.
 * @return        false: Si el envío falló.
 */
bool send_data(const char *p_json)
{
  char length[6] = "";
  snprintf(length, sizeof(length), "%u", strlen(p_json));
  
  length[sizeof(length - 1)] = '\0';

  http_client->set_server(server);
  http_client->set_port(port);  //443
  http_client->set_endpoint(data_endpoint);
  http_client->set_secure(true);
  http_client->set_header("Host", server, 0);
  http_client->set_header("Content-Type", "application/json", 1);
  http_client->set_header("Content-Length", length, 2);
  http_client->set_header("x-api-key", api_key, 3);
  return http_client->send_data_to_server((uint8_t *)p_json, strlen(p_json)) / 200 == 1 ? true : false;
}

/**
 * @brief Actualiza los datos de un producto en el servidor.
 * 
 * Esta función se encarga de enviar los datos actualizados de un producto al servidor
 * para su modificación en la base de datos.
 * 
 * @param p_json  Puntero a la estructura de actualización JSON con los datos del producto.
 * @return        true Si la actualización fue exitosa.
 * @return        false Si la actualización falló.
 */
bool update_data(json_update_struct *p_json)
{
  char json[MAX_UPDATE_JSON_LENGTH] = "";
  snprintf(json, sizeof(json) - 1, "{\"nombre\":\"%s\",\"precio\":%s}", p_json->product, p_json->price);

  json[sizeof(json) - 1] = '\0';
  logger.logln(json);

  char endpoint[32];
  snprintf(endpoint, sizeof(endpoint), "%s/%u", product_endpoint, p_json->id);

  xSemaphoreTake(http_mutex, portMAX_DELAY);
  bool update = update_data(json, endpoint);
  xSemaphoreGive(http_mutex);

  http_client->flush_headers();

  return update;
}

/**
 * @brief Actualiza los datos de un producto en el servidor a través de HTTP.
 * 
 * Esta función envía los datos de un producto al servidor para su actualización en
 * la base de datos. Retorna el estado de la operación.
 * 
 * @param p_json  Puntero a la cadena de datos JSON con la información del producto.
 * @return        true Si la actualización fue exitosa.
 * @return        false Si la actualización falló.
 */
bool update_data(const char *p_json, const char *p_endpoint)
{
  char length[6] = "";
  snprintf(length, sizeof(length), "%u", strlen(p_json));
  length[sizeof(length - 1)] = '\0';

  http_client->set_endpoint(p_endpoint);
  http_client->set_server(server);
  http_client->set_port(port);  
  http_client->set_header("Host", server, 0);
  http_client->set_header("Content-Type", "application/json", 1);
  http_client->set_header("Content-Length", length, 2);
  http_client->set_header("x-api-key", api_key, 3);
  http_client->set_secure(true);
  return http_client->update_data((uint8_t *)p_json, strlen(p_json) + 1) / 200 == 1 ? true : false;
}

/**
 * @brief Dispara un evento en el sistema.
 * 
 * Esta función se encarga de preparar y disparar un evento en el sistema, llenando
 * los datos necesarios como fecha, hora, nivel de combustible, entre otros.
 * 
 * @param p_event     Evento a disparar.
 * @param p_password  Contraseña del usuario (opcional).
 * @param p_quantity  Cantidad despachada (opcional).
 */
void trigger_event(json_event p_event, bool p_use_password, float p_quantity, bool p_use_tag)
{
  json_event_struct json_struct;
  static char password[7] = "";
  static const char *null_text = "null";
  memset(password, 0x00, 7);

  json_struct.fuel_level = lls_fuel_level;
  json_struct.fuel_temp = lls_temp;
  json_struct.int_battery_level = (uint8_t)(voltage);
  json_struct.dec_battery_level = (uint8_t)((voltage - json_struct.int_battery_level) * 100);
  json_struct.year = rtc_data[RTC_YEAR_INDEX];
  json_struct.month = rtc_data[RTC_MONTH_INDEX];
  json_struct.day = rtc_data[RTC_DAY_INDEX];
  json_struct.hour = rtc_data[RTC_HOUR_INDEX];
  json_struct.minute = rtc_data[RTC_MINUTE_INDEX];
  json_struct.second = rtc_data[RTC_SECOND_INDEX];
  json_struct.event = p_event;
  json_struct.open = open_cabinet;

  if(p_event == FUEL_DISPATCHED)
    snprintf(json_struct.ticket_number, sizeof(json_struct.ticket_number), "\"%u\"", ticket_number);
  else
    strncpy(json_struct.ticket_number, null_text, sizeof(json_struct.ticket_number));

  if(p_use_password)
    snprintf(password, sizeof(password), "\"%s\"", active_password);

  password[6] = '\0';

  json_struct.tag = p_use_tag ? &str_tag[0] : (char *)null_text;
  json_struct.password = p_use_password ? &password[0] : (char *)null_text;
  
  if(p_use_password)
    snprintf(json_struct.user_id, sizeof(json_struct.user_id), "%u", user_ids[user_index]);
  else
    strncpy(json_struct.user_id, null_text, sizeof(json_struct.user_id));
  
  if(p_quantity != 0)
  {
    uint32_t int_quantity = (uint32_t)(p_quantity);
    uint32_t dec_quantity = (uint32_t)((p_quantity - int_quantity) * 100);

    snprintf(json_struct.quantity_dispatched, sizeof(json_struct.quantity_dispatched), "%u.%02u", int_quantity, dec_quantity);
  }
  else 
    strncpy(json_struct.quantity_dispatched, null_text, sizeof(json_struct.quantity_dispatched));

  if(xQueueSend(Event_Queue, (void *)&json_struct, pdMS_TO_TICKS(10)) == pdTRUE)
    logger.logln("Enviando evento...");
}

/**
 * @brief Dispara una actualización en el sistema.
 * 
 * Esta función se encarga de preparar y disparar una actualización de precio en el
 * sistema, llenando los datos necesarios como el nuevo precio y el producto afectado.
 * 
 * @param p_update  Actualización a disparar.
 * @param p_price   Nuevo precio del producto.
 * @param p_product Producto afectado por la actualización.
 */
void trigger_update(json_update p_update, const char *p_price, const char *p_product, uint32_t p_id)
{
  json_update_struct json_struct;

  json_struct.update = p_update;
  json_struct.id = p_id;
  json_struct.product = p_product;
  strncpy(json_struct.price, p_price, sizeof(json_struct.price));

  json_struct.price[sizeof(json_struct.price) - 1] = '\0';

  xQueueSend(Update_Queue, (void *)&json_struct, pdMS_TO_TICKS(10));
}

/**
 * @brief Verifica y ejecuta los disparadores de eventos y actualizaciones.
 * 
 * Esta función se encarga de comprobar si hay eventos o actualizaciones pendientes
 * de ser enviados y los procesa en consecuencia.
 */
void check_events()
{
  json_event_struct json_struct;

  if(!ethernet_begin)
  {
    if(uxQueueSpacesAvailable(Event_Queue) == 0)
    {
      if(xQueueReceive(Event_Queue, (void *)&json_struct, 0) == pdTRUE)
        send_data(&json_struct);
    }
    return;
  }

  if(xQueueReceive(Event_Queue, (void *)&json_struct, 0) == pdTRUE)
    send_data(&json_struct);
}

void check_updates()
{
  json_update_struct json_struct;

  if(!ethernet_begin)
    return;

  if(xQueueReceive(Update_Queue, (void *)&json_struct, 0) == pdTRUE)
    update_data(&json_struct);  
}

/**
 * @brief Envía los últimos eventos registrados en la cola al servidor.
 * 
 * Esta función se encarga de enviar en un solo paquete los últimos eventos registrados
 * en la cola de eventos del sistema. Intenta borrar los eventos enviados de la cola
 * para liberar espacio.
 */
void send_last_events()
{
  if(!ethernet_begin)
    return;

  size_t n_events = 0;
  char events[10][MAX_EVENT_JSON_LENGTH] = { 0 };

  int length = get_events(events, &n_events);

  if(length == 0)
  {
    logger.logln("No hay eventos en la cola.");
    http_client->flush_headers();
    return;
  }

  logger.logln("Enviando ultimos eventos...");

  if(length > 0)
  {
    char json[MAX_EVENT_JSON_LENGTH*10];
    memset(json, 0x00, sizeof(json));

    logger.logln(n_events);
    size_t used = 0;

    for(int i = 0; i < n_events; i++)
    {
      size_t to_copy = strlen(events[i]);

      if(used + to_copy >= sizeof(json))
      {
        logger.logln("No hay espacio suficiente en el buffer.");
        memset(json, 0x00, sizeof(json));
        return;
      }

      memcpy(json + used, events[i], to_copy);
      used += to_copy;
    }

    json[sizeof(json) - 1] = '\0';
    
    logger.logln(json);
    
    xSemaphoreTake(http_mutex, portMAX_DELAY);
    bool state = send_data(json);
    xSemaphoreGive(http_mutex);
    http_client->flush_headers();

    if(!state)
    {
      logger.log("No se pudieron enviar los ultimos "); 
      logger.log(n_events);
      logger.logln(" eventos.");
      return;
    }

    logger.log("Ultimos ");
    logger.log(n_events);
    logger.logln(" eventos enviados.");

    if(!erase_events(n_events))
      logger.logln("No se pudieron eliminar los eventos.");
  }
}

/**
 * @brief Procesa los eventos del sistema y activa los disparadores correspondientes.
 * 
 * Esta función se encarga de verificar el estado de los sensores y periféricos del
 * sistema y dispara los eventos correspondientes en base a la lógica de negocio.
 */
void events()
{
  if(open_cabinet && !open_cabinet_event_flag)
  {
    trigger_event(OPEN_CABINET);
    open_cabinet_event_flag = true;
  }
  else if(!open_cabinet)
    open_cabinet_event_flag = false;

  if(voltage < 12.0 && !printing && !low_battery_event_flag)
  {
    trigger_event(LOW_BATTERY);
    low_battery_event_flag = true;
  }
  else if(voltage >= 12.0)
    low_battery_event_flag = false;

  if(printer_connected == 0 && !printer_disconnected_event_flag)
  {
    trigger_event(PRINTER_DISCONNECTED);
    printer_disconnected_event_flag = true;
  }
  else if(printer_connected == 1)
    printer_disconnected_event_flag = false;
}

void get_users()
{
  if(!ethernet_begin)
    return;

  logger.logln("Obteniendo usuarios...");
  uint8_t data[2400];
  memset(data, 0x00, sizeof(data));

  char endpoint[48] = "";
  snprintf(endpoint, sizeof(endpoint), "%s%u&listado=true", users_endpoint, client_id);

  http_client->set_server(server);
  http_client->set_endpoint(endpoint);
  http_client->set_port(port);
  http_client->set_secure(true);
  http_client->set_header("Host", server, 0);
  http_client->set_header("Content-Type", "application/json", 1);
  http_client->set_header("x-api-key", api_key, 2);

  const char *json = nullptr;

  xSemaphoreTake(http_mutex, portMAX_DELAY);
  if(http_client->get_data_from_server(data, sizeof(data)) > 0)
    json = (const char *)&data[0];
  xSemaphoreGive(http_mutex);

  http_client->flush_headers();

  if(!json)
    return;

  int tokens = parser.parse_json(json);

  int index = 0;
  if((index = parser.json_find_key_in_object(json, 0, "data")) > 0)
  {
    jsmntok_t *data_token = parser.get_token(index);
    
    if(!data_token)
      return;

    logger.log("Numero de datos en \"data\": ");
    logger.logln(data_token->size);

    parse_users(json, data_token, data_token->size, index);
  }
  else
    logger.logln("No se encontró la clave");
}

void parse_users(const char *p_json, jsmntok_t *p_token, int p_token_size, int p_index)
{
  if(p_token_size > MAX_NUMBER_OF_USERS)
  {
    logger.logln("El número de usuarios es mayor que el del buffer permitido.");
    logger.logln("Habrá perdidad de datos.");
    p_token_size = MAX_NUMBER_OF_USERS;
  }

  if(p_token->type != JSMN_ARRAY)
    return;
  
  int current_index = p_index;

  logger.logln("Usuarios disponibles: ");
  for(int i = 0; i < p_token_size; i++)
  {
    current_index = parser.json_find_object(p_json, p_index, i);
    jsmntok_t *token = parser.get_token(current_index);

    if(!token)
      continue;
    
    parser.parse_user(p_json, current_index, &users[i]);

    if(users[i].client.id != client_id)
      continue;

    char *profile_name_lwr = strlwr(users[i].profile.name);
    bool is_profile = !(strcmp(profile_name_lwr, "despachador") && strcmp(profile_name_lwr, "supervisor") && strcmp(profile_name_lwr, "administrador"));

    if(!is_profile)
      continue;

    logger.log(users[i].name);
    logger.log(" ");
    logger.logln(users[i].last_name);
  }
}

/**
 * @brief Construye la cadena de despacho para el ticket.
 * 
 * Esta función se encarga de crear la cadena que será impresa en el ticket de despacho,
 * incluyendo todos los datos relevantes del mismo como fecha, hora, litros despachados,
 * precio, entre otros.
 * 
 * @param p_dispatch_str  Puntero a la cadena de despacho a construir.
 * @param p_liters        Volumen de litros despachados.
 */
void build_dispatch_string(char p_dispatch_str[160], float p_liters)
{
  float total_amount = p_liters * price;
  float IVA = total_amount * 0.16f;
  float total = total_amount + IVA;

  uint8_t year = rtc_data[RTC_YEAR_INDEX];
  uint8_t month = rtc_data[RTC_MONTH_INDEX];
  uint8_t day = rtc_data[RTC_DAY_INDEX];
  uint8_t hour = rtc_data[RTC_HOUR_INDEX];
  uint8_t minute = rtc_data[RTC_MINUTE_INDEX];
  uint8_t seconds = rtc_data[RTC_SECOND_INDEX];
  uint16_t int_liters = (uint16_t)p_liters;
  uint16_t dec_liters = (uint16_t)((p_liters - int_liters) * 1000);
  uint16_t int_iva = (uint16_t)IVA;
  uint16_t dec_iva = (uint16_t)((IVA - int_iva) * 1000);
  uint16_t int_price = (uint16_t)price;
  uint16_t dec_price = (uint16_t)((price - int_price) * 1000);
  uint16_t int_total_amount = (uint16_t)total_amount;
  uint16_t dec_total_amount = (uint16_t)((total_amount - int_total_amount) * 1000);
  uint16_t int_total = (uint16_t)total;
  uint16_t dec_total = (uint16_t)((total - int_total) * 1000);

  snprintf(p_dispatch_str, 160,"%u;%u;%u;0;%u;%u;%u;%u.%03u;%u.%03u;%u.%03u;%u.%03u;%u.%03u;%u;%u!", 
                          year, month, day, hour, minute, seconds, int_liters, dec_liters, int_price, dec_price,
                          int_total_amount, dec_total_amount, int_iva, dec_iva, int_total, dec_total, ticket_number, user_ids[user_index]);

  p_dispatch_str[159] = '\0';
}

/**
 * @brief Verifica y borra despachos antiguos según la fecha.
 * 
 * Esta función se encarga de comprobar si el día actual es diferente al último día
 * con despachos registrados. Si es así, borra los despachos del día anterior y guarda
 * el último despacho como referencia.
 * 
 * @param init Indica si es una inicialización o una verificación normal.
 */
void check_dispatches(bool init)
{
  if(last_rtc_day != rtc_data[RTC_DAY_INDEX] && !init)
  {
    logger.logln("Borrando los despachos del día");
    char last_dispatch[128];

    if(get_last_dispatch(last_dispatch))
    {
      logger.logln(last_dispatch);
      save_dispatch(last_dispatch, true);
    }
    else
      logger.logln("No se pudo guardar el despacho.");

    copy_backup(PARAMETERS_BACKUP_PATH, PARAMETERS_PATH);
    copy_backup(GITHUB_PARAMETERS_BACKUP_PATH, GITHUB_PARAMETERS_PATH);
    copy_backup(USERS_PATH, USERS_BACKUP_PATH);

    last_rtc_day = rtc_data[RTC_DAY_INDEX];
  }

  if(!init)
    return;
  
  int lines = get_number_of_lines(DISPATCHES_PATH);
  logger.logln(lines);

  if(lines <= 0)
  {
    logger.logln("No hay despachos guardados.");
    return;
  }

  logger.logln("Borrando los despachos que no son del día...");

  get_dispatches(dispatches, lines);
  char date[16] = "";

  snprintf(date, sizeof(date),"%u;%u;%u", rtc_data[RTC_YEAR_INDEX], rtc_data[RTC_MONTH_INDEX], rtc_data[RTC_DAY_INDEX]);
  date[sizeof(date) - 1] = '\0';

  int counter = 0;

  for(int i = 0; i < lines; i++)
  {
    if(!string_contains(dispatches[i], date))
    {
      logger.logln(dispatches[i]);
      counter++;
    }
  }

  logger.log("Numero de despachos: ");
  logger.logln(counter);

  if(counter >= lines)
  {
    char last_dispatch[128];
    get_last_dispatch(last_dispatch);
    save_dispatch(last_dispatch, true);
    return;
  }
  
  int index = lines - counter;
  bool delete_dispatches = true;

  for(int i = index; i < lines; i++)
  {
    save_dispatch(dispatches[i], delete_dispatches);
    delete_dispatches = false;
  }
}

void copy_backup(const char *p_main_path, const char *p_backup_path)
{
  File backup = SD.open(p_backup_path, FILE_READ);
  File main = SD.open(p_main_path, FILE_WRITE);

  if(!main || !backup)
  {
    logger.logln("No se pudo abrir el archivo.");
    return;
  }

  while(backup.available())
  {
    char c = backup.read();
    main.print(c);
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }

  main.close();
  backup.close();
}

/**
 * @brief Construye la estructura del ticket a imprimir.
 * 
 * Esta función se encarga de llenar la estructura del ticket con los datos correspondientes
 * para su impresión, incluyendo la información del negocio, usuario, producto y los datos
 * del despacho.
 * 
 * @param p_ticket        Puntero a la estructura del ticket a llenar.
 * @param p_dispatch_str  Cadena con los datos del despacho.
 * @param p_rtc           Puntero a un arreglo con los datos del RTC.
 */
void build_ticket(ticket_struct *p_ticket, const char *p_dispatch_str, uint8_t *p_rtc)
{
  char dispatch_data[14][64] = { 0 };

  split_string(p_dispatch_str, dispatch_data, 14, ';');

  float float_data[5] = { 0 };

  for(int i = 0; i < 7; i++)
    p_rtc[i] = atoi(dispatch_data[i]);
      
  for(int i = 0; i < 5; i++)
    float_data[i] = atof(dispatch_data[i + 7]);

  uint32_t ticket_num = atoi(dispatch_data[12]);
  uint8_t usr_index = atoi(dispatch_data[13]);

  p_ticket->business_name = strlen(business_name) > 2 ? business_name : nullptr;
  p_ticket->business_address = strlen(business_address) > 2 ? business_address : nullptr;
  p_ticket->business_area = strlen(business_area) > 2 ? business_area : nullptr;
  p_ticket->business_city = strlen(business_city) > 2 ? business_city : nullptr;
  p_ticket->business_state = strlen(business_state) > 2 ? business_state : nullptr;
  p_ticket->business_postal_code = strlen(business_postal_code) > 2 ? business_postal_code : nullptr;
  p_ticket->business_RFC = strlen(business_RFC) > 2 ? business_RFC : nullptr;
  p_ticket->user_name = user_names[usr_index];
  p_ticket->product = &product[0];
  p_ticket->rtc_array = p_rtc;
  p_ticket->liters = float_data[0];
  p_ticket->price = float_data[1];
  p_ticket->total_amount = float_data[2];
  p_ticket->IVA = float_data[3];
  p_ticket->total = float_data[4];
  p_ticket->ticket_number = ticket_num;
}

/**
 * @brief Configura los pines de entrada/salida del sistema.
 * 
 * Esta función se encarga de inicializar y configurar los pines utilizados por el sistema
 * para el correcto funcionamiento de los periféricos y sensores conectados.
 */
void config_io_pins()
{
  pinMode(CALIBRATION_PIN, INPUT);
  pinMode(CABINET_PIN, INPUT);
  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  pinMode(RELAY_PIN_3, OUTPUT);

  uint8_t relay_states[3] = { RELAY_OFF, RELAY_OFF, RELAY_OFF };
  relay_control(relay_states);
}

/**
 * @brief Configura el logger del sistema.
 * 
 * Esta función se encarga de inicializar el logger del sistema, configurando el uso de
 * la tarjeta SD para el almacenamiento de logs si está disponible.
 */
void config_logger()
{
  #ifndef debug_mode
    logger.set_rtc(log_rtc);
  #endif

  unsigned long time = millis();
  bool sd_mounted = false;
  while(!sd_mounted && millis() - time < 10000)
    sd_mounted = SD.begin(CS_SD_CARD);

  if(!sd_mounted)
  {
    SD.end();
    vTaskDelay(200 / portTICK_PERIOD_MS);
    SPI.end();
    vTaskDelay(200 / portTICK_PERIOD_MS);
    SPI.begin();
    vTaskDelay(200 / portTICK_PERIOD_MS);
    SD.begin(CS_SD_CARD);
  }
  else
    #ifdef debug_mode
      logger.begin();
    #else
      logger.begin(false);
    #endif
}

/**
 * @brief Configura la conexión Ethernet del dispositivo.
 * 
 * Esta función se encarga de inicializar y configurar la conexión Ethernet, estableciendo
 * una dirección IP estática y los parámetros de red necesarios para la comunicación.
 */
void config_ethernet()
{
  Ethernet.init(CS_W5500);

  if(Ethernet.linkStatus() != LinkON)
  {
    logger.logln("Cable Ethernet no detectado.");
    return;
  }

  if((ethernet_begin = Ethernet.begin(ethernet_mac_address)) == 0)
  {
    logger.logln("No se pudo configurar DHCP.");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
  else
  {
    logger.log("Ethernet configurado correctamente con IP: ");
    logger.logln(Ethernet.localIP());
    logger.log("Mascara de subred: ");
    logger.logln(Ethernet.subnetMask());
    logger.log("Puerta de enlace: ");
    logger.logln(Ethernet.gatewayIP());
    logger.log("DNS primario: ");
    logger.logln(Ethernet.dnsServerIP());
  }
}

/**
 * @brief Verifica el estado del enlace Ethernet y reinicia DHCP si es necesario.
 *
 * Verifica si hay cable conectado fisicamente al modulo W5500. Si hay enlace
 * pero no hay IP asignada, intenta obtener una por DHCP.
 *
 * @return true  Si hay enlace fisico y DHCP configurado.
 * @return false Si no hay cable o DHCP fallo.
 */
bool check_ethernet_link()
{
  if(Ethernet.linkStatus() != LinkON)
  {
    if(ethernet_begin)
      logger.logln("Cable Ethernet desconectado.");

    ethernet_begin = false;
    return false;
  }

  if(!ethernet_begin)
  {
    logger.logln("Cable Ethernet detectado. Configurando DHCP...");
    ethernet_begin = Ethernet.begin(ethernet_mac_address) != 0;

    if(ethernet_begin)
    {
      logger.log("IP asignada: ");
      logger.logln(Ethernet.localIP());
    }
    else
      logger.logln("No se pudo configurar DHCP.");
  }

  return ethernet_begin;
}

/**
 * @brief Configura las tareas del sistema.
 *
 * Esta función se encarga de crear y configurar las tareas que serán ejecutadas en
 * paralelo por el sistema, asignando los recursos y prioridades necesarias para su
 * correcto funcionamiento.
 */
void config_tasks()
{
  http_mutex = xSemaphoreCreateMutex();
  screen_mutex = xSemaphoreCreateMutex();
  i2c_mutex = xSemaphoreCreateMutex();

  Relay_Queue = xQueueCreate(5, 3);
  Event_Queue = xQueueCreate(10, sizeof(json_event_struct));
  Update_Queue = xQueueCreate(5, sizeof(json_update_struct));
  SD_Write_Queue = xQueueCreate(10, sizeof(sd_info_struct));
  SD_Read_Queue = xQueueCreate(5, sizeof(sd_info_struct));
  SD_Read_Flag_Queue = xQueueCreate(3, sizeof(bool));

  suspend_task_group = xEventGroupCreate();

  xTaskCreatePinnedToCore(Screen_Task, "Screen", 18432, NULL, 3, &Screen_Task_Handle, 1);
 
  xSemaphoreTake(screen_mutex, portMAX_DELAY);
  xTaskCreatePinnedToCore(General_Task, "General", 11264, NULL, 2, &General_Task_Handle, 0);
  xTaskCreatePinnedToCore(HTTP_Task, "HTTP", 20480, NULL, 3, &HTTP_Task_Handle, 0);
  xTaskCreatePinnedToCore(Websocket_Task, "Websocket", 16384, NULL, 3, &Websocket_Task_Handle, 0);
  xSemaphoreGive(screen_mutex);
}

/*Funciones callback para lectura de datos por medio de websockets*/
/**
 * @brief Actualiza los datos de un usuario en el sistema.
 * 
 * Esta función es llamada cuando se recibe un mensaje por WebSocket con los nuevos
 * datos de un usuario. Actualiza la contraseña y nombre del usuario en los parámetros
 * del sistema.
 * 
 * @param p_payload Puntero a la carga útil del mensaje WebSocket con los datos del usuario.
 */
void update_user(const char *p_payload)
{
  char keys[4][32] = { 0 };
  char values[4][32] = { 0 };

  parse_json(p_payload, keys, values, 4);

  char str_user_id[32] = "";
  char first_name[32] = "";
  char last_name[32] = "";
  char password[32] = "";

  get_json_key_value(keys, values, "id", str_user_id, 4);
  get_json_key_value(keys, values, "clave", password, 4);
  get_json_key_value(keys, values, "nombre", first_name, 4);
  get_json_key_value(keys, values, "apellido", last_name, 4);
  int user_id = atoi(str_user_id);
  logger.logln(str_user_id);
  uint8_t index = 0;
  bool index_found = false;
  for(int i = 0; i < number_of_users; i++)
  {
    if(user_ids[i] == user_id)
    {
      index = i;
      index_found = true;
      break;
    }
  }

  if(!index_found)
  {
    number_of_users++;
    index = number_of_users - 1;
    char n_user_str[4];
    snprintf(n_user_str, sizeof(n_user_str), "%d", number_of_users);
    n_user_str[sizeof(n_user_str) - 1] = '\0';
    
    build_sd_info(PARAMETERS_PATH, n_user_str, SAVE_GENERAL_PARAMETER, NUMBER_OF_USERS_POS);
  }
  
  char user_info[64];
  snprintf(user_info, sizeof(user_info), "%d,%s,%s %s", user_id, password, first_name, last_name);
  user_info[sizeof(user_info) - 1] = '\0';

  build_sd_info(USERS_PATH, user_info, SAVE_USER, index);

}

/**
 * @brief Actualiza el precio de un producto en el sistema.
 * 
 * Esta función es llamada cuando se recibe un mensaje por WebSocket con el nuevo
 * precio de un producto. Actualiza el precio en los parámetros del sistema y en la
 * pantalla del dispensador.
 * 
 * @param p_payload Puntero a la carga útil del mensaje WebSocket con los datos del producto.
 */
void update_product(const char *p_payload)
{
  char keys[3][32] = { 0 };
  char values[3][32] = { 0 };

  parse_json(p_payload, keys, values, 3);

  char str_price[32] = "";
  char str_product[32] = "";
  char str_product_id[32] = "";

  if(get_json_key_value(keys, values, "precio", str_price, 3))
  {
    price = atof(str_price);
    screen->set_price(price);
  }

  if(get_json_key_value(keys, values, "nombre", str_product, 3))
    strncpy(product, (const char *)str_product, sizeof(product));

  get_json_key_value(keys, values, "id", str_product_id, 3);

  uint8_t param_pos[3] = {  PRODUCT_POS, PRICE_POS, PRODUCT_ID_POS };
  const char *params[3] = { (const char *)str_product, (const char *)str_price, (const char *)str_product_id };

  for(int i = 0; i < 3; i++)
    build_sd_info(PARAMETERS_PATH, params[i], SAVE_GENERAL_PARAMETER, param_pos[i]);
}

/**
 * @brief Actualiza el firmware del dispositivo.
 * 
 * Esta función es llamada cuando se recibe un mensaje por WebSocket con la versión
 * del firmware a actualizar. Descarga el firmware desde GitHub y lo guarda en la tarjeta SD.
 * 
 * \param p_payload Puntero a la carga útil del mensaje WebSocket con los datos del firmware.
 */
void update_firmware(const char *p_payload)
{
  char keys[32] = "";
  char values[32] = "";

  parse_json(p_payload, &keys, &values, 1);

  char version[32] = "";
  get_json_key_value(&keys, &values, "version", version, 1);

  logger.logln(version);

  char endpoint[128] = "";
  snprintf(endpoint, sizeof(endpoint), "%s%s?ref=%s", branches_endpoint, firmwares_endpoints[0], version);
  endpoint[sizeof(endpoint) - 1] = '\0';
  logger.logln(endpoint);
  char auth_value[64] = "";
  snprintf(auth_value, sizeof(auth_value),"Bearer %s", github_token);
  auth_value[sizeof(auth_value) - 1] = '\0';

  xSemaphoreTake(http_mutex, portMAX_DELAY);
  http_client->set_port(443);
  http_client->set_server("140.82.112.6");
  http_client->set_secure(true);
  http_client->set_header("Host", "api.github.com", 0);
  http_client->set_header("Accept", "application/vnd.github.raw", 1);
  http_client->set_header("Authorization", (const char *)auth_value, 2);
  http_client->set_header("User-Agent", "desarrolloidt", 3);
  http_client->set_header("X-GitHub-Api-Version", "2022-11-28", 4);

  File file = SD.open(ESP32_FW_PATH, FILE_WRITE);

  if(!file)
  {
    http_client->flush_headers();
    logger.logln("No se pudo abrir el archivo.");
    file.close();
    http_client->client_stop();
    xSemaphoreGive(http_mutex);

    send_update_state(false);
    return;
  }

  trigger_fw_update[0] = download_firmware(&file, endpoint);

  file.close();

  http_client->flush_headers();

  xSemaphoreGive(http_mutex);

  if(!trigger_fw_update[0])
    return;

  strncpy((char *)github_parameters[1], (const char *)version, github_param_sizes[1]);
  strncpy(current_esp32_fw_v, (const char *)version, sizeof(current_esp32_fw_v));
  github_parameters[1][github_param_sizes[1] - 1] = '\0';
  const char *param[] = { github_parameters[1] };
  uint8_t pos = github_param_pos[1];
  save_parameter(GITHUB_PARAMETERS_PATH, param, &pos, 1);
}

void device_connected(const char *p_payload)
{
  
}

/**
 * @brief Lee y guarda el core dump del sistema en la tarjeta SD.
 * 
 * Esta función verifica si existe un core dump en la memoria flash del dispositivo. Si está disponible,
 * reserva memoria para leerlo, lo guarda en la tarjeta SD y posteriormente borra el core dump de la memoria flash.
 * Si no hay core dump o ocurre algún error durante el proceso, se registra el error en el logger.
 * 
 * Pasos principales:
 * - Obtiene la dirección y tamaño del core dump con `esp_core_dump_image_get`.
 * - Si existe, lee el contenido desde la memoria flash con `spi_flash_read`.
 * - Guarda el core dump en la tarjeta SD en el archivo definido por `CORE_DUMP_LOG_PATH`.
 * - Borra el core dump de la memoria flash con `esp_core_dump_image_erase`.
 * - Libera la memoria reservada.
 * 
 * Consideraciones:
 * - Si no se puede reservar memoria, abrir el archivo o leer el core dump, se registra el error y se aborta el proceso.
 * - El core dump se guarda omitiendo los primeros 20 bytes (cabecera).
 */
void read_core_dump()
{
  size_t address = 0;
  size_t size = 0;

  esp_err_t err = esp_core_dump_image_get(&address, &size);

  if(err != ESP_OK || size == 0)
  {
    logger.logln("No hay core dump en la memoria flash.");

    return;
  }

  if(address == 0)  
  {
    logger.logln("No hay core dump disponible.");
    esp_core_dump_image_erase();
    return;
  }

  logger.logln("Core dump disponible:");
  logger.log("Direccion: "); 
  logger.logln(address, HEX);
  logger.log("Tamaño: ");
  logger.logln(size); 

  uint8_t *data = (uint8_t *)pvPortMalloc(size);
  if(data == nullptr)
  {
    logger.logln("No se pudo reservar memoria para el core dump.");
    return;
  }

  if(spi_flash_read(address, data, size) != ESP_OK)
  {
    logger.logln("No se pudo leer el core dump de la memoria flash.");
    free(data);
    return;
  }

  logger.logln("Guardando core dump en la tarjeta SD...");
  File coredump_file = SD.open(CORE_DUMP_LOG_PATH, FILE_WRITE); 
  if(!coredump_file)
  {
    logger.logln("No se pudo abrir el archivo de log para escribir el core dump.");
    vPortFree(data);
    return;
  }

  uint8_t *ptr = (uint8_t *)data + 20;
  size_t counter = size - 20;
  coredump_file.write(ptr, counter);

  coredump_file.close();
  logger.logln("Core dump guardado en la tarjeta SD.");

  vPortFree(data);

  if(esp_core_dump_image_erase() != ESP_OK)
    logger.logln("No se pudo borrar el core dump de la memoria flash.");
  else
    logger.logln("Core dump borrado de la memoria flash.");
}

/**
 * @brief Revisa que haya actualizaciones en Github.
 * 
 * Esta función se encarga de realizar una petición HTTP al servidor para obtener
 * las versiones disponibles y desacarga los firmware en caso de que haya una
 * actualización.
 */
void check_firmware_updates()
{
  uint8_t buffer[32] = { 0 };
  char auth_value[64] = "";
  snprintf(auth_value,  sizeof(auth_value),"Bearer %s", github_token);
  auth_value[sizeof(auth_value) - 1] = '\0';

  xSemaphoreTake(http_mutex, portMAX_DELAY);

  char endpoint[128] = "";
  snprintf(endpoint, sizeof(endpoint), "%s/versions.txt", branches_endpoint);
  endpoint[sizeof(endpoint) - 1] = '\0';
  logger.logln(endpoint);

  http_client->set_endpoint(endpoint);
  http_client->set_port(443);
  http_client->set_server("140.82.112.6");
  http_client->set_secure(true);
  http_client->set_header("Host", "api.github.com", 0);
  http_client->set_header("Accept", "application/vnd.github.raw", 1);
  http_client->set_header("Authorization", (const char *)auth_value, 2);
  http_client->set_header("User-Agent", "desarrolloidt", 3);
  http_client->set_header("X-GitHub-Api-Version", "2022-11-28", 4);
  int size = http_client->get_data_from_server(buffer, 32);

  if(size < 0)
  {
    logger.logln("No se pudo obtener información de Github.");
    http_client->flush_headers();
    xSemaphoreGive(http_mutex);
    return;
  }

  logger.logln((const char *)buffer);

  char versions[3][64] = { 0 };

  split_string((const char *)buffer, versions, 3, ';');

  for(int i = 0; i < 3; i++)
  {
    logger.logln(versions[i]);

    if(strcmp((const char *)versions[i], github_parameters[i + 1]) == 0)
    {
      logger.logln("No hay nueva version de firmware disponible.");
      continue;
    }

    File file = SD.open(firmware_paths[i], FILE_WRITE);

    if(!file)
    {
      send_update_state(false);
      logger.logln("No se pudo abrir el archivo.");
      continue;
    }

    logger.logln("Se encontró nueva version de firmware.");
    logger.logln("Descargando...");

    snprintf(endpoint, sizeof(endpoint) , "%s%s?ref=%s", branches_endpoint, firmwares_endpoints[i], versions[i]);

    endpoint[sizeof(endpoint) - 1] = '\0';
    logger.logln(endpoint);

    trigger_fw_update[i] = download_firmware(&file, endpoint);

    file.close();

    if(!trigger_fw_update[i])
      continue;

    strncpy((char *)github_parameters[i + 1], (const char *)versions[i], github_param_sizes[i + 1] - 1);
    github_parameters[i + 1][github_param_sizes[i + 1] - 1] = '\0';
    const char *param[1] = { github_parameters[i + 1] };
    uint8_t pos = github_param_pos[i + 1];
    save_parameter(GITHUB_PARAMETERS_PATH, param, &pos, 1);
  }

  http_client->flush_headers();

  xSemaphoreGive(http_mutex);
}

/**
 * @brief Dispara el flasheo del firmware descargado (display, impresora y ESP32).
 *
 * @note PENDIENTE: las pruebas de actualización OTA del ESP32 y de carga de firmware
 *       a los ATmega a través de sus bootloaders están pospuestas. Por eso el cuerpo
 *       de esta función está comentado. Para realizar dichas pruebas hay que
 *       descomentar el bloque de abajo.
 */
void check_firmware_triggers()
{
  /*if(trigger_fw_update[1])
  {
    logger.logln("Nuevo firmware encontrado para el controlador del display!");

    // Suspender tareas que usan I2C durante el flasheo
    xEventGroupSetBits(suspend_task_group, TRIGGER_BIT);
    xEventGroupWaitBits(suspend_task_group,
        GENERAL_TASK_BIT | WS_TASK_BIT | HTTP_TASK_BIT,
        pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));

    bool result = burn_firmware(screen, DISPLAY_FW_PATH);
    logger.log("Resultado display OTA: ");
    logger.logln(result ? "OK" : "FALLO");

    // Reanudar tareas
    xEventGroupClearBits(suspend_task_group,
        TRIGGER_BIT | GENERAL_TASK_BIT | WS_TASK_BIT | HTTP_TASK_BIT);
    vTaskResume(General_Task_Handle);
    vTaskResume(HTTP_Task_Handle);
    vTaskResume(Websocket_Task_Handle);

    trigger_fw_update[1] = false;
  }

  if(trigger_fw_update[2])
  {
    logger.logln("Nuevo firmware encontrado para el controlador de la impresora!");

    xEventGroupSetBits(suspend_task_group, TRIGGER_BIT);
    xEventGroupWaitBits(suspend_task_group,
        GENERAL_TASK_BIT | WS_TASK_BIT | HTTP_TASK_BIT,
        pdFALSE, pdTRUE, pdMS_TO_TICKS(5000));

    bool result = burn_firmware(printer, PRINTER_FW_PATH);
    logger.log("Resultado printer OTA: ");
    logger.logln(result ? "OK" : "FALLO");

    xEventGroupClearBits(suspend_task_group,
        TRIGGER_BIT | GENERAL_TASK_BIT | WS_TASK_BIT | HTTP_TASK_BIT);
    vTaskResume(General_Task_Handle);
    vTaskResume(HTTP_Task_Handle);
    vTaskResume(Websocket_Task_Handle);

    trigger_fw_update[2] = false;
  }

  if(trigger_fw_update[0])
  {
    logger.logln("Nuevo firmware encontrado para la ESP32.");
    logger.logln("Cargando...");
    File new_fw_file = SD.open(ESP32_FW_PATH);

    if(!new_fw_file)
    {
      logger.logln("No se encontró el archivo con el nuevo firmware.");
      return;
    }

    size_t file_size = new_fw_file.size();

    if(!update_esp32(&new_fw_file, file_size))
    {
      xEventGroupClearBits(suspend_task_group, TRIGGER_BIT | GENERAL_TASK_BIT | WS_TASK_BIT | HTTP_TASK_BIT);
      vTaskResume(HTTP_Task_Handle);
      vTaskResume(Websocket_Task_Handle);
      vTaskResume(General_Task_Handle);

      new_fw_file.close();
    }

    trigger_fw_update[0] = false;
  }*/
}

bool download_firmware(File *p_file, const char *p_endpoint)
{
  http_client->set_endpoint(p_endpoint);
  int length = http_client->download_content(p_file);

  if(length < 1)
  {
    logger.logln("No se pudo descargar el firmware.");
    send_update_state(false);
    return false;
  }

  logger.log("Firmware descargado, tamaño: ");
  logger.logln(length);

  return true;
}

bool update_esp32(File *p_file, size_t p_file_size)
{
  xEventGroupSetBits(suspend_task_group, TRIGGER_BIT);

  EventBits_t bits = xEventGroupWaitBits(
                        suspend_task_group, 
                        GENERAL_TASK_BIT | WS_TASK_BIT | HTTP_TASK_BIT, 
                        pdTRUE, 
                        pdTRUE, 
                        portMAX_DELAY);

  logger.logln("Inicializando la actualización...");

  if(!Update.begin(p_file_size))
  {
    send_update_state(false);
    logger.logln("Error al iniciar la actualización");
    return false;
  }

  uint8_t buffer[128] = { 0 };
  size_t count = 0;
  int len = 0;

  while((len = p_file->read(buffer, sizeof(buffer))) > 0)
  {
    if(Update.write(buffer, len) != len)
    {
      logger.logln("Error al escribir el firmware.");
      return false;
    }

    count += len;
  }

  p_file->close();

  if(Update.end(true))
  {
    send_update_state(true);
    websocket_client->set_status_code(NORMAL_CLOSURE);
    websocket_client->send_close_message("reinicio");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    logger.logln("Firmware escrito correctamente.\r\nReiniciando ESP32...");
    SD.end();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    SPI.end();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP.restart();
  }
  else
  {
    send_update_state(false);
    logger.logln("Error al finalizar la actualizacion.");
    logger.logln(Update.getError());
    
    return false;
  }

  return true;
}

bool burn_firmware(I2C_Master_Device *device, const char *p_path)
{
  File file = SD.open(p_path);

  if(!file)
  {
    logger.logln("No se pudo abrir el archivo de firmware del dispositivo.");
    return false;
  }

  size_t fw_size = file.size();

  return device->burn_firmware(&file, fw_size) == Device::NO_DEVICE_ERROR ? true : false;
}

void send_update_state(bool p_state)
{
  char payload[128] = "";
  snprintf(payload, sizeof(payload), "{\"identificador\":\"%s\",\"version\":\"%s\",\"estado\":%u}", mac_address, current_esp32_fw_v, p_state);
  websocket_client->send_event(socket_io_device_namespace, firmware_update_state_event, payload);

  if(p_state)
    websocket_client->send_close_message("reinicio");
}

/*Funciones constructoras*/
/**
 * @brief Construye el dispositivo LLS.
 * 
 * Esta función se encarga de crear y configurar el dispositivo LLS, estableciendo
 * los parámetros de comunicación y operación necesarios para su correcto funcionamiento.
 * 
 * @param p_builder Puntero al constructor del dispositivo LLS.
 * @return Device* Puntero al dispositivo LLS construido.
 */
Device *build_lls_device(Device_Builder *p_builder)
{
  lls_param_struct lls_device_params;
  lls_device_params.m_data_transmission_rate = 19200;
  lls_device_params.m_port = (Stream *)&Serial1;
  lls_device_params.m_rx_pin = LLS_RX_PIN;
  lls_device_params.m_tx_pin = LLS_TX_PIN;
  lls_device_params.m_periodic = false;
  lls_device_params.m_ascii = false;
  lls_device_params.m_timeout = 100;
    
  p_builder->set_device_parameters(&lls_device_params);
  return p_builder->get_device();
}

/**
 * @brief Construye el dispositivo de pantalla (Screen).
 * 
 * Esta función se encarga de crear y configurar el dispositivo de pantalla, estableciendo
 * los parámetros de comunicación y dirección I2C para su correcto funcionamiento.
 * 
 * @param p_builder Puntero al constructor del dispositivo de pantalla.
 * @return          Device* Puntero al dispositivo Screen construido.
 */
Device *build_screen_device(Device_Builder *p_builder)
{
  screen_param_struct screen_device_params;
  screen_device_params.m_port = (Stream *)&Wire;
  screen_device_params.m_address = DISPLAY_ADDR;

  p_builder->set_device_parameters(&screen_device_params);
  return p_builder->get_device();
}

/**
 * @brief Construye el dispositivo RFID MT124.
 * 
 * Esta función se encarga de crear y configurar el dispositivo MT124 para lectura de
 * etiquetas RFID, estableciendo los parámetros de comunicación necesarios.
 * 
 * @param p_builder Puntero al constructor del dispositivo MT124.
 * @return          Device* Puntero al dispositivo MT124 construido.
 */
Device *build_mt124_device(Device_Builder *p_builder)
{
  mt124_param_struct mt124_device_params;
  mt124_device_params.m_port = (Stream *)&Serial2;
  mt124_device_params.m_rx_pin = RFID_RX_PIN;
  mt124_device_params.m_tx_pin = RFID_TX_PIN;
  mt124_device_params.m_timeout = 100;

  p_builder->set_device_parameters(&mt124_device_params);
  return p_builder->get_device();
}

/**
 * @brief Construye el dispositivo de periféricos (Peripehals).
 * 
 * Esta función se encarga de crear y configurar el dispositivo de periféricos, en este
 * caso la impresora, estableciendo los parámetros de comunicación y dirección I2C.
 * 
 * @param p_builder Puntero al constructor del dispositivo de periféricos.
 * @return          Device* Puntero al dispositivo Peripehals construido.
 */
Device *build_peripehals_device(Device_Builder *p_builder)
{
  peripehals_param_struct params;

  params.m_address = PRINTER_ADDR;
  params.m_port = (Stream *)&Wire;
  strncpy(params.m_mac_address, printer_mac_address, sizeof(params.m_mac_address)); 
  params.m_mac_address[sizeof(params.m_mac_address) - 1] = '\0';
  p_builder->set_device_parameters(&params);
  return p_builder->get_device();
}

/**
 * @brief Construye el cliente Ethernet_HTTP.
 * 
 * Esta función se encarga de crear y configurar el cliente HTTP para la comunicación
 * con el servidor, estableciendo los parámetros necesarios como servidor, puerto y
 * encabezados personalizados.
 * 
 * @param p_builder Puntero al constructor del cliente HTTP.
 * @return          Global_Client* Puntero al cliente Ethernet_HTTP construido.
 */
Global_Client *build_http_client(Global_Client_Builder *p_builder)
{
  client_params params;
  params.m_server = (char *)&server[0];
  params.m_port = port;
  params.m_type = HTTP;

  p_builder->set_client_parameters(params);
  return p_builder->get_client();
}

/**
 * @brief Construye el cliente Ethernet_SocketIO.
 * 
 * Esta función se encarga de crear y configurar el cliente Ethernet_SocketIO para la comunicación
 * con el servidor, estableciendo los parámetros necesarios como servidor, puerto y
 * encabezados personalizados.
 * 
 * @param p_builder Puntero al constructor del cliente Ethernet_SocketIO.
 * @return          Global_Client* Puntero al cliente Ethernet_SocketIO construido.
 */
Global_Client *build_socketio_client(Global_Client_Builder *p_builder)
{
  client_params params;
  params.m_server = (char *)&server[0];
  params.m_port = port;
  params.m_type = SOCKET_IO;
  
  p_builder->set_client_parameters(params);
  return p_builder->get_client();
}