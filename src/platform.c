#include "platform.h"
#include "raylib.h"
#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

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
  PlayMusicStream(p->bg_music);

  // Load SFX
  p->hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(p->hit_sound, 0.5f);
}

int main(void) {
  InitWindow(VIRTUAL_WIDTH * 2, VIRTUAL_HEIGHT * 2, "Yeehaw");
  SetTargetFPS(0);
  InitAudioDevice();
  srand((unsigned int)time(NULL));
  load_assets(&memory.permanent);
  void *game_lib = dlopen("build/game.dylib", RTLD_NOW);
  game_update_and_render_fn update = dlsym(game_lib, "game_update_and_render");
  if (!update) {
    printf("Failed to load game function: %s\n", dlerror());
    return 1;
  }

  struct stat file_info;
  stat("build/game.dylib", &file_info);
  time_t last_write_time = file_info.st_mtimespec.tv_sec;

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(update_draw, 0, 1);
#else
  while (!WindowShouldClose()) {
    stat("build/game.dylib", &file_info);
    if (last_write_time < file_info.st_mtimespec.tv_sec) {
      printf("Reloading game code...\n");
      last_write_time = file_info.st_mtimespec.tv_sec;
      dlclose(game_lib);
      game_lib = dlopen("build/game.dylib", RTLD_NOW);
      update = dlsym(game_lib, "game_update_and_render");
    }

    update(&memory);
  }
#endif

  CloseWindow();
  return 0;
}
