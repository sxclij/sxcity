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
#define buf_size 4096

enum blocktype {
    blocktype_null,
    blocktype_empty,
    blocktype_human,
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
    uint32_t screen_width;
    uint32_t screen_height;
};
struct global {
    struct world world;
    struct term term;
    uint32_t camera_x;
    uint32_t camera_y;
    uint32_t cursor_x;
    uint32_t cursor_y;
};

static struct global global = {
    .term = {
        .rawmode = 0,
    },
    .camera_x = world_size / 2,
    .camera_y = world_size / 2,
    .cursor_x = world_size / 2 + 3,
    .cursor_y = world_size / 2 + 3,
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
char* block_to_char(enum blocktype type) {
    switch (type) {
        case blocktype_empty:
            return "..";
        case blocktype_human:
            return "Hu";
        case blocktype_road:
            return "Ro";
        case blocktype_house:
            return "Ho";
        case blocktype_office:
            return "Of";
        default:
            return "  ";
    }
}
void term_disableRawMode(int fd) {
    if (global.term.rawmode) {
        if (tcsetattr(fd, TCSAFLUSH, &global.term.orig) == -1) {
            perror("tcsetattr error");
        }
        global.term.rawmode = 0;
    }
}
void term_editorAtExit(void) {
    term_disableRawMode(STDIN_FILENO);
}
void term_enableRawMode(int fd) {
    struct termios raw;
    if (global.term.rawmode)
        return;
    if (!isatty(fd)) {
        fprintf(stderr, "Not a TTY\n");
        exit(1);
    }
    if (atexit(term_editorAtExit) != 0) {
        perror("atexit failed");
        exit(1);
    }
    if (tcgetattr(fd, &global.term.orig) == -1) {
        perror("tcgetattr error");
        exit(1);
    }
    raw = global.term.orig;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSAFLUSH, &raw) < 0) {
        perror("tcsetattr error");
        exit(1);
    }
    global.term.rawmode = 1;
}
void term_init() {
    term_enableRawMode(STDIN_FILENO);
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        global.term.screen_width = 80;
        global.term.screen_height = 24;
    } else {
        global.term.screen_width = ws.ws_col / 2;
        global.term.screen_height = ws.ws_row;
    }
}
void input_tick() {
}
void render_tick() {
    char buf_data[buf_size];
    struct string buf = string_init(buf_data);
    string_push_str(&buf, "\x1b[H");
    uint32_t camera_left = global.camera_x - global.term.screen_width / 2;
    uint32_t camera_top = global.camera_y - global.term.screen_height / 2;
    for (int32_t y = global.term.screen_height - 1; y >= 0; y--) {
        for (uint32_t x = 0; x < global.term.screen_width; x++) {
            uint32_t world_x = camera_left + x;
            uint32_t world_y = camera_top + y;
            if (world_x == global.cursor_x && world_y == global.cursor_y) {
                string_push_str(&buf, "Cu");
            } else if (world_x == global.camera_x && world_y == global.camera_y) {
                string_push_str(&buf, "Ca");
            } else if (world_x < world_size && world_y < world_size) {
                struct block* block = block_provide(world_x, world_y);
                string_push_str(&buf, block_to_char(block->type));
            } else {
                string_push_str(&buf, "  ");
            }
        }
    }
    write(STDOUT_FILENO, buf.data, buf.size);
}
void tick() {
    input_tick();
    render_tick();
    usleep(10000);
}
void init() {
    term_init();
}
int main() {
    init();
    while (1) {
        tick();
    }
    return 0;
}