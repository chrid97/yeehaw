#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define Terabytes(Value) (Gigabytes(Value) * 1024LL)

// SIGNED
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

// UNSIGNED
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct {
  // --- Player input ---
  bool move_up;
  bool move_down;
  bool move_left;
  bool move_right;
  bool reload;
  bool mute;

  // --- Environment ---
  u16 screen_width;
  u16 screen_height;
} GameInput;

// MEMORY
typedef struct {
  bool initalized;

  u64 permanent_storage_size;
  void *permanent_storage;

  u64 transient_storage_size;
  void *transient_storage;
} Memory;

#endif
