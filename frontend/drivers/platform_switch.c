#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <boolean.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

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

#ifndef IS_SALAMANDER
#ifdef HAVE_MENU
#include "../../menu/menu_driver.h"
#endif
#endif

static enum frontend_fork switch_fork_mode = FRONTEND_FORK_NONE;
static const char *elf_path_cst = "/retroarch/retroarch_switch.nro";

static uint64_t frontend_switch_get_mem_used(void);

static void get_first_valid_core(char *path_return)
{
    *path_return = '\0';
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

    fill_pathname_basedir(g_defaults.dirs[DEFAULT_DIR_PORT], elf_path_cst, sizeof(g_defaults.dirs[DEFAULT_DIR_PORT]));
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

#ifndef IS_SALAMANDER
    verbosity_enable();

    gfxExit();
#endif
}

static void frontend_switch_exec(const char *path, bool should_load_game)
{
    // TODO
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
#if defined(SWITCH) && defined(NXLINK)
    socketExit();
#endif
}

static void frontend_switch_init(void *data)
{
#ifndef IS_SALAMANDER // Do we want that here?
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
    verbosity_enable();
#endif

    printf("[Video]: Video initialized\n");

#endif
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
