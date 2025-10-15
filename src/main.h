#ifndef MAIN_H
#define MAIN_H

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_ENTITIES 10000
#define TILE_SIZE 32
#define TILE_WIDTH 32
#define TILE_HEIGHT 16

typedef enum { GAME_OVER, PLAYING } State;

typedef enum {
  ENTITY_NONE = 0,
  ENTITY_HAZARD,
  ENTITY_PLAYER,
  ENTITY_BULLET,
} EntityType;

typedef struct {
  EntityType type;

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
  Color color;
} Entity;

typedef struct {
  State state;

  Entity entities[MAX_ENTITIES];
  int entity_count;
  Entity *draw_list[MAX_ENTITIES];

  Entity player;

  float shake_timer;
  float game_timer;

  Camera2D camera;
} GameState;

#endif
