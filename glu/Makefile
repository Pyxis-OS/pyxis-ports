include $(SDK)/share/pyxis.mk

S := $(SOURCE)/src

# Upstream's meson.build lists these directories whole; priorityq-heap.c is
# included by priorityq.c, not compiled on its own.
C_SOURCES := $(filter-out $(S)/libtess/priorityq-heap.c, \
  $(wildcard $(S)/libutil/*.c) $(wildcard $(S)/libtess/*.c))
CXX_SOURCES := $(wildcard $(S)/libnurbs/interface/*.cc) \
  $(wildcard $(S)/libnurbs/internals/*.cc) $(wildcard $(S)/libnurbs/nurbtess/*.cc)
OBJECTS := $(patsubst $(S)/%.c,$(BUILD)/%.o,$(C_SOURCES)) \
  $(patsubst $(S)/%.cc,$(BUILD)/%.o,$(CXX_SOURCES))

INCLUDES := -DLIBRARYBUILD -I$(SOURCE)/include -I$(S)/include \
  -I$(S)/libnurbs/internals -I$(S)/libnurbs/interface -I$(S)/libnurbs/nurbtess \
  -I$(MESA)/include
# Upstream builds with the compiler's default language standards, -Wall and
# NDEBUG in release builds; its warnings stay visible.
C_FLAGS := $(filter-out -std=gnu23 -Wextra,$(PYXIS_CFLAGS)) -DNDEBUG
CXX_FLAGS := $(filter-out -std=gnu++23 -Wextra,$(PYXIS_CXXFLAGS)) -DNDEBUG

.PHONY: all
all: $(BUILD)/libGLU.a

$(BUILD)/%.o: $(S)/%.c
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(INCLUDES) $(C_FLAGS) -c $< -o $@

$(BUILD)/%.o: $(S)/%.cc
	@mkdir -p $(@D)
	$(CXX) $(PYXIS_CXX_CPPFLAGS) $(INCLUDES) $(CXX_FLAGS) -c $< -o $@

$(BUILD)/libGLU.a: $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)

-include $(OBJECTS:.o=.d)
