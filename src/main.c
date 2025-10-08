#include "main.h"
#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>

#define MAX_OBSTACLES 100

const int VIRTUAL_WIDTH = 800;
const int VIRTUAL_HEIGHT = 450;
const int PLAYER_WIDTH = 50;
const int PLAYER_HEIGHT = 40;
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
      .pos = {.x = 0, .y = VIRTUAL_HEIGHT - PLAYER_HEIGHT},
      .velocity.x = 1,
      .velocity.y = 1,
  };

  int map[90] = {0};

  float worldOffset = 0.0f;
  float worldSpeed = 300.0f;
  float scroll = fmodf(worldOffset, VIRTUAL_WIDTH);

  Entity obstacles[MAX_OBSTACLES] = {0};
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i] = (Entity){.pos.x = i * 70, .pos.y = 340};
  }
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
    if (IsKeyDown(KEY_D)) {
      player.velocity.x = player.velocity.x;
    }

    if (IsKeyDown(KEY_A)) {
      player.velocity.x = -player.velocity.x;
    }

    if (IsKeyDown(KEY_W)) {
      player.pos.y -= player.velocity.y;
    }

    if (IsKeyDown(KEY_S)) {
      player.pos.y += player.velocity.y;
    }

    player.pos.x += player.velocity.x;

    // ---------------- //
    // ---- Update ---- //
    // ---------------- //
    worldOffset += worldSpeed * dt;

    // ---------------- //
    // ----- Draw ----- //
    // ---------------- //
    BeginDrawing();
    ClearBackground((Color){235, 200, 150, 255});

    // GROUND
    // DrawRectangle(0, 250, VIRTUAL_WIDTH, 150, (Color){194, 178, 128, 255});
    // for (int y = 250; y < 250 + 150; y += 50) {
    //   for (int x = 0; x < VIRTUAL_WIDTH; x += 50) {
    //     DrawRectangleLines(x, y, 50, 50, RED);
    //   }
    // }
    //
    // DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
    //               BLUE);

    // for (int i = 0; i < MAX_OBSTACLES; i++) {
    //   DrawRectangle(obstacles[i].pos.x, obstacles[i].pos.y, 25, 60,
    //   DARKGREEN);
    // }
    // CACTUS
    // DrawRectangle(400, 260, 20, 60, DARKGREEN);
    // DrawRectangle(400, 340, 20, 60, DARKGREEN);

    // int start = screen_width / 2;
    // // DrawRectangleLines(start, 0, TILE_WIDTH, TILE_HEIGHT, BLACK);
    //
    // Vector2 top = {.x = start + (TILE_WIDTH / 2), .y = 0};
    // Vector2 right = {.x = start + TILE_WIDTH, .y = TILE_HEIGHT / 2};
    // Vector2 left = {.x = start, .y = TILE_HEIGHT / 2};
    // Vector2 bottom = {.x = start + TILE_WIDTH / 2, .y = TILE_HEIGHT};
    //
    // DrawTriangle(right, top, bottom, GRAY);
    // DrawTriangle(top, left, bottom, GRAY);
    // DrawLineV(top, left, BLACK);
    // DrawLineV(top, right, BLACK);
    // DrawLineV(bottom, left, BLACK);
    // DrawLineV(bottom, right, BLACK);
    //
    // // DrawRectangleLines(start + TILE_WIDTH / 2, TILE_HEIGHT / 2,
    // TILE_WIDTH,
    // //                    TILE_HEIGHT, BLACK);
    // Vector2 top2 = {.x = top.x + TILE_WIDTH / 2, .y = top.y + TILE_HEIGHT /
    // 2}; Vector2 right2 = {.x = right.x + (TILE_WIDTH / 2),
    //                   .y = right.y + TILE_HEIGHT / 2};
    // Vector2 left2 = {.x = left.x + TILE_WIDTH / 2,
    //                  .y = left.y + (TILE_HEIGHT / 2)};
    // Vector2 bottom2 = {.x = bottom.x + TILE_WIDTH / 2,
    //                    .y = bottom.y + (TILE_HEIGHT / 2)};
    //
    // DrawTriangle(right2, top2, bottom2, GRAY);
    // DrawTriangle(top2, left2, bottom2, GRAY);
    // DrawLineV(top2, left2, BLACK);
    // DrawLineV(top2, right2, BLACK);
    // DrawLineV(bottom2, left2, BLACK);
    // DrawLineV(bottom2, right2, BLACK);

    draw_tile((Vector2){.x = 0, .y = 0});

    draw_tile((Vector2){.x = 1, .y = 0});
    draw_tile((Vector2){.x = 2, .y = 0});

    draw_tile((Vector2){.x = 0, .y = 1});
    draw_tile((Vector2){.x = 0, .y = 2});

    draw_tile((Vector2){.x = 1, .y = 1});
    draw_tile((Vector2){.x = 2, .y = 2});
    draw_tile((Vector2){.x = 2, .y = 1});

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
