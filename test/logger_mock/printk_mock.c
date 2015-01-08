/*
 * Race capture
 */

#include "mod_string.h"
#include "printk.h"
#include "ring_buffer.h"
#include "serial.h"
#include "modp_numtoa.h"
#include <stddef.h>
#include <stdio.h>

#define LOG_BUFFER_SIZE 1024

static enum log_level curr_level = ERR;
static char _log_buffer[LOG_BUFFER_SIZE];

static struct ring_buff log_buff = {
        .buf = _log_buffer,
        .size = LOG_BUFFER_SIZE,
        .head = _log_buffer,
        .tail = _log_buffer
};
static struct ring_buff * const lbp = &log_buff;

size_t read_log_to_serial(Serial *s, int escape)
{
        char buff[16];
        size_t to_read = get_used(lbp);

        while(has_data(lbp)) {
			int read = get_data(lbp, &buff, sizeof(buff));
			for(int i = 0; i < read; i++)
					if (escape){
						put_escapedString(s, &buff[i],1);
					}
					else{
						s->put_c(buff[i]);
					}
        }

        return to_read;
}

size_t write_to_log_buff(const char *msg) {

        size_t data_written = strlen(msg) + 1;
        printf(msg);

        return data_written;
}

int writek(const char *msg){
	return write_to_log_buff(msg);
}

int writek_int(int value){
	char buf[12];
	modp_itoa10(value, buf);
	return write_to_log_buff(buf);
}

int printk(enum log_level level, const char *msg) {
        if (level > curr_level) return 0;
        return write_to_log_buff(msg);
}

int printk_int(enum log_level level, int value) {
		if (level > curr_level) return 0;
		return writek_int(value);
}

enum log_level get_log_level(){
	return curr_level;
}

enum log_level set_log_level(enum log_level level)
{
        if (level <= TRACE)
                curr_level = level;

        return curr_level;
}