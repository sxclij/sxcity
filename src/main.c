#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#define chunk_size 256
#define world_size 4096
#define buf_size 65536
enum blocktype {
    blocktype_null,
    blocktype_empty,
    blocktype_road,
    blocktype_house,
    blocktype_office,
};
struct string {
    uint32_t size;
    char* data;
};
struct block {
    enum blocktype type;
    uint8_t energy;
};
struct chunk {
    struct block block[chunk_size][chunk_size];
};
struct world {
    struct chunk chunk[world_size / chunk_size][world_size / chunk_size];
};
struct term {
    struct termios old;
    struct termios new;
    struct termios orig;
    int rawmode;
};
struct global {
    struct world world;
    struct term term;
    uint32_t tickrate;
    uint32_t cursor_edgethreshold;
    uint32_t cursor_x;
    uint32_t cursor_y;
    uint32_t camera_x;
    uint32_t camera_y;
    uint32_t screen_width;
    uint32_t screen_height;
};
static struct global global = {
    .term = {
        .rawmode = 0,
    },
    .tickrate = 10000,
    .cursor_edgethreshold = 5,
    .cursor_x = world_size / 2 + 3,
    .cursor_y = world_size / 2 + 3,
    .camera_x = world_size / 2,
    .camera_y = world_size / 2,
};
struct string string_init(char* data) {
    return (struct string){.size = 0, .data = data};
}
void string_push_char(struct string* dst, char src) {
    dst->data[dst->size] = src;
    dst->size += 1;
}
void string_push_str(struct string* dst, const char* src) {
    uint32_t src_size = strlen(src);
    memcpy(dst->data + dst->size, src, src_size);
    dst->size += src_size;
}
struct block* block_provide(uint32_t x, uint32_t y) {
    return &global.world.chunk[x / chunk_size][y / chunk_size].block[x % chunk_size][y % chunk_size];
}
const char* block_to_color(enum blocktype type) {
    switch (type) {
        case blocktype_empty:
            return "\x1b[40m";  // Black
        case blocktype_road:
            return "\x1b[47m";  // White
        case blocktype_house:
            return "\x1b[44m";  // Blue
        case blocktype_office:
            return "\x1b[42m";  // Green
        default:
            return "\x1b[40m";  // Black
    }
}
void term_disableRawMode() {
    if (global.term.rawmode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &global.term.orig);
        global.term.rawmode = 0;
    }
}
void term_editorAtExit(void) {
    term_disableRawMode(STDIN_FILENO);
}
void term_enableRawMode() {
    struct termios raw;
    if (global.term.rawmode) {
        return;
    }
    isatty(STDIN_FILENO);
    atexit(term_editorAtExit);
    tcgetattr(STDIN_FILENO, &global.term.orig);
    raw = global.term.orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    global.term.rawmode = 1;
}
void term_update() {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    global.screen_width = ws.ws_col;
    global.screen_height = ws.ws_row;
}
void term_init() {
    term_enableRawMode();
}
void update_camera() {
    global.camera_x = global.cursor_x;
    global.camera_y = global.cursor_y;
}
void update_cursor(uint32_t x, uint32_t y) {
    if (x >= world_size) {
        x = world_size - 1;
    }
    if (y >= world_size) {
        y = world_size - 1;
    }
    global.cursor_x = x;
    global.cursor_y = y;
    update_camera();
}
int input_readkey() {
    char c;
    int nread = read(STDIN_FILENO, &c, 1);
    if (nread == 0) {
        return -1;
    } else {
        return c;
    }
}
void input_update() {
    while (1) {
        int key = input_readkey();
        switch (key) {
            case -1:
                return;
            case '1': {
                struct block* block = block_provide(global.cursor_x, global.cursor_y);
                block->type = blocktype_empty;
                break;
            }
            case '2': {
                struct block* block = block_provide(global.cursor_x, global.cursor_y);
                block->type = blocktype_road;
                break;
            }
            case '3': {
                struct block* block = block_provide(global.cursor_x, global.cursor_y);
                block->type = blocktype_house;
                break;
            }
            case '4': {
                struct block* block = block_provide(global.cursor_x, global.cursor_y);
                block->type = blocktype_office;
                break;
            }
            case 's':
                update_cursor(global.cursor_x, global.cursor_y + 1);
                break;
            case 'a':
                update_cursor(global.cursor_x - 1, global.cursor_y);
                break;
            case 'w':
                update_cursor(global.cursor_x, global.cursor_y - 1);
                break;
            case 'd':
                update_cursor(global.cursor_x + 1, global.cursor_y);
                break;
            case 'q': 
                exit(0);
                break;
        }
    }
}
void render_update() {
    char buf_data[buf_size];
    struct string buf = string_init(buf_data);
    string_push_str(&buf, "\x1b[H");
    int32_t camera_left = global.camera_x - global.screen_width / 2 / 2;
    int32_t camera_top = global.camera_y - global.screen_height / 2;
    for (int32_t y = 0; y < global.screen_height; y++) {
        for (int32_t x = 0; x < global.screen_width / 2; x++) {
            int32_t world_x = camera_left + x;
            int32_t world_y = camera_top + y;
            if (world_x < 0 || world_y < 0 || world_x >= world_size || world_y >= world_size) {
                string_push_str(&buf, "\x1b[40m@@");
                continue;
            }
            struct block* block = block_provide(world_x, world_y);
            string_push_str(&buf, block_to_color(block->type));
            if (world_x == global.cursor_x && world_y == global.cursor_y) {
                string_push_str(&buf, "Cu");
            } else {
                string_push_str(&buf, "  ");
            }
        }
    }
    write(STDOUT_FILENO, buf.data, buf.size);
}
void update() {
    term_update();
    input_update();
    render_update();
    usleep(global.tickrate);
}
void init() {
    term_init();
}
int main() {
    init();
    while (1) {
        update();
    }
    return 0;
}