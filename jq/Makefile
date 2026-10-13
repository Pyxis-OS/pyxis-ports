include $(SDK)/share/pyxis.mk

# jq's configure script only emits these as -D options (it has no config.h).
# Each HAVE_ name is defined only when Pyxis libc declares and links the
# function, so a builtin that needs a missing one is absent, not faked.
# Math functions present in libc; jq defines a builtin for each. The non-ISO
# ones (j0 to yn, gamma, drem, significand, scalb, exp10, pow10, lgamma_r) are
# absent, so those builtins are undefined.
JQ_MATH := ACOS ACOSH ASIN ASINH ATAN ATAN2 ATANH CBRT CEIL COPYSIGN COS COSH \
  ERF ERFC EXP EXP2 EXPM1 FABS FDIM FLOOR FMA FMAX FMIN FMOD FREXP HYPOT ILOGB \
  LDEXP LGAMMA LOG LOG10 LOG1P LOG2 LOGB MODF NEARBYINT NEXTAFTER NEXTTOWARD \
  POW REMAINDER RINT ROUND SCALBLN SCALBN SIN SINH SQRT TAN TANH TGAMMA TRUNC

JQ_DEFINES := -DUSE_DECNUM -DIEEE_8087 -DHAVE___THREAD \
  -DHAVE_PTHREAD -DHAVE_PTHREAD_KEY_CREATE -DHAVE_PTHREAD_ONCE \
  -DHAVE_ATEXIT -DHAVE_SETENV -DHAVE_STRPTIME -DHAVE_SETLOCALE -DHAVE_STRFTIME -DHAVE_ISATTY \
  -DHAVE_GMTIME_R -DHAVE_LOCALTIME_R -DHAVE_TM_TM_GMT_OFF \
  $(addprefix -DHAVE_,$(JQ_MATH))

# jq declares function pointers of several types as `jv (*)()`, which C23
# reads as `(void)`; upstream builds as C17.
JQ_CFLAGS := -std=gnu17 -DPYXIS -Wno-unused-parameter -Wno-unused-function

LIBJQ_SOURCES := builtin bytecode compile execute jq_test jv jv_alloc jv_aux \
  jv_dtoa jv_dtoa_tsd jv_file jv_parse jv_print jv_unicode linker locfile \
  util lexer parser
DECNUMBER_SOURCES := decContext decNumber
OBJECTS := $(addprefix $(BUILD)/src/,$(addsuffix .o,main $(LIBJQ_SOURCES))) \
  $(addprefix $(BUILD)/src/decNumber/,$(addsuffix .o,$(DECNUMBER_SOURCES)))

.PHONY: all
all: $(BUILD)/jq.pxe

# builtin.jq becomes a C string literal, as in upstream's Makefile.am.
$(BUILD)/src/builtin.inc: $(SOURCE)/src/builtin.jq
	@mkdir -p $(@D)
	sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/"/' -e 's/$$/\\n"/' $< > $@

# jq --build-configuration prints this instead of configure's arguments.
$(BUILD)/src/config_opts.inc:
	@mkdir -p $(@D)
	printf '#define JQ_CONFIG "pyxis static --without-oniguruma"\n' > $@

$(BUILD)/src/builtin.o: $(BUILD)/src/builtin.inc
$(BUILD)/src/main.o: $(BUILD)/src/config_opts.inc

$(BUILD)/src/%.o: $(SOURCE)/src/%.c
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE) -I$(SOURCE)/src -I$(BUILD) \
	  $(JQ_DEFINES) $(JQ_CFLAGS) -c $< -o $@

$(BUILD)/src/decNumber/%.o: $(SOURCE)/src/decNumber/%.c
	@mkdir -p $(@D)
	$(CC) $(PYXIS_CPPFLAGS) $(PYXIS_CFLAGS) -I$(SOURCE) -I$(SOURCE)/src -I$(BUILD) \
	  $(JQ_DEFINES) $(JQ_CFLAGS) -c $< -o $@

$(BUILD)/jq.pxe: $(OBJECTS) $(PYXIS_START) $(PYXIS_LIBRARIES) $(PYXIS_LINKER_SCRIPT)
	$(CC) $(PYXIS_LDFLAGS) -o $@ $(PYXIS_START) $(OBJECTS) $(PYXIS_LDLIBS)

-include $(OBJECTS:.o=.d)
