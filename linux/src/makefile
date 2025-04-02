OUT_DIR := ../out
SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,$(OUT_DIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,$(OUT_DIR)/%.d,$(SOURCES))


.PHONY: all clean

all: volume_controller

volume_controller: $(OBJECTS)
	g++ $^ -o $(OUT_DIR)/$@ -lpulse -lpthread


$(OUT_DIR)/%.o: %.cpp makefile
	g++ -MMD -MP -c $< -o $@


clean:
	rm $(OBJECTS) $(DEPENDS) $(OUT_DIR)/volume_controller


-include $(DEPENDS)

$(shell mkdir -p $(OUT_DIR))