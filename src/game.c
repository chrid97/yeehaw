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

static inline void print_float(const char *str, float v) {
  printf("%s: %f\n", str, v);
}

static inline void print_vector(Vector2 v) {
  printf("(x: %f, y: %f)\n", v.x, v.y);
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

  // float width = player->size.x / 2.0f;
  // float height = player->size.y / 2.0f;
  // Rectangle head_rect = {player->pos.x, player->pos.y - 36,
  //                        player->size.x / 1.5f, player->size.y / 2.0f};

  // Vector2 head_origin = {head_rect.width / 2.0f, head_rect.height / 2.0f};
  // DrawRectanglePro(scale_rect(&head_rect, scale),
  //                  Vector2Scale(head_origin, scale), player->head_angle,
  //                  BROWN);
  // DrawRectangleRounded(scale_rect(&head_rect, scale), 1, 10, BROWN);

  // Draw this to show what direction the player is facing
  float radians = (player->angle + 90) * DEG2RAD;
  Vector2 forward = {cosf(radians), sinf(radians)};
  Vector2 face_pos =
      Vector2Add(player->pos, Vector2Scale(forward, player->size.y / -3.0f));
  // DrawCircle(face_pos.x * scale, face_pos.y * scale, 2 * scale, BROWN);
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

    game_state->player = (Entity){
        .type = ENTITY_PLAYER,
        // .size = {18.5, 48},
        .size = {40, 100},
        .pos = {VIRTUAL_WIDTH / 2.0f, VIRTUAL_HEIGHT / 2.0f},
        .vel = {0, 0},
        .angle = 0,
        .head_angle = 0,
        .color = COLOR_DARK_GREY,
    };
    game_state->camera.target = game_state->player.pos;
    game_state->camera.offset =
        (Vector2){input->screen_width / 2.0f, input->screen_height / 2.0f};
    game_state->camera.zoom = 1;
    game_state->camera.rotation = 0;

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
    // Calculate angular acceleration & integrate
    // -------------------------------------
    float turn_accel = 400 * turn_input;
    float turn_drag = 10;
    float lin_drag = 8.0f;
    float quad_drag = 0.2f;

    // (NOTE) maybe I'll want to reduce the angular acceleration at lower speed?
    float angular_acceleration = turn_accel - player->angular_vel * turn_drag;
    player->angular_vel += angular_acceleration * dt;
    player->angle += player->angular_vel * dt;

    // -------------------------------------
    // Calculate acceleration
    // -------------------------------------
    Vector2 facing_dir = {cosf((player->angle + 90) * DEG2RAD),
                          sinf((player->angle + 90) * DEG2RAD)};
    Vector2 dir_force = Vector2Scale(facing_dir, forward_input * 2000);
    float speed = Vector2Length(player->vel);
    Vector2 drag_force =
        Vector2Add(Vector2Scale(player->vel, -lin_drag),
                   Vector2Scale(player->vel, -quad_drag * speed));
    Vector2 net_force = Vector2Add(dir_force, drag_force);
    Vector2 acceleration = Vector2Scale(net_force, 1.0f);
    print_vector(net_force);

    // -------------------------------------
    // Integrate acceleration and velocity
    // -------------------------------------
    player->vel = Vector2Add(player->vel, Vector2Scale(acceleration, dt));
    player->pos = Vector2Add(player->pos, Vector2Scale(player->vel, dt));

    game_state->camera.target = player->pos;
    game_state->accumulator -= FIXED_DT;
  }

  // -------------------------------------
  // RENDER
  // -------------------------------------
  BeginDrawing();
  BeginMode2D(game_state->camera);
  ClearBackground(COLOR_SAND);

  DrawRectangle(0, 0, 100, 100, RED);

  draw_player(player, 1);
  const int TOTAL_SEGMENTS = 5;
  const int HORSE_SEGMENTS = player->size.y / TOTAL_SEGMENTS;

  const int SPACING = 10;
  const int RADIUS = 5;

  Vector2 dir = {cosf((player->angle - 90) * DEG2RAD),
                 sinf((player->angle - 90) * DEG2RAD)};

  int radius = 5;

  Vector2 origin = (Vector2){player->pos.x,
                             player->pos.y - (player->size.y / 2.0f) + radius};
  Vector2 p0 = (Vector2){origin.x, origin.y + 20};
  Vector2 constraint = Vector2Subtract(origin, p0);

  Color color = RED;
  DrawCircleV(origin, radius, color);
  DrawCircleV(p0, radius, color);

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
