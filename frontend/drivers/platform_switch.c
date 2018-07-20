#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <boolean.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#include <file/nbio.h>
#include <formats/rpng.h>
#include <formats/image.h>

#include <switch.h>

#include <file/file_path.h>

#ifdef HAVE_CONFIG_H
#include "../../config.h"
#endif

#ifndef IS_SALAMANDER
#include <lists/file_list.h>
#endif

#include "../frontend_driver.h"
#include "../../verbosity.h"
#include "../../defaults.h"
#include "../../paths.h"
#include "../../retroarch.h"
#include "../../file_path_special.h"
#include "../../audio/audio_driver.h"

#ifndef IS_SALAMANDER
#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif
#endif

static enum frontend_fork switch_fork_mode = FRONTEND_FORK_NONE;
static const char *elf_path_cst = "/switch/retroarch_switch.nro";

static uint64_t frontend_switch_get_mem_used(void);

// Splash
static uint32_t *splashData = NULL;

// switch_gfx.c protypes, we really need a header
extern void gfx_slow_swizzling_blit(uint32_t *buffer, uint32_t *image, int w, int h, int tx, int ty, bool blend);

static void get_first_valid_core(char *path_return)
{
    DIR *dir;
    struct dirent *ent;
    const char *extension = ".nro";

    path_return[0] = '\0';

    dir = opendir("/retroarch/cores");
    if (dir != NULL)
    {
        while (ent = readdir(dir))
        {
            if (ent == NULL)
                break;
            if (strlen(ent->d_name) > strlen(extension) && !strcmp(ent->d_name + strlen(ent->d_name) - strlen(extension), extension))
            {
                strcpy(path_return, "/retroarch/cores");
                strcat(path_return, "/");
                strcat(path_return, ent->d_name);
                break;
            }
        }
        closedir(dir);
    }
}

static void frontend_switch_get_environment_settings(int *argc, char *argv[], void *args, void *params_data)
{
    (void)args;

#ifndef IS_SALAMANDER
#if defined(HAVE_LOGGER)
    logger_init();
#elif defined(HAVE_FILE_LOGGER)
    retro_main_log_file_init("/retroarch-log.txt");
#endif
#endif

    fill_pathname_basedir(g_defaults.dirs[DEFAULT_DIR_PORT], "/retroarch/retroarch_switch.nro", sizeof(g_defaults.dirs[DEFAULT_DIR_PORT]));
    RARCH_LOG("port dir: [%s]\n", g_defaults.dirs[DEFAULT_DIR_PORT]);

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE_ASSETS], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "downloads", sizeof(g_defaults.dirs[DEFAULT_DIR_CORE_ASSETS]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_ASSETS], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "media", sizeof(g_defaults.dirs[DEFAULT_DIR_ASSETS]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "cores", sizeof(g_defaults.dirs[DEFAULT_DIR_CORE]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CORE_INFO], g_defaults.dirs[DEFAULT_DIR_CORE],
                       "info", sizeof(g_defaults.dirs[DEFAULT_DIR_CORE_INFO]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SAVESTATE], g_defaults.dirs[DEFAULT_DIR_CORE],
                       "savestates", sizeof(g_defaults.dirs[DEFAULT_DIR_SAVESTATE]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SRAM], g_defaults.dirs[DEFAULT_DIR_CORE],
                       "savefiles", sizeof(g_defaults.dirs[DEFAULT_DIR_SRAM]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_SYSTEM], g_defaults.dirs[DEFAULT_DIR_CORE],
                       "system", sizeof(g_defaults.dirs[DEFAULT_DIR_SYSTEM]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_PLAYLIST], g_defaults.dirs[DEFAULT_DIR_CORE],
                       "playlists", sizeof(g_defaults.dirs[DEFAULT_DIR_PLAYLIST]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "config", sizeof(g_defaults.dirs[DEFAULT_DIR_MENU_CONFIG]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_REMAP], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "config/remaps", sizeof(g_defaults.dirs[DEFAULT_DIR_REMAP]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_VIDEO_FILTER], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "filters", sizeof(g_defaults.dirs[DEFAULT_DIR_REMAP]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_DATABASE], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "database/rdb", sizeof(g_defaults.dirs[DEFAULT_DIR_DATABASE]));

    fill_pathname_join(g_defaults.dirs[DEFAULT_DIR_CURSOR], g_defaults.dirs[DEFAULT_DIR_PORT],
                       "database/cursors", sizeof(g_defaults.dirs[DEFAULT_DIR_CURSOR]));

    fill_pathname_join(g_defaults.path.config, g_defaults.dirs[DEFAULT_DIR_PORT],
                       file_path_str(FILE_PATH_MAIN_CONFIG), sizeof(g_defaults.path.config));
}

static void frontend_switch_deinit(void *data)
{
    (void)data;

#if defined(SWITCH) && defined(NXLINK)
    socketExit();
#endif

    // Splash
    if (splashData)
    {
        free(splashData);
        splashData = NULL;
    }

    gfxExit();
}

static void frontend_switch_exec(const char *path, bool should_load_game)
{
    char game_path[PATH_MAX];
    const char *arg_data[3];
    char error_string[200 + PATH_MAX];
    int args = 0;
    int error = 0;

    game_path[0] = NULL;
    arg_data[0] = NULL;

    arg_data[args] = elf_path_cst;
    arg_data[args + 1] = NULL;
    args++;

    RARCH_LOG("Attempt to load core: [%s].\n", path);
#ifndef IS_SALAMANDER
    if (should_load_game && !path_is_empty(RARCH_PATH_CONTENT))
    {
        strcpy(game_path, path_get(RARCH_PATH_CONTENT));
        arg_data[args] = game_path;
        arg_data[args + 1] = NULL;
        args++;
        RARCH_LOG("content path: [%s].\n", path_get(RARCH_PATH_CONTENT));
    }
#endif

    if (path && path[0])
    {
#ifdef IS_SALAMANDER
        struct stat sbuff;
        bool file_exists;

        file_exists = stat(path, &sbuff) == 0;
        if (!file_exists)
        {
            char core_path[PATH_MAX];

            /* find first valid core and load it if the target core doesnt exist */
            get_first_valid_core(&core_path[0]);

            if (core_path[0] == '\0')
            {
                /*errorInit(&error_dialog, ERROR_TEXT, CFG_LANGUAGE_EN);
                errorText(&error_dialog, "There are no cores installed, install a core to continue.");
                errorDisp(&error_dialog);*/
                svcExitProcess();
            }
        }
#endif
        char *argBuffer = (char *)malloc(PATH_MAX);
        if (should_load_game)
        {
            snprintf(argBuffer, PATH_MAX, "%s \"%s\"", path, game_path);
        }
        else
        {
            snprintf(argBuffer, PATH_MAX, "%s", path);
        }

        envSetNextLoad(path, argBuffer);
    }
}

#ifndef IS_SALAMANDER
static bool frontend_switch_set_fork(enum frontend_fork fork_mode)
{
    switch (fork_mode)
    {
    case FRONTEND_FORK_CORE:
        RARCH_LOG("FRONTEND_FORK_CORE\n");
        switch_fork_mode = fork_mode;
        break;
    case FRONTEND_FORK_CORE_WITH_ARGS:
        RARCH_LOG("FRONTEND_FORK_CORE_WITH_ARGS\n");
        switch_fork_mode = fork_mode;
        break;
    case FRONTEND_FORK_RESTART:
        RARCH_LOG("FRONTEND_FORK_RESTART\n");
        /*  NOTE: We don't implement Salamander, so just turn
             this into FRONTEND_FORK_CORE. */
        switch_fork_mode = FRONTEND_FORK_CORE;
        break;
    case FRONTEND_FORK_NONE:
    default:
        return false;
    }

    return true;
}
#endif

static void frontend_switch_exitspawn(char *s, size_t len)
{
    bool should_load_game = false;
#ifndef IS_SALAMANDER
    if (switch_fork_mode == FRONTEND_FORK_NONE)
        return;

    switch (switch_fork_mode)
    {
    case FRONTEND_FORK_CORE_WITH_ARGS:
        should_load_game = true;
        break;
    default:
        break;
    }
#endif
    frontend_switch_exec(s, should_load_game);
}

static void frontend_switch_shutdown(bool unused)
{
    (void)unused;
}

void argb_to_rgba8(uint32_t *buff, uint32_t height, uint32_t width)
{
    // Convert
    for (uint32_t h = 0; h < height; h++)
    {
        for (uint32_t w = 0; w < width; w++)
        {
            uint32_t offset = (h * width) + w;
            uint32_t c = buff[offset];

            uint32_t a = (uint32_t)((c & 0xff000000) >> 24);
            uint32_t r = (uint32_t)((c & 0x00ff0000) >> 16);
            uint32_t g = (uint32_t)((c & 0x0000ff00) >> 8);
            uint32_t b = (uint32_t)(c & 0x000000ff);

            buff[offset] = RGBA8(r, g, b, a);
        }
    }
}

void frontend_switch_showsplash()
{
    printf("[Splash] Showing splashScreen\n");

    if (splashData)
    {
        uint32_t width, height;
        width = height = 0;

        uint32_t *frambuffer = (uint32_t *)gfxGetFramebuffer(&width, &height);

        gfx_slow_swizzling_blit(frambuffer, splashData, width, height, 0, 0, false);

        gfxFlushBuffers();
        gfxSwapBuffers();
        gfxWaitForVsync();
#if 0
        // :shrug:
        svcSleepThread(10000 * 100000);
#endif
    }
}

// From rpng_test.c
bool rpng_load_image_argb(const char *path, uint32_t **data, unsigned *width, unsigned *height)
{
    int retval;
    size_t file_len;
    bool ret = true;
    rpng_t *rpng = NULL;
    void *ptr = NULL;

    struct nbio_t *handle = (struct nbio_t *)nbio_open(path, NBIO_READ);

    if (!handle)
        goto end;

    nbio_begin_read(handle);

    while (!nbio_iterate(handle))
        svcSleepThread(3);

    ptr = nbio_get_ptr(handle, &file_len);

    if (!ptr)
    {
        ret = false;
        goto end;
    }

    rpng = rpng_alloc();

    if (!rpng)
    {
        ret = false;
        goto end;
    }

    if (!rpng_set_buf_ptr(rpng, (uint8_t *)ptr))
    {
        ret = false;
        goto end;
    }

    if (!rpng_start(rpng))
    {
        ret = false;
        goto end;
    }

    while (rpng_iterate_image(rpng))
        svcSleepThread(3);

    if (!rpng_is_valid(rpng))
    {
        ret = false;
        goto end;
    }

    do
    {
        retval = rpng_process_image(rpng, (void **)data, file_len, width, height);
        svcSleepThread(3);
    } while (retval == IMAGE_PROCESS_NEXT);

    if (retval == IMAGE_PROCESS_ERROR || retval == IMAGE_PROCESS_ERROR_END)
        ret = false;

end:
    if (handle)
        nbio_free(handle);

    if (rpng)
        rpng_free(rpng);

    rpng = NULL;

    if (!ret)
        free(*data);

    return ret;
}

static void frontend_switch_init(void *data)
{
    (void)data;

    // Init Resolution before initDefault
    gfxInitResolution(1280, 720);

    gfxInitDefault();
    gfxSetMode(GfxMode_TiledDouble);

    // Needed, else its flipped and mirrored
    gfxSetDrawFlip(false);
    gfxConfigureTransform(0);

#if defined(SWITCH) && defined(NXLINK)
    socketInitializeDefault();
    nxlinkStdio();
#ifndef IS_SALAMANDER
    verbosity_enable();
#endif
#endif

    printf("[Video]: Video initialized\n");

    // Load splash
    if (!splashData)
    {
        uint32_t width, height;
        width = height = 0;

        // Meh, filesize got too big..
        rpng_load_image_argb("/retroarch/splash_01_720p.png", &splashData, &width, &height);
        if (splashData)
        {
            argb_to_rgba8(splashData, height, width);
            frontend_switch_showsplash();
        }

#if 0
            FILE *fsplash = fopen("romfs:/splash_01_720p.png", "r");
            if (fsplash)
            {
                uint64_t fsize = 0;
                fseek(fsplash, 0L, SEEK_END);
                fsize = ftell(fsplash);
                rewind(fsplash);

                splashData = malloc(fsize);
                if (splashData)
                {
                    // I am lazy
                    fread(splashData, fsize, 1, fsplash);
                    fclose(fsplash);
                    if (splashData)
                    {
                        rpng_t *rpng = NULL;
                        rpng_set_buf_ptr(rpng, splashData);
                        rpng_start(rpng);
                        while (rpng_iterate_image(rpng))
                            ;

                        if (!rpng_is_valid(rpng))
                        {
                            // Invalid image
                            free(splashData);
                            splashData = NULL;
                        }
                        else
                        {
                            int retval;
                            uint32_t *tmpswap = NULL;

                            do
                            {
                                retval = rpng_process_image(rpng, &tmpswap, fsize, 1280, 720);

                                // Yield
                                svcSleepThread(3);
                            } while (retval == IMAGE_PROCESS_NEXT);

                            rpng_free(rpng);
                            if (tmpswap)
                            {
                                free(splashData);
                                splashData = tmpswap;
                            }
                            frontend_switch_showsplash();
                        }
                    }
                }
                else
                {
                    // Uufff, no memory?
                    // What were you doing??
                    printf("??[Out of memory]??\n");
                    fclose(fsplash);
                }
            }
        }
        else
        {
            // For future updates
            frontend_switch_showsplash();
        }
#endif
    }
    else
    {
        frontend_switch_showsplash();
    }
    //}
}

static int frontend_switch_get_rating(void)
{
    // Uhh, guess thats fine?
    return 1337;
}

enum frontend_architecture frontend_switch_get_architecture(void)
{
    return FRONTEND_ARCH_ARMV8;
}

static int frontend_switch_parse_drive_list(void *data, bool load_content)
{
#ifndef IS_SALAMANDER
    file_list_t *list = (file_list_t *)data;
    enum msg_hash_enums enum_idx = load_content ? MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR : MSG_UNKNOWN;

    if (!list)
        return -1;

    menu_entries_append_enum(list, "/", msg_hash_to_str(MENU_ENUM_LABEL_FILE_DETECT_CORE_LIST_PUSH_DIR),
                             enum_idx,
                             FILE_TYPE_DIRECTORY, 0, 0);
#endif

    return 0;
}

static uint64_t frontend_switch_get_mem_total(void)
{
    uint64_t memoryTotal = 0;
    svcGetInfo(&memoryTotal, 6, CUR_PROCESS_HANDLE, 0); // avaiable
    memoryTotal += frontend_switch_get_mem_used();

    return memoryTotal;
}

static uint64_t frontend_switch_get_mem_used(void)
{
    uint64_t memoryUsed = 0;
    svcGetInfo(&memoryUsed, 7, CUR_PROCESS_HANDLE, 0); // used

    return memoryUsed;
}

static enum frontend_powerstate frontend_switch_get_powerstate(int *seconds, int *percent)
{
    // This is fine monkaS
    return FRONTEND_POWERSTATE_CHARGED;
}

static void frontend_switch_get_os(char *s, size_t len, int *major, int *minor)
{
    strlcpy(s, "Horizon OS", len);

    // There is pretty sure a better way, but this will do just fine
    if (kernelAbove500())
    {
        *major = 5;
        *minor = 0;
    }
    else if (kernelAbove400())
    {
        *major = 4;
        *minor = 0;
    }
    else if (kernelAbove300())
    {
        *major = 3;
        *minor = 0;
    }
    else if (kernelAbove200())
    {
        *major = 2;
        *minor = 0;
    }
    else
    {
        // either 1.0 or > 5.x
        *major = 1;
        *minor = 0;
    }
}

static void frontend_switch_get_name(char *s, size_t len)
{
    // TODO: Add Mariko at some point
    strlcpy(s, "Nintendo Switch", len);
}

frontend_ctx_driver_t frontend_ctx_switch =
    {
        frontend_switch_get_environment_settings,
        frontend_switch_init,
        frontend_switch_deinit,
        frontend_switch_exitspawn,
        NULL, /* process_args */
        frontend_switch_exec,
#ifdef IS_SALAMANDER
        NULL,
#else
        frontend_switch_set_fork,
#endif
        frontend_switch_shutdown,
        frontend_switch_get_name,
        frontend_switch_get_os,
        frontend_switch_get_rating,
        NULL, /* load_content */
        frontend_switch_get_architecture,
        frontend_switch_get_powerstate,
        frontend_switch_parse_drive_list,
        frontend_switch_get_mem_total,
        frontend_switch_get_mem_used,
        NULL, /* install_signal_handler */
        NULL, /* get_signal_handler_state */
        NULL, /* set_signal_handler_state */
        NULL, /* destroy_signal_handler_state */
        NULL, /* attach_console */
        NULL, /* detach_console */
        NULL, /* watch_path_for_changes */
        NULL, /* check_for_path_changes */
        "switch",
};
