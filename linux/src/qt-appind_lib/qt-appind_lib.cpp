#include "../appind_lib.h"

#include <QtWidgets/QApplication>
#include <QtWidgets/QSystemTrayIcon>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtGui/QIcon>

#include <pthread.h>
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
using namespace std;

struct GlobalData {
    QApplication *app;
    QSystemTrayIcon *tray;
    QMenu *menu;
    vector<QAction*> menuActions;
    click_cb cb;
    void *cbData;
    pthread_t thread;
    int argc;
    char **argv;
    bool running = false;
};

GlobalData _data;

void libraryInit(int argc, char *argv[], string appName, string icon) {
    cout << "Qt-AppInd library init.\n";
    _data.argc = argc;
    _data.argv = argv;
    
    pthread_create(&_data.thread, NULL, [](void *data){
        GlobalData *gd = static_cast<GlobalData*>(data);

        QApplication app(gd->argc, gd->argv);
        QSystemTrayIcon tray;
        QMenu menu;
        tray.setContextMenu(&menu);
        gd->app = &app;
        gd->tray = &tray;
        gd->menu = &menu;
        gd->running = true;
        app.exec();

        cout << "Qt-AppInd lib cleanup.\n";
        for (QAction *action : gd->menuActions) delete action;
        gd->menuActions.clear();
        
        return (void*) NULL;
    }, &_data);

    while(!_data.running) usleep(100000);
}

void setIcon(string icon) {
    QMetaObject::invokeMethod(_data.app, [icon]() {
        _data.tray->setIcon(QIcon::fromTheme(icon.c_str()));
    }, Qt::QueuedConnection);
}

void addMenuItem(string label, string icon) {
    QMetaObject::invokeMethod(_data.app, [label, icon]() {
        QAction *action = new QAction(QIcon::fromTheme(icon.c_str()), label.c_str(), _data.menu);
        _data.menuActions.push_back(action);
        QObject::connect(action, &QAction::triggered, [label] {
            if (_data.cb) _data.cb(label, _data.cbData);
        });
        _data.menu->addAction(action);
    }, Qt::QueuedConnection);
}

void addMenuSeparator() {
    QMetaObject::invokeMethod(_data.app, []() {
        _data.menu->addSeparator();
    }, Qt::QueuedConnection);
}

void showAppIndicator() {
    QMetaObject::invokeMethod(_data.app, []() {
        _data.tray->show();
    }, Qt::QueuedConnection);
}

void hideAppIndicator() {
    QMetaObject::invokeMethod(_data.app, []() {
        _data.tray->hide();
    }, Qt::QueuedConnection);
}

void destroy() {
    QMetaObject::invokeMethod(_data.app, []() {
        _data.app->quit();
    }, Qt::QueuedConnection);
    
    pthread_join(_data.thread, NULL);
    cout << "Qt-AppInd library destroy.\n";
}

void setClickCallback(click_cb cb, void *data) {
    _data.cb = cb;
    _data.cbData = data;
}