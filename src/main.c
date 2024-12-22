#include <stdio.h>
#include <string.h>
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
    int size;
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
struct global {
    struct world world;
    int camera_x;
    int camera_y;
    int cursor_x;
    int cursor_y;
};

static struct global global = (struct global){
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
    int src_size = strlen(src);
    memcpy(dst->data + dst->size, src, src_size);
    dst->size += src_size;
}
void render() {
    char buf_data[buf_size];
    struct string buf = string_init(buf_data);
    string_push_str(&buf, "\x1b[H");
    string_push_str(&buf, "Good morning world!");
    write(STDOUT_FILENO, buf.data, buf.size);
}
int main() {
    while (1) {
        render();
        usleep(10000);
    }
}