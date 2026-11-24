/*
 * main_bootloader_unified.cpp
 * 
 * Bootloader I2C Unificado para ATMega328PB
 * Corrige todos los bugs y unifica protocolo
 */

#include "main_bootloader_unified.h"

/* Variables globales */
static i2c_data_t i2c_data = {0};
static bootloader_state_t bootloader_state = STATE_IDLE;
static error_code_t last_error = ERROR_NONE;

static uint16_t firmware_size = 0;
static uint16_t current_flash_address = USER_APP_START_ADDR;
static uint16_t bytes_received = 0;
static uint16_t expected_crc = 0;

static volatile uint32_t millis_counter = 0;
static volatile uint32_t bootloader_start_time = 0;

/* Prototipos de funciones */
static void init_hardware(void);
static void init_i2c_slave(uint8_t address);
static void process_i2c_command(void);
static bool write_flash_page(uint16_t address, uint8_t *data, uint8_t size);
static bool validate_address(uint16_t address, uint16_t size);
static uint16_t calculate_flash_crc(uint16_t start, uint16_t length);
static void start_application(void) __attribute__((noreturn));
static void indicate_error(error_code_t error);
static void led_pattern_bootloader_active(void);
static void led_pattern_success(void);
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
        
        // Master leyendo del slave (para futuras confirmaciones)
        case TW_ST_SLA_ACK:
        case TW_ST_DATA_ACK:
        {
            // Enviar código de estado
            TWDR0 = (uint8_t)last_error;
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
            // Error, resetear TWI
            TWCR0 = (1 << TWEN) | (1 << TWIE) | (1 << TWINT) | (1 << TWEA);
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
    init_hardware();
    
    bootloader_start_time = get_millis();
    
    while(1)
    {
        wdt_reset();
        
        // Patrón LED para indicar bootloader activo
        led_pattern_bootloader_active();
        
        // Verificar timeout de bootloader
        if(bootloader_state == STATE_IDLE)
        {
            if((get_millis() - bootloader_start_time) > BOOTLOADER_TIMEOUT_MS)
            {
                // Timeout - arrancar aplicación
                LED_STATUS_OFF();
                start_application();
            }
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
        }
        
        // Verificar si se completó flasheo exitosamente
        if(bootloader_state == STATE_SUCCESS)
        {
            led_pattern_success();
            _delay_ms(1000);
            start_application();
        }
        
        // Manejar errores
        if(bootloader_state == STATE_ERROR)
        {
            indicate_error(last_error);
            // Mantener en error hasta watchdog reset o power cycle
            while(1)
            {
                wdt_reset();
                _delay_ms(100);
            }
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
    
    // Inicializar LEDs
    LED_STATUS_INIT();
    LED_ERROR_INIT();
    LED_STATUS_ON();
    LED_ERROR_OFF();
    
    // Patrón de inicio: 5 parpadeos rápidos
    for(uint8_t i = 0; i < 5; i++)
    {
        LED_STATUS_OFF();
        _delay_ms(100);
        LED_STATUS_ON();
        _delay_ms(100);
    }
    
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
            if(i2c_data.counter != 4)
            {
                last_error = ERROR_INVALID_SIZE;
                bootloader_state = STATE_ERROR;
                break;
            }
            
            // Byte 1 debería ser 0x02 (versión del protocolo)
            uint8_t protocol_version = i2c_data.buffer[1];
            if(protocol_version != 0x02)
            {
                last_error = ERROR_INVALID_SIZE;
                bootloader_state = STATE_ERROR;
                break;
            }
            
            // Extraer tamaño del firmware
            firmware_size = (i2c_data.buffer[2] << 8) | i2c_data.buffer[3];
            
            // Validar tamaño
            if(firmware_size == 0 || firmware_size > (USER_APP_END_ADDR - USER_APP_START_ADDR))
            {
                last_error = ERROR_INVALID_SIZE;
                bootloader_state = STATE_ERROR;
                break;
            }
            
            // Preparar para recibir firmware
            current_flash_address = USER_APP_START_ADDR;
            bytes_received = 0;
            bootloader_state = STATE_READY_TO_FLASH;
            last_error = ERROR_NONE;
            
            LED_STATUS_ON();
            break;
        }
        
        case I2C_FLASH_TO_ADDRESS_CMD:
        {
            if(bootloader_state != STATE_READY_TO_FLASH && bootloader_state != STATE_FLASHING)
            {
                last_error = ERROR_INVALID_ADDRESS;
                bootloader_state = STATE_ERROR;
                break;
            }
            
            // Formato: [CMD][SIZE][RESERVED][DATA...]
            // SIZE está en buffer[1], debería ser 128
            uint8_t page_size = i2c_data.buffer[1];
            
            if(page_size != SPM_PAGESIZE)
            {
                last_error = ERROR_INVALID_SIZE;
                bootloader_state = STATE_ERROR;
                break;
            }
            
            // Datos comienzan en buffer[2] (después de CMD y SIZE)
            // buffer[2] es reservado, datos reales desde buffer[3]
            uint8_t *page_data = &i2c_data.buffer[3];
            uint8_t data_size = i2c_data.counter - 3;  // Restar CMD, SIZE, RESERVED
            
            // Validar que no se reciban más bytes que el tamaño del firmware
            if(bytes_received + data_size > firmware_size)
            {
                data_size = firmware_size - bytes_received;
            }
            
            // Escribir página a flash
            if(!write_flash_page(current_flash_address, page_data, data_size))
            {
                last_error = ERROR_WRITE_FAILED;
                bootloader_state = STATE_ERROR;
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
                last_error = ERROR_INVALID_SIZE;
                bootloader_state = STATE_ERROR;
                break;
            }
            
            expected_crc = (i2c_data.buffer[1] << 8) | i2c_data.buffer[2];
            
            // Calcular CRC de la aplicación flasheada
            uint16_t calculated_crc = calculate_flash_crc(USER_APP_START_ADDR, firmware_size);
            
            if(calculated_crc == expected_crc)
            {
                bootloader_state = STATE_SUCCESS;
                last_error = ERROR_NONE;
            }
            else
            {
                bootloader_state = STATE_ERROR;
                last_error = ERROR_CRC_MISMATCH;
            }
            
            break;
        }
        
        case I2C_START_APPLICATION_CMD:
        {
            // Si no se hizo verificación CRC, asumir éxito si se recibió todo el firmware
            if(bytes_received >= firmware_size)
            {
                bootloader_state = STATE_SUCCESS;
            }
            else
            {
                // No se completó la transferencia
                last_error = ERROR_INVALID_SIZE;
                bootloader_state = STATE_ERROR;
            }
            
            break;
        }
        
        case I2C_WATCHDOG_RESET_CMD:
        {
            // Resetear vía watchdog
            wdt_enable(WDTO_15MS);
            while(1);  // Esperar reset
            break;
        }
        
        default:
        {
            // Comando desconocido, ignorar
            break;
        }
    }
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
static uint16_t calculate_flash_crc(uint16_t start, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    
    for(uint16_t i = 0; i < length; i++)
    {
        uint8_t byte = pgm_read_byte(start + i);
        
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

//INICIO DE APLICACIÓN
static void start_application(void)
{
    cli();
    
    // Desactivar watchdog
    MCUSR &= ~(1 << WDRF);
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = 0x00;
    
    // Desactivar TWI
    TWCR0 = 0x00;
    
    // Desactivar Timer0
    TCCR0B = 0x00;
    TIMSK0 = 0x00;
    
    // Apagar LEDs
    LED_STATUS_OFF();
    LED_ERROR_OFF();
    
    // Saltar a aplicación
    __asm__ __volatile__(
        "clr r1\n\t"
        "jmp %0"
        :
        : "i" (USER_APP_START_ADDR)
    );
    
    // Nunca llega aquí
    while(1);
}

//INDICADORES LED
static void indicate_error(error_code_t error)
{
    LED_STATUS_OFF();
    
    // Parpadear LED de error según código
    for(uint8_t i = 0; i < (uint8_t)error; i++)
    {
        LED_ERROR_ON();
        _delay_ms(200);
        LED_ERROR_OFF();
        _delay_ms(200);
    }
    
    _delay_ms(1000);
}

static void led_pattern_bootloader_active(void)
{
    static uint32_t last_toggle = 0;
    
    if((get_millis() - last_toggle) > 500)
    {
        LED_STATUS_TOGGLE();
        last_toggle = get_millis();
    }
}

static void led_pattern_success(void)
{
    LED_ERROR_OFF();
    
    for(uint8_t i = 0; i < 3; i++)
    {
        LED_STATUS_ON();
        _delay_ms(100);
        LED_STATUS_OFF();
        _delay_ms(100);
    }
    
    LED_STATUS_ON();
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