# Pyxis ports development

- Keep the host Lua runner small and explicit. Recipes are trusted build code,
  not a sandbox or a package manager. Never modify the consumed SDK.
- Pin upstream sources to exact commits and preserve licenses. Record local
  changes as ordered patches; preserve upstream formatting and architecture.
- Use the Pyxis compiler and SDK for target programs. Never link host libc.
  First-party C uses GNU C23, snake_case, two spaces and K&R control braces.
- Validate with ordinary builds and manual QEMU/debugger work through Pyxis.
  Do not add tests, self-tests, CI or boot/output automation unless requested.
- Ports are leaf tools, not Pyxis's userland. Follow the ports and native
  commands boundary in the parent repository's docs/development/ports.md: never
  port a command that duplicates a native one or defines a Pyxis concept, add
  each BusyBox applet or sbase tool separately, and include a check that scheme
  paths, given grants and explicit failure still work.
- Discuss newly discovered runtime requirements before expanding a port's scope.
  Do not add compatibility paths or version bumps without an explicit need.
- Make focused commits. Keep README practical and put each port's adaptation
  notes and remaining limitations beside its recipe.
