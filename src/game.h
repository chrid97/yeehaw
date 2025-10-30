#include "platform.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

// #define VIRTUAL_WIDTH 640
// #define VIRTUAL_HEIGHT 360

// PLAYING AROUND WITH SMALLER RESOLUTION
// maybe it would be cool if i could setup black bars like a western film
#define VIRTUAL_WIDTH 480
#define VIRTUAL_HEIGHT 270

#define FIXED_DT (1.0f / 120.0f)

#define MAX_ENTITIES 100

// -------------------------------------
// Enums
// -------------------------------------
typedef enum {
  ENTITY_NONE = 0,
  ENTITY_PLAYER,
} EntityType;

// -------------------------------------
// Structs
// -------------------------------------
typedef struct {
  EntityType type;

  /// Width & Height
  Vector2 size;
  /// Center of Entity
  Vector2 pos;
  /// Speed + Direction
  Vector2 vel;
  /// Degrees
  float angle;

  /// RGB
  Color color;
} Entity;

typedef struct {
  // Entity entities[MAX_ENTITIES];
  // int entity_count;
  // Entity *draw_list[MAX_ENTITIES];

  Entity player;

  bool game_initialized;
} GameState;

void game_update_and_render(Memory *memory, GameInput *game_input);
