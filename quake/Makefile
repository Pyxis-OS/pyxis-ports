include $(SDK)/share/pyxis.mk

QUAKE_SOURCE := $(SOURCE)/source
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(QUAKE_SOURCE) -I$(RECIPE)
CFLAGS := $(PYXIS_CFLAGS) -ffunction-sections -fdata-sections
# Upstream's object list without its null system layer and generic entry point,
# which the Pyxis adapter replaces.
ENGINE := cd_null chase cl_demo cl_input cl_main cl_parse cl_tent cmd common console \
          crc cvar d_edge d_fill d_init d_modech d_part d_polyse d_scan d_sky d_sprite \
          d_surf d_vars d_zpoint draw host_cmd host in_null keys mathlib menu model \
          net_loop net_main net_none net_vcr nonintel pr_cmds pr_edict pr_exec r_aclip \
          r_alias r_bsp r_draw r_edge r_efrag r_light r_main r_misc r_part r_sky \
          r_sprite r_surf r_vars sbar screen snd_null sv_main sv_move sv_phys sv_user \
          vid_null view wad world zone
OBJECTS := $(addprefix $(BUILD)/,$(addsuffix .o,$(ENGINE)) sys_pyxis.o quake_pyxis.o)

.PHONY: all
all: $(BUILD)/quake.pxe

# Quake's headers define their own false/true enum, which C23 rejects.
$(BUILD)/%.o: $(QUAKE_SOURCE)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=gnu99 -c $< -o $@

$(BUILD)/sys_pyxis.o: $(RECIPE)/sys_pyxis.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=gnu99 -c $< -o $@

$(BUILD)/quake_pyxis.o: $(RECIPE)/quake_pyxis.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/quake.elf: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -Wl,--gc-sections -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/quake.pxe: $(BUILD)/quake.elf
	$(PYXIS_ELF2PXE) --format p1f -o $@ $<

-include $(OBJECTS:.o=.d)
