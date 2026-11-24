/*
 * main_bootloader_unified.cpp
 *
 * Bootloader I2C Unificado para ATMega328PB
 * Corrige todos los bugs y unifica protocolo
 *
 * Protocolo (v2, sin cambios en el cable; ver I2C_Master_Device::burn_firmware en el ESP32):
 *   [0x3E][0x02][SIZE_MSB][SIZE_LSB]   Iniciar flasheo (borra la página 0: la app queda inválida)
 *   [0x3C][128][0x00][128 datos]       Una página; la página 0 se guarda en RAM hasta el CRC
 *   [0x3D][CRC_MSB][CRC_LSB]           Verificar; si coincide se escribe la página 0
 *   [0x3F]                             Arrancar la app
 *   Lectura de 1 byte                  Estado: 0 = OK, 1..6 = error, 7 = sin app (en reposo)
 */

#include "main_bootloader_unified.h"

/* Variables globales */
static i2c_data_t i2c_data = {0};
static volatile bootloader_state_t bootloader_state = STATE_IDLE;
static volatile error_code_t last_error = ERROR_NONE;

static uint16_t firmware_size = 0;
static uint16_t current_flash_address = USER_APP_START_ADDR;
static uint16_t bytes_received = 0;
static uint16_t expected_crc = 0;

// La página 0 se escribe al final y solo si el CRC coincide: mientras no esté escrita,
// app_present() es falso y el bootloader no arranca una app incompleta.
static uint8_t page0[SPM_PAGESIZE];
static bool page0_received = false;

static volatile uint32_t millis_counter = 0;
static volatile uint32_t bootloader_start_time = 0;
static uint32_t last_command_time = 0;
static uint32_t state_since = 0;

// Marca en SRAM (el crt no la inicializa: sobrevive a un reinicio por watchdog) para arrancar
// la app justo después de reiniciar, con todos los periféricos en su estado de reset.
#define START_APP_MAGIC 0x5AA5C33CUL
static volatile uint32_t start_app_flag __attribute__((section(".noinit")));

/* Prototipos de funciones */
static void init_hardware(void);
static void init_i2c_slave(uint8_t address);
static void process_i2c_command(void);
static bool write_flash_page(uint16_t address, uint8_t *data, uint8_t size);
static void erase_flash_page(uint16_t address);
static bool validate_address(uint16_t address, uint16_t size);
static uint16_t calculate_firmware_crc(uint16_t length);
static bool app_present(void);
static uint8_t status_byte(void);
static void enter_error(error_code_t error);
static void start_application(void) __attribute__((noreturn));
static void reset_bootloader(void) __attribute__((noreturn));
static void led_pattern_bootloader_active(void);
static uint32_t get_millis(void);

/* ISR para I2C */
ISR(TWI0_vect)
{
    switch(TWSR0 & TW_STATUS_MASK)
    {
        case TW_SR_SLA_ACK:         // Dirección recibida, ACK enviado
        case TW_SR_GCALL_ACK:       // General call, ACK enviado
        case TW_SR_ARB_LOST_SLA_ACK:
        case TW_SR_ARB_LOST_GCALL_ACK:
        {
            i2c_data.counter = 0;
            i2c_data.ready = false;
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }

        case TW_SR_DATA_ACK:        // Dato recibido, ACK enviado
        case TW_SR_GCALL_DATA_ACK:
        {
            // Prevenir buffer overflow
            if(i2c_data.counter < I2C_BUFFER_SIZE)
            {
                i2c_data.buffer[i2c_data.counter++] = TWDR0;
                TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            }
            else
            {
                // Buffer lleno, enviar NACK
                last_error = ERROR_BUFFER_OVERFLOW;
                TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT);
            }
            break;
        }

        case TW_SR_STOP:            // STOP o repeated START recibido
        {
            i2c_data.ready = true;
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }

        case TW_SR_DATA_NACK:       // Dato recibido, NACK enviado
        case TW_SR_GCALL_DATA_NACK:
        {
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }

        // Master leyendo del slave: código de estado
        case TW_ST_SLA_ACK:
        case TW_ST_DATA_ACK:
        {
            TWDR0 = status_byte();
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }

        case TW_ST_DATA_NACK:
        case TW_ST_LAST_DATA:
        {
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
            break;
        }

        default:
        {
            // Error de bus (START/STOP fuera de lugar) u otro estado: TWSTO en modo esclavo
            // regresa el TWI a "no direccionado" y libera SDA/SCL. Sin él, tras un error de
            // bus el TWI podía dejar de reconocer su dirección.
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA) | (1 << TWSTO);
            break;
        }
    }
}

/* ISR para Timer0 - Contador de milisegundos */
ISR(TIMER0_COMPA_vect)
{
    millis_counter++;
}

int main(void)
{
    // Arranque de la app pedido por start_application(): se llega por un reinicio de watchdog
    // y se salta antes de tocar cualquier periférico. Saltar directo desde el bootloader (con el
    // TWI, el Timer0 y los vectores ya usados) dejaba la app del display sin arrancar.
    if((MCUSR & (1 << WDRF)) && start_app_flag == START_APP_MAGIC)
    {
        start_app_flag = 0;
        MCUSR = 0;
        WDTCSR = (1 << WDCE) | (1 << WDE);
        WDTCSR = 0x00;
        __asm__ __volatile__("jmp 0");
    }

    start_app_flag = 0;
    init_hardware();

    bootloader_start_time = get_millis();
    last_command_time = bootloader_start_time;

    while(1)
    {
        wdt_reset();

        // Patrón LED para indicar bootloader activo
        if(bootloader_state != STATE_ERROR)
            led_pattern_bootloader_active();

        uint32_t now = get_millis();

        switch(bootloader_state)
        {
            case STATE_IDLE:
            {
                // Con app válida, arrancarla si el ESP32 no pidió flashear a tiempo. Sin app
                // (flasheo interrumpido, o bootloader recién grabado por ISP) se espera al ESP32.
                if(app_present() && (now - bootloader_start_time) > BOOTLOADER_TIMEOUT_MS)
                {
                    LED_STATUS_OFF();
                    start_application();
                }
                break;
            }

            case STATE_READY_TO_FLASH:
            case STATE_FLASHING:
            {
                // El maestro dejó de mandar a mitad del flasheo: la página 0 ya está borrada,
                // así que al reiniciar el bootloader se queda esperando sin app.
                if((now - last_command_time) > FLASHING_TIMEOUT_MS)
                    reset_bootloader();
                break;
            }

            case STATE_SUCCESS:
            {
                // El ESP32 manda START_APPLICATION en cuanto lee el CRC correcto.
                if((now - state_since) > SUCCESS_START_MS)
                    start_application();
                break;
            }

            case STATE_ERROR:
            {
                // El código de error se sigue respondiendo en las lecturas (ISR); después se
                // reinicia. Antes se quedaba en un ciclo infinito sin atender el I2C.
                if((now - state_since) > ERROR_REPORT_MS)
                    reset_bootloader();
                break;
            }

            default:
                break;
        }

        // Procesar comandos I2C cuando hay datos
        if(i2c_data.ready)
        {
            // Resetear watchdog
            wdt_reset();

            // Procesar comando de forma atómica
            ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
            {
                process_i2c_command();
                i2c_data.ready = false;
            }

            last_command_time = get_millis();
        }
    }

    return 0;
}

//INICIALIZACIÓN
static void init_hardware(void)
{
    // Desactivar watchdog temporalmente
    cli();
    MCUSR &= ~(1 << WDRF);
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = 0x00;

    // Mover los vectores de interrupción al inicio del bootloader (0x7000). Sin esto, las
    // interrupciones del I2C y del Timer0 saltaban a la tabla de la app, en 0x0000.
    MCUCR = (1 << IVCE);
    MCUCR = (1 << IVSEL);

    // Inicializar LEDs. Sin el patrón de inicio bloqueante (5 parpadeos, 1 s): retrasaba el
    // I2C y alargaba cada reinicio de la app.
    LED_STATUS_INIT();
    LED_ERROR_INIT();
    LED_STATUS_ON();
    LED_ERROR_OFF();

    // Configurar Timer0 para millis()
    TCCR0A = (1 << WGM01);              // CTC mode
    TCCR0B = (1 << CS01) | (1 << CS00); // Prescaler 64
    OCR0A = 124;                        // ~1ms @ 8MHz/64
    TIMSK0 = (1 << OCIE0A);             // Enable compare interrupt

    // Inicializar I2C
    init_i2c_slave(SLAVE_ADDRESS);

    // Habilitar interrupciones globales
    sei();

    // Configurar watchdog a 2 segundos
    wdt_enable(WDTO_2S);
}

static void init_i2c_slave(uint8_t address)
{
    // Configurar dirección I2C
    TWAR0 = (address << 1);

    // Habilitar TWI, ACK, e interrupciones
    TWCR0 = (1 << TWEN) | (1 << TWEA) | (1 << TWIE) | (1 << TWINT);
}

//PROCESAMIENTO DE COMANDOS
static void process_i2c_command(void)
{
    if(i2c_data.counter == 0)
        return;

    uint8_t command = i2c_data.buffer[0];

    switch(command)
    {
        case I2C_START_FLASHING_CMD:
        {
            // Formato: [CMD][0x02][SIZE_MSB][SIZE_LSB]
            // Byte 1 debe ser 0x02 (versión del protocolo)
            if(i2c_data.counter != 4 || i2c_data.buffer[1] != 0x02)
            {
                enter_error(ERROR_INVALID_SIZE);
                break;
            }

            // Extraer tamaño del firmware
            firmware_size = (i2c_data.buffer[2] << 8) | i2c_data.buffer[3];

            // Validar tamaño
            if(firmware_size == 0 || firmware_size > (USER_APP_END_ADDR - USER_APP_START_ADDR))
            {
                enter_error(ERROR_INVALID_SIZE);
                break;
            }

            // Desde aquí la app queda inválida: si el flasheo se interrumpe, al reiniciar el
            // bootloader espera al ESP32 en lugar de arrancar una app a medias.
            erase_flash_page(USER_APP_START_ADDR);

            // Preparar para recibir firmware
            current_flash_address = USER_APP_START_ADDR;
            bytes_received = 0;
            page0_received = false;
            bootloader_state = STATE_READY_TO_FLASH;
            last_error = ERROR_NONE;

            LED_STATUS_ON();
            break;
        }

        case I2C_FLASH_TO_ADDRESS_CMD:
        {
            if(bootloader_state != STATE_READY_TO_FLASH && bootloader_state != STATE_FLASHING)
            {
                enter_error(ERROR_INVALID_ADDRESS);
                break;
            }

            // Formato: [CMD][SIZE][RESERVED][DATA...], siempre 128 datos (el último se rellena
            // con 0xFF). Una trama más corta llegó truncada y corrompería la app.
            if(i2c_data.counter != FLASH_FRAME_SIZE || i2c_data.buffer[1] != SPM_PAGESIZE)
            {
                enter_error(ERROR_INVALID_SIZE);
                break;
            }

            if(bytes_received >= firmware_size)
            {
                enter_error(ERROR_INVALID_ADDRESS);   // más páginas de las anunciadas
                break;
            }

            uint8_t *page_data = &i2c_data.buffer[3];
            uint16_t remaining = firmware_size - bytes_received;
            uint8_t data_size = remaining < SPM_PAGESIZE ? (uint8_t)remaining : SPM_PAGESIZE;

            if(current_flash_address == USER_APP_START_ADDR)
            {
                // La página 0 se queda en RAM hasta verificar el CRC
                for(uint16_t i = 0; i < SPM_PAGESIZE; i++)
                    page0[i] = i < data_size ? page_data[i] : 0xFF;

                page0_received = true;
            }
            else if(!write_flash_page(current_flash_address, page_data, data_size))
            {
                enter_error(ERROR_WRITE_FAILED);
                break;
            }

            // Actualizar contadores
            current_flash_address += SPM_PAGESIZE;
            bytes_received += data_size;
            bootloader_state = STATE_FLASHING;

            // Toggle LED para indicar actividad
            LED_STATUS_TOGGLE();

            break;
        }

        case I2C_CRC_CMD:
        {
            // Formato: [CMD][CRC_MSB][CRC_LSB]
            if(i2c_data.counter != 3)
            {
                enter_error(ERROR_INVALID_SIZE);
                break;
            }

            if(bootloader_state != STATE_FLASHING || bytes_received < firmware_size || !page0_received)
            {
                enter_error(ERROR_INVALID_SIZE);      // transferencia incompleta
                break;
            }

            expected_crc = (i2c_data.buffer[1] << 8) | i2c_data.buffer[2];

            if(calculate_firmware_crc(firmware_size) != expected_crc)
            {
                enter_error(ERROR_CRC_MISMATCH);
                break;
            }

            // CRC correcto: ahora sí se escribe la página 0 y se comprueba
            bool page0_ok = write_flash_page(USER_APP_START_ADDR, page0, SPM_PAGESIZE);

            for(uint16_t i = 0; page0_ok && i < SPM_PAGESIZE; i++)
                page0_ok = pgm_read_byte(USER_APP_START_ADDR + i) == page0[i];

            if(!page0_ok)
            {
                erase_flash_page(USER_APP_START_ADDR);  // no dejar una página 0 a medias
                enter_error(ERROR_WRITE_FAILED);
                break;
            }

            bootloader_state = STATE_SUCCESS;
            last_error = ERROR_NONE;
            state_since = get_millis();
            LED_STATUS_ON();
            break;
        }

        case I2C_START_APPLICATION_CMD:
        {
            // Solo con el CRC verificado, o si ya había una app válida y no se está flasheando
            if(bootloader_state == STATE_SUCCESS || (bootloader_state == STATE_IDLE && app_present()))
            {
                LED_STATUS_OFF();
                start_application();
            }

            enter_error(ERROR_INVALID_SIZE);          // no se completó la transferencia
            break;
        }

        case I2C_WATCHDOG_RESET_CMD:
        {
            reset_bootloader();
        }

        default:
        {
            // Comando desconocido, ignorar
            break;
        }
    }
}

static void enter_error(error_code_t error)
{
    last_error = error;
    bootloader_state = STATE_ERROR;
    state_since = get_millis();
    LED_STATUS_OFF();
    LED_ERROR_ON();
}

//ESCRITURA A FLASH
static bool write_flash_page(uint16_t address, uint8_t *data, uint8_t size)
{
    // Validar dirección
    if(!validate_address(address, size))
        return false;

    // Guardar estado de interrupciones
    uint8_t sreg = SREG;
    cli();

    // Esperar a que EEPROM esté lista
    eeprom_busy_wait();

    // CORRECCIÓN: Solo borrar UNA vez antes de escribir
    boot_page_erase(address);
    boot_spm_busy_wait();

    // Llenar buffer de página (2 bytes a la vez)
    for(uint16_t i = 0; i < SPM_PAGESIZE; i += 2)
    {
        uint16_t word = 0xFFFF;  // Por defecto, flash vacío

        if(i < size)
        {
            word = data[i];
            if(i + 1 < size)
                word |= (data[i + 1] << 8);
            else
                word |= 0xFF00;
        }

        boot_page_fill(address + i, word);
    }

    // Escribir página desde buffer
    boot_page_write(address);
    boot_spm_busy_wait();

    // Re-habilitar sección RWW
    boot_rww_enable();

    // Restaurar interrupciones
    SREG = sreg;

    return true;
}

static void erase_flash_page(uint16_t address)
{
    uint8_t sreg = SREG;
    cli();

    eeprom_busy_wait();
    boot_page_erase(address);
    boot_spm_busy_wait();
    boot_rww_enable();

    SREG = sreg;
}

static bool validate_address(uint16_t address, uint16_t size)
{
    // Verificar alineación a página
    if((address % SPM_PAGESIZE) != 0)
        return false;

    // Verificar rango válido
    if(address < USER_APP_START_ADDR)
        return false;

    if(address + size > USER_APP_END_ADDR)
        return false;

    // Verificar que no se escriba sobre el bootloader
    if(address >= BOOTLOADER_START_ADDR)
        return false;

    return true;
}

//CÁLCULO DE CRC
// CRC-16 reflejado (polinomio 0xA001, inicio 0xFFFF), igual que calculate_firmware_crc() del
// ESP32. La página 0 todavía no está en flash: se toma del buffer en RAM.
static uint16_t calculate_firmware_crc(uint16_t length)
{
    uint16_t crc = 0xFFFF;

    for(uint16_t i = 0; i < length; i++)
    {
        uint8_t byte = i < SPM_PAGESIZE ? page0[i] : pgm_read_byte(USER_APP_START_ADDR + i);

        crc ^= byte;
        for(uint8_t j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }

    return crc;
}

// Hay app si la página 0 está escrita: el bootloader la borra al empezar a flashear y solo
// la escribe cuando el CRC coincide.
static bool app_present(void)
{
    return pgm_read_word(USER_APP_START_ADDR) != 0xFFFF;
}

static uint8_t status_byte(void)
{
    if(bootloader_state == STATE_IDLE && !app_present())
        return ERROR_NO_APP;

    return (uint8_t)last_error;
}

//INICIO DE APLICACIÓN
// La app no se arranca con un salto directo: se marca start_app_flag y se reinicia por
// watchdog. main() ve la marca y salta a la app antes de configurar nada, así que la app
// arranca con todos los periféricos (TWI, timers, puertos, IVSEL) en su estado de reset.
static void start_application(void)
{
    LED_STATUS_OFF();
    LED_ERROR_OFF();
    start_app_flag = START_APP_MAGIC;
    reset_bootloader();
}

// Reinicio por watchdog: el bootloader vuelve a empezar (y se queda esperando si no hay app)
static void reset_bootloader(void)
{
    cli();
    wdt_enable(WDTO_15MS);
    while(1);
}

//INDICADORES LED
static void led_pattern_bootloader_active(void)
{
    static uint32_t last_toggle = 0;

    // Más rápido sin app, para distinguirlo a simple vista
    uint32_t period = app_present() ? 500 : 150;

    if((get_millis() - last_toggle) > period)
    {
        LED_STATUS_TOGGLE();
        last_toggle = get_millis();
    }
}

/* Función segura para obtener el contador de milisegundos */
static uint32_t get_millis(void)
{
    uint32_t ms;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        ms = millis_counter;
    }

    return ms;
}
