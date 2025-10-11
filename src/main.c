#include "main.h"
#include "raylib.h"
// #include "raymath.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 360;
const int TILE_WIDTH = 32;
const int TILE_HEIGHT = 16;
static int current_frame = 0;
static float frame_timer = 0.0f;

#define MAX_OBSTACLES 100
static Entity obstacles[MAX_OBSTACLES];
static Entity player;
static Camera2D camera;

// Textures
static Texture2D tile000;
static Texture2D player_sprite;
static Texture2D tile_wall;

static Music bg_music;
static Sound hit_sound;

Vector2 isometric_projection(Vector3 pos) {
  return (Vector2){(pos.x - pos.y) * (TILE_WIDTH / 2.0f),
                   (pos.x + pos.y) * (TILE_HEIGHT / 2.0f) - pos.z};
}

void init_game(void) {
  // Load Textures
  player_sprite = LoadTexture("assets/horse.png");
  tile_wall = LoadTexture("assets/tileset/tile_057.png");
  tile000 = LoadTexture("assets/tileset/tile_000.png");

  // Load Music
  bg_music = LoadMusicStream("assets/spagetti-western.ogg");
  SetMusicVolume(bg_music, 0.35f);
  PlayMusicStream(bg_music);

  // Load SFX
  hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(hit_sound, 0.5f);

  // Init Game Objects
  player = (Entity){
      .pos.x = 0, .pos.y = 0, .width = 32, .height = 32, .color = WHITE};
  camera = (Camera2D){.target = (Vector2){0, 0},
                      .offset = (Vector2){0, 0},
                      .rotation = 0.0f,
                      .zoom = 1.0f};
}

void update_draw(void) {
  float dt = GetFrameTime();
  UpdateMusicStream(bg_music);

  float scale_x = (float)GetScreenWidth() / VIRTUAL_WIDTH;
  float scale_y = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
  camera.zoom = fminf(scale_x, scale_y);

  // --- Input ---
  float speed = 5.0f;
  if (IsKeyDown(KEY_A)) {
    player.pos.x -= dt * speed;
  }
  if (IsKeyDown(KEY_D)) {
    player.pos.x += dt * speed;
  }
  if (IsKeyDown(KEY_W)) {
    player.pos.y -= dt * speed;
  }
  if (IsKeyDown(KEY_S)) {
    player.pos.y += dt * speed;
  }

  // --- Update ---

  player.pos.y -= 5 * dt;
  Vector2 player_screen = isometric_projection((Vector3){0, player.pos.y, 0});
  camera.target = player_screen;
  camera.offset = (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

  // --- Draw ---
  BeginDrawing();
  ClearBackground(ORANGE);

  BeginMode2D(camera);

  // Draw left edge of world
  // (NOTE) This hsould probably be offset by the play area
  // float start_x = -2;
  // float start_y = 20;
  // for (int y = -37; y < 1; y++) {
  //   for (int layer = 0; layer < 1; layer++) {
  //     Vector3 wall_pos = {start_x - 1, start_y + y, layer * 10};
  //     Vector2 wall_screen = isometric_projection(wall_pos);
  //     DrawTextureV(tile_wall, wall_screen, WHITE);
  //   }
  // }

  // Draw world
  // (NOTE) figure out how to start from 0 instead of a negative number
  for (int y = -200; y < 30; y++) {
    for (int x = -5; x < 8; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      DrawTextureV(tile000, screen, WHITE);
    }
  }

  // Draw player
  int frame_count = 2;
  frame_timer += dt;
  if (frame_timer >= 0.15f) { // adjust for speed
    frame_timer = 0.0f;
    current_frame = (current_frame + 1) % frame_count;
  }
  Vector2 projected =
      isometric_projection((Vector3){player.pos.x, player.pos.y, 0});
  Rectangle source = {16 * current_frame, 16, 16, 16};
  Rectangle dest = {.x = projected.x,
                    .y = projected.y,
                    .width = player.width,
                    .height = player.height};
  DrawTexturePro(player_sprite, source, dest, (Vector2){0, 0}, 0, player.color);

  EndMode2D();

  DrawFPS(0, 0);
  EndDrawing();
}

int main(void) {
  InitWindow(1920, 1080, "Yeehaw");
  // InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Yeehaw");
  SetTargetFPS(60);
  InitAudioDevice();
  srand((unsigned int)time(NULL));
  init_game();

#ifdef PLATFORM_WEB
  emscripten_set_main_loop(update_draw, 0, 1);
#else
  while (!WindowShouldClose()) {
    update_draw();
  }
#endif

  CloseWindow();
  return 0;
}
