#include <cstdio>

extern "C" {
    void update(float dt) {
        printf("[game:cpp] update dt=%.3f\n", dt);
    }
    void render(void) {
        printf("[game:cpp] render\n");
    }
}
