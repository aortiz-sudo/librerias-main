#include "Logger.h"

Logger logger;

/**
 * \brief Salida que descarta todo. Sustituye al puerto serie mientras el log está
 *        silenciado, así ninguna de las funciones log/logln necesita revisarlo.
 */
class Null_Print : public Print
{
    public:
        size_t write(uint8_t) override { return 1; }
        size_t write(const uint8_t *, size_t p_length) override { return p_length; }
};

static Null_Print null_print;

void Logger::set_serial_muted(bool p_muted)
{
    if(this->m_use_file)
        return;

    if(p_muted && this->m_logger != &null_print)
    {
        this->m_unmuted_logger = this->m_logger;
        this->m_logger = &null_print;
    }
    else if(!p_muted && this->m_logger == &null_print)
        this->m_logger = this->m_unmuted_logger;
}

void Logger::begin(bool p_debug)
{
    if(!p_debug)
    {
        if(!SD.exists(LOG_PATH))
        {
            this->m_file_logger = SD.open(LOG_PATH, FILE_WRITE, true);
            this->m_file_logger.close();
        }

        set_logger(&this->m_file_logger);

        this->m_use_file = true;
    }
    else
    {
        Serial.begin(115200);
        set_logger(&Serial);
    }
}

size_t Logger::write(uint8_t p_data)
{
    size_t count = 0;

    if(!this->m_logger)
        return 0;

    count = this->m_logger->write(p_data);

    return count;
}

size_t Logger::write(const uint8_t *p_data, size_t p_length)
{
    size_t count = 0;

    if(!this->m_logger)
        return 0;

    count = this->m_logger->write(p_data, p_length);

    return count;
}

void Logger::log_date()
{
    if(this->m_use_file)
    {
        char date[32];
        snprintf(date, sizeof(date) - 1,"20%02u/%02u/%02u %02u:%02u:%02u --> ", *this->m_rtc[0], *this->m_rtc[1], *this->m_rtc[2], 
                                                              *this->m_rtc[3], *this->m_rtc[4], *this->m_rtc[5]);

         
        date[sizeof(date) - 1] = '\0';
        print(date);
    }
}

void Logger::log(const __FlashStringHelper *ifsh)
{ 
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(reinterpret_cast<const char *>(ifsh)); 
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(const char *p_data)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(char p_data)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(unsigned char p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(int p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(unsigned int p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(unsigned long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);    

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(long long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(unsigned long long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, base);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(double p_data, int dec)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data, dec);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(const Printable& p_data)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->print(p_data);
    
    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(const __FlashStringHelper *ifsh)
{ 
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(reinterpret_cast<const char *>(ifsh)); 
    
    if(this->m_use_file)
        this->m_file_logger.close();
    
}

void Logger::logln(const char *p_data)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(char p_data)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(unsigned char p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(int p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(unsigned int p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(unsigned long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(long long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(unsigned long long p_data, int base)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, base);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(double p_data, int dec)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data, dec);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(const Printable& p_data)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    log_date();
    this->m_logger->println(p_data);

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::logln(void)
{
    if(this->m_use_file)
        this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    this->m_logger->println();

    if(this->m_use_file)
        this->m_file_logger.close();
}