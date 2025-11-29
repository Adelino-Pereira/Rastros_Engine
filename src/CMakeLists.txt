cmake_minimum_required(VERSION 3.14)

project(Rastros LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- App (unchanged) ---
set(SOURCES
  main.cpp
  LogMsgs.cpp
  Board.cpp
  AI.cpp
  GameController.cpp
  TestController.cpp
  Heuristic1.cpp
  Heuristic2.cpp
  HeuristicsUtils.cpp
)
add_executable(Rastros ${SOURCES})
# target_include_directories(Rastros PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include) # if you later split headers

# --- Tests ---
option(BUILD_TESTS "Build unit tests" ON)
option(ENABLE_PERF_GUARD "Enforce perf budget in IntegrationTests" OFF)
option(RASTROS_MINIMAX_NO_TT "Use minimax_noTT in choose_move" OFF)
option(RASTROS_MINIMAX_NO_PRUNE "Use minimax_no_pruning in choose_move" OFF)

if(BUILD_TESTS)
  enable_testing()

  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  # Board tests
  add_executable(BoardTests
    tests/test_board.cpp
    # reuse engine sources
    LogMsgs.cpp
    Board.cpp AI.cpp GameController.cpp TestController.cpp
    Heuristic1.cpp Heuristic2.cpp HeuristicsUtils.cpp
  )
  target_include_directories(BoardTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(BoardTests PRIVATE RASTROS_TESTS=1)
  target_link_libraries(BoardTests PRIVATE gtest_main)

  # AI tests (includes minimax tests)
  add_executable(AITests
    tests/test_ai.cpp
    tests/test_minimax.cpp
    # reuse engine sources
    LogMsgs.cpp
    Board.cpp AI.cpp GameController.cpp TestController.cpp
    Heuristic1.cpp Heuristic2.cpp HeuristicsUtils.cpp
  )
  target_include_directories(AITests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(AITests PRIVATE RASTROS_TESTS=1)
  target_link_libraries(AITests PRIVATE gtest_main)

  # Integration tests
  add_executable(IntegrationTests
    tests/test_integration.cpp
    # reuse engine sources
    LogMsgs.cpp
    Board.cpp AI.cpp GameController.cpp TestController.cpp
    Heuristic1.cpp Heuristic2.cpp HeuristicsUtils.cpp
  )
  target_include_directories(IntegrationTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(IntegrationTests PRIVATE RASTROS_TESTS=1)
  if(ENABLE_PERF_GUARD)
    target_compile_definitions(IntegrationTests PRIVATE ENABLE_PERF_GUARD=1)
  endif()
  target_link_libraries(IntegrationTests PRIVATE gtest_main)

  include(GoogleTest)
  gtest_discover_tests(BoardTests       TEST_PREFIX Board:)
  gtest_discover_tests(AITests          TEST_PREFIX AI:)
  gtest_discover_tests(IntegrationTests TEST_PREFIX Integration:)

  # Build all tests at once
  add_custom_target(RastrosTests DEPENDS BoardTests AITests IntegrationTests)
endif()

foreach(tgt Rastros BoardTests AITests IntegrationTests)
if(TARGET ${tgt})
  if(RASTROS_MINIMAX_NO_TT)
    target_compile_definitions(${tgt} PRIVATE RASTROS_MINIMAX_NO_TT)
  endif()
  if(RASTROS_MINIMAX_NO_PRUNE)
    target_compile_definitions(${tgt} PRIVATE RASTROS_MINIMAX_NO_PRUNE)
  endif()
endif()
endforeach()
