#ifndef PLATFORM_H
#define PLATFORM_H

// -------------------------------------
// Constants
// -------------------------------------
#include "raylib.h"
#include <stdint.h>
#define TILE_SIZE 32

#define VIRTUAL_WIDTH 640
#define VIRTUAL_HEIGHT 360
#define FIXED_DT (1.0f / 120.0f)

#define MAX_ENTITIES 1000

// -------------------------------------
// Enums
// -------------------------------------

typedef enum {
  ENTITY_NONE = 0,
  ENTITY_HAZARD,
  ENTITY_PLAYER,
  ENTITY_PROJECTILE,
  ENTITY_GUNMEN,
  ENTITY_PARTICLE,
} EntityType;

typedef enum {
  EntityFlags_IsDestructable = (1 << 0),
  EntityFlags_IsProjectile = (1 << 1),
  EntityFlags_IsPlayer = (1 << 2),
} EntityFlags;

typedef struct {
  EntityType type;
  uint32_t flags;
  bool active;

  float height;
  float width;
  Vector2 pos;
  Vector2 vel;
  float angle;
  float angle_vel;
  float bank_angle;

  int current_health;
  int max_health;

  float damage_cooldown;
  float weapon_cooldown;
  bool is_firing;

  bool parry_processed;
  Rectangle parry_area;
  float parry_window_timer;

  float fade_timer;

  Color color;
} Entity;

// -------------------------------------
// Transient (resettable) storage
// -------------------------------------
typedef struct {

  Entity entities[MAX_ENTITIES];
  int entity_count;
  Entity *draw_list[MAX_ENTITIES];

  Entity player;

  float shake_timer;
  float game_timer;

  Camera2D camera;

  double accumulator;
  bool game_initialized;

  // scoring stuff
  int enemies_killed;
  // I don't know if this one makes sense since the map is always the same size
  // so if you complete the level it should always be the same
  float distance_traveled;
  // int obstacles_destroyed;
  int score;
  // map stuff
  Vector2 map_end;

  float hitstop_timer;
  Vector2 parry_events[10];
  int parry_events_count;
} TransientStorage;

// -------------------------------------
// Permanent (persistent) storage (Lifetime is the duration of the game)
// -------------------------------------
typedef struct {
  Texture2D tilesheet;
  Music bg_music;

  Sound hit_sound;
  Sound enemy_death_sound;
  Sound player_gunshot;
  Sound parry_sound;

  bool debug_on;
} PermanentStorage;

typedef struct {
  PermanentStorage permanent;
  TransientStorage transient;
} Memory;

#endif
