#include "main.h"
#include "raylib.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int VIRTUAL_WIDTH = 800;
const int VIRTUAL_HEIGHT = 450;
const int PLAYER_WIDTH = 50;
const int PLAYER_HEIGHT = 40;

void test(char *text) {
  DrawText(text, GetScreenWidth() / 2, GetScreenHeight() / 2, 100, RED);
}

int random_between(int min, int max) { return min + rand() % (max - min + 1); }

int main(void) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Yeehaw");
  SetTargetFPS(60);

  Entity player = {
      .width = PLAYER_WIDTH,
      .height = PLAYER_HEIGHT,
      .pos = {.x = 0, .y = VIRTUAL_HEIGHT - PLAYER_HEIGHT},
      .velocity.x = 4,
      .velocity.y = 4,
  };

  int map[90] = {0};

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
      player.pos.x += player.velocity.x;
    }

    if (IsKeyDown(KEY_A)) {
      player.pos.x -= player.velocity.x;
    }

    // ---------------- //
    // ---- Update ---- //
    // ---------------- //

    // ---------------- //
    // ----- Draw ----- //
    // ---------------- //
    BeginDrawing();
    ClearBackground((Color){235, 200, 150, 255});

    for (int i = -10; i < 50; i++) {
      DrawRectangle(i * 200, 300 + 40, 200, 100, (Color){194, 178, 128, 255});
    }

    DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
                  BLUE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
