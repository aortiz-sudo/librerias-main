#include "I2C_Master_Device.h"
#include "Definitions.h"

bool I2C_Master_Device::m_port_ready = false;
int8_t I2C_Master_Device::m_scl_pin = -1;
int8_t I2C_Master_Device::m_sda_pin = -1;
TwoWire * I2C_Master_Device::m_i2c_port = nullptr;

void I2C_Master_Device::set_device_parameters(const device_param_struct *p_params)
{
    const i2c_param_struct *p = static_cast<const i2c_param_struct *>(p_params);
    set_transmission_rate(p->m_data_transmission_rate);
    this->m_sda_pin = p->m_sda_pin;
    this->m_scl_pin = p->m_scl_pin;
    set_type(p->m_type);
    set_crc_type(p->m_crc);
    set_port(p_params->m_port);
    this->m_address = p->m_address;
}

void I2C_Master_Device::get_device_parameters(device_param_struct *p_params)
{
    i2c_param_struct *p = static_cast<i2c_param_struct *>(p_params);
    p->m_data_transmission_rate = get_transmission_rate();
    p->m_sda_pin = this->m_sda_pin;
    p->m_scl_pin = this->m_scl_pin;
    p->m_type = get_type();
    p->m_crc = get_crc_type();
    p->m_port = get_port();
    p->m_address = this->m_address;
}

void I2C_Master_Device::begin(const device_param_struct *p_params)
{
    this->set_device_parameters(p_params);
}

size_t I2C_Master_Device::write_data(uint8_t *p_data, size_t p_data_legth)
{
    size_t count = 0;
    uint8_t transmission_status = 1;
    uint8_t counter = 10;

    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    while(transmission_status != 0 && counter--)
    {
        static_cast<TwoWire *>(get_port())->beginTransmission(this->m_address);
        count = this->get_port()->write(p_data, p_data_legth);
        transmission_status = static_cast<TwoWire *>(get_port())->endTransmission();

        //if(transmission_status != 0)
            vTaskDelay(20 / portTICK_PERIOD_MS);
    }

    xSemaphoreGive(i2c_mutex);

    if(transmission_status != 0)
    {
        logger.log(this->m_address, HEX);
        logger.log(" I2C ERROR: ");
        logger.logln(transmission_status);
    }

    if(transmission_status == 1)
    {
        set_status(DATA_OUT_OF_RANGE_ERROR);
        return 0;
    }

    if(transmission_status == 2 || transmission_status == 3)
    {
        set_status(NACK_ERROR);
        return 0;
    }

    if(transmission_status == 5)
    {
        set_status(TIMEOUT_ERROR);
        return 0;
    }

    set_status(NO_DEVICE_ERROR);
    return count;
}

size_t I2C_Master_Device::read_data(uint8_t *p_buffer, size_t p_buffer_length)
{
    set_status(NO_DEVICE_ERROR);

    xSemaphoreTake(i2c_mutex, portMAX_DELAY);

    size_t count = 0;
    count = static_cast<TwoWire *>(get_port())->requestFrom(m_address, p_buffer_length);

    size_t received_bytes = 0;
    if(get_port()->available())
    {
        for(int i = 0; i < count; i++)
        {
            p_buffer[i] = get_port()->read();
            received_bytes++;
        }
    }
    else
        set_status(TIMEOUT_ERROR);

    xSemaphoreGive(i2c_mutex);

    return received_bytes;
}

status I2C_Master_Device::reset_device()
{
    command_struct command;

    command.address = this->m_address;
    command.type = I2C_WATCHDOG_RESET_CMD;
    command.data = nullptr;
    command.length = 0;

    return send_command(&command);
}

status I2C_Master_Device::burn_firmware(File *p_file, size_t p_file_size)
{
    uint8_t main_app_address = this->m_address;
    reset_device();

    // Esperar a que el bootloader arranque y esté listo
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Esperar 1 segundo para dar tiempo al bootloader a arrancar

    if(!device_ready())
    {
        logger.logln("El dispositivo no responde.");
        return set_status(TIMEOUT_ERROR);
    }

    set_address(this->m_address + 0x10);  // Asumimos que el bootloader responde en la dirección principal + 0x10
    
    uint8_t start_buffer[4];
    start_buffer[0] = I2C_START_FLASHING_CMD;  // 0x3E
    start_buffer[1] = 0x02;                     // Versión de protocolo
    start_buffer[2] = (p_file_size >> 8) & 0xFF;  // MSB
    start_buffer[3] = p_file_size & 0xFF;          // LSB
    
    command_struct command;
    command.type = I2C_START_FLASHING_CMD;
    command.data = &start_buffer[1];  // Solo enviar bytes después del comando
    command.length = 3;
    command.address = this->m_address;  
    
    if(send_command(&command) != NO_DEVICE_ERROR)
    {
        logger.logln("No se pudo inicializar el flasheo del nuevo firmware.");
        set_address(main_app_address);
        return get_status();
    }
    
    logger.logln("Quemando el firmware nuevo...");
    
    const size_t PAGE_SIZE = 128;  // SPM_PAGESIZE del ATMega328PB
    size_t total_pages = (p_file_size + PAGE_SIZE - 1) / PAGE_SIZE;  // Redondear hacia arriba
    size_t current_page = 0;
    size_t total_bytes_written = 0;
    
    // Formato: [CMD][SIZE][RESERVED][DATA...]
    // Tamaño total: 131 bytes (1 + 1 + 1 + 128)
    uint8_t flash_buffer[131];
    
    while(current_page < total_pages)
    {
        flash_buffer[0] = I2C_FLASH_TO_ADDRESS_CMD;  // 0x3C
        flash_buffer[1] = 128;  // SIZE: siempre 128 bytes
        flash_buffer[2] = 0x00; // RESERVED (byte reservado)

        // Los datos comienzan en flash_buffer[3]
        uint8_t *page_data = &flash_buffer[3];

        // Leer hasta 128 bytes del archivo
        size_t bytes_to_read = PAGE_SIZE;
        if(total_bytes_written + bytes_to_read > p_file_size)
        {
            bytes_to_read = p_file_size - total_bytes_written;
        }

        if(bytes_to_read > 0)
        {
            size_t bytes_read = p_file->readBytes(reinterpret_cast<char*>(page_data), bytes_to_read);
            total_bytes_written += bytes_read;

            // Rellenar el resto con 0xFF (flash vacío)
            for(size_t i = bytes_read; i < PAGE_SIZE; i++)
                page_data[i] = 0xFF;
        }
        else
        {
            // Página completa con 0xFF
            for(size_t i = 0; i < PAGE_SIZE; i++)
                page_data[i] = 0xFF;
        }

        logger.log("Escribiendo pagina ");
        logger.log(current_page);
        logger.log(" de ");
        logger.logln(total_pages);

        // Enviar página directamente por I2C (131 bytes, no cabe en send_command)
        if(write_data(flash_buffer, sizeof(flash_buffer)) == 0)
        {
            logger.logln("No se pudo escribir la pagina.");
            set_address(main_app_address);
            return get_status();
        }
        
       // Esperar a que el bootloader procese la página (erase + write ~4.5ms)
        vTaskDelay(50 / portTICK_PERIOD_MS);

        // Leer estado del bootloader para verificar escritura correcta
        uint8_t page_status = 0xFF;
        size_t status_bytes = read_data(&page_status, 1);

        if(status_bytes != 1 || page_status != 0x00)  // 0x00 = ERROR_NONE
        {
            logger.log("Error en pagina ");
            logger.log(current_page);
            logger.log(": codigo ");
            logger.logln(page_status);
            set_address(main_app_address);
            return set_status(NACK_ERROR);
        }

        current_page++;
    }
    
    // Verificación CRC
    
    uint16_t crc = calculate_firmware_crc(p_file, p_file_size);
    
    uint8_t crc_buffer[3];
    crc_buffer[0] = I2C_CRC_CMD;
    crc_buffer[1] = (crc >> 8) & 0xFF;  // MSB del CRC
    crc_buffer[2] = crc & 0xFF;     // LSB del CRC
    
    command.type = I2C_CRC_CMD;
    command.data = &crc_buffer[1];
    command.length = 2;
    command.address = this->m_address;
    
    if(send_command(&command) != NO_DEVICE_ERROR)
    {
        logger.logln("Error al verificar CRC.");
        set_address(main_app_address);
        return get_status();
    }
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
    
    uint8_t error_code = 0xFF;
    size_t bytes_read = read_data(&error_code, 1);

    if(bytes_read != 1)
    {
        logger.logln("No se recibió respuesta de verificación CRC.");
        set_address(main_app_address);
        return set_status(TIMEOUT_ERROR);
    }

   switch(error_code)
{
    case 0:  // ERROR_NONE
        logger.logln("CRC verificado correctamente.");
        break;
        
    case 4:  // ERROR_CRC_MISMATCH
        logger.logln("ERROR CRITICO: CRC no coincide!");
        logger.logln("El firmware corrupto. Abortando actualizacion.");
        set_address(main_app_address);
        return set_status(NACK_ERROR);

    case 1:  // ERROR_INVALID_SIZE
        logger.logln("ERROR: Tamaño de firmware invalido.");
        set_address(main_app_address);
        return set_status(DATA_OUT_OF_RANGE_ERROR);

    case 2:  // ERROR_INVALID_ADDRESS
        logger.logln("ERROR: Direccion de memoria invalida.");
        set_address(main_app_address);
        return set_status(DATA_OUT_OF_RANGE_ERROR);

    case 3:  // ERROR_WRITE_FAILED
        logger.logln("ERROR: Fallo al escribir en flash.");
        set_address(main_app_address);
        return set_status(NACK_ERROR);

    default:
        logger.log("ERROR DESCONOCIDO: ");
        logger.logln(error_code);
        set_address(main_app_address);
        return set_status(NACK_ERROR);
}
    
    // Enviar comando para iniciar aplicación
    logger.logln("Firmware flasheado exitosamente. Iniciando aplicación...");
    
    command.type = I2C_START_APPLICATION_CMD;
    command.data = nullptr;
    command.length = 0;
    command.address = this->m_address;
    
    if(send_command(&command) != NO_DEVICE_ERROR)
    {
        logger.logln("Error al iniciar aplicación.");
        set_address(main_app_address);
        return get_status();
    }
    
    // Esperar un poco antes de continuar
    vTaskDelay(500 / portTICK_PERIOD_MS);

    set_address(main_app_address);
    return get_status();
}

bool I2C_Master_Device::device_ready()
{
    int state = 1;
    unsigned long time = millis();

    while(state != 0 && millis() - time < 5000)
    {
        static_cast<TwoWire *>(get_port())->beginTransmission(this->m_address + 0x10);
        state = static_cast<TwoWire *>(get_port())->endTransmission();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }

    return state == 0 ? true : false;
}

status I2C_Master_Device::start_device()
{
    uint8_t main_app_address = get_address();
    set_address(main_app_address + 0x10);

    command_struct command;

    command.type = I2C_START_APPLICATION_CMD;
    command.data = nullptr;
    command.length = 0;
    command.address = this->m_address;

    status state = send_command(&command);
    set_address(main_app_address);

    return state;
}

status I2C_Master_Device::send_command(command_struct *p_command)
{
    uint8_t data[I2C_BUFFER_LENGTH] = { 0 };
    data[0] = p_command->type;

    size_t length = 1 + p_command->length;

    if(length > I2C_BUFFER_LENGTH)
        length = I2C_BUFFER_LENGTH;

    if(p_command->data && length > 1)
        memcpy(&data[1], p_command->data, length - 1);

    write_data(data, length);

    return get_status();
}

// Función para calcular el CRC del firmware utilizando el algoritmo CRC-16-IBM (polinomio 0x8005)
uint16_t I2C_Master_Device::calculate_firmware_crc(File *p_file, size_t p_file_size)
{
    uint16_t crc = 0xFFFF;
    
    // Regresar al inicio del archivo
    p_file->seek(0);
    
    for(size_t i = 0; i < p_file_size; i++)
    {
        uint8_t byte = p_file->read();
        
        crc ^= byte;
        for(uint8_t j = 0; j < 8; j++)
        {
            if(crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001;
            else
                crc >>= 1;
        }
    }
    
    // Regresar al inicio del archivo nuevamente
    p_file->seek(0);
    
    return crc;
}