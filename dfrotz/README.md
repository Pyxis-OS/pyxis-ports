# dfrotz

Frotz 2.55's unchanged dumb terminal frontend, with Blorb support and no sound.
The upstream makefiles use the selected Pyxis SDK and pinned release identity;
no host Git checkout is inspected. The SDK supplies `sys/param.h` MIN/MAX.

Run `dfrotz home://story.z5` with a user-supplied Z-machine story. Plain terminal
input/output works through libc; no ncurses or display grant is needed. Story,
save and transcript paths use the caller's actual filesystem grants. No story
files are packaged. Running `dfrotz` without a story prints upstream options;
`-h` sets screen height.

The dumb frontend retains upstream scrolling, formatting and manually advanced
timed-input behavior; it provides no graphical or audio frontend. Copyright,
GPL terms, original source and the dfrotz manual are in the bundle's metadata.
Upstream's test-story directory is excluded from that source archive.
