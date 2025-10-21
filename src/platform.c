#include "platform.h"
#include "raylib.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#ifdef __APPLE__
#define GET_FILE_MOD_TIME(statbuf) ((statbuf).st_mtimespec.tv_sec)
#define DYNAMIC_LIB "build/game.dylib"
#define OPEN dlopen("build/game.dylib", RTLD_NOW);
#elif defined(__linux__)
#define GET_FILE_MOD_TIME(statbuf) ((statbuf).st_mtim.tv_sec)
#define DYNAMIC_LIB "build/game.so"
#else
#error "Unsupported platform"
#endif

Memory memory = {0};
typedef void (*game_update_and_render_fn)(Memory *memory);

// --------------------------------------------------
// Asset Loading
// --------------------------------------------------
void load_assets(PermanentStorage *p) {
  // Load textures
  p->tilesheet = LoadTexture("assets/tiles.png");

  // Load Music
  p->bg_music = LoadMusicStream("assets/spagetti-western.ogg");
  SetMusicVolume(p->bg_music, 0.05f);

  // Load SFX
  p->hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(p->hit_sound, 0.5f);

  p->enemy_death_sound = LoadSound("assets/sfx_deathscream_human1.wav");
  SetSoundVolume(p->enemy_death_sound, 0.25f);

  p->player_gunshot = LoadSound("assets/desert-eagle-gunshot.wav");
  SetSoundVolume(p->player_gunshot, 0.1f);
}

int main(void) {
  InitWindow(VIRTUAL_WIDTH * 2, VIRTUAL_HEIGHT * 2, "Yeehaw");
  // InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Yeehaw");
  SetTargetFPS(0);
  InitAudioDevice();
  srand((unsigned int)time(NULL));
  load_assets(&memory.permanent);

  void *game_lib = dlopen(DYNAMIC_LIB, RTLD_NOW);
  if (!game_lib) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
    return 1;
  }

  game_update_and_render_fn update = dlsym(game_lib, "game_update_and_render");
  if (!update) {
    fprintf(stderr, "dlsym failed: %s\n", dlerror());
    return 1;
  }

  struct stat file_info;
  stat(DYNAMIC_LIB, &file_info);
  time_t last_write_time = GET_FILE_MOD_TIME(file_info);

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(update_draw, 0, 1);
#else
  while (!WindowShouldClose()) {
    stat(DYNAMIC_LIB, &file_info);
    if (last_write_time < GET_FILE_MOD_TIME(file_info)) {
      printf("Reloading game code...\n");
      last_write_time = GET_FILE_MOD_TIME(file_info);
      dlclose(game_lib);
      game_lib = dlopen(DYNAMIC_LIB, RTLD_NOW);
      if (!game_lib) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return 1;
      }
      update = dlsym(game_lib, "game_update_and_render");
      if (!update) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        return 1;
      }
    }

    update(&memory);
  }
#endif

  CloseWindow();
  return 0;
}
