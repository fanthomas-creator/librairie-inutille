#include "hotreload.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void (*update_fn)(float);
typedef void (*render_fn)(void);
typedef int  (*get_score_fn)(void);

static void on_reload(const char* path, hr_result_t result) {
    printf("[host] reload %s -> %s\n", path, hr_result_str(result));
}

int main(void) {
    printf("hotreload demo v%s\n", hr_version());
    printf("Edit game_module.c while this runs to see hot reload in action.\n\n");

    hr_config_t cfg    = hr_default_config();
    cfg.log_level      = HR_LOG_INFO;
    cfg.on_reload      = on_reload;
    cfg.enable_patching = 1;

    hr_context_t* ctx = hr_init(".", HR_LANG_C, &cfg);
    if (!ctx) { fprintf(stderr, "hr_init failed\n"); return 1; }

    hr_module_t* mod = hr_load(ctx, "game_module.c");
    if (!mod) { fprintf(stderr, "hr_load failed\n"); hr_shutdown(ctx); return 1; }

    int running = 1;
    int frame   = 0;

    while (running && frame < 200) {
        hr_poll(ctx);

        update_fn   update    = (update_fn)   hr_get_fn(mod, "update");
        render_fn   render    = (render_fn)   hr_get_fn(mod, "render");
        get_score_fn get_score = (get_score_fn) hr_get_fn(mod, "get_score");

        if (update)    update(0.016f);
        if (render)    render();
        if (get_score) printf("[host] score=%d\n", get_score());

        frame++;

#ifdef _WIN32
        Sleep(500);
#else
        struct timespec ts = {0, 500000000L};
        nanosleep(&ts, NULL);
#endif
    }

    hr_shutdown(ctx);
    return 0;
}
