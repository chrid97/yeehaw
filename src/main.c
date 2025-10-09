#include "main.h"
#include "raylib.h"
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

#define MAX_OBSTACLES 100

const int VIRTUAL_WIDTH = 800;
const int VIRTUAL_HEIGHT = 450;
const int PLAYER_WIDTH = 40;
const int PLAYER_HEIGHT = 25;
const int TILE_WIDTH = 100;
const int TILE_HEIGHT = 50;

// Player movement area
// (NOTE) maybe I want to move this a bit lower? and make it smaller? shrugs
float PLAYER_LOWER_BOUND_Y = VIRTUAL_HEIGHT * 0.5f;
float PLAYER_UPPER_BOUND_Y = VIRTUAL_HEIGHT * 0.85f;

static Entity player;
static Entity obstacles[MAX_OBSTACLES];
static Camera2D camera;
static float spawn_timer = 2.0f;
static float shake_timer = 0.0f;

/// inclusive
int random_between(int min, int max) {
  if (max <= min) {
    return min;
  }
  return min + rand() % (max - min + 1);
}

int random_betweenf(float min, float max) {
  if (max <= min) {
    return min;
  }
  float scale = rand() / (float)RAND_MAX;
  return min + scale * (max - min);
}

float clamp(float value, float min, float max) {
  if (value <= min) {
    return min;
  }
  if (value >= max) {
    return max;
  }
  return value;
}

void init_game(void) {
  float player_starting_y_position =
      (PLAYER_LOWER_BOUND_Y +
       (PLAYER_UPPER_BOUND_Y - PLAYER_LOWER_BOUND_Y) / 2.0f) -
      PLAYER_HEIGHT / 2.0f;
  player = (Entity){
      .width = PLAYER_WIDTH,
      .height = PLAYER_HEIGHT,
      .pos = {.x = 100, .y = player_starting_y_position},
      .velocity = {.x = 1, .y = 1},
      .current_health = 5,
      .max_health = 5,
      .color = BLUE,
      .damage_cooldown = 0,
  };

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i] = (Entity){0};
  }

  camera = (Camera2D){
      .target = player.pos,
      .offset = (Vector2){50, player_starting_y_position},
      .rotation = 0.0f,
      .zoom = 1.0f,
  };

  spawn_timer = 2.0f;
  shake_timer = 0.0f;
}

void update_draw(void) {
  float dt = GetFrameTime();
  player.velocity.y = 0;
  player.color = BLUE;

  // --- Input ---
  if (IsKeyDown(KEY_D)) {
    player.velocity.x = 1.0f;
  }
  if (IsKeyDown(KEY_W)) {
    player.velocity.y = -1.0f;
  }
  if (IsKeyDown(KEY_S)) {
    player.velocity.y = 1.0f;
  }

  // --- Update ---
  player.pos.x += player.velocity.x * 700.0f * dt;
  player.pos.y += player.velocity.y * 200.0f * dt;
  player.pos.y = clamp(player.pos.y, PLAYER_LOWER_BOUND_Y,
                       PLAYER_UPPER_BOUND_Y - player.height);

  camera.target.x = floorf(player.pos.x);

  if (player.damage_cooldown > 0.0f) {
    player.damage_cooldown -= dt;
    player.color = RED;
  }

  // Spawn obstacles periodically
  spawn_timer -= dt;
  if (spawn_timer <= 0.0f) {
    spawn_timer = 2.0f;
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      if (!obstacles[i].is_active) {
        obstacles[i].is_active = true;
        obstacles[i].width = 25;
        obstacles[i].height = 25;
        obstacles[i].pos.x = camera.target.x + VIRTUAL_WIDTH + (rand() % 300);
        // obstacles[i].pos.y = rand() % VIRTUAL_HEIGHT;
        obstacles[i].pos.y = random_betweenf(PLAYER_LOWER_BOUND_Y - 100,
                                             PLAYER_LOWER_BOUND_Y + 100);
        break;
      }
    }
  }

  // Move and recycle obstacles
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    Entity *obstacle = &obstacles[i];
    if (!obstacle->is_active)
      continue;

    if (obstacle->pos.x + obstacle->width < camera.target.x) {
      int rand_x = (camera.target.x + VIRTUAL_WIDTH) + rand() % 300;
      int rand_y = rand() % VIRTUAL_HEIGHT;
      *obstacle = (Entity){
          .pos.x = rand_x,
          .pos.y = rand_y,
          .width = 25,
          .height = 25,
          .is_active = true,
      };
    }
  }

  // Check collisions
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    Entity *obstacle = &obstacles[i];
    Rectangle player_rect = {player.pos.x, player.pos.y, player.width,
                             player.height};
    Rectangle object_rect = {obstacle->pos.x, obstacle->pos.y, obstacle->width,
                             obstacle->height};

    if (CheckCollisionRecs(player_rect, object_rect) &&
        player.damage_cooldown <= 0) {
      player.current_health--;
      player.damage_cooldown = 0.5f;
      shake_timer = 0.5f;
    }
  }

  // Screen shake
  if (shake_timer > 0) {
    shake_timer -= dt;
    float offset_x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
    float shake_magnitude = 10.0f;
    camera.target.x += offset_x * shake_magnitude;
  }

  // Reset on death
  if (player.current_health <= 0) {
    init_game();
  }

  // --- Draw ---
  BeginDrawing();
  ClearBackground((Color){235, 200, 150, 255});
  // GROUND
  DrawRectangle(0, PLAYER_LOWER_BOUND_Y, VIRTUAL_WIDTH,
                PLAYER_UPPER_BOUND_Y - PLAYER_LOWER_BOUND_Y, BROWN);
  BeginMode2D(camera);
  DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
                player.color);
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (obstacles[i].is_active)
      DrawRectangle(obstacles[i].pos.x, obstacles[i].pos.y, 25, 25, DARKGREEN);
  }
  EndMode2D();

  for (int i = 0; i < player.max_health; i++) {
    DrawRectangle((i * 35), 30, 25, 25, i < player.current_health ? RED : GRAY);
  }

  DrawFPS(0, 0);
  EndDrawing();
}

int main(void) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Yeehaw");
  SetTargetFPS(60);

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
