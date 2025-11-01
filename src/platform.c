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

typedef void (*GameUpdateAndRender)(Memory *memory, GameInput *game_input);

void collect_input(GameInput *input) {
  input->move_up = IsKeyDown(KEY_W);
  input->move_down = IsKeyDown(KEY_S);
  input->move_left = IsKeyDown(KEY_A);
  input->move_right = IsKeyDown(KEY_D);
  input->reload = IsKeyPressed(KEY_R);
  input->mute = IsKeyPressed(KEY_M);

  input->screen_width = GetScreenWidth();
  input->screen_height = GetScreenHeight();
}

void load_game() {
  void *game_lib = dlopen(DYNAMIC_LIB, RTLD_NOW);
  if (!game_lib) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
  }
}

int main(void) {
  Memory game_memory = {0};
  game_memory.permanent_storage_size = Megabytes(64);
  game_memory.transient_storage_size = Gigabytes(1);
  game_memory.permanent_storage = calloc(1, game_memory.permanent_storage_size);
  game_memory.transient_storage = calloc(1, game_memory.transient_storage_size);

  InitWindow(960, 540, "Yeehaw");
  SetTargetFPS(60);

  InitAudioDevice();
  srand((unsigned int)time(NULL));

  void *game_lib = dlopen(DYNAMIC_LIB, RTLD_NOW);
  if (!game_lib) {
    fprintf(stderr, "dlopen failed: %s\n", dlerror());
    return 1;
  }

  GameUpdateAndRender game_update_and_render =
      dlsym(game_lib, "game_update_and_render");
  if (!game_update_and_render) {
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
      game_update_and_render = dlsym(game_lib, "game_update_and_render");
      if (!game_update_and_render) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        return 1;
      }
    }
    GameInput input = {0};
    collect_input(&input);
    game_update_and_render(&game_memory, &input);
  }
#endif

  CloseWindow();
  return 0;
}
