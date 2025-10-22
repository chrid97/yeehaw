// ------------- GUN TODO ----------------
// Add slight aim assit
// Make collision hitbox bigger
// (TODO) destroy bullets when they fly off the top of the screen
// (TODO) if a ritchoetd bullet kills an enemy it should ritochet again to the
// nearest enemy if its within certain tiles or soemthing or myabe it can
// ritchoet off another close bullet
// maybe bullets should shoot at an angle when I'm moving sideways?
//
// AMMO / RELOAD
//
// ---------- MECHANICS TODO -------------
// SPRITNING / STAMINA
//
// (TODO) Destroy entities not on screen
// (TODO) Move to units for time and world pos
// (TODO) Make input state machine
// (TODO) auto generate compile_commands.json
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

Color col_dark_gray = {0x21, 0x25, 0x29, 255};  // #212529
Color col_mauve = {0x61, 0x47, 0x50, 255};      // #614750
Color col_warm_brown = {0x8C, 0x5C, 0x47, 255}; // #8c5c47
Color col_tan = {0xB4, 0x7D, 0x58, 255};        // #b47d58
Color col_sand = {0xD9, 0xB7, 0x7E, 255};       // #d9b77e
Color col_cream = {0xEA, 0xD9, 0xA7, 255};      // #ead9a7

// Game boundaries
int PLAY_AREA_START = -5;
int PLAY_AREA_END = 8;

// (TODO) cleanup game summary ui code
typedef struct {
  Rectangle rect;
  int number_rows;
  float padding_x;
  float gap_y;
  float header_y_end;
  int font_size;
  float scale;
} PostGameSummary;

void set_flag(Entity *entity, uint32_t flags) { entity->flags |= flags; }
bool is_set(Entity *entity, uint32_t flags) { return entity->flags & flags; }

void draw_post_game_summary_header(PostGameSummary *summary, float scale) {
  Rectangle rect = summary->rect;
  DrawRectangle(rect.x * scale, rect.y * scale, rect.width * scale,
                rect.height * scale, col_cream);

  float center_x = rect.x + (rect.width / 2.0f);
  const char *title = "LEVEL COMPLETE";
  int title_width = MeasureText(title, summary->font_size);
  DrawText(title, (center_x - (title_width / 2.0f)) * scale,
           (rect.y + summary->gap_y) * scale, summary->font_size * scale,
           BLACK);

  float header_y_end = summary->header_y_end;
  DrawLine(rect.x * scale, header_y_end * scale, (rect.x + rect.width) * scale,
           header_y_end * scale, BLACK);
}

void draw_score_breakdown(PostGameSummary *summary, const char *text,
                          const char *points_text) {
  float x = summary->rect.x;
  float y = summary->rect.y;
  float width = summary->rect.width;
  int font_size = summary->font_size;
  float scale = summary->scale;

  summary->number_rows++;
  y = summary->header_y_end + (summary->gap_y * summary->number_rows);

  DrawText(text, (x + summary->padding_x) * scale, y * scale, 10 * scale,
           BLACK);

  int points_width = MeasureText(points_text, font_size);
  DrawText(points_text, (x + width - points_width - summary->padding_x) * scale,
           y * scale, font_size * scale, BLACK);
}

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
      .active = false,
  };

  return &t->entities[t->entity_count - 1];
}

Entity *entity_projectile_spawn(TransientStorage *t, float x, float y) {
  Entity *projectile = entity_spawn(t, x, y, ENTITY_PROJECTILE);
  projectile->color = PURPLE;
  projectile->width = 0.25;
  projectile->height = 0.25;
  projectile->vel.x = 0;
  projectile->vel.y = 25.0f;
  set_flag(projectile, EntityFlags_IsProjectile);

  return projectile;
};

bool is_onscreen(Entity *a, Entity *player) {
  if (a->pos.y < player->pos.y - 20 || a->pos.y > player->pos.y + 20) {
    return false;
  }
  return true;
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
        Entity *entity =
            entity_spawn(t, x + PLAY_AREA_START + 1, y - 20, ENTITY_HAZARD);
      }
      if (c == 'x') {

        Entity *entity =
            entity_spawn(t, x + PLAY_AREA_START + 1, y - 20, ENTITY_GUNMEN);
        set_flag(entity, EntityFlags_IsDestructable);
      }
      if (c == '1') {
        t->map_end = (Vector2){0, y};
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
    t->entities[i].active = false;
  }

  // Initialize player
  t->player.type = ENTITY_PLAYER;
  t->player.width = 0.35f;
  t->player.height = 0.75f;
  t->player.color = WHITE;
  t->player.current_health = 2;
  t->player.max_health = 2;
  t->player.vel = (Vector2){8, 6};
  // t->player.parry_frame_time = 6;
  t->player.parry_duration = 0;

  t->camera.zoom = 1.0f;

  PlayMusicStream(p->bg_music);
  load_map(t, "assets/map.txt");
}

void compact_entities(TransientStorage *t) {
  int new_count = 0;
  for (int i = 0; i < t->entity_count; i++) {
    if (t->entities[i].type != ENTITY_NONE) {
      t->entities[new_count++] = t->entities[i];
    }
  }
  t->entity_count = new_count;
}

void update_timers(TransientStorage *t) {
  t->player.weapon_cooldown -= FIXED_DT;
  t->game_timer += FIXED_DT;
  if (t->player.damage_cooldown > 0.0f) {
    t->player.damage_cooldown -= FIXED_DT;
    t->player.color = RED;
  }

  // (TODO) shaking stopped working at some point gotta fix
  if (t->shake_timer > 0) {
    t->shake_timer -= FIXED_DT;
    float offset_x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
    float shake_magnitude = 10.0f;
    t->camera.target.x += offset_x * shake_magnitude;
  }

  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];
    if (entity->weapon_cooldown > 0) {
      entity->weapon_cooldown -= FIXED_DT;
    }
  }
}

void update_entity_movement(TransientStorage *t) {
  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];
    switch (entity->type) {
    case ENTITY_GUNMEN: {
    } break;

    case ENTITY_PROJECTILE: {
      entity->pos.y += entity->vel.y * FIXED_DT;
      entity->pos.x += entity->vel.x * FIXED_DT;
    } break;
    default:
      break;
    }
  }
}

void update_player(TransientStorage *t, float turn_input, bool movement,
                   PermanentStorage *p) {
  float dt = FIXED_DT;
  // reset players color if damaged
  t->player.color = WHITE;
  // --- Player Movement ---
  if (movement) {
    float turn_speed = 540.0f;
    float turn_drag = 50.0f;
    float target_angle = 45.0f * turn_input;
    float angle_accel = (target_angle - t->player.angle) * turn_speed;
    t->player.angle_vel += angle_accel * dt;
    t->player.angle_vel -= t->player.angle_vel * turn_drag * dt;
    t->player.angle += t->player.angle_vel * dt;
    Vector2 forward = {cosf((t->player.angle - 90.0f) * DEG2RAD),
                       sinf((t->player.angle - 90.0f) * DEG2RAD)};
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
  } else {
    t->player.pos.x += (turn_input * t->player.vel.x) * dt;
  }

  t->player.pos.y -= t->player.vel.y * dt;
  // (TODO)clamp find a better way to reuse these tile values
  t->player.pos.x =
      Clamp(t->player.pos.x, -5 + t->player.width * 2.0f, 8 + t->player.width);

  t->player.parry_area = (Rectangle){
      .x = t->player.pos.x - 2.0f,
      .y = t->player.pos.y - 3.5f,
      .width = 4.0f,
      .height = 1.5f,
  };

  bool parried = false;

  if (t->player.is_firing && t->player.parry_duration <= 0.0f) {
    t->player.parry_duration = 0.2f;
    t->player.pending_shot = true; // queue normal shot in case parry misses
  }

  if (t->player.parry_duration > 0.0f) {
    t->player.parry_duration -= FIXED_DT;

    for (int i = 0; i < t->entity_count; i++) {
      Entity *a = &t->entities[i];
      if (!is_set(a, EntityFlags_IsProjectile) ||
          is_set(a, EntityFlags_IsPlayer))
        continue;

      Rectangle parry = t->player.parry_area;
      Rectangle entity_rect = {a->pos.x, a->pos.y, a->width, a->height};
      if (!CheckCollisionRecs(parry, entity_rect))
        continue;

      if (a->parry_detected)
        continue;

      a->parry_detected = true;
      parried = true;

      float lead_time = 0.1f;
      Vector2 predicted = Vector2Add(a->pos, Vector2Scale(a->vel, lead_time));
      Vector2 dir_to_predicted =
          Vector2Normalize(Vector2Subtract(predicted, t->player.pos));

      float spawn_offset = 0.2f;
      Vector2 spawn_pos = Vector2Add(
          t->player.pos, Vector2Scale(dir_to_predicted, spawn_offset));

      PlaySound(p->player_gunshot);
      printf("boom\n");
      Entity *projectile = entity_projectile_spawn(t, spawn_pos.x, spawn_pos.y);
      projectile->vel = Vector2Scale(dir_to_predicted, projectile->vel.y);
      projectile->color = WHITE;
      set_flag(projectile, EntityFlags_IsPlayer);
    }

    if (!parried && t->player.parry_duration <= 0.0f &&
        t->player.pending_shot) {
      PlaySound(p->player_gunshot);
      float x = t->player.pos.x - 0.2f;
      float y = t->player.pos.y - 0.2f;
      Entity *projectile = entity_projectile_spawn(t, x, y);
      projectile->vel.y = -projectile->vel.y;
      projectile->color = WHITE;
      set_flag(projectile, EntityFlags_IsPlayer);
      t->player.pending_shot = false;
    }

    if (parried) {
      t->player.pending_shot = false;
    }
  }

  t->player.weapon_cooldown = 0.5f;
  t->player.is_firing = false;
}

void resolve_collisions(TransientStorage *t, PermanentStorage *p) {
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
      // t->player.current_health--;
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
    if (a->type == ENTITY_NONE || !a->active) {
      continue;
    }

    if (a->pos.y < t->player.pos.y - 40 || a->pos.y > t->player.pos.y + 20) {
      continue;
    }
    Rectangle a_rect = {a->pos.x, a->pos.y, a->width, a->height};
    for (int j = i + 1; j < t->entity_count; j++) {
      Entity *b = &t->entities[j];
      Rectangle b_rect = {b->pos.x, b->pos.y, b->width, b->height};

      if (!CheckCollisionRecs(a_rect, b_rect)) {
        continue;
      }

      if (is_set(a, EntityFlags_IsProjectile) &&
          is_set(b, EntityFlags_IsProjectile)) {
        if (!is_set(a, EntityFlags_IsPlayer)) {
          a->vel.y = -a->vel.y;
          a->vel.x = -a->vel.x;

          a->color = GOLD;

          // move it slightly along new direction so it
          // doesn't overlap again
          a->pos.y += a->vel.y * FIXED_DT;
          b->type = ENTITY_NONE;
        }
        if (!is_set(b, EntityFlags_IsPlayer)) {
          b->vel.y = -b->vel.y;
          b->vel.x = -b->vel.x;

          b->color = GOLD;
          b->pos.y += b->vel.y * FIXED_DT;
          a->type = ENTITY_NONE;
        }
      } else if (is_set(a, EntityFlags_IsProjectile)) {
        a->type = ENTITY_NONE;
      } else if (is_set(b, EntityFlags_IsProjectile)) {
        b->type = ENTITY_NONE;
      }

      if (is_set(a, EntityFlags_IsDestructable)) {
        if (a->type == ENTITY_GUNMEN) {
          b->color = WHITE;
          PlaySound(p->enemy_death_sound);
          t->enemies_killed++;
        }
        a->type = ENTITY_NONE;
      }

      if (is_set(b, EntityFlags_IsDestructable)) {
        if (b->type == ENTITY_GUNMEN) {
          b->color = WHITE;
          PlaySound(p->enemy_death_sound);
          t->enemies_killed++;
        }
        b->type = ENTITY_NONE;
      }
    }
  }
}

void update_entities(Memory *memory) {
  PermanentStorage *p = &memory->permanent;
  TransientStorage *t = &memory->transient;

  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];

    // TODO Destroy offscreen bullets

    if (is_onscreen(entity, &t->player)) {
      entity->active = true;
    }

    // skip all updates on inactive entities
    if (!entity->active) {
      continue;
    }

    // (NOTE) maybe I don't have to mark offscreen entities
    // for destruction it's not like i'm replacing them, i
    // dont plan for this to be an infinite runner anymore.
    // Oh but I do want to mark bullets for destruction
    if (entity->pos.y > t->player.pos.y + 25) {
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
        Entity *bullet = entity_projectile_spawn(t, spawn_pos.x, spawn_pos.y);

        bullet->vel = Vector2Scale(direction, bullet->vel.y);
        entity->weapon_cooldown = 2.5f;
      }
    } break;

    default:
      break;
    }
  }
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
    t->player.is_firing = true;
  }

  // t->player.is_firing = IsKeyDown(KEY_SPACE);

  if (IsKeyPressed(KEY_ONE)) {
    p->physics_movement = !p->physics_movement;
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
    if (t->player.pos.y <= t->map_end.y) {
      return;
    }

    update_timers(t);
    update_player(t, turn_input, p->physics_movement, p);
    update_entities(memory);
    update_entity_movement(t);
    resolve_collisions(t, p);
    compact_entities(t);

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
  // (NOTE) figure out how to start from 0 instead of a
  // negative number
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
      Vector3 cube_center = {entity->pos.x + entity->width * 0.5f,
                             entity->pos.y + entity->height * 0.5f, 0};
      float cube_depth = entity->height;
      float cube_height = 2.0f;
      float tilt_angle = 0.0f;
      draw_iso_cube(cube_center, entity->width, cube_depth, cube_height,
                    tilt_angle, entity->color);

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
      draw_entity_collision_box(entity);
      DrawPlayerDebug(&t->player);
      DrawRectangleLines(entity->pos.x, entity->pos.y, entity->width,
                         entity->height, BLACK);

      // parry hitbox
      if (entity->type == ENTITY_PLAYER) {
        Rectangle rect = entity->parry_area;
        Vector2 p1 = isometric_projection((Vector3){rect.x, rect.y, 0});
        Vector2 p2 =
            isometric_projection((Vector3){rect.x + rect.width, rect.y, 0});
        Vector2 p3 = isometric_projection(
            (Vector3){rect.x + rect.width, rect.y + rect.height, 0});
        Vector2 p4 =
            isometric_projection((Vector3){rect.x, rect.y + rect.height, 0});

        Color c = ORANGE;
        DrawLineV(p1, p2, c);
        DrawLineV(p2, p3, c);
        DrawLineV(p3, p4, c);
        DrawLineV(p4, p1, c);

        if (t->player.parry_duration > 0) {
          Color fill = (Color){255, 0, 0, 120};
          DrawTriangle(p1, p3, p2, fill);
          DrawTriangle(p1, p4, p3, fill);
        }
      }
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

  // (TODO) clean this junk up
  if (t->player.pos.y <= t->map_end.y) {
    Rectangle summary = {0};
    summary.width = 250;
    summary.height = 300;
    summary.x = (VIRTUAL_WIDTH / 2.0f) - summary.width / 2.0f;
    summary.y = (VIRTUAL_HEIGHT / 2.0f) - summary.height / 2.0f;
    PostGameSummary game_summary = {0};
    game_summary.rect = summary;
    game_summary.gap_y = 10.0f;
    game_summary.padding_x = 10.0f;
    game_summary.font_size = 10;
    game_summary.header_y_end = 30 + summary.y;
    game_summary.scale = scale;

    int enemies_killed_points = 250 * t->enemies_killed;
    int current_health_bonus = 500 * t->player.current_health;
    // (TODO) can depend on the difficulty or level
    int completion_bonus = 1000;
    int score = enemies_killed_points + current_health_bonus + completion_bonus;
    // (TODO) difficulty bonus multiplier

    draw_post_game_summary_header(&game_summary, scale);
    draw_score_breakdown(&game_summary, "TIME SURVIVED",
                         TextFormat("%.2f", t->game_timer));
    draw_score_breakdown(&game_summary, "REMAINING HEALTH",
                         TextFormat("%i", current_health_bonus));
    draw_score_breakdown(&game_summary, "ENEMIES KILLED",
                         TextFormat("%i", enemies_killed_points));
    draw_score_breakdown(&game_summary, "COMPLETION BONUS", "1000");

    // (TODO fix later)
    game_summary.gap_y += 20;
    draw_score_breakdown(&game_summary, "FINAL SCORE", TextFormat("%i", score));
    draw_score_breakdown(&game_summary, "RANK", "C");
  }

  DrawFPS(0, 0);
  EndDrawing();
}

void game_update_and_render(Memory *memory) {
  update(memory);
  render(memory);
}
