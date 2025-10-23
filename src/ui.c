#include "game.h" // your own shared types
#include "raylib.h"

Color col_dark_gray = {0x21, 0x25, 0x29, 255};  // #212529
Color col_mauve = {0x61, 0x47, 0x50, 255};      // #614750
Color col_warm_brown = {0x8C, 0x5C, 0x47, 255}; // #8c5c47
Color col_tan = {0xB4, 0x7D, 0x58, 255};        // #b47d58
Color col_sand = {0xD9, 0xB7, 0x7E, 255};       // #d9b77e
Color col_cream = {0xEA, 0xD9, 0xA7, 255};      // #ead9a7

// (TODO) cleanup game summary ui code
typedef struct {
  Rectangle rect;
  int number_rows;
  float padding_x;
  float gap_y;
  float header_y_end;
  int font_size;
  float scale;
} PostGameSummary;

void draw_post_game_summary_header(PostGameSummary *summary, float scale) {
  Rectangle rect = summary->rect;
  DrawRectangle(rect.x * scale, rect.y * scale, rect.width * scale,
                rect.height * scale, col_cream);

  float center_x = rect.x + (rect.width / 2.0f);
  const char *title = "LEVEL COMPLETE";
  int title_width = MeasureText(title, summary->font_size);
  DrawText(title, (center_x - (title_width / 2.0f)) * scale,
           (rect.y + summary->gap_y) * scale, summary->font_size * scale,
           BLACK);

  float header_y_end = summary->header_y_end;
  DrawLine(rect.x * scale, header_y_end * scale, (rect.x + rect.width) * scale,
           header_y_end * scale, BLACK);
}

void draw_score_breakdown(PostGameSummary *summary, const char *text,
                          const char *points_text) {
  float x = summary->rect.x;
  float y = summary->rect.y;
  float width = summary->rect.width;
  int font_size = summary->font_size;
  float scale = summary->scale;

  summary->number_rows++;
  y = summary->header_y_end + (summary->gap_y * summary->number_rows);

  DrawText(text, (x + summary->padding_x) * scale, y * scale, 10 * scale,
           BLACK);

  int points_width = MeasureText(points_text, font_size);
  DrawText(points_text, (x + width - points_width - summary->padding_x) * scale,
           y * scale, font_size * scale, BLACK);
}
