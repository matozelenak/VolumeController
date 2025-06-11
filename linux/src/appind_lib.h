#pragma once
#include <string>

typedef void (*click_cb)(std::string, void*);

const char* LIBINIT_NAME = "libraryInit";
const char* SETICON_NAME = "setIcon";
const char* ADDMENUITEM_NAME = "addMenuItem";
const char* ADDMENUSEPARATOR_NAME = "addMenuSeparator";
const char* SHOWAPPIND_NAME = "showAppIndicator";
const char* HIDEAPPIND_NAME = "hideAppIndicator";
const char* DESTROY_NAME = "destroy";
const char* SETCLICKCB_NAME = "setClickCallback";

typedef void (*libinit_t)(int, char*[], std::string, std::string);
typedef void (*seticon_t)(std::string);
typedef void (*addmenuitem_t)(std::string, std::string);
typedef void (*addmenusep_t)();
typedef void (*showappind_t)();
typedef void (*hideappind_t)();
typedef void (*destroy_t)();
typedef void (*setclickcb_t)(click_cb, void*);


extern "C" {
    void libraryInit(int argc, char *argv[], std::string appName, std::string icon);
    void setIcon(std::string icon);
    void addMenuItem(std::string label, std::string icon);
    void addMenuSeparator();
    void showAppIndicator();
    void hideAppIndicator();
    void destroy();
    void setClickCallback(click_cb cb, void *data);
}
