#include <stdio.h>

void update(float dt) {
    printf("[game] update dt=%.3f\n", dt);
}

void render(void) {
    printf("[game] render frame\n");
}

int get_score(void) {
    return 42;
}
