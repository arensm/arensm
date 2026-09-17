/*
 * LCDd display support, derived from the interactive display code in
 * squeezelite-KLCD.  IR handling deliberately remains in squeezelite's ir.c.
 */

#include "squeezelite.h"

#if INTERACTIVE

#include <ctype.h>
#include <locale.h>
#include <netdb.h>
#include <poll.h>
#include <curses.h>

#define LCDD_PORT "13666"
#define LCDD_GREETING_TIMEOUT_MS 3000
#define VFDC_DATA_OFFSET 10

static log_level loglevel = lWARN;
static SCREEN *terminal_screen;
static WINDOW *terminal_window;
static FILE *terminal_input;
static FILE *terminal_output;

static bool terminal_open(interactive_str *state) {
	int rows;
	int columns;
	int window_width = state->linelen + 2;
	FILE *input = stdin;
	FILE *output = stdout;

	if (!state->terminal_enabled) return true;
	if (!isatty(fileno(stdin)) || !isatty(fileno(stdout))) {
		terminal_input = fopen("/dev/console", "r");
		terminal_output = fopen("/dev/console", "w");
		if (!terminal_input || !terminal_output) {
			if (terminal_input) fclose(terminal_input);
			if (terminal_output) fclose(terminal_output);
			terminal_input = NULL;
			terminal_output = NULL;
			LOG_WARN("cannot open /dev/console for terminal display");
			return false;
		}
		input = terminal_input;
		output = terminal_output;
	}
	setlocale(LC_ALL, "");
	terminal_screen = newterm(NULL, output, input);
	if (!terminal_screen) {
		if (terminal_input) fclose(terminal_input);
		if (terminal_output) fclose(terminal_output);
		terminal_input = NULL;
		terminal_output = NULL;
		LOG_WARN("cannot initialize terminal display");
		return false;
	}

	cbreak();
	noecho();
	nonl();
	nodelay(stdscr, true);
	keypad(stdscr, true);
	leaveok(stdscr, true);
	curs_set(0);
	getmaxyx(stdscr, rows, columns);
	if (rows < 4 || columns < window_width) {
		endwin();
		delscreen(terminal_screen);
		terminal_screen = NULL;
		LOG_WARN("terminal must be at least %dx4 characters", window_width);
		return false;
	}

	terminal_window = newwin(4, window_width, (rows - 4) / 2, (columns - window_width) / 2);
	if (!terminal_window) {
		endwin();
		delscreen(terminal_screen);
		terminal_screen = NULL;
		LOG_WARN("cannot create terminal display window");
		return false;
	}
	box(terminal_window, 0, 0);
	wrefresh(terminal_window);
	return true;
}

static void terminal_close(void) {
	if (terminal_screen) {
		endwin();
		delscreen(terminal_screen);
		terminal_screen = NULL;
		terminal_window = NULL;
	}
	if (terminal_input) fclose(terminal_input);
	if (terminal_output) fclose(terminal_output);
	terminal_input = NULL;
	terminal_output = NULL;
}

static bool lcd_send(interactive_str *state, const char *data, size_t len) {
	const char *ptr = data;

	while (len) {
		ssize_t written = send(state->lcd_fd, ptr, len, MSG_NOSIGNAL);
		if (written < 0 && errno == EINTR) continue;
		if (written <= 0) {
			LOG_WARN("LCDd write to %s failed: %s", state->host, strerror(errno));
			interactive_close(state);
			return false;
		}
		ptr += written;
		len -= written;
	}

	return true;
}

static bool lcd_command(interactive_str *state, const char *command) {
	return lcd_send(state, command, strlen(command));
}

static bool lcd_drain(interactive_str *state) {
	char response[1024];

	for (;;) {
		ssize_t count = recv(state->lcd_fd, response, sizeof(response), MSG_DONTWAIT);
		if (count > 0) continue;
		if (count == 0) {
			LOG_WARN("LCDd at %s closed the connection", state->host);
			interactive_close(state);
			return false;
		}
		if (errno == EINTR) continue;
		if (errno == EAGAIN || errno == EWOULDBLOCK) return true;
		LOG_WARN("LCDd read from %s failed: %s", state->host, strerror(errno));
		interactive_close(state);
		return false;
	}
}

static void parse_greeting(interactive_str *state, const char *greeting) {
	const char *width = strstr(greeting, "wid ");

	if (width) {
		char *end;
		long value = strtol(width + 4, &end, 10);
		if (end != width + 4 && value >= INTERACTIVE_MIN_WIDTH && value <= INTERACTIVE_MAX_WIDTH) {
			state->linelen = (int)value;
		}
	}
}

static bool read_greeting(interactive_str *state) {
	struct pollfd pfd = { .fd = state->lcd_fd, .events = POLLIN };
	char greeting[1024];
	ssize_t used = 0;

	while (used < (ssize_t)sizeof(greeting) - 1) {
		int ready = poll(&pfd, 1, LCDD_GREETING_TIMEOUT_MS);
		if (ready < 0 && errno == EINTR) continue;
		if (ready <= 0) return false;
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) return false;

		ssize_t count = recv(state->lcd_fd, greeting + used, sizeof(greeting) - 1 - used, 0);
		if (count < 0 && errno == EINTR) continue;
		if (count <= 0) return false;
		used += count;
		if (memchr(greeting, '\n', used)) break;
	}

	greeting[used] = '\0';
	parse_greeting(state, greeting);
	return true;
}

static void sanitize_text(char *target, const char *source, size_t len) {
	size_t i;

	for (i = 0; i < len; i++) {
		unsigned char value = (unsigned char)source[i];
		if (value == '\n' || value == '\r' || value == '{' || value == '}' || value == '\\') {
			target[i] = ' ';
		} else {
			target[i] = (char)value;
		}
	}
	target[len] = '\0';
}

static bool set_lcd_line(interactive_str *state, int line, const char *text) {
	char clean[INTERACTIVE_MAX_WIDTH * 2 + 1];
	char command[INTERACTIVE_MAX_WIDTH * 2 + 64];
	int length;

	sanitize_text(clean, text, strlen(text));
	length = snprintf(command, sizeof(command), "widget_set main %s 1 %d {%s}\n",
				  line == 1 ? "one" : "two", line, clean);
	if (length < 0 || (size_t)length >= sizeof(command)) return false;
	return lcd_send(state, command, (size_t)length);
}

static unsigned char printable(unsigned char value) {
	switch (value) {
	case 11:  return '#';
	case 16:  return '>';
	case 22:  return '@';
	case 145: return ' ';
	case 152: return 'o';
	case 224:
	case 226: return 'a';
	case 231: return 'c';
	case 232:
	case 233:
	case 234:
	case 235: return 'e';
	case 238:
	case 239: return 'i';
	case 244: return 'o';
	case 249:
	case 251: return 'u';
	case 255: return 'y';
	default:  return isprint(value) ? value : ' ';
	}
}

static bool is_german_latin1(unsigned char value) {
	switch (value) {
	case 0xc4:
	case 0xd6:
	case 0xdc:
	case 0xdf:
	case 0xe4:
	case 0xf6:
	case 0xfc:
		return true;
	default:
		return false;
	}
}

static void display_line_latin1(char *target, const char *source, size_t cells) {
	size_t pos;

	for (pos = 0; pos < cells; pos++) {
		unsigned char value = (unsigned char)source[pos];
		target[pos] = (char)(is_german_latin1(value) ? value : printable(value));
	}
	target[cells] = '\0';
}

static void display_line_utf8(char *target, const char *source, size_t cells) {
	size_t source_pos;
	size_t target_pos = 0;

	for (source_pos = 0; source_pos < cells; source_pos++) {
		unsigned char value = (unsigned char)source[source_pos];
		unsigned char second = 0;

		switch (value) {
		case 0xc4: second = 0x84; break; /* A umlaut */
		case 0xd6: second = 0x96; break; /* O umlaut */
		case 0xdc: second = 0x9c; break; /* U umlaut */
		case 0xdf: second = 0x9f; break; /* sharp s */
		case 0xe4: second = 0xa4; break; /* a umlaut */
		case 0xf6: second = 0xb6; break; /* o umlaut */
		case 0xfc: second = 0xbc; break; /* u umlaut */
		default: break;
		}

		if (second) {
			target[target_pos++] = (char)0xc3;
			target[target_pos++] = (char)second;
		} else {
			target[target_pos++] = (char)printable(value);
		}
	}
	target[target_pos] = '\0';
}

static void show_display(interactive_str *state) {
	char lcd_lines[2][INTERACTIVE_MAX_WIDTH + 1];
	char terminal_lines[2][INTERACTIVE_MAX_WIDTH * 2 + 1];
	int row;

	if (state->connected && !lcd_drain(state)) return;

	for (row = 0; row < 2; row++) {
		display_line_latin1(lcd_lines[row], state->display + row * state->linelen, state->linelen);
		display_line_utf8(terminal_lines[row], state->display + row * state->linelen, state->linelen);
		if (state->connected && !set_lcd_line(state, row + 1, lcd_lines[row])) return;
	}

	if (terminal_window) {
		mvwaddstr(terminal_window, 1, 1, terminal_lines[0]);
		mvwaddstr(terminal_window, 2, 1, terminal_lines[1]);
		wrefresh(terminal_window);
	}
}

u32_t interactive_read_key(interactive_str *state) {
	int key;

	if (!state->terminal_enabled || !terminal_screen) return 0;

	while ((key = getch()) != ERR) {
		switch (key) {
		case KEY_UP:    return 0x7689e01f;
		case KEY_DOWN:  return 0x7689b04f;
		case KEY_LEFT:  return 0x7689906f;
		case KEY_RIGHT:
		case KEY_ENTER:
		case '\n':      return 0x7689d02f;
		case ' ':
		case 'P':       return 0x768920df;
		case 'p':       return 0x768910ef;
		case '+':
		case '=':       return 0x7689807f;
		case '-':       return 0x768900ff;
		case 'm':
		case 'M':       return 0x7689c43b;
		case 'h':
		case 'H':       return 0x768922dd;
		case '0':       return 0x76899867;
		case '1':       return 0x7689f00f;
		case '2':       return 0x768908f7;
		case '3':       return 0x76898877;
		case '4':       return 0x768948b7;
		case '5':       return 0x7689c837;
		case '6':       return 0x768928d7;
		case '7':       return 0x7689a857;
		case '8':       return 0x76896897;
		case '9':       return 0x7689e817;
		case 'q':
		case 'Q':       return INTERACTIVE_KEY_QUIT;
		default:        break;
		}
	}

	return 0;
}

void interactive_init(interactive_str *state) {
	memset(state, 0, sizeof(*state));
	state->lcd_fd = -1;
	state->linelen = INTERACTIVE_DEFAULT_WIDTH;
	strncpy(state->host, INTERACTIVE_DEFAULT_HOST, sizeof(state->host) - 1);
	memset(state->display, ' ', sizeof(state->display));
}

bool interactive_set_host(interactive_str *state, const char *host) {
	if (!host || !host[0] || strlen(host) >= sizeof(state->host)) return false;
	strcpy(state->host, host);
	return true;
}

bool interactive_set_width(interactive_str *state, const char *width) {
	char *end;
	long value;

	if (!width || !width[0]) return false;
	errno = 0;
	value = strtol(width, &end, 10);
	if (errno || *end || value < INTERACTIVE_MIN_WIDTH || value > INTERACTIVE_MAX_WIDTH) return false;
	state->linelen = (int)value;
	return true;
}

bool interactive_open(interactive_str *state) {
	struct addrinfo hints;
	struct addrinfo *addresses = NULL;
	struct addrinfo *address;
	int error;

	if (!state->enabled) return true;
	if (!terminal_open(state)) state->terminal_enabled = false;
	if (!state->lcd_enabled || state->connected) return state->terminal_enabled || state->connected;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(state->host, LCDD_PORT, &hints, &addresses);
	if (error) {
		LOG_WARN("cannot resolve LCDd host %s: %s", state->host, gai_strerror(error));
		return false;
	}

	for (address = addresses; address; address = address->ai_next) {
		state->lcd_fd = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
		if (state->lcd_fd < 0) continue;
		if (connect(state->lcd_fd, address->ai_addr, address->ai_addrlen) == 0) break;
		close(state->lcd_fd);
		state->lcd_fd = -1;
	}
	freeaddrinfo(addresses);

	if (state->lcd_fd < 0) {
		LOG_WARN("cannot connect to LCDd at %s:%s", state->host, LCDD_PORT);
		return false;
	}

	state->connected = true;
	if (!lcd_command(state, "hello\n") || !read_greeting(state) ||
		!lcd_command(state, "client_set name {squeezelite-ALCD}\n") ||
		!lcd_command(state, "screen_add main\n") ||
		!lcd_command(state, "screen_set main name {Squeezelite}\n") ||
		!lcd_command(state, "screen_set main heartbeat off\n") ||
		!lcd_command(state, "widget_add main one string\n") ||
		!lcd_command(state, "widget_add main two string\n") ||
		!lcd_command(state, state->lcdd_compat ? "screen_set main -priority 256\n" :
										 "screen_set main -priority info\n")) {
		LOG_WARN("LCDd initialization at %s failed", state->host);
		interactive_close(state);
		return false;
	}

	LOG_INFO("connected to LCDd at %s:%s, width %d", state->host, LCDD_PORT, state->linelen);
	return true;
}

void interactive_close(interactive_str *state) {
	if (state->lcd_fd >= 0) close(state->lcd_fd);
	state->lcd_fd = -1;
	state->connected = false;
	terminal_close();
}

void interactive_receive_display(interactive_str *state, const u8_t *packet, size_t len) {
	size_t offset;
	int display_size = state->linelen * 2;

	if (!state->enabled || len <= VFDC_DATA_OFFSET) return;

	/* Each LMS vfdc packet contains a complete two-line frame.  The KLCD
	 * implementation started with a fresh DDRAM buffer and cursor for every
	 * packet; retaining the cursor drops line one after the first frame. */
	memset(state->display, ' ', sizeof(state->display));
	state->cursor = 0;

	/* KLCD's vfdc packets contain six header bytes after the opcode. */
	for (offset = VFDC_DATA_OFFSET; offset + 1 < len; offset += 2) {
		u8_t type = packet[offset];
		u8_t value = packet[offset + 1];

		if (type == 0x03) {
			if (state->cursor >= 0 && state->cursor < display_size) {
				state->display[state->cursor++] = (char)value;
			}
		} else if (type == 0x02) {
			if (value == 0x01 || value == 0x06) {
				memset(state->display, ' ', sizeof(state->display));
				state->cursor = 0;
			} else if (value == 0x02 || value == 0x80) {
				state->cursor = 0;
			} else if (value == 0xc0) {
				state->cursor = state->linelen;
			} else if (value & 0x80) {
				int address = value & 0x7f;
				if (address >= 0x40) address = state->linelen + address - 0x40;
				state->cursor = address < display_size ? address : display_size;
			}
		}
	}

	show_display(state);
}

#endif
