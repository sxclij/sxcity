#include <stdio.h>
#include <unistd.h>

#define chunk_size 256
#define world_size 4096

enum blocktype {
    blocktype_null,
    blocktype_empty,
    blocktype_human,
    blocktype_road,
    blocktype_house_0,
    blocktype_house_1,
    blocktype_office_0,
    blocktype_office_1,
};

struct block {
    enum blocktype ty;
};
struct chunk {
    struct block block[chunk_size][chunk_size];
};
struct world {
    struct chunk chunk[world_size / chunk_size][world_size / chunk_size];
};
struct global {
    struct world world;
};

static struct global global;

void render() {
    write(STDOUT_FILENO, "\x1b[H", 3);
    write(STDOUT_FILENO, "Good morning world!", 19);
}

int main() {
    while (1) {
        render();
        usleep(10000);
    }
}