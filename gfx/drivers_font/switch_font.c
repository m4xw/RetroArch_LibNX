#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <encodings/utf.h>

#include <retro_math.h>

#include "../font_driver.h"
#include "../video_driver.h"

#include "../../verbosity.h"

#include "../common/switch_common.h"

typedef struct
{
      struct font_atlas *atlas;

      const font_renderer_driver_t *font_driver;
      void *font_data;
} switch_font_t;

static void *switch_font_init_font(void *data, const char *font_path,
                                   float font_size, bool is_threaded)
{
      switch_font_t *font = (switch_font_t *)calloc(1, sizeof(*font));

      if (!font)
            return NULL;

      font_size = 10;
      if (!font_renderer_create_default((const void **)&font->font_driver,
                                        &font->font_data, font_path, font_size))
      {
            RARCH_WARN("Couldn't initialize font renderer.\n");
            free(font);
            return NULL;
      }

      font->atlas = font->font_driver->get_atlas(font->font_data);

      RARCH_LOG("Switch font driver initialized with backend %s\n", font->font_driver->ident);

      return font;
}

static void switch_font_free_font(void *data, bool is_threaded)
{
      switch_font_t *font = (switch_font_t *)data;

      if (!font)
            return;

      if (font->font_driver && font->font_data)
            font->font_driver->free(font->font_data);

      free(font);
}

static int switch_font_get_message_width(void *data, const char *msg,
                                         unsigned msg_len, float scale)
{
      switch_font_t *font = (switch_font_t *)data;

      unsigned i;
      int delta_x = 0;

      if (!font)
            return 0;

      for (i = 0; i < msg_len; i++)
      {
            const char *msg_tmp = &msg[i];
            unsigned code = utf8_walk(&msg_tmp);
            unsigned skip = msg_tmp - &msg[i];

            if (skip > 1)
                  i += skip - 1;

            const struct font_glyph *glyph =
                font->font_driver->get_glyph(font->font_data, code);

            if (!glyph) /* Do something smarter here ... */
                  glyph = font->font_driver->get_glyph(font->font_data, '?');

            if (!glyph)
                  continue;

            delta_x += glyph->advance_x;
      }

      return delta_x * scale;
}

#define FONT_SCALE 2

static void switch_font_render_line(
    video_frame_info_t *video_info,
    switch_font_t *font, const char *msg, unsigned msg_len,
    float scale, const unsigned int color, float pos_x,
    float pos_y, unsigned text_align)
{
      int delta_x = 0;
      int delta_y = 0;

      unsigned fbWidth = 0;
      unsigned fbHeight = 0;

      uint32_t *out_buffer = (uint32_t *)gfxGetFramebuffer(&fbWidth, &fbHeight);
      if (out_buffer)
      {
            int x = roundf(pos_x * fbWidth);
            int y = roundf((1.0f - pos_y) * fbHeight);

            switch (text_align)
            {
            case TEXT_ALIGN_RIGHT:
                  x -= switch_font_get_message_width(font, msg, msg_len, scale);
                  break;
            case TEXT_ALIGN_CENTER:
                  x -= switch_font_get_message_width(font, msg, msg_len, scale) / 2.0;
                  break;
            }

            for (int i = 0; i < msg_len; i++)
            {
                  int off_x, off_y, tex_x, tex_y, width, height;
                  const char *msg_tmp = &msg[i];
                  unsigned code = utf8_walk(&msg_tmp);
                  unsigned skip = msg_tmp - &msg[i];

                  if (skip > 1)
                        i += skip - 1;

                  const struct font_glyph *glyph =
                      font->font_driver->get_glyph(font->font_data, code);

                  if (!glyph) /* Do something smarter here ... */
                        glyph = font->font_driver->get_glyph(font->font_data, '?');

                  if (!glyph)
                        continue;

                  off_x = glyph->draw_offset_x * FONT_SCALE;
                  off_y = glyph->draw_offset_y * FONT_SCALE;
                  width = glyph->width;
                  height = glyph->height;

                  tex_x = glyph->atlas_offset_x;
                  tex_y = glyph->atlas_offset_y;

                  uint32_t *glyph_buffer = malloc((width * FONT_SCALE) * (height * FONT_SCALE) * sizeof(uint32_t)); //TODO Replace this by a large-enough buffer depending on FONT_SCALE

                  for (int y = tex_y; y < tex_y + height; y++)
                  {
                        uint8_t *row = &font->atlas->buffer[y * font->atlas->width];
                        for (int x = tex_x; x < tex_x + width; x++)
                        {
                              uint8_t alpha = row[x];
                              uint32_t pixel = RGBA8(0, 255, 0, alpha);
                              for (int i = 0; i < FONT_SCALE; i++)
                              {
                                    for (int j = 0; j < FONT_SCALE; j++)
                                    {
                                          int px = (x - tex_x) * FONT_SCALE + j;
                                          int py = (y - tex_y) * FONT_SCALE + i;
                                          glyph_buffer[py * (width * FONT_SCALE) + px] = pixel;
                                    }
                              }
                        }
                  }

                  int glyphx = x + off_x + delta_x * FONT_SCALE + FONT_SCALE * 2;
                  int glyphy = y + off_y + delta_y * FONT_SCALE - FONT_SCALE * 2;

                  // Gonna try to catch it prior
#if 1
                  bool x_ok = true, y_ok = true;
                  if ((glyphx + width * FONT_SCALE) > 1280)
                  {
                        x_ok = false;
                        printf("glpyhx %i (x: %i, off_x: %i, delta_x: %i), violated calc: %i, width: %i\n", glyphx, x, off_x, delta_x, (glyphx + width * FONT_SCALE), width);
                  }
                  if ((glyphy + height * FONT_SCALE) > 720)
                  {
                        y_ok = false;
                        printf("glyphy %i (y: %i, off_y: %i, delta_y: %i) , violated %i, glyphy: %i, heigth: %i\n", glyphy, y, off_y, delta_y, (glyphy + height * FONT_SCALE), glyphy, height);
                  }
#endif
                  if (x_ok && y_ok)
                  {
                        gfx_slow_swizzling_blit(out_buffer, glyph_buffer, width * FONT_SCALE, height * FONT_SCALE, glyphx, glyphy, true);
                  }

                  free(glyph_buffer);

                  delta_x += glyph->advance_x;
                  delta_y += glyph->advance_y;
            }
      }
}

static void switch_font_render_message(
    video_frame_info_t *video_info,
    switch_font_t *font, const char *msg, float scale,
    const unsigned int color, float pos_x, float pos_y,
    unsigned text_align)
{
      int lines = 0;
      float line_height;

      if (!msg || !*msg)
            return;

      /* If the font height is not supported just draw as usual */
      if (!font->font_driver->get_line_height)
      {
            switch_font_render_line(video_info, font, msg, strlen(msg),
                                    scale, color, pos_x, pos_y, text_align);
            return;
      }
      line_height = scale / font->font_driver->get_line_height(font->font_data);

      for (;;)
      {
            const char *delim = strchr(msg, '\n');

            /* Draw the line */
            if (delim)
            {
                  unsigned msg_len = delim - msg;
                  switch_font_render_line(video_info, font, msg, msg_len,
                                          scale, color, pos_x, pos_y - (float)lines * line_height,
                                          text_align);
                  msg += msg_len + 1;
                  lines++;
            }
            else
            {
                  unsigned msg_len = strlen(msg);
                  switch_font_render_line(video_info, font, msg, msg_len,
                                          scale, color, pos_x, pos_y - (float)lines * line_height,
                                          text_align);
                  break;
            }
      }
}

static void switch_font_render_msg(
    video_frame_info_t *video_info,
    void *data, const char *msg,
    const struct font_params *params)
{
      float x, y, scale, drop_mod, drop_alpha;
      int drop_x, drop_y;
      unsigned max_glyphs;
      enum text_alignment text_align;
      unsigned color, color_dark, r, g, b,
          alpha, r_dark, g_dark, b_dark, alpha_dark;
      switch_font_t *font = (switch_font_t *)data;
      unsigned width = video_info->width;
      unsigned height = video_info->height;

      if (!font || !msg || msg && !*msg)
            return;

      if (params)
      {
            x = params->x;
            y = params->y;
            scale = params->scale;
            text_align = params->text_align;
            drop_x = params->drop_x;
            drop_y = params->drop_y;
            drop_mod = params->drop_mod;
            drop_alpha = params->drop_alpha;

            r = FONT_COLOR_GET_RED(params->color);
            g = FONT_COLOR_GET_GREEN(params->color);
            b = FONT_COLOR_GET_BLUE(params->color);
            alpha = FONT_COLOR_GET_ALPHA(params->color);

            color = params->color;
      }
      else
      {
            x = 0.0f;
            y = 0.0f;
            scale = 1.0f;
            text_align = TEXT_ALIGN_LEFT;

            r = (video_info->font_msg_color_r * 255);
            g = (video_info->font_msg_color_g * 255);
            b = (video_info->font_msg_color_b * 255);
            alpha = 255;
            color = COLOR_ABGR(r, g, b, alpha);

            drop_x = -2;
            drop_y = -2;
            drop_mod = 0.3f;
            drop_alpha = 1.0f;
      }

      max_glyphs = strlen(msg);
      // Garbage data on threading :shrug:
      if (max_glyphs > 140)
            return; // This is max length on 5 avg width

      /*if (drop_x || drop_y)
      max_glyphs    *= 2;

   if (drop_x || drop_y)
   {
      r_dark         = r * drop_mod;
      g_dark         = g * drop_mod;
      b_dark         = b * drop_mod;
      alpha_dark     = alpha * drop_alpha;
      color_dark     = COLOR_ABGR(r_dark, g_dark, b_dark, alpha_dark);

      switch_font_render_message(video_info, font, msg, scale, color_dark,
                              x + scale * drop_x / width, y +
                              scale * drop_y / height, text_align);
   }*/

      switch_font_render_message(video_info, font, msg, scale,
                                 color, x, y, text_align);
}

static const struct font_glyph *switch_font_get_glyph(
    void *data, uint32_t code)
{
      switch_font_t *font = (switch_font_t *)data;

      if (!font || !font->font_driver)
            return NULL;

      if (!font->font_driver->ident)
            return NULL;

      return font->font_driver->get_glyph((void *)font->font_driver, code);
}

static void switch_font_bind_block(void *data, void *userdata)
{
      (void)data;
}

font_renderer_t switch_font =
    {
        switch_font_init_font,
        switch_font_free_font,
        switch_font_render_msg,
        "switchfont",
        switch_font_get_glyph,
        switch_font_bind_block,
        NULL, /* flush_block */
        switch_font_get_message_width,
};
