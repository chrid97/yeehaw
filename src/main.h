#ifndef MAIN_H
#define MAIN_H

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#define MAX_TILES 100

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
} GameState;

const int TILE_WIDTH = 32;
const int TILE_HEIGHT = 16;

#endif
