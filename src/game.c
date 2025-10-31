// ---------- MECHANICS TODO -------------
// (TODO) Move to units for time and world pos
// (TODO) Add fixed time step
// (TODO)  Horse movement
// (TODO) Setup Camera
// - flip camera y
//
// (TODO) Hazards
// (TODO) SPRITNING / STAMINA
// ---------- MISC TODO -------------
// (TODO) auto generate compile_commands.json
// (TODO) rewrite platform.c
#include "game.h"
#include "platform.h"
#include "raylib.h"
#include "raymath.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

const Color COLOR_SAND = {0xD9, 0xB7, 0x7E, 255};      // #d9b77e
const Color COLOR_DARK_GREY = {0x21, 0x25, 0x29, 255}; // #212529

static inline void print_vector(Vector2 v) {
  printf("(x: %f, y: %f)\n", v.x, v.y);
}

Rectangle scale_rect(Rectangle *rect, float scale) {
  return (Rectangle){rect->x * scale, rect->y * scale, rect->width * scale,
                     rect->height * scale};
}

void draw_player(Entity *player, float scale) {
  Rectangle rect = {player->pos.x, player->pos.y, player->size.x,
                    player->size.y};
  Vector2 origin = {player->size.x / 2.0f, player->size.y / 2.0f};
  DrawRectanglePro(scale_rect(&rect, scale), Vector2Scale(origin, scale),
                   player->angle, player->color);

  // Draw this to show what direction the player is facing
  float radians = (player->angle + 90) * DEG2RAD;
  Vector2 forward = {cosf(radians), sinf(radians)};
  Vector2 face_pos =
      Vector2Add(player->pos, Vector2Scale(forward, player->size.y / -3.0f));
  DrawCircle(face_pos.x * scale, face_pos.y * scale, 2 * scale, BROWN);
}

// (NOTE) maybe I can get rid of this
void draw_entity(Entity *entity, float scale) {
  Rectangle rect = {entity->pos.x, entity->pos.y, entity->size.x,
                    entity->size.y};
  Vector2 origin = {entity->size.x / 2.0f, entity->size.y / 2.0f};
  DrawRectanglePro(scale_rect(&rect, scale), Vector2Scale(origin, scale),
                   entity->angle, entity->color);
}

Rectangle entity_from_rect(Entity *entity) {
  // Get top left corner of entity
  return (Rectangle){entity->pos.x - (entity->size.x / 2.0f),
                     entity->pos.y - (entity->size.y / 2.0f), entity->size.x,
                     entity->size.y};
}

void game_update_and_render(Memory *memory, GameInput *input) {
  assert(sizeof(GameState) <= memory->permanent_storage_size);
  GameState *game_state = (GameState *)memory->permanent_storage;

  float scale_x = (float)input->screen_width / VIRTUAL_WIDTH;
  float scale_y = (float)input->screen_height / VIRTUAL_HEIGHT;
  float scale = fminf(scale_x, scale_y);

  if (!memory->initalized) {
    game_state->player = (Entity){
        .type = ENTITY_PLAYER,
        .size = {15, 20},
        .pos = {VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f},
        .vel = {0, 0},
        .angle = 0,
        .color = COLOR_DARK_GREY,
    };
    memory->initalized = true;
  }

  Entity *player = &game_state->player;

  if (input->reload) {
    memory->initalized = false;
    printf("Reload game\n");
  }

  float dt = FIXED_DT;
  game_state->accumulator += GetFrameTime();
  while (game_state->accumulator >= FIXED_DT) {

    int turn_input = (int)input->move_right - (int)input->move_left;
    int forward_input = (int)input->move_down - (int)input->move_up;

    // -------------------------------------
    // Calculate angular acceleration
    // -------------------------------------
    float turn_accel = 400 * turn_input;
    float turn_drag = 10;
    // (NOTE) maybe I'll want to reduce the angular acceleration at lower speed?
    float angular_acceleration = turn_accel - player->angular_vel * turn_drag;
    player->angular_vel += angular_acceleration * dt;
    player->angle += player->angular_vel * dt;

    // -------------------------------------
    // Calculate acceleration
    // -------------------------------------
    Vector2 dir = {cosf((player->angle + 90) * DEG2RAD),
                   sinf((player->angle + 90) * DEG2RAD)};
    Vector2 dir_force = Vector2Scale(dir, forward_input * 3000);
    float drag_coeff = 1;
    float speed = Vector2Length(player->vel);
    Vector2 drag_force = Vector2Scale(player->vel, -drag_coeff * speed);
    // Vector2 drag_force = Vector2Scale(player->vel, -drag_coeff);
    Vector2 acceleration = Vector2Add(dir_force, drag_force);

    // -------------------------------------
    // Integrate acceleration and velocity
    // -------------------------------------
    player->vel = Vector2Add(player->vel, Vector2Scale(acceleration, dt));
    player->pos = Vector2Add(player->pos, Vector2Scale(player->vel, dt));

    player->pos.x = Clamp(player->pos.x, player->size.x / 2.0f,
                          VIRTUAL_WIDTH - player->size.x / 2.0f);
    player->pos.y = Clamp(player->pos.y, player->size.y / 2.0f,
                          VIRTUAL_HEIGHT - player->size.y / 2.0f);
    game_state->accumulator -= FIXED_DT;
  }

  // -------------------------------------
  // RENDER
  // -------------------------------------
  BeginDrawing();
  ClearBackground(COLOR_SAND);

  draw_player(player, scale);

  DrawFPS(0, 0);
  EndDrawing();
}
