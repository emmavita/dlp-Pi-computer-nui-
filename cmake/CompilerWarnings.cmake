# CompilerWarnings.cmake — centralized warning flags for all C++ targets.
# Included from the root CMakeLists so every target is built with the same
# strict, warning-clean settings the project is validated against.
#
# Usage: include(CompilerWarnings); nui_set_project_warnings() sets the flags
# globally via add_compile_options.

function(nui_set_project_warnings)
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
  elseif(MSVC)
    add_compile_options(/W4)
  endif()
endfunction()
