#ifndef INTERACTIVE_H
#define INTERACTIVE_H

#define INTERACTIVE_DEFAULT_HOST "127.0.0.1"
#define INTERACTIVE_HOST_LEN 256
#define INTERACTIVE_DEFAULT_WIDTH 40
#define INTERACTIVE_MIN_WIDTH 11
#define INTERACTIVE_MAX_WIDTH 99
#define INTERACTIVE_KEY_QUIT 1

typedef struct {
	bool enabled;
	bool lcd_enabled;
	bool terminal_enabled;
	bool connected;
	bool lcdd_compat;
	int linelen;
	int lcd_fd;
	int cursor;
	char host[INTERACTIVE_HOST_LEN];
	char display[INTERACTIVE_MAX_WIDTH * 2];
} interactive_str;

void interactive_init(interactive_str *state);
bool interactive_set_host(interactive_str *state, const char *host);
bool interactive_set_width(interactive_str *state, const char *width);
bool interactive_open(interactive_str *state);
void interactive_close(interactive_str *state);
void interactive_receive_display(interactive_str *state, const u8_t *packet, size_t len);
u32_t interactive_read_key(interactive_str *state);

#endif
