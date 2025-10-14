// (TODO) Squish player on damage
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

static float WrapAngle(float angle) {
  while (angle > 180.0f)
    angle -= 360.0f;
  while (angle < -180.0f)
    angle += 360.0f;
  return angle;
}

const int VIRTUAL_WIDTH = 640;
const int VIRTUAL_HEIGHT = 360;
const int TILE_WIDTH = 32;
const int TILE_HEIGHT = 16;
static int current_frame = 0;
static float frame_timer = 0.0f;
static float shake_timer = 0.0f;
static float game_timer = 0.0f;

#define MAX_ENTITIES 1000
static Entity entitys[MAX_ENTITIES];
// (NOTE) maybe I'll need a max drawables or something and its own length but I
// think this is ok for now
static Entity *draw_list[MAX_ENTITIES];
static int entity_length = 0;
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
static bool automove_on = true;

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

Entity *entity_spawn(float x, float y, EntityType type) {
  assert(entity_length < MAX_ENTITIES && "Entity overflow!");
  entitys[entity_length++] = (Entity){.pos.x = x,
                                      .pos.y = y,
                                      .width = 1,
                                      .height = 1,
                                      .type = type,
                                      .color = WHITE};

  return &entitys[entity_length];
}

void load_map(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    printf("Failed to open file!\n");
    return;
  }

  char line[14];
  int y = 0;
  while (fgets(line, sizeof(line), f)) {
    for (int x = 0; line[x] != '\0' && line[x] != '\n'; x++) {
      char c = line[x];
      if (c == '^') {
        entity_spawn(x + PLAY_AREA_START + 1, y - 20, ENTITY_HAZARD);
      }
    }
    y--;
  }
  fclose(f);
}

void init_game(void) {
  // Entity/map stuff
  entity_length = 0;
  for (int i = 0; i < MAX_ENTITIES; i++) {
    entitys[i].pos = (Vector2){9999.0f, 9999.0f}; // hide uninitialized
    entitys[i].type = ENTITY_NONE;
  }

  // Load Textures
  player_sprite = LoadTexture("assets/horse.png");
  tile_wall = LoadTexture("assets/tileset/tile_057.png");
  tile000 = LoadTexture("assets/tileset/tile_009.png");
  // tile000 = LoadTexture("assets/tileset/tile_014.png");
  // rock_texture = LoadTexture("assets/tileset/tile_055.png");
  rock_texture = LoadTexture("assets/tileset/tile_000.png");
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
      .vel.x = 0,
      .vel.y = 0,
      .width = 1.0f * 0.5,
      .height = 0.75f,
      .color = WHITE,
      .damage_cooldown = 0.0f,
      .current_health = 5,
      .max_health = 5,
      .angle = 0.0f,
      // .angle_vel = 0.0f,
  };
  camera = (Camera2D){
      .target = (Vector2){0, 0},
      .offset = (Vector2){0, 0},
      .rotation = 0.0f,
      .zoom = 1.0f,
  };

  // Reset timers
  game_timer = 0;

  load_map("maps/map1.txt");
}

void DrawIsoCube(Vector3 center, float w, float h, float height,
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
  if (!debug_on)
    return;

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
  if (IsKeyPressed(KEY_SPACE)) {
    entitys[entity_length++] = (Entity){.pos.x = player.pos.x,
                                        .pos.y = player.pos.y,
                                        .color = RED,
                                        .width = 0.2,
                                        .height = 0.2,
                                        .vel.x = 1.0f,
                                        .vel.y = 1.0f,
                                        .type = ENTITY_BULLET};
  }

  if (IsKeyPressed(KEY_P)) {
    debug_on = !debug_on;
  }
  if (IsKeyPressed(KEY_M)) {
    automove_on = !automove_on;
  }
  if (IsKeyPressed(KEY_R)) {
    init_game();
  }

  float turn_input = 0;
  if (IsKeyDown(KEY_A)) {
    turn_input = -1.0f;
  }
  if (IsKeyDown(KEY_D)) {
    turn_input = 1.0f;
  }

  // --- Player Movement ---

  // this feels ok
  // float drag = 3.0f;
  // float accel = turn_input * 20.0f;
  // player.vel.x += accel * dt;
  // player.vel.x -= player.vel.x * drag * dt;
  // player.pos.x += player.vel.x * dt;

  float target_angle = 45.0f * turn_input;
  float diff = target_angle - player.angle;
  player.angle += diff * 5.0f * dt;
  Vector2 forward = {
      cosf((player.angle - 90.0f) * DEG2RAD), // rotate 90° so 0° faces up
      sinf((player.angle - 90.0f) * DEG2RAD)};
  float base_speed = 10.0f;
  player.pos.x += forward.x * base_speed * dt;
  player.pos.y -= base_speed * dt;

  // --- Update ---
  player.color = WHITE;
  game_timer += dt;
  // (TODO)clamp find a better way to reuse these tile values
  player.pos.x = Clamp(player.pos.x, PLAY_AREA_START + player.width * 2.0f,
                       8 + player.width);
  Vector2 player_screen = isometric_projection((Vector3){0, player.pos.y, 0});

  // Update camera position to follow player
  camera.target = player_screen;
  camera.offset = (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

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

    if (entity->type == ENTITY_BULLET) {
      entity->pos.y -= 25.0f * dt; // adjust 10.0f to tune speed
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
  camera.rotation = player.bank_angle * 0.25f;

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
  for (int i = 0; i < draw_count; i++) {
    Entity *entity = draw_list[i];
    // cull
    if (entity->pos.y < player.pos.y - 40 ||
        entity->pos.y > player.pos.y + 20) {
      continue;
    }

    Vector2 projected =
        isometric_projection((Vector3){entity->pos.x, entity->pos.y, 0});
    if (entity->type == ENTITY_PLAYER) {
      // DRAW PLAYER
      Vector3 cube_center = {player.pos.x + player.width / 2.0f,
                             player.pos.y + player.height / 2.0f, 0};
      DrawIsoCube(cube_center, player.width, player.height, 10.5f, player.angle,
                  WHITE);

    } else if (entity->type == ENTITY_HAZARD) {
      Vector2 origin = {rock_texture.width / 2.0f, rock_texture.height / 2.0f};
      Rectangle dst = {projected.x, projected.y, rock_texture.width,
                       rock_texture.height};
      DrawTexturePro(rock_texture, (Rectangle){0, 0, 32, 32}, dst, origin, 0,
                     entity->color);
    } else if (entity->type == ENTITY_BULLET) {
      DrawRectangle(projected.x, projected.y, entity->width * 32,
                    entity->height * 16, entity->color);
    }

    if (debug_on && entity->type != ENTITY_BULLET) {
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
    }
  }
  DrawPlayerDebug(&player);
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
