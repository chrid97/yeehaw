#include "game.h" // your own shared types
#include "platform.h"
#include "raylib.h"
#include <assert.h>

void set_flag(Entity *entity, uint32_t flags) { entity->flags |= flags; }
bool is_set(Entity *entity, uint32_t flags) { return entity->flags & flags; }

Entity *entity_spawn(TransientStorage *t, float x, float y, EntityType type) {
  assert(t->entity_count < MAX_ENTITIES && "Entity overflow!");
  t->entities[t->entity_count++] = (Entity){
      .pos = {x, y},
      .vel = {0, 0},
      .width = 1,
      .height = 1,
      .type = type,
      .color = WHITE,
      .angle = 0,
      .angle_vel = 0,
      .bank_angle = 0,
      .current_health = 0,
      .damage_cooldown = 0,
      .active = false,
  };

  return &t->entities[t->entity_count - 1];
}

Entity *entity_projectile_spawn(TransientStorage *t, float x, float y) {
  Entity *projectile = entity_spawn(t, x, y, ENTITY_PROJECTILE);
  projectile->color = PURPLE;
  projectile->width = 0.25;
  projectile->height = 0.25;
  projectile->vel.x = 0;
  projectile->vel.y = 25.0f;
  set_flag(projectile, EntityFlags_IsProjectile);

  return projectile;
};
