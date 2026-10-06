#include <obs-module.h>
#include <obs-frontend-api.h>
#include <QWidget>
#include "yarkwan-source.hpp"
#include "yarkwan-dock.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("yarkwan-j-a-scripture-lyrics", "en-US")

static YarkwanDock *dock = nullptr;

bool obs_module_load(void)
{
    obs_register_source(&yarkwan_scripture_source_info);
    dock = new YarkwanDock();
    if (!obs_frontend_add_dock_by_id("yarkwan_j_a_scripture_lyrics", "YARKWAN J. A. Scripture & Lyrics", dock)) {
        delete dock; dock = nullptr;
        blog(LOG_WARNING, "YARKWAN J. A.: unable to create OBS dock");
    }
    blog(LOG_INFO, "YARKWAN J. A. Scripture & Lyrics loaded");
    return true;
}

void obs_module_unload(void)
{
    if (dock) {
        obs_frontend_remove_dock("yarkwan_j_a_scripture_lyrics");
        delete dock;
        dock = nullptr;
    }
    blog(LOG_INFO, "YARKWAN J. A. Scripture & Lyrics unloaded");
}
