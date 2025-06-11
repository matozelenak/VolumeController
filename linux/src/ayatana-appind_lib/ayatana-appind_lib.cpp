#include "../appind_lib.h" // common library header
#include "ayatana-appind_lib.h"

#include <libayatana-appindicator/app-indicator.h>
#include <gtk/gtk.h>
#include <iostream>
#include <string>
using namespace std;

// string appind::_appName;
// string appind::_icon;
// AppIndicator *appind::_indicator;
// GtkMenu *appind::_menu;


void libraryInit(int argc, char *argv[], string appName, string icon) {
    cout << "Ayatana-AppInd library init.\n";
    appind::_appName = appName;
    appind::_icon = icon;
    appind::_cb = nullptr;
    appind::_cbData = nullptr;

    gtk_init(&argc, &argv);
    appind::_indicator = app_indicator_new(appName.c_str(), icon.c_str(),
        APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(appind::_indicator, APP_INDICATOR_STATUS_PASSIVE);
    appind::_menu = GTK_MENU(gtk_menu_new());

    pthread_create(&appind::_thread, NULL, [](void *data){
        gtk_main();
        return (void*) NULL;
    }, NULL);
}

void setIcon(std::string icon) {
    appind::_icon = icon;
    app_indicator_set_icon(appind::_indicator, icon.c_str());
}

void addMenuItem(std::string label, std::string icon) {
    GtkWidget *item = gtk_menu_item_new_with_label(label.c_str());
    gtk_menu_shell_append(GTK_MENU_SHELL(appind::_menu), item);
    g_signal_connect(item, "activate", G_CALLBACK(appind::item_clicked), NULL);
}
void addMenuSeparator() {
    GtkWidget *item = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(appind::_menu), item);
}

void showAppIndicator() {
    app_indicator_set_status(appind::_indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_menu(appind::_indicator, GTK_MENU(appind::_menu));
    gtk_widget_show_all(GTK_WIDGET(appind::_menu));
}

void hideAppIndicator() {
    app_indicator_set_status(appind::_indicator, APP_INDICATOR_STATUS_PASSIVE);
}

void destroy() {
    gtk_main_quit();
    pthread_join(appind::_thread, NULL);
    cout << "Ayatana-AppInd library destroy.\n";
}


void setClickCallback(click_cb cb, void *data) {
    appind::_cb = cb;
    appind::_cbData = data;
}

void appind::item_clicked(GtkMenuItem *item, gpointer data) {
    string label = gtk_menu_item_get_label(item);
    if (_cb) _cb(label, _cbData);
}