// (TODO) Fix collision
// (TODO) Read from the spritesheet propely
// (TODO) Implement shadows
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
static float game_timer = 0.0f;

#define MAX_ENTITIES 100
static Entity entitys[MAX_ENTITIES];
// (NOTE) maybe I'll need a max drawables or something and its own length but I
// think this is ok for now
static Entity *draw_list[MAX_ENTITIES];
static int entity_length;
static Entity player;
static Camera2D camera;

// Textures
static Texture2D tile000;
static Texture2D player_sprite;
static Texture2D tile_wall;
static Texture2D rock_texture;

static Texture2D border_tile;

static Music bg_music;
static Sound hit_sound;

// DEBUG
static bool debug_on = true;

int PLAY_AREA_START = -5;
int PLAY_AREA_END = 8;

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
static int cmp_draw_ptrs(const void *A, const void *B) {
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

void load_map1() {
  Vector2 first_wall = {PLAY_AREA_START, -20};
  Vector2 last_wall = {PLAY_AREA_END, -20};
  int wall_length = PLAY_AREA_END - PLAY_AREA_START;
  int start = 0;
  entitys[start] = (Entity){.pos.x = PLAY_AREA_START, .pos.y = first_wall.y};
  // entitys[wall_length] =
  //     (Entity){.pos.x = PLAY_AREA_END - 1.5, .pos.y = last_wall.y};

  for (int i = 0; i < MAX_ENTITIES; i++) {
    if (i < wall_length) {
      // entitys[i] = (Entity){.pos.x = i + PLAY_AREA_START, .pos.y = -20};
      // entity_length++;
    }
  }
}

void init_game(void) {
  // Load Textures
  player_sprite = LoadTexture("assets/horse.png");
  tile_wall = LoadTexture("assets/tileset/tile_057.png");
  tile000 = LoadTexture("assets/tileset/tile_009.png");
  // tile000 = LoadTexture("assets/tileset/tile_014.png");
  rock_texture = LoadTexture("assets/tileset/tile_055.png");
  border_tile = LoadTexture("assets/tileset/tile_036.png");

  // Load Music
  bg_music = LoadMusicStream("assets/spagetti-western.ogg");
  SetMusicVolume(bg_music, 0.05f);
  PlayMusicStream(bg_music);

  // Load SFX
  hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(hit_sound, 0.5f);

  // Init Game Objects
  player = (Entity){
      .type = PLAYER,
      .pos.x = 0,
      .pos.y = 0,
      .width = 1.0f,
      .height = 1.0f,
      .color = WHITE,
      .damage_cooldown = 0,
      .current_health = 5,
      .max_health = 5,
  };
  camera = (Camera2D){
      .target = (Vector2){0, 0},
      .offset = (Vector2){0, 0},
      .rotation = 0.0f,
      .zoom = 1.0f,
  };

  // Reset timers
  game_timer = 0;
  load_map1();
}

void update_draw(void) {
  // Reset on death
  if (player.current_health <= 0) {
    init_game();
  }

  UpdateMusicStream(bg_music);
  float dt = GetFrameTime();

  // Scale game to window size
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

  if (IsKeyPressed(KEY_P)) {
    debug_on = !debug_on;
  }

  // --- Update ---
  player.color = WHITE;
  game_timer += dt;
  // player.pos.y -= 10 * dt;
  // player.pos.y -= 5 * dt;
  // (TODO)clamp find a better way to reuse these tile values
  player.pos.x = Clamp(player.pos.x, -5.5, 7);
  Vector2 player_screen = isometric_projection((Vector3){0, player.pos.y, 0});

  // Update camera position to follow player
  camera.target = player_screen;
  camera.offset = (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

  // Player-entity collision
  for (int i = 0; i < MAX_ENTITIES; i++) {
    Entity *entity = &entitys[i];
    entity->width = 0.5f;
    entity->height = 0.1f;
    if (entity->type != PLAYER && player.damage_cooldown <= 0) {
      Vector2 player_center = {player.pos.x, player.pos.y};
      float player_radius = player.width / 2.0f;

      Vector2 entity_center = {entity->pos.x, entity->pos.y};
      float entity_radius = entity->width / 2.0f;

      float sum = entity_radius + player_radius;
      float dist = Vector2Distance(player_center, entity_center);
      if (dist < sum) {
        player.damage_cooldown = 0.5f;
        player.current_health--;
        shake_timer = 0.5f;
        PlaySound(hit_sound);
      }
    }
  }

  // Player-entity collision
  // Rectangle player_rect = {player.pos.x, player.pos.y, player.width,
  //                          player.height};
  // for (int i = 0; i < MAX_ENTITIES; i++) {
  //   Entity *entity = &entitys[i];
  //   Rectangle harard_rect = {entity->pos.x, entity->pos.y, entity->width,
  //                            entity->height};
  //   if (CheckCollisionRecs(player_rect, harard_rect) &&
  //       player.damage_cooldown <= 0) {
  //     PlaySound(hit_sound);
  //     // player.current_health--;
  //     player.damage_cooldown = 0.5f;
  //     // shake_timer = 0.5f;
  //
  //     DrawRectangleLines(player_rect.x, player_rect.y, player_rect.width,
  //                        player_rect.height, BLACK);
  //   }
  // }

  if (player.damage_cooldown > 0.0f) {
    player.damage_cooldown -= dt;
    player.color = RED;
  }

  // Screen shake
  if (shake_timer > 0) {
    shake_timer -= dt;
    float offset_x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float shake_magnitude = 10.0f;
    camera.target.x += offset_x * shake_magnitude;
  }

  // Update Draw List
  draw_list[0] = &player;
  for (int i = 1; i < MAX_ENTITIES; i++) {
    Entity *entity = &entitys[i];
    draw_list[i] = entity;
  }
  qsort(draw_list, MAX_ENTITIES, sizeof(Entity *), cmp_draw_ptrs);

  // --- Draw ---
  BeginDrawing();
  ClearBackground(ORANGE);

  BeginMode2D(camera);

  int tile_y = floorf(player.pos.y);
  int tile_y_offset = 30;
  // Draw world
  // (NOTE) figure out how to start from 0 instead of a negative number
  for (int y = tile_y - 30; y < tile_y + 14; y++) {

    // draw from the edge of the screen to the player area (-5)
    for (int x = -21; x < -5; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      DrawTextureV(border_tile, screen, WHITE);
    }

    // Draw play area
    for (int x = PLAY_AREA_START; x < PLAY_AREA_END; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      // not sure if this shit smooths teh game either, idts
      screen.x = floorf(screen.x);
      screen.y = floorf(screen.y);
      DrawTextureV(tile000, screen, WHITE);
    }

    // draw from the end of the play are to the right half of the screen
    for (int x = 8; x < 22; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      DrawTextureV(border_tile, screen, WHITE);
    }
  }

  // --- Draw Entities ---
  for (int i = 0; i < MAX_ENTITIES; i++) {
    Entity *entity = draw_list[i];
    // cull
    if (entity->pos.y < player.pos.y - 40 ||
        entity->pos.y > player.pos.y + 20) {
      continue;
    }

    Vector2 projected =
        isometric_projection((Vector3){entity->pos.x, entity->pos.y, 0});

    if (entity->type == PLAYER) {
      // PLAYER ANIMATION
      int frame_count = 2;
      frame_timer += dt;
      if (frame_timer >= 0.15f) { // adjust for speed
        frame_timer = 0.0f;
        current_frame = (current_frame + 1) % frame_count;
      }

      // DRAW PLAYER
      Rectangle source = {16 * frame_count, 16, 16, 16};
      Rectangle dest = {.x = projected.x,
                        .y = projected.y,
                        .width = player.width * 28,
                        .height = player.height * 28};
      Vector2 origin = {player.width / 2.0f, player.height};
      DrawTexturePro(player_sprite, source, dest, origin, 0, player.color);
    } else {
      DrawTextureV(rock_texture, projected, WHITE);
    }

    if (debug_on) {
      float radius = entity->width / 2.0f;
      DrawEllipseLines(projected.x + TILE_WIDTH / 2.0f,
                       projected.y + TILE_WIDTH / 2.0f, TILE_WIDTH * radius,
                       TILE_HEIGHT * radius, RED);
    }
  }

  EndMode2D();

  // Draw player health
  for (int i = 0; i < player.max_health; i++) {
    DrawRectangle((i * 35), 30, 25, 25, i < player.current_health ? RED : GRAY);
  }

  float scale = fminf(scale_x, scale_y);
  int font_size = (int)(40 * scale);
  const char *timer_text = TextFormat("%.1f", game_timer);
  int text_width = MeasureText(timer_text, font_size);
  DrawText(timer_text, (GetScreenWidth() - text_width) / 2, 0, font_size,
           WHITE);
  DrawFPS(0, 0);
  EndDrawing();
}

int main(void) {
  // InitWindow(1920, 1080, "Yeehaw");
  InitWindow(VIRTUAL_WIDTH * 3, VIRTUAL_HEIGHT * 3, "Yeehaw");
  // Uncap FPS because it makes my game feel like tash
  SetTargetFPS(0);
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
