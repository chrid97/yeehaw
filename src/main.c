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

bool DEBUG = 0;

void test(char *text) {
  DrawText(text, GetScreenWidth() / 2, GetScreenHeight() / 2, 100, RED);
}

int random_between(int min, int max) { return min + rand() % (max - min + 1); }

int main(void) {
  InitWindow(VIRTUAL_WIDTH, VIRTUAL_HEIGHT, "Yeehaw");
  SetTargetFPS(60);

  Entity player = {.width = PLAYER_WIDTH,
                   .height = PLAYER_HEIGHT,
                   .pos = {.x = 0, .y = VIRTUAL_HEIGHT - PLAYER_HEIGHT}};

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
    if (IsKeyPressed(KEY_O)) {
      DEBUG = !DEBUG;
    }

    // ---------------- //
    // ---- Update ---- //
    // ---------------- //

    // ---------------- //
    // ----- Draw ----- //
    // ---------------- //
    BeginDrawing();
    ClearBackground(BROWN);

    DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
                  BLUE);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
