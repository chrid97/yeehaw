#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_TILES 100

typedef enum { GAME_OVER, PLAYING } State;

typedef enum {
  ENTITY_NONE = 0,
  ENTITY_HAZARD,
  ENTITY_PLAYER,
} EntityType;

typedef struct {
  float height;
  float width;
  Vector2 pos;
  Vector2 velocity;
  // maybe is_on_screen is a better name?
  bool is_active;
  int current_health;
  int max_health;
  Color color;
  float damage_cooldown;
  EntityType type;
} Entity;

typedef struct {
  State state;
} GameState;
