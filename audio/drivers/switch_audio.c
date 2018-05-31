/*  RetroArch - A frontend for libretro.
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdint.h>

#include <switch.h>

#include "../audio_driver.h"
#include "../../verbosity.h"

#define ALIGN_VAL(x, a) (((x) + ((a)-1)) & ~((a)-1))

typedef struct
{
      bool blocking;
      bool is_paused;
      uint64_t last_append;
      unsigned latency;
      AudioOutBuffer buffers[3];
      AudioOutBuffer *current_buffer;
} switch_audio_t;

static size_t switch_audio_buffer_size(void *data)
{
      (void)data;
      size_t sample_buffer_size = ((audoutGetSampleRate() * audoutGetChannelCount() * sizeof(uint16_t)) + 0xfff) & ~0xfff;
      return ALIGN_VAL(sample_buffer_size, 0x1000);
}

// TODO: Needs testing
static ssize_t switch_audio_write(void *data, const void *buf, size_t size)
{
      return size;
      size_t to_write = size;
      switch_audio_t *swa = (switch_audio_t *)data;

      if (!swa->current_buffer)
      {
            uint32_t num;
            if (R_FAILED(audoutGetReleasedAudioOutBuffer(&swa->current_buffer, &num)))
            {
                  RARCH_LOG("Failed to get released buffer?\n");
                  return -1;
            }

            if (num < 1)
            {
                  swa->current_buffer = NULL;

                  if (swa->blocking)
                  {
                        RARCH_LOG("No buffer, blocking...\n");

                        while (swa->current_buffer == NULL)
                        {
                              // TODO: Fix Blocking
                              return -1;
                        }
                  }
                  else
                  {
                        return 0;
                  }
            }

            swa->current_buffer->data_size = 0;
      }

      if (to_write > switch_audio_buffer_size(NULL) - swa->current_buffer->data_size)
            to_write = switch_audio_buffer_size(NULL) - swa->current_buffer->data_size;

      memcpy(((uint8_t *)swa->current_buffer->buffer) + swa->current_buffer->data_size, buf, to_write);
      swa->current_buffer->data_size += to_write;
      swa->current_buffer->buffer_size = switch_audio_buffer_size(NULL);

      if (swa->current_buffer->data_size > (48000 * swa->latency) / 1000)
      {
            Result r = audoutAppendAudioOutBuffer(swa->current_buffer);
            if (R_FAILED(r))
            {
                  printf("failed to append buffer: 0x%x\n", r);
                  return -1;
            }
            swa->current_buffer = NULL;
      }

      swa->last_append = svcGetSystemTick();

      return to_write;
}

static bool switch_audio_stop(void *data)
{
      return true;
      switch_audio_t *swa = (switch_audio_t *)data;
      if (!swa)
            return false;

      if (!swa->is_paused)
      {
            Result rc = audoutStopAudioOut();
            if (R_FAILED(rc))
            {
                  return false;
            }
      }

      swa->is_paused = true;
      return true;
}

static bool switch_audio_start(void *data, bool is_shutdown)
{
      return true;
      switch_audio_t *swa = (switch_audio_t *)data;

      if (swa->is_paused)
      {
            Result rc = audoutStartAudioOut();
            if (R_FAILED(rc))
            {
                  return false;
            }
      }

      swa->is_paused = false;
      return true;
}

static bool switch_audio_alive(void *data)
{
      return true;
      switch_audio_t *swa = (switch_audio_t *)data;
      if (!swa)
            return false;
      return !swa->is_paused;
}

static void switch_audio_free(void *data)
{
      return;
      switch_audio_t *swa = (switch_audio_t *)data;


      free(swa);
}

static bool switch_audio_use_float(void *data)
{
      (void)data;
      return false; /* force INT16 */
}

static size_t switch_audio_write_avail(void *data)
{
      return 0;
      switch_audio_t *swa = (switch_audio_t *)data;

      if (!swa || !swa->current_buffer)
            return 0;

      return swa->current_buffer->buffer_size;
}

static void switch_audio_set_nonblock_state(void *data, bool state)
{
      return;
      switch_audio_t *swa = (switch_audio_t *)data;

      if (swa)
            swa->blocking = !state;
}

static void *switch_audio_init(const char *device,
                               unsigned rate, unsigned latency,
                               unsigned block_frames,
                               unsigned *new_rate)
{
      return NULL;
      switch_audio_t *swa = (switch_audio_t *)calloc(1, sizeof(*swa));
      printf("Init Audio..\n");
      if (!swa)
            return NULL;

      // Init Audio Output
      Result rc = audoutInitialize();
      if (R_FAILED(rc))
      {
            goto cleanExit;
      }

      rc = audoutStartAudioOut();
      if (R_FAILED(rc))
      {
            goto cleanExit;
      }

      // Set audio rate
      *new_rate = audoutGetSampleRate();

      // Create Buffers
      for (int i = 0; i < 3; i++)
      {
            swa->buffers[i].next = &swa->buffers[i];
            swa->buffers[i].buffer = memalign(0x1000, switch_audio_buffer_size(NULL));
            swa->buffers[i].buffer_size = switch_audio_buffer_size(NULL);
            swa->buffers[i].data_size = switch_audio_buffer_size(NULL);
            swa->buffers[i].data_offset = 0;

            if (swa->buffers[i].buffer == NULL)
            {
                  // TODO: we might memory leak here, but it doesn't really matter
                  goto cleanExit;
            }

            /* 
               This might be the cause of the Audio noise at load
               But since we gonna use audoutGetReleasedAudioOutBuffer we actually need to append them earlier
               Maybe do a memset 0?
            */
            if (R_FAILED(audoutAppendAudioOutBuffer(&swa->buffers[i])))
            {
                  goto cleanExit;
            }
      }

      swa->is_paused = true;
      swa->current_buffer = NULL;
      swa->latency = latency;
      swa->last_append = svcGetSystemTick();

      return swa;

cleanExit:;
      free(swa);
      printf("Something failed in Audio Init!\n");
      
      while(1)
            ;

      return NULL;
}

audio_driver_t audio_switch = {
    switch_audio_init,
    switch_audio_write,
    switch_audio_stop,
    switch_audio_start,
    switch_audio_alive,
    switch_audio_set_nonblock_state,
    switch_audio_free,
    switch_audio_use_float,
    "switch",
    NULL, /* device_list_new */
    NULL, /* device_list_free */
    switch_audio_write_avail,
    switch_audio_buffer_size, /* buffer_size */
};
