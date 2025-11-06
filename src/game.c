// ---------- MECHANICS TODO -------------
// (TODO) Move to units for time and world pos
// (TODO)  Horse movement
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
#include <string.h>
#include <unistd.h>

const Color COLOR_SAND = {0xD9, 0xB7, 0x7E, 255};      // #d9b77e
const Color COLOR_DARK_GREY = {0x21, 0x25, 0x29, 255}; // #212529

static inline void print_float(const char *str, float v) {
  printf("%s: %f\n", str, v);
}

static inline void print_vector(Vector2 v) {
  printf("(x: %f, y: %f)\n", v.x, v.y);
}

Entity *add_entity(GameState *game_state) {
  int index = game_state->entity_count++;
  game_state->entities[index] = (Entity){
      .type = ENTITY_NONE,
      .size = {0},
      .pos = {0},
      .vel = {0},
      .angle = 0,
      .angular_vel = 0,
      .head_angle = 0,
      .color = PURPLE,
  };

  return &game_state->entities[index];
}

/// Draw text starting from the right edge of the screen
void draw_text_right_aligned_screen(const char *text, float y, int font_size,
                                    float scale) {
  float text_width = MeasureText(text, font_size * scale);
  DrawText(text, GetScreenWidth() - text_width, y, font_size * scale, BLUE);
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
}

// (NOTE) maybe I can get rid of this
void draw_entity(Entity *entity, float scale) {
  Rectangle rect = {entity->pos.x, entity->pos.y, entity->size.x,
                    entity->size.y};
  Vector2 origin = {entity->size.x / 2.0f, entity->size.y / 2.0f};
  DrawRectanglePro(scale_rect(&rect, scale), Vector2Scale(origin, scale),
                   entity->angle, entity->color);
}

Rectangle rect_from_entity(Entity *entity) {
  return (Rectangle){entity->pos.x, entity->pos.y, entity->size.x,
                     entity->size.y};
}

/// Since we draw entities starting from the center but Raylib collision assumes
/// the rectangle is drawn from the topleft i made this helper function to draw
/// entity from the top left corner of rect
Rectangle collision_rect_from_entity(Entity *entity) {
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
    game_state->accumulator = 0;
    game_state->entity_count = 0;

    memset(game_state->segments, 0, sizeof(game_state->segments));

    game_state->player = (Entity){
        .type = ENTITY_PLAYER,
        // .size = {18.5, 48},
        .size = {40, 100},
        .pos = {VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f},
        .vel = {0, 0},
        .angle = 0,
        .angular_vel = 0,
        .head_angle = 0,
        .color = COLOR_DARK_GREY,
    };
    game_state->camera.target = game_state->player.pos;
    game_state->camera.offset =
        (Vector2){input->screen_width / 2.0f, input->screen_height - 100};
    game_state->camera.zoom = 0.45;
    game_state->camera.rotation = 0;

    memory->initalized = true;

    for (int i = 0; i < 10; i++) {
      Entity *entity = add_entity(game_state);
      entity->type = ENTITY_WALL;
      entity->size = (Vector2){50, 50};
      entity->pos = (Vector2){VIRTUAL_WIDTH / 2.0f - (i * 10), -100 * i};
    }
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
    // Calculate angular acceleration & integrate
    // -------------------------------------
    float turn_accel = 400.0f * turn_input;
    float turn_drag = 8.0f;
    float lin_drag = 2.0f;
    float quad_drag = 0.1f;

    // (NOTE) maybe I'll want to reduce the angular acceleration at lower speed?
    float angular_acceleration = turn_accel - player->angular_vel * turn_drag;
    player->angular_vel += angular_acceleration * dt;
    player->angle += player->angular_vel * dt;

    // -------------------------------------
    // Calculate acceleration
    // -------------------------------------
    Vector2 facing_dir = {cosf((player->angle + 90) * DEG2RAD),
                          sinf((player->angle + 90) * DEG2RAD)};
    Vector2 dir_force = Vector2Scale(facing_dir, forward_input * 40000.0f);
    float speed = Vector2Length(player->vel);
    Vector2 drag_force =
        Vector2Add(Vector2Scale(player->vel, -lin_drag),
                   Vector2Scale(player->vel, -quad_drag * speed));
    Vector2 net_force = Vector2Add(dir_force, drag_force);
    Vector2 acceleration = Vector2Scale(net_force, 1.0f);

    // -------------------------------------
    // Integrate acceleration and velocity
    // -------------------------------------
    player->vel = Vector2Add(player->vel, Vector2Scale(acceleration, dt));
    player->pos = Vector2Add(player->pos, Vector2Scale(player->vel, dt));
    print_vector(Vector2Scale(player->vel, dt));
    // print_float("", speed);
    // player->pos = Vector2Add(player->pos, (Vector2){0, -10});

    game_state->camera.target = player->pos;
    game_state->accumulator -= FIXED_DT;
  }

  // -------------------------------------
  // RENDER
  // -------------------------------------
  BeginDrawing();
  BeginMode2D(game_state->camera);
  ClearBackground(COLOR_SAND);

  int width = 1000;
  for (int i = 0; i < 100; i++) {
    DrawRectangle(VIRTUAL_WIDTH / 2.0f - width / 2.0f, VIRTUAL_HEIGHT * -i,
                  width, 1000, BEIGE);
    DrawRectangle(width, VIRTUAL_HEIGHT * -i, 50, 100, RED);
  }

  for (int i = 0; i < game_state->entity_count; i++) {
    Entity *entity = &game_state->entities[i];
    draw_entity(entity, 1);
  }

  draw_player(player, 1);

  // int distance = 20;
  // Vector2 dir = {cosf((player->angle + 90) * DEG2RAD),
  //                sinf((player->angle + 90) * DEG2RAD)};
  // Vector2 back_dir = Vector2Negate(dir);
  // Vector2 anchor =
  //     Vector2Add(player->pos, Vector2Scale(dir, -player->size.y / 2.0f));
  // int radius = 5;
  // Color color = RED;
  // game_state->segments[0] = anchor;
  // // game_state->segments[0] =
  // //     GetScreenToWorld2D(GetMousePosition(), game_state->camera);
  // DrawCircleV(game_state->segments[0], radius, color);
  // for (int i = 1; i < 5; i++) {
  //   Vector2 anchor = game_state->segments[i - 1];
  //   Vector2 point = game_state->segments[i];
  //   point = Vector2Subtract(point, anchor);
  //   point = Vector2Normalize(point);
  //   point = Vector2Scale(point, distance);
  //   point = Vector2Add(point, anchor);
  //   game_state->segments[i] = point;
  //   DrawCircleV(point, radius, color);
  // }
  // DrawLineV(anchor, Vector2Add(anchor, Vector2Scale(dir, 50)), BLUE);
  // DrawLineV(anchor, Vector2Add(anchor, Vector2Scale(back_dir, 50)), GREEN);

  EndMode2D();

  int y_offset = 0;
  int font_size = 20;
  const char *text =
      TextFormat("V: (x: %f, y: %f)", player->vel.x, player->vel.y);
  draw_text_right_aligned_screen(text, y_offset, font_size, scale);
  y_offset += font_size * scale;

  float speed = Vector2Length(player->vel);
  const char *speed_text = TextFormat("SPEED: %.1f", speed);
  draw_text_right_aligned_screen(speed_text, y_offset, font_size, scale);

  DrawFPS(0, 0);
  EndDrawing();
}
