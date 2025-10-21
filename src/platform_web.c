// src/platform_web.c
#include "platform.h"
#include "raylib.h"
#include <emscripten/emscripten.h>
#include <stdlib.h>
#include <time.h>

Memory memory = {0};

extern void game_update_and_render(Memory *memory);

void load_assets(PermanentStorage *p) {
  p->tilesheet = LoadTexture("assets/tiles.png");
  p->bg_music = LoadMusicStream("assets/spagetti-western.ogg");
  SetMusicVolume(p->bg_music, 0.05f);
  p->hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(p->hit_sound, 0.5f);
  p->enemy_death_sound = LoadSound("assets/sfx_deathscream_human1.wav");
  SetSoundVolume(p->enemy_death_sound, 0.25f);
  p->player_gunshot = LoadSound("assets/desert-eagle-gunshot.wav");
  SetSoundVolume(p->player_gunshot, 0.1f);
}

void update_draw(void) { game_update_and_render(&memory); }

int main(void) {
  InitWindow(VIRTUAL_WIDTH * 2, VIRTUAL_HEIGHT * 2, "Yeehaw");
  InitAudioDevice();
  srand((unsigned int)time(NULL));

  load_assets(&memory.permanent);
  emscripten_set_main_loop(update_draw, 0, 1);
  return 0;
}
