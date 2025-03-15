OUT_DIR := ../out
SOURCES := $(wildcard *.cpp)
OBJECTS := $(patsubst %.cpp,$(OUT_DIR)/%.o,$(SOURCES))
DEPENDS := $(patsubst %.cpp,$(OUT_DIR)/%.d,$(SOURCES))


.PHONY: all clean


volume_controller: $(OBJECTS)
	g++ $^ -o $(OUT_DIR)/$@


$(OUT_DIR)/%.o: %.cpp makefile
	g++ -MMD -MP -c $< -o $@


all: volume_controller

clean:
	rm $(OBJECTS) $(DEPENDS) $(OUT_DIR)/volume_controller

-include $(DEPENDS)
