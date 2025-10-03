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
PROJ_CXXFLAGS += -Wall -Wextra -Wdeprecated -Wextra-semi -Wimplicit-fallthrough
PROJ_CXXFLAGS += -Wconversion -Wno-implicit-int-float-conversion# Conversion warnings, but without the silly ones.
PROJ_CXXFLAGS += -ftemplate-backtrace-limit=0
PROJ_CXXFLAGS += -fmacro-backtrace-limit=1# 1 = minimal, 0 = infinite
PROJ_CXXFLAGS += -Isrc
PROJ_CXXFLAGS += -Ideps/cppdecl/include -Ideps/macros/include -Ideps/math/include -Ideps/meta/include -Ideps/minitest/include -Ideps/reflection/include -Ideps/zstring_view/include

ifeq ($(TARGET_OS),windows)
PROJ_LDFLAGS += $(_win_subsystem)
endif

# The common PCH rules for all projects.
# override _pch_rules := src/game/*->src/game/master.hpp

$(call Project,exe,imp-re)
$(call ProjectSetting,source_dirs,src $(filter-out deps/minitest/%,$(wildcard deps/*/test deps/*/src)))
$(call ProjectSetting,ignored_sources,*.test.cpp)
# $(call ProjectSetting,pch,$(_pch_rules))
$(call ProjectSetting,libs,*)

$(call Project,exe,tests)
$(call ProjectSetting,source_dirs,src $(filter-out deps/minitest/test,$(wildcard deps/*/test deps/*/src)))
$(call ProjectSetting,sources,deps/minitest/src/main.cpp)
$(call ProjectSetting,cxxflags,-DEM_ENABLE_TESTS)
# $(call ProjectSetting,pch,$(_pch_rules))
$(call ProjectSetting,libs,*)


# Shader compilation:
ASSETS_IGNORED_PATTERNS += *.glsl
ASSETS_GENERATED += $(patsubst %.glsl,%.spv,$(wildcard assets/assets/shaders/*.glsl))
assets/%.vert.spv: assets/%.vert.glsl
	$(call log_now,[GLSL Vertex] $<)
	@glslc -fshader-stage=vert $< -o $@ -O
assets/%.frag.spv: assets/%.frag.glsl
	$(call log_now,[GLSL Fragment] $<)
	@glslc -fshader-stage=frag $< -o $@ -O


# --- Dependencies ---

# On Windows, install following for SDL3:
#   pacman -S --needed mingw-w64-ucrt-x86_64-libiconv mingw-w64-ucrt-x86_64-vulkan
# Additionally, following on Windows hosts as opposed to Quasi-MSYS2:
#   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf
# Thoses lists are from https://packages.msys2.org/packages/mingw-w64-x86_64-sdl3 (minus gcc-libs).
# Last updated for MSYS2 SDL 3.2.8-1.

# On Arch, install following for SDL3 (from https://gitlab.archlinux.org/archlinux/packaging/packages/sdl2/-/blob/main/PKGBUILD)
#   pacman -S --needed alsa-lib cmake glibc hidapi ibus jack libdecor libgl libpulse libusb libx11 libxcursor libxext libxinerama libxkbcommon libxrandr libxrender libxss mesa ninja pipewire sndio vulkan-headers wayland wayland-protocols
# This list was last updated for MSYS2 SDL 3.2.8.
# Not sure if those allow all features to be build, but since we're not going to distribute Arch binaries anyway, it shouldn't matter.


# --- Libraries ---

_win_is_x32 :=
_win_sdl3_arch := $(if $(_win_is_x32),i686-w64-mingw32,x86_64-w64-mingw32)


# $(call Library,box2d,https://github.com/erincatto/box2d/archive/3e968638a5d0b0ff7ff0dd5a1e8e88844927b2d2.zip)
#   $(call LibrarySetting,cmake_flags,-DBOX2D_SAMPLES:BOOL=OFF -DBOX2D_UNIT_TESTS:BOOL=OFF)
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

$(call Library,fmt,https://github.com/fmtlib/fmt/releases/download/11.2.0/fmt-11.2.0.zip)
  $(call LibrarySetting,cmake_flags,-DFMT_TEST=OFF)

# ifeq ($(TARGET_OS),emscripten)
# $(call LibraryStub,freetype,-sUSE_FREETYPE=1)
# else
# $(call Library,freetype,freetype-2.13.3.tar.gz)
#   $(call LibrarySetting,deps,zlib)
# endif


# Using a commit directly from `master` because new CMake chokes on release 1.24.3.
$(call Library,openal_soft,https://github.com/kcat/openal-soft/archive/b72944e4c36486fee75f1c654321fed82dfa20b5.zip)
  # I think AL can also utlize zlib, but we don't build zlib for now, and it's not clear what it gives AL anyway.
  $(call LibrarySetting,deps,sdl3)
  # `ALSOFT_UTILS=FALSE` disables helper executables. Otherwise Windows builds fail because of missing Qt.
  # We used to disable other backends here, but it seems our CMake isolation should make this unnecessary.
  $(call LibrarySetting,cmake_flags,-DALSOFT_EXAMPLES=FALSE -DALSOFT_UTILS=FALSE -DALSOFT_REQUIRE_SDL3=TRUE -DALSOFT_BACKEND_SDL3=TRUE)

$(call Library,phmap,https://github.com/greg7mdp/parallel-hashmap/archive/88123934b46b77c3b6d80167382734cbff6eff74.zip)
  $(call LibrarySetting,cmake_flags,-DPHMAP_BUILD_TESTS=OFF -DPHMAP_BUILD_EXAMPLES=OFF)# Otherwise it downloads GTest, which is nonsense.

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
$(call Library,sdl3,https://github.com/libsdl-org/SDL/releases/download/release-3.2.22/SDL3-3.2.22.tar.gz)
  $(call LibrarySetting,cmake_allow_using_system_deps,1)
# $(call Library,sdl3_net,SDL2_net-2.2.0.tar.gz)
#   $(call LibrarySetting,deps,sdl3)
# endif

$(call Library,sdl_shadercross,https://github.com/libsdl-org/SDL_shadercross/archive/4ce748310f57d405b4eb2a79fbbc7e974d6491ec.zip)
  $(call LibrarySetting,deps,sdl3 spirv_cross)
  # Disabling HLSL input, only keeping SPIRV input.
  # Enable installation, otherwise CMake installs nothing.
  $(call LibrarySetting,cmake_flags,-DSDLSHADERCROSS_DXC=OFF -DSDLSHADERCROSS_INSTALL=ON)

$(call Library,spirv_cross,https://github.com/KhronosGroup/SPIRV-Cross/archive/b26ac3fa8bcfe76c361b56e3284b5276b23453ce.zip)
  # Disabling tests for speed.
  # Switching from static to dynamic libs just in case, in case we need to link it to our own app.
  # Disabling CLI tools because we don't need them and because they need static libs.
  $(call LibrarySetting,cmake_flags,-DSPIRV_CROSS_ENABLE_TESTS=OFF -DSPIRV_CROSS_SHARED=ON -DSPIRV_CROSS_STATIC=OFF -DSPIRV_CROSS_CLI=OFF)

# $(call Library,spirv_reflect,https://github.com/KhronosGroup/SPIRV-Reflect/archive/c6c0f5c9796bdef40c55065d82e0df67c38a29a4.zip)
#   # Disable the executable and enable the library. They only support a static library, whatever.
#   $(call LibrarySetting,cmake_flags,-DSPIRV_REFLECT_EXECUTABLE=OFF -DSPIRV_REFLECT_STATIC_LIB=ON)

$(call Library,stb,https://github.com/nothings/stb/archive/fede005abaf93d9d7f3a679d1999b2db341b360f.zip)
  $(call LibrarySetting,build_system,dummy)
  # Out of those, `rectpack` is used both by us and ImGui.
  # There's also `textedit`, which ImGui uses and we don't but we let ImGui keep its version, since it's slightly patched.
  $(call LibrarySetting,install_files,*.h->include)
