#include "Logger.h"
#include <freertos/semphr.h>

Logger logger;

/// Serializa el log entre tareas. En release cada mensaje abre, escribe y cierra /data.log
/// sobre el mismo objeto File; varias tareas en los dos núcleos lo hacían a la vez y
/// corrompían el heap (CORRUPT HEAP en File::close()). Recursivo: log() llama a log_date()
/// y a write() dentro de la misma tarea.
static SemaphoreHandle_t log_mutex = nullptr;

namespace
{
    struct Log_Lock
    {
        Log_Lock()  { if(log_mutex) xSemaphoreTakeRecursive(log_mutex, portMAX_DELAY); }
        ~Log_Lock() { if(log_mutex) xSemaphoreGiveRecursive(log_mutex); }
    };
}

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
    Log_Lock lock;
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
    if(log_mutex == nullptr)
        log_mutex = xSemaphoreCreateRecursiveMutex();

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
    Log_Lock lock;
    size_t count = 0;

    if(!this->m_logger)
        return 0;

    count = this->m_logger->write(p_data);

    return count;
}

size_t Logger::write(const uint8_t *p_data, size_t p_length)
{
    Log_Lock lock;
    size_t count = 0;

    if(!this->m_logger)
        return 0;

    count = this->m_logger->write(p_data, p_length);

    return count;
}

void Logger::log_date()
{
    Log_Lock lock;
    if(this->m_use_file)
    {
        char date[32];
        snprintf(date, sizeof(date) - 1,"20%02u/%02u/%02u %02u:%02u:%02u --> ", *this->m_rtc[0], *this->m_rtc[1], *this->m_rtc[2],
                                                              *this->m_rtc[3], *this->m_rtc[4], *this->m_rtc[5]);


        date[sizeof(date) - 1] = '\0';
        print(date);
    }
}

bool Logger::open_file()
{
    this->m_file_logger = SD.open(LOG_PATH, FILE_APPEND);

    if(!this->m_file_logger)
        return false;

    // Solo se rota entre renglones, para no partir uno en los dos archivos.
    if(this->m_at_line_start && this->m_file_logger.size() >= LOG_MAX_SIZE)
    {
        this->m_file_logger.close();

        if(SD.exists(LOG_OLD_PATH))
            SD.remove(LOG_OLD_PATH);

        // Si no se pudiera renombrar, FILE_WRITE empieza /data.log desde cero: se pierde
        // lo anterior, pero el log no crece sin límite.
        SD.rename(LOG_PATH, LOG_OLD_PATH);
        this->m_file_logger = SD.open(LOG_PATH, FILE_WRITE, true);
    }

    return (bool)this->m_file_logger;
}

template<typename... Args>
void Logger::entry(bool p_newline, const Args&... p_args)
{
    Log_Lock lock;

    // Sin begin() (en release, si la SD no montó) no hay salida. Antes print() se
    // llamaba sobre nullptr y el equipo se reiniciaba en el arranque.
    if(this->m_logger == nullptr)
        return;

    if(this->m_use_file && !open_file())
        return;

    TaskHandle_t task = xTaskGetCurrentTaskHandle();

    if(!this->m_at_line_start && task != this->m_line_owner)
    {
        this->m_logger->println();
        this->m_at_line_start = true;
    }

    // logln() sin argumentos solo termina el renglón: no lleva fecha.
    if constexpr (sizeof...(p_args) == 0)
        this->m_logger->println();
    else
    {
        if(this->m_at_line_start)
            log_date();

        if(p_newline)
            this->m_logger->println(p_args...);
        else
            this->m_logger->print(p_args...);
    }

    this->m_at_line_start = p_newline;
    this->m_line_owner = task;

    if(this->m_use_file)
        this->m_file_logger.close();
}

void Logger::log(const __FlashStringHelper *ifsh)              { entry(false, reinterpret_cast<const char *>(ifsh)); }
void Logger::log(const char *p_data)                           { entry(false, p_data); }
void Logger::log(char p_data)                                  { entry(false, p_data); }
void Logger::log(unsigned char p_data, int base)               { entry(false, p_data, base); }
void Logger::log(int p_data, int base)                         { entry(false, p_data, base); }
void Logger::log(unsigned int p_data, int base)                { entry(false, p_data, base); }
void Logger::log(long p_data, int base)                        { entry(false, p_data, base); }
void Logger::log(unsigned long p_data, int base)               { entry(false, p_data, base); }
void Logger::log(long long p_data, int base)                   { entry(false, p_data, base); }
void Logger::log(unsigned long long p_data, int base)          { entry(false, p_data, base); }
void Logger::log(double p_data, int dec)                       { entry(false, p_data, dec); }
void Logger::log(const Printable& p_data)                      { entry(false, p_data); }

void Logger::logln(const __FlashStringHelper *ifsh)            { entry(true, reinterpret_cast<const char *>(ifsh)); }
void Logger::logln(const char *p_data)                         { entry(true, p_data); }
void Logger::logln(char p_data)                                { entry(true, p_data); }
void Logger::logln(unsigned char p_data, int base)             { entry(true, p_data, base); }
void Logger::logln(int p_data, int base)                       { entry(true, p_data, base); }
void Logger::logln(unsigned int p_data, int base)              { entry(true, p_data, base); }
void Logger::logln(long p_data, int base)                      { entry(true, p_data, base); }
void Logger::logln(unsigned long p_data, int base)             { entry(true, p_data, base); }
void Logger::logln(long long p_data, int base)                 { entry(true, p_data, base); }
void Logger::logln(unsigned long long p_data, int base)        { entry(true, p_data, base); }
void Logger::logln(double p_data, int dec)                     { entry(true, p_data, dec); }
void Logger::logln(const Printable& p_data)                    { entry(true, p_data); }
void Logger::logln(void)                                       { entry(true); }
