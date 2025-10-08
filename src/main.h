#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_TILES 100

typedef enum { GAME_OVER, PLAYING } State;

typedef struct {
  float height;
  float width;
  Vector2 pos;
  Vector2 velocity;
  bool is_active;
} Entity;

typedef struct {
  State state;
} GameState;
