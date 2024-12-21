#include <unistd.h>
#include <stdio.h>

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