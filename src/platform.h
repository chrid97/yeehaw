#ifndef PLATFORM_H
#define PLATFORM_H

// -------------------------------------
// Constants
// -------------------------------------
#include "raylib.h"
#define TILE_SIZE 32

#define VIRTUAL_WIDTH 640
#define VIRTUAL_HEIGHT 360
#define FIXED_DT (1.0f / 120.0f)

#define MAX_ENTITIES 10000

// -------------------------------------
// Enums
// -------------------------------------

typedef enum {
  ENTITY_NONE = 0,
  ENTITY_HAZARD,
  ENTITY_PLAYER,
  ENTITY_PROJECTILE,
  ENTITY_GUNMEN,
} EntityType;

typedef struct {
  EntityType type;
  // if an entity type is none it should be inactive

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
} TransientStorage;

// -------------------------------------
// Permanent (persistent) storage (Lifetime is the duration of the game)
// -------------------------------------
typedef struct {
  Texture2D tilesheet;
  Music bg_music;
  Sound hit_sound;
  bool debug_on;
} PermanentStorage;

typedef struct {
  PermanentStorage permanent;
  TransientStorage transient;
} Memory;

#endif
