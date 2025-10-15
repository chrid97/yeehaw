#include "main.h" // your own shared types
#include "raylib.h"
#include <math.h>
#include <stdlib.h>

extern Texture2D tilesheet; // exists in main.c

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

// Sort pointers in draw_list by on-screen depth
int compare_entities_for_draw_order(const void *A, const void *B) {
  const Entity *a = *(Entity *const *)A; // note the extra *
  const Entity *b = *(Entity *const *)B;

  float da = a->pos.x + a->pos.y;
  float db = b->pos.x + b->pos.y;

  if (da < db)
    return -1;
  if (da > db)
    return 1;

  if (a->pos.y < b->pos.y)
    return -1;
  if (a->pos.y > b->pos.y)
    return 1;
  if (a->pos.x < b->pos.x)
    return -1;
  if (a->pos.x > b->pos.x)
    return 1;

  return 0;
}

void draw_iso_cube(Vector3 center, float w, float h, float height,
                   float tilt_angle, Color color) {
  Vector3 base[4] = {
      {center.x - w / 2, center.y - h / 2, center.z}, // NW
      {center.x + w / 2, center.y - h / 2, center.z}, // NE
      {center.x + w / 2, center.y + h / 2, center.z}, // SE
      {center.x - w / 2, center.y + h / 2, center.z}  // SW
  };

  Vector3 top[4];
  for (int i = 0; i < 4; i++) {
    top[i] = base[i];
    top[i].z += height;
  }

  // --- Apply visible tilt only to top face ---
  float bank_strength = sinf(tilt_angle * DEG2RAD) * height * 0.6f;

  // raise/lower the top corners instead of shifting base horizontally
  for (int i = 0; i < 4; i++) {
    if (i == 0 || i == 3) { // left side
      top[i].z += bank_strength;
    } else { // right side
      top[i].z -= bank_strength;
    }
  }

  // --- Project ---
  Vector2 bp[4], tp[4];
  for (int i = 0; i < 4; i++) {
    bp[i] = isometric_projection(base[i]);
    tp[i] = isometric_projection(top[i]);
  }

  // --- Draw edges ---
  for (int i = 0; i < 4; i++)
    DrawLineV(bp[i], tp[i], Fade(color, 0.6f));
  for (int i = 0; i < 4; i++) {
    int next = (i + 1) % 4;
    DrawLineV(bp[i], bp[next], Fade(color, 0.4f));
    DrawLineV(tp[i], tp[next], color);
  }
}

void DrawPlayerDebug(Entity *player) {
  float a = (player->angle - 90.0f) * DEG2RAD; // same basis as movement
  Vector2 forward = {cosf(a), sinf(a)};
  Vector2 right = {-sinf(a), cosf(a)};

  Vector2 pos =
      isometric_projection((Vector3){player->pos.x, player->pos.y, 0});
  Vector2 f_end = isometric_projection((Vector3){
      player->pos.x + forward.x * 2.0f, player->pos.y + forward.y * 2.0f, 0});
  Vector2 r_end = isometric_projection((Vector3){
      player->pos.x + right.x * 1.5f, player->pos.y + right.y * 1.5f, 0});
  Vector2 v_end =
      isometric_projection((Vector3){player->pos.x + player->vel.x * 0.25f,
                                     player->pos.y + player->vel.y * 0.25f, 0});

  DrawLineV(pos, f_end, GREEN); // forward
  DrawLineV(pos, r_end, BLUE);  // right
  DrawLineV(pos, v_end, RED);   // velocity

  DrawText(TextFormat("Angle: %.1f°", player->angle), pos.x + 20, pos.y - 20, 8,
           WHITE);
}

Rectangle get_tile_source_rect(int tile_index) {
  int tiles_per_row = tilesheet.width / TILE_SIZE;
  int tx = tile_index % tiles_per_row;
  int ty = tile_index / tiles_per_row;
  return (Rectangle){tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE};
}
