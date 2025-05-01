SRC_DIR := src
OUT_DIR := out
SOURCES := $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS := $(patsubst $(SRC_DIR)/%.cpp,$(OUT_DIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst $(SRC_DIR)/%.cpp,$(OUT_DIR)/%.d,$(SOURCES))


.PHONY: all clean

all: volume_controller

volume_controller: $(OBJECTS)
	g++ $^ -o $(OUT_DIR)/$@ -lpulse -lpthread


$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp makefile
	g++ -MMD -MP -c $< -o $@


clean:
	rm $(OBJECTS) $(DEPENDS) $(OUT_DIR)/volume_controller


-include $(DEPENDS)

$(shell mkdir -p $(OUT_DIR))