// (TODO) destroy bullets when they fly off the top of the screen
// (tood) dont perfom collision detection on entiteis off screen
// todo
// (TODO) Destroy entities not on screen
// (TODO) Move to units for time and world pos
// (TODO) Make input state machine
// (TODO) auto generate compile_commands.json
// (MAYBE) bullet on bullet ritochet?
//
// (MAYBE) in hades you can get extra lives, maybe you can do the same thing
// here but playign with fewer lives gives you some bonus. Myabe a score bonus?
// This way you can play through the game on easy mode if you want but if youre
// chasing a higher score you have an incentive to play with fewer lives
// (TODO) Fix drawing background/world
// (TODO) Squish player on damage
// (TODO) Implement shadows
#include "game.h"
#include "platform.h"
#include "raylib.h"
#include "raymath.h"
#include "util.c"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

// Game boundaries
int PLAY_AREA_START = -5;
int PLAY_AREA_END = 8;

Entity *entity_spawn(TransientStorage *t, float x, float y, EntityType type) {
  assert(t->entity_count < MAX_ENTITIES && "Entity overflow!");
  t->entities[t->entity_count++] = (Entity){
      .pos = {x, y},
      .vel = {0, 0},
      .width = 1,
      .height = 1,
      .type = type,
      .color = WHITE,
      .angle = 0,
      .angle_vel = 0,
      .bank_angle = 0,
      .current_health = 0,
      .damage_cooldown = 0,

  };

  return &t->entities[t->entity_count - 1];
}

void load_map(TransientStorage *t, const char *path) {
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
        entity_spawn(t, x + PLAY_AREA_START + 1, y - 20, ENTITY_HAZARD);
      }
      if (c == 'X') {
        entity_spawn(t, x + PLAY_AREA_START + 1, y - 20, ENTITY_GUNMEN);
      }
    }
    y -= 0.5f;
  }
  fclose(f);
}

// --------------------------------------------------
// Game Initialization (resets transient state only)
// --------------------------------------------------
void init_game(TransientStorage *t, PermanentStorage *p) {
  *t = (TransientStorage){0};

  // Initialize entities off screen
  for (int i = 0; i < MAX_ENTITIES; i++) {
    t->entities[i].pos = (Vector2){9999.0f, 9999.0f};
    t->entities[i].type = ENTITY_NONE;
  }

  // Initialize player
  t->player.type = ENTITY_PLAYER;
  t->player.width = 0.35f;
  t->player.height = 0.75f;
  t->player.color = WHITE;
  t->player.current_health = 2;
  t->player.max_health = 2;

  t->camera.zoom = 1.0f;

  PlayMusicStream(p->bg_music);
  load_map(t, "assets/map.txt");
  // entity_spawn(t, PLAY_AREA_START + 2, -10, ENTITY_GUNMEN);
  printf("%i\n", t->entity_count);
}

void update_player(TransientStorage *t, float turn_input, float dt) {
  // reset players color if damaged
  t->player.color = WHITE;
  // --- Player Movement ---
  float turn_speed = 540.0f;
  float turn_drag = 50.0f;
  float target_angle = 45.0f * turn_input;
  float angle_accel = (target_angle - t->player.angle) * turn_speed;
  t->player.angle_vel += angle_accel * dt;
  t->player.angle_vel -= t->player.angle_vel * turn_drag * dt;
  t->player.angle += t->player.angle_vel * dt;
  Vector2 forward = {cosf((t->player.angle - 90.0f) * DEG2RAD),
                     sinf((t->player.angle - 90.0f) * DEG2RAD)};
  float forward_speed = 5.0f;
  t->player.pos.y -= forward_speed * dt;
  float accel_strength = 250.0f;
  float drag = 25.0f;

  Vector2 accel = {forward.x * accel_strength, 0};
  t->player.vel.x += accel.x * dt;
  t->player.vel.x -= t->player.vel.x * drag * dt;

  // direction-flip damping
  if ((turn_input > 0 && t->player.vel.x < 0) ||
      (turn_input < 0 && t->player.vel.x > 0)) {
    t->player.vel.x *= 0.75f;
  }
  t->player.pos.x += t->player.vel.x * dt;
  // (TODO)clamp find a better way to reuse these tile values
  t->player.pos.x =
      Clamp(t->player.pos.x, -5 + t->player.width * 2.0f, 8 + t->player.width);
}

void update_entities(Memory *memory, float dt) {
  PermanentStorage *p = &memory->permanent;
  TransientStorage *t = &memory->transient;

  // --- Timers ---
  t->game_timer += dt;
  if (t->player.damage_cooldown > 0.0f) {
    t->player.damage_cooldown -= dt;
    t->player.color = RED;
  }
  if (t->shake_timer > 0) {
    t->shake_timer -= dt;
    float offset_x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float shake_magnitude = 10.0f;
    t->camera.target.x += offset_x * shake_magnitude;
  }

  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];

    if (entity->weapon_cooldown > 0) {
      entity->weapon_cooldown -= dt;
    }

    if (entity->pos.y > t->player.pos.y + 8) {
      entity->type = ENTITY_NONE;
    }

    switch (entity->type) {
    case ENTITY_GUNMEN: {
      if (entity->weapon_cooldown <= 0) {
        Vector2 entity_center = {
            entity->pos.x + entity->width * 0.5f,
            entity->pos.y + entity->height * 0.5f,
        };
        Vector2 player_center = {
            t->player.pos.x + t->player.width * 0.5f,
            t->player.pos.y + t->player.height * 0.5f,
        };

        Vector2 direction =
            Vector2Normalize(Vector2Subtract(player_center, entity_center));
        Vector2 spawn_pos = Vector2Add(direction, entity_center);
        Entity *bullet =
            entity_spawn(t, spawn_pos.x, spawn_pos.y, ENTITY_PROJECTILE);

        bullet->vel = Vector2Scale(direction, 25);
        bullet->color = RED;
        bullet->width = 0.2;
        bullet->height = 0.2;
        entity->weapon_cooldown = 2.5f;
      }
    } break;

    case ENTITY_PROJECTILE: {
      entity->pos.y += entity->vel.y * dt;
      entity->pos.x += entity->vel.x * dt;
    } break;
    default:
      break;
    }
  }

  // Player-entity collision
  Rectangle player_rect = {t->player.pos.x, t->player.pos.y, t->player.width,
                           t->player.height};
  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];

    Rectangle entity_rect = {entity->pos.x, entity->pos.y, entity->width,
                             entity->height};
    if (CheckCollisionRecs(player_rect, entity_rect) &&
        t->player.damage_cooldown <= 0) {
      PlaySound(p->hit_sound);
      t->player.current_health--;
      t->player.damage_cooldown = 0.5f;
      t->shake_timer = 0.5f;

      if (entity->type == ENTITY_PROJECTILE) {
        entity->type = ENTITY_NONE;
      }
    }
  }

  // Entity-entity collision
  for (int i = 0; i < t->entity_count; i++) {
    Entity *a = &t->entities[i];
    if (a->type == ENTITY_NONE) {
      continue;
    }

    Rectangle a_rect = {a->pos.x, a->pos.y, a->width, a->height};
    for (int j = i + 1; j < t->entity_count; j++) {
      Entity *b = &t->entities[j];
      if (b->type == ENTITY_NONE) {
        continue;
      }

      Rectangle b_rect = {b->pos.x, b->pos.y, b->width, b->height};
      if (CheckCollisionRecs(a_rect, b_rect)) {
        a->type = ENTITY_NONE;
        b->type = ENTITY_NONE;
      }
    }
  }

  // Clean up/Compact entities
  int new_count = 0;
  for (int i = 0; i < t->entity_count; i++) {
    if (t->entities[i].type != ENTITY_NONE) {
      t->entities[new_count++] = t->entities[i];
    }
  }
  t->entity_count = new_count;
}

// --------------------------------------------------
// Update
// --------------------------------------------------
void update(Memory *memory) {
  PermanentStorage *p = &memory->permanent;
  TransientStorage *t = &memory->transient;

  if (!t->game_initialized) {
    init_game(t, p);
    t->game_initialized = true;
  }

  float dt = GetFrameTime();
  // --- Input ---
  if (IsKeyPressed(KEY_SPACE)) {
    t->entities[t->entity_count++] = (Entity){.pos.x = t->player.pos.x - 0.35f,
                                              .pos.y = t->player.pos.y - 0.5f,
                                              .color = RED,
                                              .width = 0.2,
                                              .height = 0.2,
                                              .vel.x = 0,
                                              .vel.y = -25.0f,
                                              .type = ENTITY_PROJECTILE};
  }

  if (IsKeyPressed(KEY_P)) {
    p->debug_on = !p->debug_on;
  }
  if (IsKeyPressed(KEY_R)) {
    init_game(t, p);
  }
  float turn_input = 0;
  if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) {
    turn_input = -1.0f;
  }
  if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) {
    turn_input = 1.0f;
  }

  if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) {
    t->player.pos.y += -7.0f * dt;
  }
  if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) {
    t->player.pos.y += 7.0f * dt;
  }

  UpdateMusicStream(p->bg_music);

  // Reset on death
  if (t->player.current_health <= 0) {
    StopMusicStream(p->bg_music);
    // init_game(t);
    return;
  }

  t->accumulator += dt;
  int steps = 0;
  const int MAX_STEPS = 8;
  while (t->accumulator >= FIXED_DT && steps < MAX_STEPS) {
    update_player(t, turn_input, FIXED_DT);
    update_entities(memory, FIXED_DT);
    // collision
    steps++;
    t->accumulator -= FIXED_DT;
  }

  Vector2 player_screen =
      isometric_projection((Vector3){0, t->player.pos.y, 0});
  t->camera.target = player_screen;
  t->camera.offset =
      (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};
}

// --------------------------------------------------
// Rendering
// --------------------------------------------------
void render(Memory *gs) {
  PermanentStorage *p = &gs->permanent;
  TransientStorage *t = &gs->transient;
  Texture2D *tilesheet = &p->tilesheet;

  // Update Draw List
  int draw_count = 0;
  for (int i = 0; i < t->entity_count; i++) {
    t->draw_list[draw_count++] = &t->entities[i];
  }
  t->draw_list[draw_count++] = &t->player;

  qsort(t->draw_list, draw_count, sizeof(Entity *),
        compare_entities_for_draw_order);

  BeginDrawing();
  ClearBackground(ORANGE);
  BeginMode2D(t->camera);

  int tile_y = floorf(t->player.pos.y);
  Rectangle tile = get_tile_source_rect(tilesheet, 36);

  // --- Draw world ---
  // (NOTE) figure out how to start from 0 instead of a negative number
  for (int y = tile_y - 30; y < tile_y + 14; y++) {
    // Left side
    for (int x = -21; x < -5; x++) {
      Vector2 screen = isometric_projection((Vector3){x, y, 0});
      DrawTextureRec(p->tilesheet, tile, screen, WHITE);
    }

    // Play area
    for (int x = -5; x < 8; x++) {
      Vector2 screen = isometric_projection((Vector3){x, y, 0});
      Rectangle floor_tile = get_tile_source_rect(tilesheet, 9);
      DrawTextureRec(p->tilesheet, floor_tile, screen, WHITE);
    }

    // Right side
    for (int x = 8; x < 22; x++) {
      Vector2 screen = isometric_projection((Vector3){x, y, 0});
      DrawTextureRec(p->tilesheet, tile, screen, WHITE);
    }
  }

  // --- Draw Entities ---
  for (int i = 0; i < draw_count; i++) {
    Entity *entity = t->draw_list[i];
    // Cull entities the player can't see
    if (entity->pos.y < t->player.pos.y - 40 ||
        entity->pos.y > t->player.pos.y + 20) {
      continue;
    }

    Vector2 projected =
        isometric_projection((Vector3){entity->pos.x, entity->pos.y, 0});
    switch (entity->type) {
    case ENTITY_PLAYER: {
      Vector3 cube_center = {t->player.pos.x + t->player.width / 2.0f,
                             t->player.pos.y + t->player.height / 2.0f, 0};
      draw_iso_cube(cube_center, t->player.width, t->player.height, 10.5f,
                    t->player.angle, t->player.color);
    } break;

    case ENTITY_HAZARD: {
      Rectangle src = get_tile_source_rect(tilesheet, 55);
      Vector2 origin = {TILE_SIZE / 2.0f, TILE_SIZE / 2.0f};
      Rectangle dst = {projected.x, projected.y, TILE_SIZE, TILE_SIZE};
      DrawTexturePro(p->tilesheet, src, dst, origin, 0, entity->color);
    } break;

    case ENTITY_PROJECTILE: {
      // printf("drawing bullet");
      DrawRectangle(projected.x, projected.y, entity->width * 32,
                    entity->height * 16, entity->color);
    } break;

    case ENTITY_GUNMEN: {
      Rectangle src = get_tile_source_rect(tilesheet, 55);
      Vector2 origin = {TILE_SIZE / 2.0f, TILE_SIZE / 2.0f};
      Rectangle dst = {projected.x, projected.y, TILE_SIZE, TILE_SIZE};
      DrawTexturePro(p->tilesheet, src, dst, origin, 0, RED);
    }

    default:
      break;
    }

    if (p->debug_on && entity->type != ENTITY_PROJECTILE) {
      // Draw Collision Debug Iso Box
      draw_entity_collision_box(entity);
      DrawPlayerDebug(&t->player);
      DrawRectangleLines(entity->pos.x, entity->pos.y, entity->width,
                         entity->height, BLACK);
    }
  }

  EndMode2D();

  // Draw player health
  for (int i = 0; i < t->player.max_health; i++) {
    DrawRectangle((i * 35), 30, 25, 25,
                  i < t->player.current_health ? RED : GRAY);
  }

  // Scale game to window size
  // maybe i can store the scale in my game state structs
  // we'd only have to update these on screen resize
  float scale_x = (float)GetScreenWidth() / VIRTUAL_WIDTH;
  float scale_y = (float)GetScreenHeight() / VIRTUAL_HEIGHT;
  t->camera.zoom = fminf(scale_x, scale_y);

  float scale = fminf(scale_x, scale_y);
  int font_size = (int)(40 * scale);
  const char *timer_text = TextFormat("%.1f", t->game_timer);
  int text_width = MeasureText(timer_text, font_size);
  DrawText(timer_text, (GetScreenWidth() - text_width) / 2, 0, font_size,
           WHITE);

  if (t->player.current_health <= 0) {
    ClearBackground(BLACK);

    int game_over_font_size = (int)(40 * scale);
    const char *game_over_text = "GAME OVER";
    int text_width = MeasureText(game_over_text, game_over_font_size);
    DrawText(game_over_text, (GetScreenWidth() - text_width) / 2.0f,
             (GetScreenHeight() - font_size * 2) / 2.0f, font_size, WHITE);

    int restart_font_size = (int)(12 * scale);
    // maybe change this to press any key to restart
    const char *restart_text = "PRESS R TO RESTART";
    int restart_width = MeasureText(restart_text, restart_font_size);
    DrawText(restart_text, (GetScreenWidth() - restart_width) / 2.0f,
             (GetScreenHeight()) / 2.0f + 10, restart_font_size, RED);
  }
  DrawFPS(0, 0);
  EndDrawing();
}

void game_update_and_render(Memory *memory) {
  update(memory);
  render(memory);
}
