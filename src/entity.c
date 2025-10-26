#include "game.h" // your own shared types
#include "platform.h"
#include "raylib.h"
#include <assert.h>

void set_flag(Entity *entity, uint32_t flags) { entity->flags |= flags; }
bool is_set(Entity *entity, uint32_t flags) { return entity->flags & flags; }
/// Mark Entity for deletion
void delete_entity(Entity *entity) { set_flag(entity, EntityFlags_Deleted); }

void add_entity(TransientStorage *t, Entity entity) {
  assert(t->entity_count < MAX_ENTITIES && "Entity overflow!");
  t->entities[t->entity_count++] = entity;
}

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
      .weapon_cooldown = 0,
      .ammo = 0,
      .max_ammo = 0,
      .is_firing = false,
  };

  return &t->entities[t->entity_count - 1];
}

Entity *entity_projectile_spawn(TransientStorage *t, float x, float y) {
  Entity *projectile = entity_spawn(t, x, y, ENTITY_PROJECTILE);
  projectile->color = BLACK;
  projectile->width = 0.25;
  projectile->height = 0.25;
  projectile->vel.x = 0;
  projectile->vel.y = 25.0f;
  set_flag(projectile, EntityFlags_Projectile);

  return projectile;
};

Rectangle rect_from_entity(Entity *entity) {
  return (Rectangle){
      .x = entity->pos.x,
      .y = entity->pos.y,
      .width = entity->width,
      .height = entity->height,
  };
}
