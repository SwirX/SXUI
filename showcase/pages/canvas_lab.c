#include "../../sxui.h"
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PARTICLES 60

typedef struct {
  float x, y;
  float vx, vy;
  Uint32 color;
} Particle;

static Particle particles[MAX_PARTICLES];
static int particles_init = 0;
static float phase = 0;
static int current_anim = 0; // 0=Sine, 1=Particles

static void draw_sine_wave(UIElement *canvas) {
  int cw = sxui_get_width(canvas), ch = sxui_get_height(canvas);
  phase += 0.05f;
  int last_y = ch / 2;
  for (int i = 0; i < cw; i += 8) {
    int y = ch / 2 + (int)(sinf(phase + i * 0.01f) * 80 * sinf(phase * 0.2f));
    sxui_canvas_draw_line(canvas, i - 8 > 0 ? i - 8 : 0, last_y, i, y,
                          0x00FF7AFF);
    last_y = y;
  }
}

static void draw_particles(UIElement *canvas) {
  int cw = sxui_get_width(canvas), ch = sxui_get_height(canvas);
  if (!particles_init) {
    srand(time(NULL));
    for (int i = 0; i < MAX_PARTICLES; i++) {
      particles[i].x = rand() % cw;
      particles[i].y = rand() % ch;
      particles[i].vx = (rand() % 100 - 50) / 40.0f;
      particles[i].vy = (rand() % 100 - 50) / 40.0f;
      particles[i].color = (rand() % 0xFFFFFF) << 8 | 0xFF;
    }
    particles_init = 1;
  }

  for (int i = 0; i < MAX_PARTICLES; i++) {
    particles[i].x += particles[i].vx;
    particles[i].y += particles[i].vy;

    if (particles[i].x < 0 || particles[i].x > cw)
      particles[i].vx *= -1;
    if (particles[i].y < 0 || particles[i].y > ch)
      particles[i].vy *= -1;

    sxui_canvas_draw_circle(canvas, (int)particles[i].x, (int)particles[i].y, 2,
                            particles[i].color, 1);

    for (int j = i + 1; j < MAX_PARTICLES; j++) {
      float dx = particles[i].x - particles[j].x;
      if (fabsf(dx) > 50)
        continue;
      float dy = particles[i].y - particles[j].y;
      if (fabsf(dy) > 50)
        continue;

      float dist_sq = dx * dx + dy * dy;
      if (dist_sq < 2500) {
        sxui_canvas_draw_line(canvas, (int)particles[i].x, (int)particles[i].y,
                              (int)particles[j].x, (int)particles[j].y,
                              0x33333366);
      }
    }
  }
}

static void update_canvas(UIElement *canvas) {
  sxui_canvas_clear(canvas, 0x050505FF);
  if (current_anim == 0)
    draw_sine_wave(canvas);
  else
    draw_particles(canvas);
}

static UIElement *lab_canvas = NULL;

void update_canvas_lab_animation(void) {
  if (lab_canvas)
    update_canvas(lab_canvas);
}

static void on_anim_change(void *el, int index, const char *val) {
  (void)el;
  (void)val;
  current_anim = index;
}

UIElement *create_canvas_lab_page(UIElement *parent) {
  UIElement *page = sxui_frame(parent, 240, 0, 1040, 800, UI_LAYOUT_VERTICAL);
  sxui_frame_set_padding(page, 20);
  sxui_frame_set_spacing(page, 20);

  lab_canvas = sxui_canvas(page, 0, 0, 1000, 600);
  sxui_set_outline(lab_canvas, 1, 0x333333FF, 255);

  UIElement *toolbar = sxui_frame(page, 0, 0, 1000, 60, UI_LAYOUT_HORIZONTAL);
  sxui_label(toolbar, "SELECT ANIMATION:");
  UIElement *dd =
      sxui_dropdown(toolbar, (const char *[]){"Sine Wave", "Atom Particles"}, 2,
                    current_anim);
  sxui_set_size(dd, 200, 40);
  sxui_on_dropdown_changed(dd, on_anim_change);

  return page;
}
