#include "main.h"
#include "raylib.h"
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_OBSTACLES 100

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
    DrawRectangle(0, 250, VIRTUAL_WIDTH, 150, (Color){194, 178, 128, 255});
    for (int y = 250; y < 250 + 150; y += 50) {
      for (int x = 0; x < VIRTUAL_WIDTH; x += 50) {
        DrawRectangleLines(x, y, 50, 50, RED);
      }
    }

    DrawRectangle(player.pos.x, player.pos.y, player.width, player.height,
                  BLUE);

    // for (int i = 0; i < MAX_OBSTACLES; i++) {
    //   DrawRectangle(obstacles[i].pos.x, obstacles[i].pos.y, 25, 60,
    //   DARKGREEN);
    // }
    // CACTUS
    // DrawRectangle(400, 260, 20, 60, DARKGREEN);
    // DrawRectangle(400, 340, 20, 60, DARKGREEN);

    EndDrawing();
  }

  CloseWindow();
  return 0;
}
