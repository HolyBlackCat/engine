# --- Build modes ---

_win_subsystem :=

$(call NewMode,debug)
$(Mode)GLOBAL_COMMON_FLAGS := -g
$(Mode)GLOBAL_CXXFLAGS := -D_GLIBCXX_DEBUG

$(call NewMode,debug_soft)
$(Mode)GLOBAL_COMMON_FLAGS := -g
$(Mode)GLOBAL_CXXFLAGS := -D_GLIBCXX_ASSERTIONS

$(call NewMode,release)
$(Mode)GLOBAL_COMMON_FLAGS := -O3
$(Mode)GLOBAL_CXXFLAGS := -DNDEBUG
$(Mode)GLOBAL_LDFLAGS := -s
$(Mode)PROJ_COMMON_FLAGS := -flto
$(Mode)_win_subsystem := -mwindows

$(call NewMode,profile)
$(Mode)GLOBAL_COMMON_FLAGS := -O3 -pg
$(Mode)GLOBAL_CXXFLAGS := -DNDEBUG
$(Mode)_win_subsystem := -mwindows

$(call NewMode,sanitize_address_ub)
$(Mode)GLOBAL_COMMON_FLAGS := -g -fsanitize=address -fsanitize=undefined
$(Mode)GLOBAL_CXXFLAGS := -D_GLIBCXX_DEBUG
$(Mode)PROJ_RUNTIME_ENV += LSAN_OPTIONS=suppressions=misc/leak_sanitizer_suppressions.txt

DIST_NAME := $(APP)_$(TARGET_OS)_v1.*
ifneq ($(MODE),release)
DIST_NAME := $(DIST_NAME)_$(MODE)
endif

# --- Project config ---

PROJ_CXXFLAGS += -std=c++26 -pedantic-errors
PROJ_CXXFLAGS += -Wall -Wextra -Wdeprecated -Wextra-semi -Wimplicit-fallthrough -Wconversion -Wno-implicit-int-float-conversion
PROJ_CXXFLAGS += -Wconversion -Wno-implicit-int-float-conversion# Conversion warnings, but without the silly ones.
PROJ_CXXFLAGS += -ftemplate-backtrace-limit=0 -fmacro-backtrace-limit=0
PROJ_CXXFLAGS += -Isrc
PROJ_CXXFLAGS += -Ideps/macros/include -Ideps/math/include -Ideps/meta/include -Ideps/reflection/include -Ideps/zstring_view/include

ifeq ($(TARGET_OS),windows)
PROJ_LDFLAGS += $(_win_subsystem)
endif

# The common PCH rules for all projects.
# override _pch_rules := src/game/*->src/game/master.hpp

$(call Project,exe,imp-re)
$(call ProjectSetting,source_dirs,src)
$(call ProjectSetting,cxxflags,-DDOCTEST_CONFIG_DISABLE)
# $(call ProjectSetting,pch,$(_pch_rules))
$(call ProjectSetting,libs,*)


# --- Dependencies ---

# On Windows, install following for SDL3:
#   pacman -S --needed mingw-w64-ucrt-x86_64-libiconv mingw-w64-ucrt-x86_64-vulkan
# Additionally, following on Windows hosts as opposed to Quasi-MSYS2:
#   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf
# Thoses lists are from https://packages.msys2.org/packages/mingw-w64-x86_64-SDL2, slightly modified.
# Last updated for SDL 2.30.8-1, with the assumption that it's enough for SDL3.

# On Arch, install following for SDL3 (from https://gitlab.archlinux.org/archlinux/packaging/packages/sdl2/-/blob/main/PKGBUILD)
#   pacman -S --needed glibc libxext libxrender libx11 libgl libxcursor hidapi libusb alsa-lib mesa libpulse libxrandr libxinerama wayland libxkbcommon wayland-protocols ibus fcitx5 libxss cmake jack ninja pipewire libdecor
# This list was last updated for SDL 2.30.8, with the assumption that it's enough for SDL3.
# Not sure if those allow all features to be build, but since we're not going to distribute Arch binaries anyway, it shouldn't matter.


# --- Libraries ---

_win_is_x32 :=
_win_sdl3_arch := $(if $(_win_is_x32),i686-w64-mingw32,x86_64-w64-mingw32)

# Disable unnecessary stuff.

_openal_flags := -DALSOFT_EXAMPLES=FALSE
# Disable helper executables. Otherwise Windows builds fails because of missing Qt.
_openal_flags += -DALSOFT_UTILS=FALSE
# Enable SDL2 backend.
_openal_flags += -DALSOFT_REQUIRE_SDL2=TRUE -DALSOFT_BACKEND_SDL2=TRUE
# We used to disable other backends here, but it seems our CMake isolation works well enough to make this unnecessary.


# # When you update this, check if they added installation rules for headers.
# # To generate the new archive filename when updating (commit hash and date), you can use the comment at the beginning of our `box2cpp.h`.
# $(call Library,box2d,box2d-b864f53-2024-09-29.zip)
#   $(call LibrarySetting,cmake_flags,-DBOX2D_SAMPLES:BOOL=OFF -DBOX2D_UNIT_TESTS:BOOL=OFF)
#   $(call LibrarySetting,build_system,box2d)
# override buildsystem-box2d =\
# 	$(call, ### Forward to CMake.)\
# 	$(buildsystem-cmake)\
# 	$(call, ### Install headers.)\
# 	$(call safe_shell_exec,cp -rT $(call quote,$(__source_dir)/include) $(call quote,$(__install_dir)/include))

# # Delaunay triangulation library.
# # It supports pre-baking the algorithms into a library, but that requires setting a global macro, which we don't have a convenient way of doing yet.
# # So instead we use it in header-only mode...
# # This also lets us move their headers to a `CDT` subdirectory, otherwise they have too much junk at the top level `include/`.
# $(call Library,cdt,CDT-1.4.1+46f1ce1.zip)
#   $(call LibrarySetting,build_system,copy_files)
#   $(call LibrarySetting,copy_files,CDT/include/*->include/CDT)

# $(call Library,enkits,enkiTS-686d0ec-2024-05-29.zip)
#   $(call LibrarySetting,cmake_flags,-DENKITS_INSTALL=ON -DENKITS_BUILD_SHARED=ON -DENKITS_BUILD_EXAMPLES=OFF)

$(call Library,fmt,https://github.com/fmtlib/fmt/releases/download/11.1.4/fmt-11.1.4.zip)
  $(call LibrarySetting,cmake_flags,-DFMT_TEST=OFF)

# ifeq ($(TARGET_OS),emscripten)
# $(call LibraryStub,freetype,-sUSE_FREETYPE=1)
# else
# $(call Library,freetype,freetype-2.13.3.tar.gz)
#   $(call LibrarySetting,deps,zlib)
# endif

# $(call Library,phmap,parallel-hashmap-1.4.0.tar.gz)
#   $(call LibrarySetting,cmake_flags,-DPHMAP_BUILD_TESTS=OFF -DPHMAP_BUILD_EXAMPLES=OFF)# Otherwise it downloads GTest, which is nonsense.

# ifeq ($(TARGET_OS),emscripten)
# $(call LibraryStub,sdl3,-sUSE_SDL=3)# This wasn't tested yet.
# else ifeq ($(TARGET_OS),windows)
# $(call Library,sdl3,SDL2-devel-2.30.8-mingw.tar.gz)
#   $(call LibrarySetting,build_system,copy_files)
#   $(call LibrarySetting,copy_files,$(_win_sdl3_arch)/*->.)
# $(call Library,sdl3_net,SDL2_net-devel-2.2.0-mingw.tar.gz)
#   $(call LibrarySetting,build_system,copy_files)
#   $(call LibrarySetting,copy_files,$(_win_sdl3_arch)/*->.)
# else
$(call Library,sdl3,https://github.com/libsdl-org/SDL/releases/download/release-3.2.8/SDL3-3.2.8.tar.gz)
  # Allow SDL to see system packages. If we were using `configure+make`, we'd need `configure_vars = env -uPKG_CONFIG_PATH -uPKG_CONFIG_LIBDIR` instead.
  $(call LibrarySetting,cmake_flags,-DCMAKE_FIND_USE_CMAKE_SYSTEM_PATH=ON)
# $(call Library,sdl3_net,SDL2_net-2.2.0.tar.gz)
#   $(call LibrarySetting,deps,sdl3)
# endif

# $(call Library,stb,stb-31707d1-2024-10-03.zip)
#   $(call LibrarySetting,build_system,copy_files)
#   # Out of those, `rectpack` is used both by us and ImGui.
#   # There's also `textedit`, which ImGui uses and we don't but we let ImGui keep its version, since it's slightly patched.
#   $(call LibrarySetting,copy_files,stb_image.h->include stb_image_write.h->include stb_rect_pack.h->include)
