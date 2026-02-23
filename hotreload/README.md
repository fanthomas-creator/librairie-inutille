# hotreload

Moteur de hot reload universel écrit en C. Modifie ton code source pendant que ton programme tourne — il se met à jour automatiquement, sans redémarrer.

Supporte : **C, C++, Rust, Zig, Go** — sur **Linux, macOS, Windows**.

---

## Comment ça fonctionne

```
Tu sauvegardes un fichier
        ↓
Le watcher détecte le changement (inotify / kqueue / ReadDirectoryChanges)
        ↓
Le differ analyse ce qui a changé
        ↓
L'adapter du bon langage recompile en .so / .dylib / .dll
        ↓
Le loader fait dlclose + dlopen (swap atomique)
        ↓
Ton programme continue avec le nouveau code, sans s'arrêter
```

---

## Prérequis

| Outil | Usage |
|---|---|
| `cmake >= 3.16` | Build system |
| `gcc` / `clang` | Pour les modules C |
| `g++` / `clang++` | Pour les modules C++ |
| `rustc` + `rustfilt` | Pour les modules Rust |
| `zig` | Pour les modules Zig |
| `go` | Pour les modules Go |

Tu n'as besoin que des outils correspondant aux langages que tu utilises.

---

## Installation

### Depuis les sources

```bash
git clone --no-checkout https://github.com/fanthomas-creator/librairie-inutille.git
cd librairie-inutille
git sparse-checkout init --cone
git sparse-checkout set hotreload
git checkout main
cd hotreload
make build
```

Cela produit `build/libhotreload.so` (Linux), `build/libhotreload.dylib` (macOS) ou `build/hotreload.dll` (Windows).

### Build statique

```bash
make static
```

Produit `build/libhotreload.a` — aucune dépendance dynamique.

### Build release (optimisé, sans exemples)

```bash
make release
```

### Installer dans le système

```bash
make install PREFIX=/usr/local
```

Cela copie :
- `libhotreload.so` → `/usr/local/lib/`
- `hotreload.h`    → `/usr/local/include/`

### Vérifier les outils disponibles

```bash
make info
```

---

## Intégration dans un projet existant

### Option A — CMake (recommandé)

```cmake
add_subdirectory(hotreload)
target_link_libraries(ton_projet PRIVATE hotreload)
```

### Option B — Compilation directe

```bash
gcc -I hotreload/include main.c -L hotreload/build -lhotreload -o mon_programme
```

### Option C — Statique (aucune dépendance runtime)

```bash
gcc -I hotreload/include main.c hotreload/build/libhotreload.a -o mon_programme
```

---

## Utilisation dans le code

### Étape 1 — Inclure le header

```c
#include "hotreload.h"
```

### Étape 2 — Initialiser le contexte

```c
hr_config_t cfg = hr_default_config();

hr_context_t* ctx = hr_init(
    "./src",      // dossier à surveiller
    HR_LANG_AUTO, // détection automatique du langage
    &cfg
);
```

### Étape 3 — Charger un module

```c
hr_module_t* mod = hr_load(ctx, "./src/game.c");
```

### Étape 4 — Récupérer des fonctions

```c
typedef void (*update_fn)(float);
update_fn update = (update_fn) hr_get_fn(mod, "update");
```

### Étape 5 — Appeler hr_poll dans ta boucle principale

```c
while (running) {
    hr_poll(ctx);  // vérifie les changements de fichiers

    // toujours récupérer l'adresse après poll — elle peut avoir changé
    update_fn update = (update_fn) hr_get_fn(mod, "update");
    if (update) update(delta_time);
}
```

### Étape 6 — Nettoyer

```c
hr_shutdown(ctx);
```

---

## Exemple complet (C)

**main.c** — le host, ne change pas

```c
#include "hotreload.h"
#include <stdio.h>

typedef void (*update_fn)(float);
typedef void (*render_fn)(void);

static void on_reload(const char* path, hr_result_t res) {
    printf("reloaded %s -> %s\n", path, hr_result_str(res));
}

int main(void) {
    hr_config_t cfg  = hr_default_config();
    cfg.on_reload    = on_reload;
    cfg.log_level    = HR_LOG_INFO;

    hr_context_t* ctx = hr_init(".", HR_LANG_C, &cfg);
    hr_module_t*  mod = hr_load(ctx, "game.c");

    while (1) {
        hr_poll(ctx);

        update_fn update = (update_fn) hr_get_fn(mod, "update");
        render_fn render = (render_fn) hr_get_fn(mod, "render");

        if (update) update(0.016f);
        if (render) render();

        sleep_ms(16);
    }

    hr_shutdown(ctx);
}
```

**game.c** — le module reloadable, modifie-le à la volée

```c
#include <stdio.h>

void update(float dt) {
    printf("update dt=%.3f\n", dt);  // change cette ligne et sauvegarde
}

void render(void) {
    printf("render\n");
}
```

---

## Adapter ton module selon le langage

### C
Aucun changement requis. Les fonctions C sont exportées par défaut.

```c
void update(float dt) { ... }
```

### C++
Utilise `extern "C"` pour éviter le name mangling.

```cpp
extern "C" {
    void update(float dt) { ... }
    void render(void) { ... }
}
```

### Rust
Utilise `#[no_mangle]` et `extern "C"`.

```rust
#[no_mangle]
pub extern "C" fn update(dt: f32) {
    println!("update {}", dt);
}
```

Compile avec `rustc --crate-type=cdylib` (géré automatiquement par hotreload).

### Zig
Utilise `export` — la C ABI est native en Zig.

```zig
export fn update(dt: f32) void {
    std.debug.print("update {}\n", .{dt});
}
```

### Go
Utilise `//export` via cgo.

```go
package main

import "C"
import "fmt"

//export update
func update(dt float32) {
    fmt.Printf("update %f\n", dt)
}

func main() {}
```

---

## Préserver l'état lors d'un reload

Par défaut l'état est perdu lors d'un reload (les variables locales du module sont réinitialisées). Pour le préserver, utilise les callbacks `save_state` / `restore_state`.

```c
typedef struct {
    int  score;
    int  level;
    float player_x;
    float player_y;
} game_state_t;

static game_state_t g_state = {0};

void my_save(hr_state_t* s) {
    s->data = malloc(sizeof(game_state_t));
    s->size = sizeof(game_state_t);
    memcpy(s->data, &g_state, sizeof(game_state_t));
}

void my_restore(hr_state_t* s) {
    if (s->data && s->size == sizeof(game_state_t)) {
        memcpy(&g_state, s->data, sizeof(game_state_t));
    }
}

// dans main()
cfg.save_state    = my_save;
cfg.restore_state = my_restore;
```

---

## Configuration complète

```c
hr_config_t cfg = hr_default_config();

cfg.log_level        = HR_LOG_DEBUG;   // NONE, ERROR, WARN, INFO, DEBUG
cfg.compiler_flags   = "-DDEBUG=1 -I./include";  // flags passés au compilateur
cfg.build_dir        = "./.hotreload"; // dossier des .so compilés
cfg.poll_interval_ms = 50;             // intervalle de polling en ms
cfg.enable_patching  = 1;              // 1 = memory patching activé, 0 = reload complet uniquement
cfg.save_state       = my_save;        // optionnel
cfg.restore_state    = my_restore;     // optionnel
cfg.on_reload        = my_callback;    // appelé après chaque reload
```

---

## API complète

```c
// Initialisation
hr_config_t   hr_default_config(void);
hr_context_t* hr_init(const char* watch_dir, hr_lang_t lang, const hr_config_t* config);
void          hr_shutdown(hr_context_t* ctx);

// Modules
hr_module_t*  hr_load(hr_context_t* ctx, const char* source_path);
void          hr_unload(hr_context_t* ctx, hr_module_t* mod);
hr_result_t   hr_reload_module(hr_context_t* ctx, hr_module_t* mod);

// Boucle principale
hr_result_t   hr_poll(hr_context_t* ctx);

// Fonctions
void*         hr_get_fn(hr_module_t* mod, const char* name);

// Utilitaires
const char*   hr_result_str(hr_result_t result);
const char*   hr_version(void);
```

### Langages disponibles pour `hr_init`

| Constante | Langage |
|---|---|
| `HR_LANG_AUTO` | Détection automatique par extension de fichier |
| `HR_LANG_C` | C (.c) |
| `HR_LANG_CPP` | C++ (.cpp, .cc, .cxx) |
| `HR_LANG_RUST` | Rust (.rs) |
| `HR_LANG_ZIG` | Zig (.zig) |
| `HR_LANG_GO` | Go (.go) |

### Codes de retour `hr_result_t`

| Code | Signification |
|---|---|
| `HR_OK` | Succès |
| `HR_ERR_COMPILE` | Erreur de compilation (voir stderr) |
| `HR_ERR_LOAD` | Impossible de charger la bibliothèque |
| `HR_ERR_SYMBOL` | Symbole introuvable |
| `HR_ERR_PATCH` | Échec du memory patching |
| `HR_ERR_PLATFORM` | Erreur de plateforme |
| `HR_ERR_INVALID` | Argument invalide |

---

## Structure du projet

```
hotreload/
├── include/
│   └── hotreload.h              API publique
├── src/
│   ├── core/
│   │   ├── hr_engine.c          Coordination principale
│   │   ├── hr_watcher.c         Surveillance des fichiers
│   │   ├── hr_differ.c          Analyse du type de changement
│   │   ├── hr_loader.c          Chargement / swap dlopen
│   │   ├── hr_patcher.c         Memory patching (trampolines JMP)
│   │   └── hr_symbols.c         Table des symboles
│   ├── platform/
│   │   ├── hr_platform.h        Interface commune
│   │   ├── hr_platform_linux.c  inotify + mmap
│   │   ├── hr_platform_mac.c    kqueue + mmap
│   │   └── hr_platform_windows.c ReadDirectoryChanges + VirtualAlloc
│   └── adapters/
│       ├── hr_adapter.h         Interface commune
│       ├── hr_adapter_c.c       gcc/clang
│       ├── hr_adapter_cpp.c     g++/clang++
│       ├── hr_adapter_rust.c    rustc
│       ├── hr_adapter_zig.c     zig
│       └── hr_adapter_go.c      go build
├── examples/
│   ├── demo_c/
│   ├── demo_cpp/
│   └── demo_rust/
├── CMakeLists.txt
└── Makefile
```

---

## Limitations connues

- **Memory patching** : fonctionne uniquement sur x86_64 et ARM64. Sur les autres architectures, le reload complet (dlopen) est utilisé à la place.
- **Go** : le hot reload Go via cgo est le plus lent à compiler. Pour les projets Go complexes, préférer une architecture modulaire explicite.
- **Windows + DLL lock** : sur Windows, les `.dll` peuvent être lockés par l'OS. La librairie copie le `.dll` dans un fichier temporaire avant de le charger pour contourner ce problème.
- **Threads** : si une fonction est en cours d'exécution dans un autre thread au moment du reload, comportement indéfini. Toujours reloader entre deux frames ou à un point de synchronisation connu.
- **C++ vtables** : les vtables ne sont pas mises à jour automatiquement. Les objets existants continuent d'utiliser l'ancien code. Pour les classes C++, préférer des fonctions `extern "C"` stateless.
- **État global du module** : les variables statiques et globales du module reloadé sont réinitialisées à chaque reload. Utilise `save_state` / `restore_state` pour les conserver.

---

## Licence

MIT
