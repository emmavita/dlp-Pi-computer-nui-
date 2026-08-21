cmake_minimum_required(VERSION 3.16)
project(nui_computer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Project CMake modules (cmake/): warnings + HailoRT finder.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(CompilerWarnings)
nui_set_project_warnings()

# Optional HailoRT probe (used on target by the perception Hailo path, which is
# currently blocked pending on-target post-process). Off-target builds skip it.
find_package(HailoRT QUIET)
if(HailoRT_FOUND)
  message(STATUS "HailoRT found: ${HailoRT_LIBRARIES}")
else()
  message(STATUS "HailoRT not found — perception Hailo path will not build (expected off-target)")
endif()

# Shared contract header (proto/) is header-only.
set(PROTO_INC ${CMAKE_SOURCE_DIR}/proto)

# --- nui_protocol (UDS transport) ---
add_library(nui_protocol STATIC
  libs/nui_protocol/src/uds.cpp)
target_include_directories(nui_protocol PUBLIC
  ${PROTO_INC}
  ${CMAKE_SOURCE_DIR}/libs/nui_protocol/include)

# --- engine_core (N1..N6, hardware-independent) ---
add_library(engine_core STATIC
  services/engine/src/gesture_fsm.cpp
  services/engine/src/engine_core.cpp)
target_include_directories(engine_core PUBLIC
  ${PROTO_INC}
  ${CMAKE_SOURCE_DIR}/services/engine/include)

# --- nui-engine service ---
add_executable(nui-engine services/engine/src/main.cpp)
target_link_libraries(nui-engine PRIVATE engine_core nui_protocol)

# --- engine self-test ---
add_executable(engine_selftest services/engine/tests/engine_selftest.cpp)
target_link_libraries(engine_selftest PRIVATE engine_core)

# --- perception: hef integrity (self-contained SHA-256, no external dep) ---
add_library(hef_integrity STATIC
  services/perception/src/hef_integrity.cpp)
target_include_directories(hef_integrity PUBLIC
  ${CMAKE_SOURCE_DIR}/services/perception/include)

add_executable(hef_integrity_test services/perception/tests/hef_integrity_test.cpp)
target_link_libraries(hef_integrity_test PRIVATE hef_integrity)

# --- perception: tracking gate (O2, header-only pure logic) ---
add_executable(tracking_gate_test services/perception/tests/tracking_gate_test.cpp)
target_include_directories(tracking_gate_test PRIVATE
  ${CMAKE_SOURCE_DIR}/services/perception/include)

# --- perception: performance probe (latency/FPS instrumentation) ---
add_library(perf_probe STATIC services/perception/src/perf_probe.cpp)
target_include_directories(perf_probe PUBLIC
  ${CMAKE_SOURCE_DIR}/services/perception/include)

add_executable(perf_probe_test services/perception/tests/perf_probe_test.cpp)
target_link_libraries(perf_probe_test PRIVATE perf_probe)

# --- perception: shadow contact (OpenCV) ---
find_package(OpenCV REQUIRED COMPONENTS core imgproc)
add_library(perception_contact STATIC
  services/perception/src/shadow_contact.cpp)
target_include_directories(perception_contact PUBLIC
  ${CMAKE_SOURCE_DIR}/services/perception/include
  ${OpenCV_INCLUDE_DIRS})
target_link_libraries(perception_contact PUBLIC ${OpenCV_LIBS})

add_executable(shadow_contact_test services/perception/tests/shadow_contact_test.cpp)
target_link_libraries(shadow_contact_test PRIVATE perception_contact)

# NOTE: the GStreamer/Hailo hand-cascade pipeline and its three post-process
# .so files are NOT built here — they are blocked pending the target's Hailo
# tensor layouts and TAPPAS headers (see services/perception/README.md).

# --- ui: Qt6 Quick fullscreen Wayland client ---
# Optional so engine/perception still build on hosts without Qt6.
find_package(Qt6 QUIET COMPONENTS Quick Gui)
if(Qt6_FOUND)
  set(CMAKE_AUTOMOC ON)
  qt6_add_resources(UI_RCC services/ui/qml.qrc)
  add_executable(nui-ui
    services/ui/src/main.cpp
    services/ui/src/EventClient.hpp
    services/ui/src/EventClient.cpp
    ${UI_RCC})
  target_include_directories(nui-ui PRIVATE services/ui/src)
  target_link_libraries(nui-ui PRIVATE Qt6::Quick Qt6::Gui nui_protocol)
else()
  message(STATUS "Qt6 not found — skipping nui-ui target")
endif()

enable_testing()
add_test(NAME engine_selftest      COMMAND engine_selftest)
add_test(NAME hef_integrity_test   COMMAND hef_integrity_test)
add_test(NAME tracking_gate_test   COMMAND tracking_gate_test)
add_test(NAME perf_probe_test      COMMAND perf_probe_test)
add_test(NAME shadow_contact_test  COMMAND shadow_contact_test)

# --- QA integration harness (end-to-end over the real UDS bus) ---
find_package(Threads REQUIRED)

add_executable(qa_perception tests/integration/qa_perception.cpp)
target_include_directories(qa_perception PRIVATE ${CMAKE_SOURCE_DIR}/tests/integration)
target_link_libraries(qa_perception PRIVATE nui_protocol Threads::Threads)

add_executable(qa_ui_client tests/integration/qa_ui_client.cpp)
target_link_libraries(qa_ui_client PRIVATE nui_protocol)

add_executable(qa_malformed tests/integration/qa_malformed.cpp)
target_include_directories(qa_malformed PRIVATE ${CMAKE_SOURCE_DIR}/tests/integration)
# qa_malformed uses raw sockets + proto only (no nui_protocol needed), but link
# nui_protocol for the include path convenience.
target_include_directories(qa_malformed PRIVATE ${CMAKE_SOURCE_DIR}/proto)

add_test(NAME integration
         COMMAND bash ${CMAKE_SOURCE_DIR}/tests/integration/run_integration.sh
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_test(NAME malformed
         COMMAND bash ${CMAKE_SOURCE_DIR}/tests/integration/run_malformed.sh
         WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
