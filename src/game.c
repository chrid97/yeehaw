// (TODO) I think everything in my game moves faster diagnolly
//
// ------------- GUN TODO ----------------
// (TODO) destroy bullets when they fly off the top of the screen
// (TODO) if a ricochet bullet kills an enemy it should ricochet again to the
// nearest enemy if its within certain tiles or soemthing or myabe it can
// ricochet off another close bullet
//
// (TODO) Add some randomness to shooting bullets
// (TODO) Add more randomness if you shoot while moving
// (TODO) Reduce bullet lifetime to like 5 seconds
//
// ------------- ENEMIES TODO ----------------
// (TODO) horse riding enemies
//
// ---------- MECHANICS TODO -------------
// SPRITNING / STAMINA
//
// (TODO) Move to units for time and world pos
// (TODO) auto generate compile_commands.json
//
// (TODO) Create InputState
// (MAYBE) in hades you can get extra lives, maybe you can do the same thing
// here but playign with fewer lives gives you some bonus. Myabe a score bonus?
// This way you can play through the game on easy mode if you want but if youre
// chasing a higher score you have an incentive to play with fewer lives
// (TODO) Fix drawing background/world
// ---------- VISUAL TODO -------------
// (TODO) Lighting
// (TODO) Implement shadows
// (TODO) Squish player on damage
//
//
// -------------------------------------
#include "game.h"
#include "entity.c"
#include "platform.h"
#include "raylib.h"
#include "raymath.h"
#include "ui.c"
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

Vector2 cursor_pos;

Vector2 screen_to_iso(Vector2 screen) {
  float a = screen.x / (TILE_SIZE / 2.0f);
  float b = screen.y / (TILE_SIZE / 4.0f);
  return (Vector2){(a + b) / 2.0f, (b - a) / 2.0f};
}

bool is_onscreen(Entity *a, Entity *player) {
  if (a->pos.y < player->pos.y - 25 || a->pos.y > player->pos.y + 25) {
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
        set_flag(entity, EntityFlags_NotDestructable);
      }
      if (c == 'x') {
        Entity *entity =
            entity_spawn(t, x + PLAY_AREA_START + 1, y - 20, ENTITY_GUNMEN);
        set_flag(entity, EntityFlags_Destructable);
      }
      if (c == '1') {
        t->map_end = (Vector2){0, y};
      }
      if (c == 'h') {
        Entity *entity = entity_spawn(t, x + PLAY_AREA_START + 1, y - 20,
                                      ENTITY_HORSE_GUNMEN);
        entity->vel.y = -8;
        entity->color = ORANGE;
        entity->width = 0.35f;
        entity->height = 0.75f;
        set_flag(entity, EntityFlags_Destructable);
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
  t->player.vel = (Vector2){0, 7.2};
  t->player.parry_window_timer = 0;

  // (NOTE)(THINKIN) I feel like I should have soemthing that says fill up to
  // max ammo/max health so I don't have to remember to assign the current value
  // to be the same as the max
  t->player.current_health = 3;
  t->player.max_health = 3;

  t->player.max_ammo = 6;
  t->player.ammo = 6;

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
  t->game_timer += FIXED_DT;
  if (t->shake_timer > 0) {
    t->shake_timer -= FIXED_DT;
  }

  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];
    entity->weapon_cooldown -= FIXED_DT;
    entity->fade_timer -= FIXED_DT;
  }
}

// (NOTE) maybe rename to move_entities?
void update_entity_movement(TransientStorage *t) {
  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];
    switch (entity->type) {
    case ENTITY_GUNMEN: {
    } break;
    case ENTITY_HORSE_GUNMEN: {
      Vector2 scaled_vel = Vector2Scale(entity->vel, FIXED_DT);
      entity->pos = Vector2Add(scaled_vel, entity->pos);
    } break;
    case ENTITY_PROJECTILE: {
      entity->pos.y += entity->vel.y * FIXED_DT;
      entity->pos.x += entity->vel.x * FIXED_DT;
    } break;
    case ENTITY_PARTICLE: {
      entity->pos.x += entity->vel.x * FIXED_DT;
      entity->pos.y += entity->vel.y * FIXED_DT;
      entity->vel = Vector2Scale(entity->vel, 0.9f); // slow down
    } break;
    default:
      break;
    }
  }
}

void update_player(TransientStorage *t, float turn_input, PermanentStorage *p) {
  float dt = FIXED_DT;
  // --- Player Timers -----------------------------------
  t->player.reload_time = fmaxf(t->player.reload_time - FIXED_DT, 0);
  // (NOTE) theres probably a better way to know we've finished reloading
  if (t->player.reload_time == 0 && t->player.ammo == 0) {
    t->player.ammo = t->player.max_ammo;
  }
  t->player.weapon_cooldown = fmaxf(t->player.weapon_cooldown - FIXED_DT, 0);
  if (t->player.damage_cooldown > 0.0f) {
    t->player.damage_cooldown -= FIXED_DT;
    t->player.color = RED;
  } else {
    t->player.color = WHITE;
  }

  // --- Player Movement -----------------------------------
  float turn_speed = 540.0f;
  float turn_drag = 50.0f;
  float target_angle = 45.0f * turn_input;
  float angle_accel = (target_angle - t->player.angle) * turn_speed;
  t->player.angle_vel += angle_accel * dt;
  t->player.angle_vel -= t->player.angle_vel * turn_drag * dt;
  t->player.angle += t->player.angle_vel * dt;
  Vector2 forward = {cosf((t->player.angle - 90.0f) * DEG2RAD),
                     sinf((t->player.angle - 90.0f) * DEG2RAD)};
  float accel_strength = 325.0f;
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
  // t->player.pos.y -= t->player.vel.y * dt;

  // ------ PARRY & SHOOTING ----------------------------------------------
  // (NOTE) we parry bullets from behind too, should probably turn that off
  // (NOTE) Maybe i should make the bullet collision box bigger than the visual
  // TODO fix shooing twice when parrying - i think the bullet fires because
  // theres no entity in the box but then the next frame we're still checking if
  // anythings in the box and there is so we spawn an additonal bullet or maybe
  // the original bullet isnt being destroyed in collision
  // (TODO) maybe predict trajectory of enemy bullet then fire where it will be
  t->player.parry_area = (Rectangle){
      .x = t->player.pos.x - 2.0f,
      .y = t->player.pos.y - 5.0f,
      .width = 4.0f,
      .height = 2.0f,
  };

  bool parry_count = 0;
  if (t->player.parry_window_timer > 0) {
    t->player.parry_window_timer = fmaxf(0, t->player.parry_window_timer - dt);
    for (int i = 0; i < t->entity_count; i++) {
      Entity *entity = &t->entities[i];

      // Skip non-projectiles entities and skip player projectiles
      if (entity->type != ENTITY_PROJECTILE ||
          is_set(entity, EntityFlags_Player) || entity->parry_processed)
        continue;

      // Skip projectiles outside the detection cone
      Rectangle entity_rect = rect_from_entity(entity);
      if (!CheckCollisionRecs(entity_rect, t->player.parry_area))
        continue;

      entity->parry_processed = true;
      parry_count++;

      Vector2 entity_center = get_rect_center(entity_rect);
      Vector2 player_center = get_rect_center(rect_from_entity(&t->player));

      // Direction from player to projectile
      Vector2 detected_location = Vector2Subtract(player_center, entity_center);
      Entity *projectile =
          entity_projectile_spawn(t, t->player.pos.x, t->player.pos.y);
      projectile->pos.y -= projectile->height;
      projectile->pos.x += projectile->width / 5.0f;
      projectile->color = GREEN;

      Vector2 detected_location_direction = Vector2Normalize(detected_location);
      float speed = Vector2Length(projectile->vel);
      projectile->vel = Vector2Scale(detected_location_direction, speed);
      // (NOTE) this Vector2Negate only works because the player is only ever
      // moving and only shoots from one direction but maybe this is worth
      // changing to using the players direction
      projectile->vel = Vector2Negate(projectile->vel);

      set_flag(projectile, EntityFlags_Player);
      PlaySound(p->player_gunshot);
    }
  }

  bool is_reloading = (t->player.reload_time > 0);
  if (t->player.is_firing && parry_count == 0 && !is_reloading &&
      t->player.weapon_cooldown <= 0) {

    Vector2 player_center = get_rect_center(rect_from_entity(&t->player));
    Entity *projectile =
        entity_projectile_spawn(t, player_center.x, player_center.y);
    projectile->color = WHITE;

    Vector2 mouse_screen = GetMousePosition();
    Vector2 mouse_world = GetScreenToWorld2D(mouse_screen, t->camera);
    Vector2 mouse_iso = screen_to_iso(mouse_world);

    Vector2 dir = Vector2Normalize(Vector2Subtract(mouse_iso, t->player.pos));
    float speed = Vector2Length(projectile->vel);

    // ADD SPREAD
    float base_angle = atan2f(dir.y, dir.x);
    float spread = random_betweenf(-5.0f, 5.0f) * DEG2RAD;
    float new_angle = base_angle + spread;
    Vector2 final_dir = {cosf(new_angle), sinf(new_angle)};
    projectile->vel = Vector2Scale(final_dir, speed);

    // Spawn projectile outside of the players hitbox
    float player_radius =
        0.5 * sqrtf(powf(t->player.width, 2) + powf(t->player.height, 2));
    float projectile_radius =
        0.5 * sqrtf(powf(projectile->width, 2) + powf(projectile->height, 2));
    Vector2 spawn_offset =
        Vector2Scale(final_dir, player_radius + projectile_radius);
    projectile->pos = Vector2Add(spawn_offset, projectile->pos);

    PlaySound(p->player_gunshot);
    // i should probably not have to specify that the bullet is owned by the
    // player to ignore it for ricochet
    set_flag(projectile, EntityFlags_Player);
    t->player.weapon_cooldown = 0.125f;
    t->player.ammo--;
  }

  if (t->player.ammo == 0 && t->player.reload_time <= 0) {
    t->player.reload_time = 1.0f;
  }

  t->player.is_firing = false;
}

void resolve_collisions(TransientStorage *t, PermanentStorage *p) {
  // Player-entity collision
  Rectangle player_rect = {t->player.pos.x, t->player.pos.y, t->player.width,
                           t->player.height};
  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];

    if (entity->type == ENTITY_PARTICLE) {
      continue;
    }

    if (CheckCollisionRecs(player_rect, rect_from_entity(entity)) &&
        t->player.damage_cooldown <= 0) {
      PlaySound(p->hit_sound);
      t->player.current_health--;
      t->player.damage_cooldown = 0.5f;
      t->shake_timer = 0.5f;

      if (entity->type == ENTITY_PROJECTILE) {
        set_flag(entity, EntityFlags_Deleted);
      }
    }
  }

  // Entity-entity collision
  for (int i = 0; i < t->entity_count; i++) {
    Entity *a = &t->entities[i];
    if (a->type == ENTITY_NONE || is_set(a, EntityFlags_Deleted))
      continue;

    if (!is_onscreen(a, &t->player))
      continue;

    for (int j = i + 1; j < t->entity_count; j++) {
      Entity *b = &t->entities[j];
      if (b->type == ENTITY_NONE || is_set(b, EntityFlags_Deleted))
        continue;

      if (!CheckCollisionRecs(rect_from_entity(a), rect_from_entity(b)))
        continue;

      if (is_set(a, EntityFlags_Projectile) &&
          is_set(b, EntityFlags_Projectile)) {

        if (is_set(a, EntityFlags_Player)) {
          delete_entity(a);
          b->vel = Vector2Negate(b->vel);
          b->color = GOLD;
          b->pos.y += b->vel.y * FIXED_DT;
        } else if (is_set(b, EntityFlags_Player)) {
          delete_entity(b);
          a->vel = Vector2Negate(a->vel);
          a->color = GOLD;
          a->pos.y += a->vel.y * FIXED_DT;
        } else {
          delete_entity(a);
          delete_entity(b);
        }
        continue;
      }

      if (is_set(a, EntityFlags_Destructable)) {
        if (a->type == ENTITY_GUNMEN) {
          PlaySound(p->enemy_death_sound);
          t->enemies_killed++;
        }
        delete_entity(a);
        delete_entity(b);
        continue;
      }

      if (is_set(a, EntityFlags_NotDestructable) &&
          is_set(b, EntityFlags_Projectile)) {
        delete_entity(b);
        continue;
      }
    }
  }
}

void update_entities(Memory *memory) {
  PermanentStorage *p = &memory->permanent;
  TransientStorage *t = &memory->transient;

  for (int i = 0; i < t->entity_count; i++) {
    Entity *entity = &t->entities[i];

    if (!is_onscreen(entity, &t->player))
      continue;

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
    case ENTITY_HORSE_GUNMEN: {
      // Follow behavior for horse enemies
      Vector2 target_offset = {1.5, -3.5}; // stay slightly behind the player
      Vector2 target = Vector2Add(t->player.pos, target_offset);
      Vector2 to_target = Vector2Subtract(target, entity->pos);
      float distance = Vector2Length(to_target);

      if (distance > 0.05f) {
        Vector2 dir = Vector2Normalize(to_target);

        // speed scales with distance (faster when far away)
        float min_speed = 3.0f;
        float max_speed = 10.0f;
        float t_speed = fminf(distance / 5.0f, 1.0f); // smooth ramp up
        float follow_speed = Lerp(min_speed, max_speed, t_speed);

        entity->vel = Vector2Scale(dir, follow_speed);

        // prevent overlap: stop short of the player
        if (distance < 0.75f) {
          entity->vel = (Vector2){0, 0};
        }
      } else {
        // damp to stop jitter
        entity->vel = Vector2Scale(entity->vel, 0.9f);
      }

      if (false) {
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
    if (t->player.weapon_cooldown <= 0) {
      t->player.is_firing = true;
      t->player.parry_window_timer = FIXED_DT * 2;
    }
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

  t->cursor_pos = GetMousePosition();
  if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
    if (t->player.weapon_cooldown <= 0) {
      t->player.is_firing = true;
      t->player.parry_window_timer = FIXED_DT * 2;
    }
  }

  // UpdateMusicStream(p->bg_music);

  // Reset on death
  if (t->player.current_health <= 0) {
    StopMusicStream(p->bg_music);
    // init_game(t);
    return;
  }

  if (t->hitstop_timer > 0) {
    t->hitstop_timer -= dt;
    if (t->shake_timer > 0.0f) {
      t->shake_timer -= FIXED_DT;
      float offset_x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
      t->camera.target.x += offset_x * 5.0f;
    }
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
    update_player(t, turn_input, p);
    update_entities(memory);
    update_entity_movement(t);
    resolve_collisions(t, p);

    // Create parry particle
    for (int i = 0; i < t->parry_events_count; i++) {
      Vector2 *event = &t->parry_events[i];
      for (int j = 0; j < 12; j++) {
        float angle = ((float)j / 12.0f) * 2 * PI;
        Vector2 dir = {cosf(angle), sinf(angle)};
        Entity particle = {
            .type = ENTITY_PARTICLE,
            .pos = *event,
            .vel = Vector2Scale(dir, 5.0f + (rand() % 3)),
            .fade_timer = 0.5f,
            .color = GOLD,
            .width = 0.1f,
            .flags = 0,
            .height = 0.1f,
        };
        add_entity(t, particle);
      }
    }
    t->parry_events_count = 0;

    Vector2 player_screen =
        isometric_projection((Vector3){0, t->player.pos.y, 0});
    t->camera.target = player_screen;
    t->camera.offset =
        // (TODO) move screenwidth/height to game state or perm storage or
        // something
        (Vector2){GetScreenWidth() / 4.0f, GetScreenHeight() / 1.5f};

    if (t->shake_timer > 0) {
      float offset_x = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
      float shake_magnitude = 10.0f;
      t->camera.target.x += offset_x * shake_magnitude;
    }

    for (int i = 0; i < t->entity_count; i++) {
      Entity *entity = &t->entities[i];
      if (is_set(entity, EntityFlags_Deleted)) {
        entity->type = ENTITY_NONE;
      }
    }
    compact_entities(t);

    steps++;
    t->accumulator -= FIXED_DT;
  }
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
  Rectangle water_tile = get_tile_source_rect(tilesheet, 120);
  Rectangle tile2 = get_tile_source_rect(tilesheet, 23);

  // --- Draw world ---
  // (NOTE) figure out how to start from 0 instead of a
  // negative number
  for (int y = tile_y - 30; y < tile_y + 14; y++) {
    // Left side
    // for (int x = -21; x < -5; x++) {
    //   Vector2 screen = isometric_projection((Vector3){x, y, 0});
    //   DrawTextureRec(p->tilesheet, tile, screen, WHITE);
    // }

    // Left side
    for (int x = -21; x < -8; x++) {
      Vector2 screen = isometric_projection((Vector3){x, y, 0});
      DrawTextureRec(p->tilesheet, tile2, screen, WHITE);
    }

    for (int x = -8; x < -5; x++) {
      Vector2 screen = isometric_projection((Vector3){x, y, 0});
      DrawTextureRec(p->tilesheet, water_tile, screen, WHITE);
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
    } break;

    case ENTITY_HORSE_GUNMEN: {
      Vector3 cube_center = {entity->pos.x + entity->width / 2.0f,
                             entity->pos.y + entity->height / 2.0f, 0};
      draw_iso_cube(cube_center, entity->width, entity->height, 10.5f,
                    entity->angle, entity->color);
    } break;

    case ENTITY_PARTICLE: {
      if (entity->fade_timer > 0) {
        Vector2 project = project_iso(entity->pos);
        DrawCircle(project.x, project.y, 1, GOLD);
      } else {
        entity->type = ENTITY_NONE;
      }
    } break;

    default:
      break;
    }

    if (p->debug_on && entity->type != ENTITY_PROJECTILE) {
      draw_entity_collision_box(entity);
      DrawPlayerDebug(&t->player);

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

        if (t->player.parry_window_timer > 0) {
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
  // the sim region should be the camera area
  //
  // (NOTE) My fps on my macbook goes from 200-400 to 400-500 when i turn camera
  // zoom off
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

  if (t->hitstop_timer > 0.0f) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                  (Color){255, 255, 255, 10});
  }

  DrawCircleLines(t->cursor_pos.x, t->cursor_pos.y, 10, WHITE);
  DrawCircle(t->cursor_pos.x, t->cursor_pos.y, 2, WHITE);
  HideCursor();

  int remaining_ammo_font_size = 30 * scale;
  const char *remaining_ammo =
      TextFormat("%i / %i", t->player.ammo, t->player.max_ammo);
  float ammo_width = MeasureText(remaining_ammo, remaining_ammo_font_size);
  DrawText(remaining_ammo, GetScreenWidth() - (ammo_width),
           GetScreenHeight() - remaining_ammo_font_size,
           remaining_ammo_font_size, WHITE);

  DrawFPS(0, 0);
  EndDrawing();
}

void game_update_and_render(Memory *memory) {
  update(memory);
  render(memory);
}
