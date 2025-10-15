// (TODO) fix build scripts
// (TODO) Squish player on damage
// (TODO) Implement shadows
#include "main.h"
#include "raylib.h"
#include "raymath.h"
#include "util.c"
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

GameState game_state;
Texture2D tilesheet;
static Music bg_music;
static Sound hit_sound;
static bool debug_on = false;
static double accumulator = 0.0;
int PLAY_AREA_START = -5;
int PLAY_AREA_END = 8;

Entity *entity_spawn(float x, float y, EntityType type) {
  assert(game_state.entity_count < MAX_ENTITIES && "Entity overflow!");
  game_state.entities[game_state.entity_count++] = (Entity){.pos.x = x,
                                                            .pos.y = y,
                                                            .width = 1,
                                                            .height = 1,
                                                            .type = type,
                                                            .color = WHITE};

  return &game_state.entities[game_state.entity_count - 1];
}

void load_assets() {
  // Load textures
  tilesheet = LoadTexture("assets/tiles.png");
  // Load Music
  bg_music = LoadMusicStream("assets/spagetti-western.ogg");
  SetMusicVolume(bg_music, 0.05f);
  PlayMusicStream(bg_music);
  // Load SFX
  hit_sound = LoadSound("assets/sfx_sounds_impact12.wav");
  SetSoundVolume(hit_sound, 0.5f);
}

void load_map(const char *path) {
  FILE *f = fopen(path, "r");
  if (!f) {
    printf("Failed to open file!\n");
    return;
  }

  char line[14];
  float y = 0;
  while (fgets(line, sizeof(line), f)) {
    for (int x = 0; line[x] != '\0' && line[x] != '\n'; x++) {
      char c = line[x];
      if (c == '^') {
        entity_spawn(x + PLAY_AREA_START + 1, y - 20, ENTITY_HAZARD);
      }
    }
    y -= 0.5f;
  }
  fclose(f);
}

void init_game(void) {
  game_state = (GameState){0};

  // Render empty entities off screen
  for (int i = 0; i < MAX_ENTITIES; i++) {
    game_state.entities[i].pos = (Vector2){9999.0f, 9999.0f};
    game_state.entities[i].type = ENTITY_NONE;
  }

  game_state.player.type = ENTITY_PLAYER;
  game_state.player.width = 0.35f;
  game_state.player.height = 0.75f;
  game_state.player.color = WHITE;
  game_state.player.current_health = 2;
  game_state.player.max_health = 2;

  game_state.camera.zoom = 1.0f;

  load_map("assets/map.txt");
}

void render(GameState *game_state) {
  // Update Draw List
  int draw_count = 0;
  for (int i = 0; i < game_state->entity_count; i++) {
    game_state->draw_list[draw_count++] = &game_state->entities[i];
  }

  game_state->draw_list[draw_count++] = &game_state->player;
  qsort(game_state->draw_list, draw_count, sizeof(Entity *), cmp_draw_ptrs);
  BeginDrawing();
  ClearBackground(ORANGE);
  BeginMode2D(game_state->camera);

  int tile_y = floorf(game_state->player.pos.y);
  int tile_y_offset = 30;
  Rectangle unplayable_area_tile = get_tile_source_rect(36);
  // Draw world
  // (NOTE) figure out how to start from 0 instead of a negative number
  for (int y = tile_y - 30; y < tile_y + 14; y++) {
    // draw from the edge of the screen to the player area (-5)
    for (int x = -21; x < -5; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      DrawTextureRec(tilesheet, unplayable_area_tile, screen, WHITE);
    }

    // Draw play area
    for (int x = PLAY_AREA_START; x < PLAY_AREA_END; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      // not sure if this shit smooths teh game either, idts
      screen.x = floorf(screen.x);
      screen.y = floorf(screen.y);
      Rectangle floor_tile = get_tile_source_rect(9);
      DrawTextureRec(tilesheet, floor_tile, screen, WHITE);
    }

    // draw from the end of the play are to the right half of the screen
    for (int x = 8; x < 22; x++) {
      Vector3 world = {x, y, 0};
      Vector2 screen = isometric_projection(world);
      DrawTextureRec(tilesheet, unplayable_area_tile, screen, WHITE);
    }
  }

  // --- Draw Entities ---
  for (int i = 0; i < draw_count; i++) {
    Entity *entity = game_state->draw_list[i];
    // cull
    if (entity->pos.y < game_state->player.pos.y - 40 ||
        entity->pos.y > game_state->player.pos.y + 20) {
      continue;
    }

    Vector2 projected =
        isometric_projection((Vector3){entity->pos.x, entity->pos.y, 0});
    if (entity->type == ENTITY_PLAYER) {
      // DRAW PLAYER
      Vector3 cube_center = {
          game_state->player.pos.x + game_state->player.width / 2.0f,
          game_state->player.pos.y + game_state->player.height / 2.0f, 0};
      DrawIsoCube(cube_center, game_state->player.width,
                  game_state->player.height, 10.5f, game_state->player.angle,
                  game_state->player.color);

    } else if (entity->type == ENTITY_HAZARD) {
      Rectangle source = get_tile_source_rect(55);
      Vector2 origin = {TILE_SIZE / 2.0f, TILE_SIZE / 2.0f};
      Rectangle dst = {projected.x, projected.y, TILE_SIZE, TILE_SIZE};
      DrawTexturePro(tilesheet, source, dst, origin, 0, entity->color);
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

  if (debug_on) {
    DrawPlayerDebug(&game_state->player);
  }
  EndMode2D();

  // Draw player health
  for (int i = 0; i < game_state->player.max_health; i++) {
    DrawRectangle((i * 35), 30, 25, 25,
                  i < game_state->player.current_health ? RED : GRAY);
  }

  // Scale game to window size
  float scale_x = (float)GetScreenWidth() / VIRTUAL_WIDTH;
  float scale_y = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
  game_state->camera.zoom = fminf(scale_x, scale_y);

  float scale = fminf(scale_x, scale_y);
  int font_size = (int)(40 * scale);
  const char *timer_text = TextFormat("%.1f", game_state->game_timer);
  int text_width = MeasureText(timer_text, font_size);
  DrawText(timer_text, (GetScreenWidth() - text_width) / 2, 0, font_size,
           WHITE);
  DrawFPS(0, 0);
  EndDrawing();
}

void update_draw(void) {
  UpdateMusicStream(bg_music);

  // Reset on death
  if (game_state.player.current_health <= 0) {
    init_game();
  }

  float frame_dt = GetFrameTime();
  accumulator += frame_dt;

  // --- Input ---
  if (IsKeyPressed(KEY_SPACE)) {
    game_state.entities[game_state.entity_count++] =
        (Entity){.pos.x = game_state.player.pos.x,
                 .pos.y = game_state.player.pos.y,
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
  if (IsKeyPressed(KEY_R)) {
    init_game();
  }
  int steps = 0;
  const int MAX_STEPS = 8; // safety cap
  while (accumulator >= FIXED_DT && steps < MAX_STEPS) {
    float dt = FIXED_DT;
    float turn_input = 0;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
      turn_input = -1.0f;
    }
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
      turn_input = 1.0f;
    }

    // --- Player Movement ---
    // --- Turning / banking ---
    float turn_speed = 540.0f; // how fast angle responds
    float turn_drag = 50.0f;   // how quickly rotation stops leaning
    float target_angle = 45.0f * turn_input;
    float angle_accel = (target_angle - game_state.player.angle) * turn_speed;
    game_state.player.angle_vel += angle_accel * dt;
    game_state.player.angle_vel -= game_state.player.angle_vel * turn_drag * dt;
    game_state.player.angle += game_state.player.angle_vel * dt;

    // --- Movement ---
    Vector2 forward = {cosf((game_state.player.angle - 90.0f) * DEG2RAD),
                       sinf((game_state.player.angle - 90.0f) * DEG2RAD)};

    // Forward motion (always galloping)
    float forward_speed = 10.0f;
    game_state.player.pos.y -= forward_speed * dt;

    // Lateral motion (guiding left/right)
    float accel_strength = 250.0f; // low acceleration = gentle pull on reins
    float drag = 25.0f;            // high drag = instant correction after input

    Vector2 accel = {forward.x * accel_strength, 0}; // only lateral accel
    game_state.player.vel.x += accel.x * dt;
    game_state.player.vel.x -= game_state.player.vel.x * drag * dt;

    // direction-flip damping
    if ((turn_input > 0 && game_state.player.vel.x < 0) ||
        (turn_input < 0 && game_state.player.vel.x > 0)) {
      game_state.player.vel.x *= 0.75f; // tiny resistance when switching
    }

    game_state.player.pos.x += game_state.player.vel.x * dt;

    // --- Update ---
    game_state.player.color = WHITE;
    game_state.game_timer += dt;
    // (TODO)clamp find a better way to reuse these tile values
    game_state.player.pos.x =
        Clamp(game_state.player.pos.x,
              PLAY_AREA_START + game_state.player.width * 2.0f,
              8 + game_state.player.width);
    Vector2 player_screen =
        isometric_projection((Vector3){0, game_state.player.pos.y, 0});

    // Update camera position to follow player
    game_state.camera.target = player_screen;
    game_state.camera.offset =
        (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

    // Player-entity collision
    Rectangle player_rect = {game_state.player.pos.x, game_state.player.pos.y,
                             game_state.player.width, game_state.player.height};
    for (int i = 0; i < game_state.entity_count; i++) {
      Entity *entity = &game_state.entities[i];
      Rectangle harard_rect = {entity->pos.x, entity->pos.y, entity->width,
                               entity->height};
      if (CheckCollisionRecs(player_rect, harard_rect) &&
          game_state.player.damage_cooldown <= 0) {
        PlaySound(hit_sound);
        game_state.player.current_health--;
        game_state.player.damage_cooldown = 0.5f;
        game_state.shake_timer = 0.5f;

        DrawRectangleLines(player_rect.x, player_rect.y, player_rect.width,
                           player_rect.height, BLACK);
      }

      if (entity->type == ENTITY_BULLET) {
        entity->pos.y -= 25.0f * dt; // adjust 10.0f to tune speed
      }
    }

    if (game_state.player.damage_cooldown > 0.0f) {
      game_state.player.damage_cooldown -= dt;
      game_state.player.color = RED;
    }

    // Screen shake
    if (game_state.shake_timer > 0) {
      game_state.shake_timer -= dt;
      float offset_x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
      float shake_magnitude = 10.0f;
      game_state.camera.target.x += offset_x * shake_magnitude;
    }
    steps++;
    accumulator -= FIXED_DT;
  }

  render(&game_state);
}

int main(void) {
  InitWindow(VIRTUAL_WIDTH * 2, VIRTUAL_HEIGHT * 2, "Yeehaw");
  // Uncap FPS because it makes my game feel like tash
  SetTargetFPS(0);
  InitAudioDevice();
  srand((unsigned int)time(NULL));
  load_assets();
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
