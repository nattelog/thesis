#ifndef __EVENT_HANDLER_h__
#define __EVENT_HANDLER_h__

#define EVENT_HANDLER_IO_FILE "EVENT_HANDLER_IO_FILE"
#define EVENT_HANDLER_IO_CONTENT "EVENT_HANDLER_IO_CONTENT"

void event_handler_do_cpu(double intensity);

long event_handler_calc_io_rounds(double intensity);

void event_handler_serial(double cpu_intensity, double io_intensity);

#endif
