include $(SDK)/share/pyxis.mk

DOOM_SOURCE := $(SOURCE)/doomgeneric
CPPFLAGS := $(PYXIS_CPPFLAGS) -I$(DOOM_SOURCE) -I$(RECIPE) -DDOOMGENERIC_RESX=320 -DDOOMGENERIC_RESY=200
CFLAGS := $(PYXIS_CFLAGS) -ffunction-sections -fdata-sections
OBJECTS := $(addprefix $(BUILD)/,dummy.o am_map.o doomdef.o doomstat.o dstrings.o d_event.o d_items.o d_iwad.o d_loop.o d_main.o d_mode.o d_net.o f_finale.o f_wipe.o g_game.o hu_lib.o hu_stuff.o info.o i_cdmus.o i_endoom.o i_joystick.o i_scale.o i_sound.o i_system.o i_timer.o memio.o m_argv.o m_bbox.o m_cheat.o m_config.o m_controls.o m_fixed.o m_menu.o m_misc.o m_random.o p_ceilng.o p_doors.o p_enemy.o p_floor.o p_inter.o p_lights.o p_map.o p_maputl.o p_mobj.o p_plats.o p_pspr.o p_saveg.o p_setup.o p_sight.o p_spec.o p_switch.o p_telept.o p_tick.o p_user.o r_bsp.o r_data.o r_draw.o r_main.o r_plane.o r_segs.o r_sky.o r_things.o sha1.o sounds.o statdump.o st_lib.o st_stuff.o s_sound.o tables.o v_video.o wi_stuff.o w_checksum.o w_file.o w_main.o w_wad.o z_zone.o w_file_stdc.o i_input.o i_video.o doomgeneric.o doomgeneric_pyxis.o doomgeneric_save.o)

.PHONY: all
all: $(BUILD)/doom.pxe

# Upstream uses pre-C23 boolean definitions and old-style function declarations.
$(BUILD)/%.o: $(DOOM_SOURCE)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -std=gnu11 -c $< -o $@

$(BUILD)/doomgeneric_%.o: $(RECIPE)/doomgeneric_%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(BUILD)/doom.elf: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -Wl,--gc-sections -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

$(BUILD)/doom.pxe: $(BUILD)/doom.elf
	$(PYXIS_ELF2PXE) --format p1f -o $@ $<

-include $(OBJECTS:.o=.d)
