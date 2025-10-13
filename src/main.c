// (TODO) Squish player on damage
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
// Start at one becasue I add the player at the end of every update loop
static int entity_length = 1;
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
int cmp_draw_ptrs(const void *A, const void *B) {
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

// I guess this can add to the entity array then reutrn a pointer to the newly
// created enitty? or should i just return a new entitY?
Entity *entity_spawn() {}

void load_map1() {
  Vector2 first_wall = {PLAY_AREA_START, -20};
  Vector2 last_wall = {PLAY_AREA_END, -20};
  int wall_length = PLAY_AREA_END - PLAY_AREA_START;
  // entitys[0] = (Entity){.pos.x = PLAY_AREA_START,
  //                       .pos.y = first_wall.y,
  //                       .width = 1,
  //                       .height = 1,
  //                       .type = ENTITY_HAZARD};
  // entity_length++;
  // entitys[wall_length] =
  //     (Entity){.pos.x = PLAY_AREA_END - 1.5, .pos.y = last_wall.y};

  for (int i = 0; i < wall_length; i++) {
    if (i % 9 == 0) {
      continue;
    }
    if (i < wall_length) {
      entitys[i] = (Entity){.pos.x = i + PLAY_AREA_START,
                            .pos.y = -20,
                            .width = 1,
                            .height = 1,
                            .type = ENTITY_HAZARD,
                            .color = WHITE};
      entity_length++;
    }
  }

  // printf("%i\n", entity_length);

  for (int i = 0; i < wall_length; i++) {
    if (i % 9 == 0) {
      continue;
    }

    if (i < wall_length + wall_length) {
      entitys[entity_length++] = (Entity){.pos.x = i + PLAY_AREA_START,
                                          .pos.y = -22,
                                          .width = 1,
                                          .height = 1,
                                          .type = ENTITY_HAZARD,
                                          .color = WHITE};
    }
  }

  for (int i = 0; i < wall_length; i++) {
    if (i % 9 == 0) {
      continue;
    }

    if (i < wall_length + wall_length) {
      entitys[entity_length++] = (Entity){.pos.x = i + PLAY_AREA_START,
                                          .pos.y = -24,
                                          .width = 1,
                                          .height = 1,
                                          .type = ENTITY_HAZARD,
                                          .color = WHITE};
    }
  }
}

void init_game(void) {
  // Load Textures
  player_sprite = LoadTexture("assets/iso-horse.png");
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
      .type = ENTITY_PLAYER,
      .pos.x = 0,
      .pos.y = 0,
      .width = 1.0f * 0.5,
      .height = 0.75f,
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

  // Entity/map stuff
  for (int i = 0; i < MAX_ENTITIES; i++) {
    entitys[i].pos = (Vector2){9999.0f, 9999.0f}; // hide uninitialized entities
  }
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
  if (IsKeyPressed(KEY_R)) {
    init_game();
  }

  // --- Update ---
  player.color = WHITE;
  game_timer += dt;
  if (debug_on) {
    player.pos.y -= 8 * dt;
  }
  // player.pos.y -= 5 * dt;
  // (TODO)clamp find a better way to reuse these tile values
  player.pos.x = Clamp(player.pos.x, -5.5, 8);
  Vector2 player_screen = isometric_projection((Vector3){0, player.pos.y, 0});

  // Update camera position to follow player
  camera.target = player_screen;
  camera.offset = (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

  // Player-entity collision
  // for (int i = 0; i < entity_length; i++) {
  // Entity *entity = &entitys[i];
  // entity->width = 1.0f;
  // entity->height = 1.0f;
  // if (entity->type == ENTITY_HAZARD && player.damage_cooldown <= 0) {
  //   Vector2 player_center = {player.pos.x, player.pos.y};
  //   float player_radius = player.width / 2.0f;
  //
  //   Vector2 entity_center = {entity->pos.x, entity->pos.y};
  //   float entity_radius = entity->width / 2.0f;
  //
  //   float sum = entity_radius + player_radius;
  //   float dist = Vector2Distance(player_center, entity_center);
  //   if (dist < sum) {
  //     player.damage_cooldown = 0.5f;
  //     // player.current_health--;
  //     // shake_timer = 0.5f;
  //     PlaySound(hit_sound);
  //   }
  // }
  // }

  // Player-entity collision
  Rectangle player_rect = {player.pos.x, player.pos.y, player.width,
                           player.height};
  for (int i = 0; i < MAX_ENTITIES; i++) {
    Entity *entity = &entitys[i];
    Rectangle harard_rect = {entity->pos.x, entity->pos.y, entity->width,
                             entity->height};
    if (CheckCollisionRecs(player_rect, harard_rect) &&
        player.damage_cooldown <= 0) {
      PlaySound(hit_sound);
      // player.current_health--;
      player.damage_cooldown = 0.5f;
      // shake_timer = 0.5f;

      DrawRectangleLines(player_rect.x, player_rect.y, player_rect.width,
                         player_rect.height, BLACK);
    }
  }

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
  int draw_count = 0;
  for (int i = 0; i < entity_length; i++) {
    draw_list[draw_count++] = &entitys[i];
  }
  draw_list[draw_count++] = &player;
  qsort(draw_list, draw_count, sizeof(Entity *), cmp_draw_ptrs);

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
  for (int i = 0; i < entity_length; i++) {
    // printf("whats happening\n");
    Entity *entity = draw_list[i];
    // cull
    if (entity->pos.y < player.pos.y - 40 ||
        entity->pos.y > player.pos.y + 20) {
      continue;
    }

    Vector2 projected =
        isometric_projection((Vector3){entity->pos.x, entity->pos.y, 0});

    if (entity->type == ENTITY_PLAYER) {
      // PLAYER ANIMATION
      int frame_count = 2;
      frame_timer += dt;
      if (frame_timer >= 0.15f) { // adjust for speed
        frame_timer = 0.0f;
        current_frame = (current_frame + 1) % frame_count;
      }

      // DRAW PLAYER
      // Rectangle source = {16 * frame_count, 16, 16, 16};
      // Rectangle source = {0, 0, 28, 28};
      // Rectangle dest = {.x = projected.x,
      //                   .y = projected.y,
      //                   .width = player.width * 28,
      //                   .height = player.height * 28};
      // Vector2 origin = {player.width / 2.0f, player.height};
      // DrawTexturePro(player_sprite, source, dest, origin, 0, player.color);
    }

    if (entity->type == ENTITY_HAZARD) {
      // The debug circles are being drawn in the middle of the sprite so i
      // think if we want to the collision to be at the bottom of the sprite we
      // should move the sprite up?
      // Vector2 origin = {entity->width / 2.0f, entity->height};
      // Vector2 origin = {entity->width / 2.0f, 0};
      // DrawTextureV(rock_texture, projected, WHITE);
      // DrawTexturePro(rock_texture, (Rectangle){0, 0, 32, 32},
      //                (Rectangle){projected.x, projected.y,
      //                rock_texture.width,
      //                            rock_texture.height},
      //                origin, 0, entity->color);
    }

    // if (debug_on) {
    Rectangle rect = {entity->pos.x, entity->pos.y, entity->width,
                      entity->height};

    // Project corners to screen
    Vector2 p1 = isometric_projection((Vector3){rect.x, rect.y, 0});
    Vector2 p2 =
        isometric_projection((Vector3){rect.x + rect.width, rect.y, 0});
    Vector2 p3 = isometric_projection(
        (Vector3){rect.x + rect.width, rect.y + rect.height, 0});
    Vector2 p4 =
        isometric_projection((Vector3){rect.x, rect.y + rect.height, 0});

    // Draw outline in world space (red for hazards, green for player)
    Color c = (entity->type == ENTITY_HAZARD) ? RED : GREEN;
    DrawLineV(p1, p2, c);
    DrawLineV(p2, p3, c);
    DrawLineV(p3, p4, c);
    DrawLineV(p4, p1, c);
    // }
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
