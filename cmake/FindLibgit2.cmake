find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(PC_LIBGIT2 QUIET libgit2>=1.7)
endif()

find_path(Libgit2_INCLUDE_DIR
  NAMES git2.h
  HINTS ${PC_LIBGIT2_INCLUDE_DIRS}
)

find_library(Libgit2_LIBRARY
  NAMES git2 libgit2
  HINTS ${PC_LIBGIT2_LIBRARY_DIRS}
)

if(Libgit2_INCLUDE_DIR AND NOT PC_LIBGIT2_VERSION)
  file(STRINGS "${Libgit2_INCLUDE_DIR}/git2/version.h" _libgit2_version_lines
    REGEX "^#define LIBGIT2_VER_(MAJOR|MINOR|REVISION) [0-9]+$")
  foreach(_line IN LISTS _libgit2_version_lines)
    if(_line MATCHES "LIBGIT2_VER_MAJOR ([0-9]+)")
      set(_libgit2_major "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "LIBGIT2_VER_MINOR ([0-9]+)")
      set(_libgit2_minor "${CMAKE_MATCH_1}")
    elseif(_line MATCHES "LIBGIT2_VER_REVISION ([0-9]+)")
      set(_libgit2_patch "${CMAKE_MATCH_1}")
    endif()
  endforeach()
  if(DEFINED _libgit2_major AND DEFINED _libgit2_minor
     AND DEFINED _libgit2_patch)
    set(PC_LIBGIT2_VERSION
      "${_libgit2_major}.${_libgit2_minor}.${_libgit2_patch}")
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libgit2
  REQUIRED_VARS Libgit2_LIBRARY Libgit2_INCLUDE_DIR
  VERSION_VAR PC_LIBGIT2_VERSION
)

if(Libgit2_FOUND AND NOT TARGET Libgit2::Libgit2)
  add_library(Libgit2::Libgit2 UNKNOWN IMPORTED)
  set_target_properties(Libgit2::Libgit2 PROPERTIES
    IMPORTED_LOCATION "${Libgit2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Libgit2_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(Libgit2_INCLUDE_DIR Libgit2_LIBRARY)
