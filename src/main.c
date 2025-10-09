#include "main.h"
#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
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

float clamp(float value, float min, float max) {
  if (value <= min) {
    return min;
  }
  if (value >= max) {
    return max;
  }

  return value;
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
      .pos = {.x = 100, .y = VIRTUAL_HEIGHT / 2.0f},
      .velocity.x = 1,
      .velocity.y = 1,
      .current_health = 5,
      .max_health = 5,
      .color = BLUE,
      .damage_cooldown = 0,
  };

  srand((unsigned int)time(NULL));

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
  Camera2D camera = {
      .target = player.pos,
      .offset = (Vector2){50, VIRTUAL_HEIGHT / 2.0f},
      .rotation = 0.0f,
      .zoom = 1.0f,
  };
  while (!WindowShouldClose()) {
    float dt = GetFrameTime();

    // player.velocity = (Vector2){0};
    player.velocity.y = 0;
    player.color = BLUE;
    // --------------- //
    // ---- Input ---- //
    // --------------- //

    if (IsKeyDown(KEY_D)) {
      player.velocity.x = 1.0f;
    }

    // (NOTE) Disable this until i change the movement code to increase/decrease
    // velocity gradually since this is letting the player move backwards
    //
    // if (IsKeyDown(KEY_A)) {
    //   player.velocity.x = -1.0f;
    // }

    if (IsKeyDown(KEY_W)) {
      player.velocity.y = -1.0f;
    }

    if (IsKeyDown(KEY_S)) {
      player.velocity.y = 1.0f;
    }

    // ---------------- //
    // ---- Update ---- //
    // ---------------- //

    float move_speed = 200.0f;
    player.pos.x += player.velocity.x * move_speed * dt;
    player.pos.y += player.velocity.y * move_speed * dt;
    player.pos.y = clamp(player.pos.y, 0, VIRTUAL_HEIGHT - player.height);

    camera.target.x = floorf(player.pos.x);

    if (player.damage_cooldown > 0.0f) {
      player.damage_cooldown -= dt;
    }

    spawn_timer -= dt;
    if (spawn_timer <= 0.0) {
      spawn_timer = 2.0f;
      for (int i = 0; i < MAX_OBSTACLES; i++) {
        if (!obstacles[i].is_active) {
          obstacles[i].is_active = true;
          obstacles[i].width = 25;
          obstacles[i].height = 25;
          obstacles[i].pos.x = camera.target.x + VIRTUAL_WIDTH + (rand() % 300);
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

      if (obstacle->pos.x + obstacle->width < camera.target.x) {
        // (NOTE)
        // this can just be is->active false but I like the amount of blocks
        // being spawned. i guess i could also get rid of the other code
        // obstacle->is_active = false;
        int rand_x = (camera.target.x + VIRTUAL_WIDTH) + rand() % 300;
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

    for (int i = 0; i < MAX_OBSTACLES; i++) {
      Entity *obstacle = &obstacles[i];
      Rectangle player_rect = {.x = player.pos.x,
                               .y = player.pos.y,
                               .width = player.width,
                               .height = player.height};
      Rectangle object_rect = {.x = obstacle->pos.x,
                               .y = obstacle->pos.y,
                               .width = obstacle->width,
                               .height = obstacle->height};

      if (CheckCollisionRecs(player_rect, object_rect) &&
          player.damage_cooldown <= 0) {
        player.current_health--;
        player.color = RED;
        player.damage_cooldown = 0.5f;
      }
    }

    // Start over
    if (player.current_health <= 0) {
      player = (Entity){
          .width = PLAYER_WIDTH,
          .height = PLAYER_HEIGHT,
          .pos = {.x = 0, .y = VIRTUAL_HEIGHT / 2.0f},
          .velocity.x = 1,
          .velocity.y = 1,
          .current_health = 5,
          .max_health = 5,
          .color = BLUE,
          .damage_cooldown = 0,
      };

      for (int i = 0; i < MAX_OBSTACLES; i++) {
        obstacles[i] = (Entity){0};
      }
      spawn_timer = 2.0f;
      camera.target = player.pos;
    }

    // ---------------- //
    // ----- Draw ----- //
    // ---------------- //
    BeginDrawing();
    ClearBackground((Color){235, 200, 150, 255});
    BeginMode2D(camera);
    DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
                  player.color);
    for (int i = 0; i < MAX_OBSTACLES; i++) {
      DrawRectangle(obstacles[i].pos.x, obstacles[i].pos.y, 25, 25, DARKGREEN);
    }
    EndMode2D();

    // DRAW HEALTH
    for (int i = 0; i < player.max_health; i++) {
      DrawRectangle((i * 35), 30, 25, 25,
                    i < player.current_health ? RED : GRAY);
    }

    DrawFPS(0, 0);
    EndDrawing();
  }

  CloseWindow();
  return 0;
}
