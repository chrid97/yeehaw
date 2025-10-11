// (NOTE) Replace obstacle with hazard
#include "main.h"
#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
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
static float shake_timer = 0.0f;
static float last_spawn_y = 0.0f;

#define MAX_OBSTACLES 100
static Entity obstacles[MAX_OBSTACLES];
static Entity player;
static Camera2D camera;

// Textures
static Texture2D tile000;
static Texture2D player_sprite;
static Texture2D tile_wall;
static Texture2D rock_texture;

static Music bg_music;
static Sound hit_sound;

int random_between(int min, int max) {
  if (max <= min) {
    return min;
  }
  return min + rand() % (max - min + 1);
}

Vector2 isometric_projection(Vector3 pos) {
  return (Vector2){(pos.x - pos.y) * (TILE_WIDTH / 2.0f),
                   (pos.x + pos.y) * (TILE_HEIGHT / 2.0f) - pos.z};
}

void init_game(void) {
  // Load Textures
  player_sprite = LoadTexture("assets/horse.png");
  tile_wall = LoadTexture("assets/tileset/tile_057.png");
  tile000 = LoadTexture("assets/tileset/tile_000.png");
  rock_texture = LoadTexture("assets/tileset/tile_055.png");

  // Load Music
  bg_music = LoadMusicStream("assets/spagetti-western.ogg");
  SetMusicVolume(bg_music, 0.35f);
  PlayMusicStream(bg_music);

  // Load SFX
  hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(hit_sound, 0.5f);

  // Init Game Objects
  player = (Entity){
      .pos.x = 0,
      .pos.y = 0,
      .width = 1,
      .height = 1,
      .color = WHITE,
      .damage_cooldown = 0,
  };
  camera = (Camera2D){
      .target = (Vector2){0, 0},
      .offset = (Vector2){0, 0},
      .rotation = 0.0f,
      // (TODO) Add button screen rotation to test
      // rotation 10 doesnt look terrible maybe ill use that
      // .rotation = 10.0f,
      .zoom = 1.0f,
  };
}

void update_draw(void) {
  static float smooth_dt = 0.016f;
  float raw_dt = GetFrameTime();
  smooth_dt = Lerp(smooth_dt, raw_dt, 0.1f);
  float dt = smooth_dt;
  UpdateMusicStream(bg_music);

  // scale
  float scale_x = (float)GetScreenWidth() / VIRTUAL_WIDTH;
  float scale_y = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
  camera.zoom = fminf(scale_x, scale_y);

  player.color = WHITE;

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
  // (TODO)clamp find a better way to reuse these tile values
  player.pos.x = Clamp(player.pos.x, -5.5, 7);
  Vector2 player_screen = isometric_projection((Vector3){0, player.pos.y, 0});
  // camera.target = player_screen;
  // not sure if this lerp shit makes it smoother or not
  camera.target = Vector2Lerp(camera.target, player_screen, 10 * dt);
  camera.offset = (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

  if (player.damage_cooldown > 0.0f) {
    player.damage_cooldown -= dt;
    player.color = RED;
  }

  // Player-hazard collision
  Rectangle player_rect = {player.pos.x, player.pos.y, player.width,
                           player.height};
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    Entity *hazard = &obstacles[i];
    Rectangle harard_rect = {hazard->pos.x, hazard->pos.y, hazard->width,
                             hazard->height};
    if (CheckCollisionRecs(player_rect, harard_rect) &&
        player.damage_cooldown <= 0) {
      PlaySound(hit_sound);
      player.current_health--;
      player.damage_cooldown = 0.5f;
      shake_timer = 0.5f;
    }
  }

  // Screen shake
  if (shake_timer > 0) {
    shake_timer -= dt;
    float offset_x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float shake_magnitude = 10.0f;
    camera.target.x += offset_x * shake_magnitude;
  }

  // spawn hazards
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    Entity *obstacle = &obstacles[i];
    if (obstacle->pos.y > player.pos.y + 10) {
      last_spawn_y -= random_between(0, 10);
      obstacle->pos.x = random_between(-5, 7);
      obstacle->pos.y = last_spawn_y;

      if (rand() % 4 == 0) {
        int cluster_count = random_between(3, 5);
        float cluster_offset_x = random_between(1, 3);
        float cluster_offset_y = random_between(0, 2);

        for (int j = 0; j < cluster_count; j++) {
          for (int k = 0; k < MAX_OBSTACLES; k++) {
            Entity *extra = &obstacles[k];
            if (extra->pos.y > player.pos.y + 10) {
              extra->width = 1;
              extra->height = 1;
              extra->pos.x = Clamp(obstacle->pos.x +
                                       (rand() % 2 ? cluster_offset_x * j
                                                   : -cluster_offset_x * j) +
                                       ((rand() % 3) - 1) * 0.3f,
                                   -5, 7);
              extra->pos.y = obstacle->pos.y - (cluster_offset_y * j) +
                             ((rand() % 3) - 1) * 0.5f;
              break;
            }
          }
        }
      }
    }
  }

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
  int tile_y = floorf(player.pos.y);
  for (int y = tile_y - 30; y < tile_y + 14; y++) {
    for (int x = -5; x < 8; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      screen.x = floorf(screen.x);
      screen.y = floorf(screen.y);
      DrawTextureV(tile000, screen, WHITE);
    }
  }

  // Draw Hazards
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    Entity *obstacle = &obstacles[i];
    Vector2 obj =
        isometric_projection((Vector3){obstacle->pos.x, obstacle->pos.y, 0});
    DrawTextureV(rock_texture, obj, WHITE);
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
                    .width = player.width * 28,
                    .height = player.height * 28};
  Vector2 origin = {player.width / 2.0f, player.height};
  DrawTexturePro(player_sprite, source, dest, origin, 0, player.color);

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
