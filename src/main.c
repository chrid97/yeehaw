#include "main.h"
#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define MAX_OBSTACLES 100

const int VIRTUAL_WIDTH = 800;
const int VIRTUAL_HEIGHT = 450;
const int PLAYER_WIDTH = 40;
const int PLAYER_HEIGHT = 25;
const int TILE_WIDTH = 100;
const int TILE_HEIGHT = 50;

void test(char *text) {
  DrawText(text, GetScreenWidth() / 2, GetScreenHeight() / 2, 100, RED);
}

int x_start = VIRTUAL_WIDTH / 2 - TILE_WIDTH / 2;
int y_start = 50;
void draw_tile(Vector2 pos) {
  int half_width = TILE_WIDTH / 2;
  int half_height = TILE_HEIGHT / 2;

  int screen_x = x_start + (pos.x - pos.y) * half_width;
  int screen_y = y_start + (pos.x + pos.y) * half_height;

  // DrawRectangleLines(screen_x, screen_y, TILE_WIDTH, TILE_HEIGHT, BLACK);
  Vector2 top = {.x = screen_x + half_width, screen_y};
  Vector2 left = {.x = screen_x, screen_y + half_height};
  Vector2 right = {.x = screen_x + TILE_WIDTH, screen_y + half_height};
  Vector2 bottom = {.x = screen_x + half_width, screen_y + TILE_HEIGHT};
  // DrawCircle(right.x, right.y, 5, RED);
  // DrawCircle(top.x, top.y, 5, RED);
  // DrawCircle(left.x, left.y, 5, RED);
  // DrawCircle(bottom.x, bottom.y, 5, RED);

  DrawTriangle(top, left, right, GRAY);
  DrawTriangle(bottom, right, left, GRAY);
  DrawLineV(top, left, BLACK);
  DrawLineV(top, right, BLACK);
  DrawLineV(bottom, right, BLACK);
  DrawLineV(bottom, left, BLACK);
}

int main(void) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Yeehaw");
  SetTargetFPS(60);

  Entity player = {
      .width = PLAYER_WIDTH,
      .height = PLAYER_HEIGHT,
      .pos = {.x = 0, .y = VIRTUAL_HEIGHT / 2.0f},
      .velocity.x = 1,
      .velocity.y = 1,
  };

  srand(time(NULL));

  Entity obstacles[MAX_OBSTACLES] = {0};
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    // int rand_x = rand() % VIRTUAL_WIDTH + VIRTUAL_WIDTH;
    // int rand_y = rand() % VIRTUAL_HEIGHT;
    // obstacles[i] = (Entity){
    //     .pos.x = rand_x,
    //     .pos.y = rand_y,
    //     .width = 25,
    //     .height = 25,
    // };
  }

  float spawn_timer = 2.0f;
  float world_offset = 0;
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();
    // Screen scaling
    int screen_width = GetScreenWidth();
    int screen_height = GetScreenHeight();
    float delta = GetFrameTime();
    float scale_x = (float)screen_width / VIRTUAL_WIDTH;
    float scale_y = (float)screen_height / VIRTUAL_HEIGHT;
    float scale = (scale_x < scale_y) ? scale_x : scale_y;

    // --------------- //
    // ---- Input ---- //
    // --------------- //
    world_offset += 100.0f * dt;
    player.velocity = (Vector2){0};
    if (IsKeyDown(KEY_D)) {
      player.velocity.x = 1.0f;
    }

    if (IsKeyDown(KEY_A)) {
      player.velocity.x = -1.0f;
    }

    if (IsKeyDown(KEY_W)) {
      player.velocity.y = -1.0f;
    }

    if (IsKeyDown(KEY_S)) {
      player.velocity.y = 1.0f;
    }

    // ---------------- //
    // ---- Update ---- //
    // ---------------- //
    float move_speed = 100.0f;
    player.pos.x += player.velocity.x * move_speed * dt;
    player.pos.y += player.velocity.y * move_speed * dt;

    spawn_timer -= dt;
    if (spawn_timer < 0) {
      spawn_timer = 0.0;
    }
    if (spawn_timer == 0.0) {
      spawn_timer = 2.0f;
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].is_active) {
          obstacles[i].is_active = true;
          obstacles[i].width = 25;
          obstacles[i].height = 25;
          obstacles[i].pos.x = world_offset + VIRTUAL_WIDTH + (rand() % 300);
          obstacles[i].pos.y = rand() % VIRTUAL_HEIGHT;
          break;
        }
      }
    }

    for (int i = 0; i < MAX_OBSTACLES; i++) {
      Entity *obstacle = &obstacles[i];
      if (!obstacle->is_active) {
        continue;
      }

      if (obstacle->pos.x + obstacle->width < world_offset) {
        int rand_x = (world_offset + VIRTUAL_WIDTH) + rand() % 300;
        int rand_y = rand() % VIRTUAL_HEIGHT;
        obstacles[i] = (Entity){
            .pos.x = rand_x,
            .pos.y = rand_y,
            .width = 25,
            .height = 25,
            .is_active = true,
        };
      }
    }

    // ---------------- //
    // ----- Draw ----- //
    // ---------------- //
    BeginDrawing();
    ClearBackground((Color){235, 200, 150, 255});

    DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
                  BLUE);
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      DrawRectangle(obstacles[i].pos.x - world_offset, obstacles[i].pos.y, 25,
                    25, DARKGREEN);
    }

    DrawFPS(0, 0);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
