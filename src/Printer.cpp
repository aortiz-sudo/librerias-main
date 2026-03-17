#include "Printer.h"

void Printer::init_printer()
{
    uint8_t data_out[] = { ESC, '@' };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_charset(printer_charset p_charset)
{
    uint8_t data_out[] = { ESC, 'R', p_charset };

    printer_write(data_out, sizeof(data_out));
}

void Printer::print_line_feed()
{
    uint8_t data_out[] = { LF };

    printer_write(data_out, sizeof(data_out));
}

void Printer::print_line_feed(uint8_t n)
{
    uint8_t data_out[] = { ESC, 'd', n };

    printer_write(data_out, sizeof(data_out));
}

void Printer::print_horizontal_tab()
{
    uint8_t data_out[] = { HT };

    printer_write(data_out, sizeof(data_out));
}

void Printer::partial_cut()
{
    uint8_t data_out[] = { GS, 'V', 0x01 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::full_cut()
{
    uint8_t data_out[] = { GS, 'V', 0x00 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_justification(justification n)
{
    uint8_t data_out[] = { ESC, 'a', n };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_80mm()
{
    uint8_t data_out[] = { GS, 'W', 0x35, 0x02 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_54mm()
{
    uint8_t data_out[] = { GS, 'W', 0xA4, 0x01 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_area_width(uint16_t n)
{
    uint8_t data_out[] = { GS, 'W', (uint8_t)n, (uint8_t)(n >> 8) };
    
    printer_write(data_out, sizeof(data_out));
}

void Printer::set_default_line_spacing()
{
    uint8_t data_out[] = { ESC, '2' };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_line_spacing(uint8_t n)
{
    uint8_t data_out[] = { ESC, '3', n };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_char_width(char_size p_width)
{
    uint8_t data_out[] = { GS, '!', (uint8_t)(p_width << 4) };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_char_height(char_size p_height)
{ 
    uint8_t data_out[] = { GS, '!', p_height };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_double_width()
{
    uint8_t data_out[] = { ESC, '!', 0x20 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_double_height()
{
    uint8_t data_out[] = { ESC, '!', 0x10 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_bold()
{
    uint8_t data_out[] = { ESC, '!', 0x08 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_underline()
{
    uint8_t data_out[] = { ESC, '!', 0x80 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::cancel_chinese_mode()
{
    uint8_t data_out[] = { FS, '.' };

    printer_write(data_out, sizeof(data_out));
}

void Printer::no_effects()
{
    uint8_t data_out[] = { ESC, '!', 0x00 };

    printer_write(data_out, sizeof(data_out));
}

void Printer::set_left_margin(uint16_t n)
{
    uint8_t data_out[] = { GS, 'L', (uint8_t)n, (uint8_t)(n >> 8) };

    printer_write(data_out, sizeof(data_out));
}

void Printer::print_last_ticket(ticket_struct *p_ticket)
{
    char date[15];
    char hour[15];
    char number[10];

    snprintf(date, sizeof(date), "%02u/%02u/20%02u", p_ticket->rtc_array[2], p_ticket->rtc_array[1], p_ticket->rtc_array[0]);
    snprintf(hour, sizeof(hour), "%02u:%02u:%02u", p_ticket->rtc_array[4], p_ticket->rtc_array[5], p_ticket->rtc_array[6]);
    snprintf(number, sizeof(number), "%05u\0", p_ticket->ticket_number);
    date[sizeof(date) - 1] = '\0';
    hour[sizeof(hour) - 1] = '\0';
    number[sizeof(number) - 1] = '\0';

    cancel_chinese_mode();
    init_printer(); 
    set_left_margin(0x00);

    if(p_ticket->business_name)
    {
        set_charset(LATIN_AMERICA);
        set_justification(CENTER);
        set_bold();
        set_char_height(_4X);
        set_char_width(_1X); 
        printer_write(p_ticket->business_name);
        print_line_feed();
    }

    if(p_ticket->business_address)
    {
        set_charset(LATIN_AMERICA);
        set_justification(CENTER);
        no_effects();
        set_char_height(_2X);
        set_char_width(_1X);
        printer_write(p_ticket->business_address);
        print_line_feed();
    }

    if(p_ticket->business_area)
    {
        printer_write(p_ticket->business_area);
        print_line_feed();
    }

    if(p_ticket->business_city)
    {
        printer_write(p_ticket->business_city);
        printer_write(", ");
        printer_write(p_ticket->business_state);
        print_line_feed();
    }

    if(p_ticket->business_postal_code)
    {
        printer_write(p_ticket->business_postal_code);
        print_line_feed();
    }

    if(p_ticket->business_RFC)
    {
        printer_write(p_ticket->business_RFC);
        print_line_feed();
        print_line_feed();
    }

    no_effects();
    set_char_height(_3X);
    set_char_width(_1X);
    set_justification(LEFT);
    printer_write("Fecha: ");
    printer_write((const char *)date);
    print_line_feed();
    printer_write("Hora: ");
    printer_write((const char *)hour);
    print_line_feed();
    printer_write("No. Ticket: ");
    printer_write((const char *)number);
    print_horizontal_tab();
    print_line_feed();
    printer_write("Operador: ");
    printer_write(p_ticket->user_name);
    print_horizontal_tab();
    print_line_feed();
    print_line_feed();
    set_char_width(_2X);
    set_bold();
    printer_write("________________________________");
    print_line_feed();
    set_char_height(_2X);
    set_char_width(_1X);
    set_bold();
    set_justification(CENTER); 
    printer_write("Producto  Lts  Precio  Importe");
    print_line_feed();
    set_char_width(_2X);
    set_justification(LEFT);
    set_bold(); 
    printer_write("________________________________");
    print_line_feed();
    set_char_height(_2X);
    set_char_width(_1X);
    no_effects();
    set_justification(CENTER);
    printer_write(p_ticket->product);
    print_horizontal_tab();
    print_float(p_ticket->liters);
    print_horizontal_tab();
    print_float(p_ticket->price);
    print_horizontal_tab();
    print_float(p_ticket->total_amount);
    print_line_feed();
    print_line_feed();
    set_justification(RIGHT);
    printer_write("Total Neto:  ");
    print_float(p_ticket->total_amount);
    print_line_feed();
    printer_write("IVA:   ");
    print_float(p_ticket->IVA);
    print_line_feed();
    printer_write("__________________");
    print_line_feed();
    printer_write("Total:  ");
    print_float(p_ticket->total);
    print_line_feed(0x03);
}

void Printer::print_day_ticket(ticket_struct *p_tickets, size_t p_services)
{
    char date[15];
    float total_amount = 0.0;
    float IVA = 0.0;
    float total = 0.0;
    float total_liters = 0.0;

    snprintf(date, sizeof(date) - 1, "%02u/%02u/20%02u", p_tickets[0].rtc_array[2],p_tickets[0].rtc_array[1], p_tickets[0].rtc_array[0]);
    date[sizeof(date) - 1] = '\0';

    
    init_printer();
    set_left_margin(0x00);

    if(p_tickets[0].business_name)
    {
        set_charset(LATIN_AMERICA);
        set_justification(CENTER);
        set_bold();
        set_char_height(_4X);
        set_char_width(_1X); 
        printer_write(p_tickets[0].business_name);
        print_line_feed();
    }

    if(p_tickets[0].business_address)
    {
        set_charset(LATIN_AMERICA);
        set_justification(CENTER);
        no_effects();
        set_char_height(_2X);
        set_char_width(_1X);
        printer_write(p_tickets[0].business_address);
        print_line_feed();
    }

    if(p_tickets[0].business_area)
    {
        printer_write(p_tickets[0].business_area);
        print_line_feed();
    }

    if(p_tickets[0].business_city)
    {
        printer_write(p_tickets[0].business_city);
        printer_write(", ");
        printer_write(p_tickets[0].business_state);
        print_line_feed();
    }

    if(p_tickets[0].business_postal_code)
    {
        printer_write(p_tickets[0].business_postal_code);
        print_line_feed();
    }

    if(p_tickets[0].business_RFC)
    {
        printer_write(p_tickets[0].business_RFC);
        print_line_feed();
        print_line_feed();
    }

    no_effects();
    set_char_height(_3X);
    set_char_width(_1X);
    set_justification(LEFT);
    printer_write("Fecha: ");
    printer_write((const char *)date);
    print_line_feed();
    printer_write("Hora: ");
    printer_write("00:00 - 23:59");
    print_line_feed();
    printer_write("Producto: ");
    printer_write(p_tickets[0].product);
    print_line_feed();
    print_line_feed();
    set_char_width(_2X);
    set_bold();
    printer_write("________________________________");
    print_line_feed();
    set_char_height(_2X);
    set_char_width(_1X);
    set_bold();
    set_justification(CENTER);
    printer_write("Ticket  Lts  Precio  Importe");
    print_line_feed();
    set_char_width(_2X);
    set_justification(LEFT);
    set_bold();
    printer_write("________________________________");
    print_line_feed();
    set_char_height(_2X);
    set_char_width(_1X);
    no_effects();
    set_justification(CENTER);
    print_line_feed();

    for(int i = 0; i < p_services; i++)
    {
        char number[10];
        snprintf(number, sizeof(number) - 1, "%05u", p_tickets[i].ticket_number);
        number[sizeof(number) - 1] = '\0';
        printer_write(number);
        printer_write("  ");
        print_float(p_tickets[i].liters);
        printer_write("  ");
        print_float(p_tickets[i].price);
        printer_write("  ");
        print_float(p_tickets[i].total_amount);
        print_line_feed();

        total_amount += p_tickets[i].total_amount;
        IVA += p_tickets[i].IVA;
        total_liters += p_tickets[i].liters;
    }

    total = total_amount + IVA;

    print_line_feed();
    set_justification(RIGHT);
    printer_write("Litros totales: ");
    print_float(total_liters);
    print_line_feed();
    printer_write("Total Neto:  ");
    print_float(total_amount);
    print_line_feed();
    printer_write("IVA:   ");
    print_float(IVA);
    print_line_feed();
    printer_write("__________________");
    print_line_feed();
    printer_write("Total:  ");
    print_float(total);
    print_line_feed(0x03);
}