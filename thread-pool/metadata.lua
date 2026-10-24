return {
  source = {
    url = "https://github.com/bshoshany/thread-pool.git",
    commit = "097aa718f25d44315cadb80b407144ad455ee4f9",
    archive = {
      url = "https://github.com/bshoshany/thread-pool/archive/097aa718f25d44315cadb80b407144ad455ee4f9.tar.gz",
      mirror = "https://repo.internal/repository/raw-github/bshoshany/thread-pool/archive/097aa718f25d44315cadb80b407144ad455ee4f9.tar.gz",
      sha256 = "8e54280b4d9af3e4d2fd3dfa3f8d1198fc1c3a572d6f22c5eaf1abe38f2db0a8",
    },
  },
  version = "4.1.0",
  license = "MIT",
  dependencies = {
    host = { "curl", "sha256sum", "tar", "gzip", "install", "cp" },
    pyxis = { "libc", "libc++" },
  },
  patches = { "patches/0001-keep-completion-state-under-task-mutex.patch" },
  outputs = {
    header = "dev/include/BS_thread_pool.hpp",
    utility_header = "dev/include/BS_thread_pool_utils.hpp",
    pc_thread_pool = "dev/lib/pkgconfig/thread-pool.pc",
    license = "dev/share/licenses/thread-pool/LICENSE.txt",
    notice = "dev/share/licenses/thread-pool/PORT-NOTICE",
    pyxis_license = "dev/share/licenses/thread-pool/MPL-2.0",
    licensing = "dev/share/licenses/thread-pool/LICENSING.md",
    provenance = "dev/share/thread-pool/source.txt",
    recipe = "dev/share/thread-pool/pyxis/build.lua",
    recipe_metadata = "dev/share/thread-pool/pyxis/metadata.lua",
    patch = "dev/share/thread-pool/pyxis/patches/0001-keep-completion-state-under-task-mutex.patch",
  },
}
