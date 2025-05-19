SRC_DIR := src
OUT_DIR := out
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OUT_DIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst $(SRC_DIR)/%.cpp,$(OUT_DIR)/%.d,$(SOURCES))

APPIND_LIB_COMMON_NAME := appind_lib.so

.PHONY: all clean

all: volume_controller

volume_controller: $(OBJECTS)
	g++ $^ -o $(OUT_DIR)/$@ -lpulse -lpthread -ludev -ldl


$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp makefile
	g++ -MMD -MP -c $< -o $@



ayatana-appind_lib:
	g++ -shared -fPIC -o $(OUT_DIR)/$(APPIND_LIB_COMMON_NAME) $(SRC_DIR)/$@/$@.cpp `pkg-config --cflags --libs ayatana-appindicator3-0.1`

clean:
	rm $(OBJECTS) $(DEPENDS) $(OUT_DIR)/volume_controller || true
	rm $(OUT_DIR)/$(APPIND_LIB_COMMON_NAME) || true
	rmdir $(OUT_DIR) || true

-include $(DEPENDS)

$(shell mkdir -p $(OUT_DIR))