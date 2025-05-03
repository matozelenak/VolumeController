#pragma once
#include "../appind_lib.h"

#include <libayatana-appindicator/app-indicator.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include <string>

namespace appind {
    std::string _appName;
    std::string _icon;

    AppIndicator *_indicator;
    GtkMenu *_menu;
    pthread_t _thread;
    click_cb _cb;
    void *_cbData;

    void item_clicked(GtkMenuItem *item, gpointer data);
}