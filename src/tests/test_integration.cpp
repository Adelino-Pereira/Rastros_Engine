#include <gtest/gtest.h>
#include "AI.hpp"
#include "Board.hpp"
#include <algorithm>
#include <cmath>
#include <utility>


#include <chrono>
#include <cstdint>

// Simple 64-bit FNV-1a rolling hash for move sequences
static uint64_t hash_moves(uint64_t h, int r, int c) {
  uint64_t v = (static_cast<uint64_t>(r) << 32) ^ static_cast<uint64_t>(c);
  h ^= v;
  h *= 1099511628211ull; // FNV prime
  return h;
}

// Plays AI vs AI to terminal on a given board, depth=1.
// Returns: {max_won, sequence_hash, moves_made}
static std::tuple<bool,uint64_t,int>
play_to_end_and_hash(Board& b, int rows, int cols) {
  int rounds = 0;
  int moves_made = 0;
  const int max_moves = rows * cols;
  uint64_t h = 1469598103934665603ull; // FNV offset basis

  while (!b.is_terminal() && moves_made < max_moves) {
    bool is_max = b.current_player_is_max();
    AI ai(is_max, /*depth=*/1);

    auto valids = b.get_valid_moves();
    if (valids.empty()) {
      ADD_FAILURE() << "No valid moves before AI move";
      break;
    }

    auto before = b.get_marker();
    auto chosen = ai.choose_move(b, /*depth_override=*/1, /*rounds=*/rounds);

    if (std::find(valids.begin(), valids.end(), chosen) == valids.end()) {
      ADD_FAILURE() << "AI chose an invalid move: (" << chosen.first << "," << chosen.second << ")";
      break;
    }

    b.make_move(chosen);

    auto grid = b.get_grid();
    if (grid[before.first][before.second] != 0) {
      ADD_FAILURE() << "Previous cell not blocked after move";
    }

    h = (h ^ ((uint64_t(chosen.first) << 32) ^ uint64_t(chosen.second))) * 1099511628211ull;
    ++rounds;
    ++moves_made;
  }


  // Determine winner:
  // - goal reached → marker is at one of the goals
  // - no moves available → current player to move has zero moves → previous player won
  auto m = b.get_marker();
  bool max_goal = (m.first == rows - 1 && m.second == 0);
  bool min_goal = (m.first == 0 && m.second == cols - 1);

  bool max_won;
  if (max_goal)      max_won = true;
  else if (min_goal) max_won = false;
  else               max_won = !b.current_player_is_max(); // stalemate → previous player

  return {max_won, h, moves_made};
}

/* -------------------------------------------------------------------
   3) Deterministic winner & golden sequence (7x7, fixed RNG in tests)
   ------------------------------------------------------------------- */
TEST(IntegrationDeterminism, WinnerAndSequence_7x7_Depth1) {
  const int rows = 7, cols = 7;
  Board b(rows, cols);
  b.reset_board(rows, cols, /*block_initial=*/false);

  auto [max_won, seq_hash, moves] = play_to_end_and_hash(b, rows, cols);

  GTEST_LOG_(INFO) << "winner=" << (max_won ? "MAX" : "MIN")
                 << " hash=" << seq_hash
                 << " moves=" << moves;

  // ---- Fill these after the FIRST successful run, then re-run to lock regressions ----
  // 1) Observe printed values in the test output (add TEMP prints if you want),
  // 2) Put them into the constexprs below, 3) Re-run; mismatches will fail the test.

  constexpr bool   EXPECT_MAX_WIN = true;            // <-- set to observed winner
  
constexpr std::uint64_t EXPECT_HASH = 9679338556779340539ULL;            // <-- set to observed hash
  constexpr int    EXPECT_MOVES   = 6;               // <-- optional: exact # moves

  // If you haven't filled in EXPECT_HASH/EXPECT_MOVES yet, just assert winner for now.
  EXPECT_EQ(max_won, EXPECT_MAX_WIN) << "Deterministic winner changed on 7x7 (depth 1)";

  if (EXPECT_HASH != 0ull) {
    EXPECT_EQ(seq_hash, EXPECT_HASH) << "Move-sequence hash changed (7x7, depth 1)";
  }
  if (EXPECT_MOVES != 0) {
    EXPECT_EQ(moves, EXPECT_MOVES) << "Number of moves changed (7x7, depth 1)";
  }
}

/* -----------------------------------------------------------
   4) Performance guard (average choose_move time ≤ threshold)
   ----------------------------------------------------------- */
TEST(IntegrationPerf, AvgDecisionTime_Depth1) {
#ifndef ENABLE_PERF_GUARD
  GTEST_SKIP() << "Perf guard disabled. Define ENABLE_PERF_GUARD to enforce.";
#endif

  const int rows = 7, cols = 7;
  Board b(rows, cols);
  b.reset_board(rows, cols, /*block_initial=*/false);

  using clock = std::chrono::steady_clock;
  const int max_samples = rows * cols; // up to whole game
  int samples = 0;
  double total_ms = 0.0;

  int rounds = 0;
  while (!b.is_terminal() && samples < max_samples) {
    bool is_max = b.current_player_is_max();
    AI ai(is_max, /*depth=*/1);

    auto start = clock::now();
    auto chosen = ai.choose_move(b, /*depth_override=*/1, /*rounds=*/rounds);
    auto end   = clock::now();

    std::chrono::duration<double, std::milli> dt = end - start;
    total_ms += dt.count();
    ++samples;

    // apply to progress; if invalid, test will fail
    auto valids = b.get_valid_moves();
    ASSERT_TRUE(std::find(valids.begin(), valids.end(), chosen) != valids.end());
    b.make_move(chosen);
    ++rounds;
  }

  ASSERT_GT(samples, 0) << "No samples collected";
  double avg_ms = total_ms / samples;

  // Set a sensible budget for your machine/CI. Start generous, tighten later.
  constexpr double BUDGET_MS = 5.0; // e.g., 5 ms per decision at depth 1
  EXPECT_LE(avg_ms, BUDGET_MS)
      << "Average choose_move time (" << avg_ms << " ms) exceeded budget ("
      << BUDGET_MS << " ms)";
}







// Chebyshev distance helper
static int cheb(int r1, int c1, int r2, int c2) {
  return std::max(std::abs(r1 - r2), std::abs(c1 - c2));
}

/*
  Helper to execute an AI move end-to-end:
  - Asserts there are valid moves
  - Chooses a move with AI::choose_move
  - Asserts it's valid
  - Applies it with b.make_move()
  - Checks the previous cell becomes blocked
  - Returns the chosen move via chosen_out
*/
static void run_ai_move(Board& b,
                        bool is_max,
                        int depth_override,
                        int rounds,
                        std::pair<int,int>& chosen_out) {
  AI ai(is_max, depth_override);

  auto valids = b.get_valid_moves();
  ASSERT_FALSE(valids.empty()) << "No valid moves before AI move";

  auto before = b.get_marker();
  auto chosen = ai.choose_move(b, depth_override, rounds);

  ASSERT_TRUE(std::find(valids.begin(), valids.end(), chosen) != valids.end())
      << "AI chose an invalid move: (" << chosen.first << "," << chosen.second << ")";

  b.make_move(chosen);

  auto grid = b.get_grid();
  EXPECT_EQ(grid[before.first][before.second], 0) << "Previous cell must be blocked after move";

  chosen_out = chosen;
}

/* ------------------------------------------------------------------
   1) Parameterized: MAX's first move is not adjacent, and is applied
   ------------------------------------------------------------------ */
class IntegrationFirstMoveSizes : public ::testing::TestWithParam<std::pair<int,int>> {};

TEST_P(IntegrationFirstMoveSizes, AIFirstMove_NotAdjacentAndApplied) {
  const auto [rows, cols] = GetParam();
  Board b(rows, cols);

  // Clean board; place marker near MIN goal (0, cols-1) so adjacency is possible.
  b.reset_board(rows, cols, /*block_initial=*/false);
  const int start_r = std::min(1, rows - 2);
  const int start_c = std::max(1, cols - 2);
  b.set_marker_pos(start_r, start_c, /*also_block_here=*/true);

  // First move (rounds=0) should trigger the first-move policy inside AI
  const int depth = 1;
  int rounds = 0;

  auto before = b.get_marker();
  std::pair<int,int> chosen;
  run_ai_move(b, /*is_max=*/true, depth, rounds, chosen);
  EXPECT_NE(chosen, before);

  // Must NOT be adjacent to MIN goal (0, cols-1)
  const int opp_goal_r = 0, opp_goal_c = cols - 1;
  int d = cheb(chosen.first, chosen.second, opp_goal_r, opp_goal_c);
  EXPECT_GT(d, 1) << "First AI move landed adjacent to opponent goal on "
                  << rows << "x" << cols;
}

INSTANTIATE_TEST_SUITE_P(
  Sizes_5_7_11,
  IntegrationFirstMoveSizes,
  ::testing::Values(
    std::make_pair(5,5),
    std::make_pair(7,7),
    std::make_pair(11,11)
  )
);

/* ---------------------------------------------------------
   2) Parameterized: AI vs AI reaches terminal within a bound
   --------------------------------------------------------- */
class IntegrationPlayToEndSizes : public ::testing::TestWithParam<std::pair<int,int>> {};

TEST_P(IntegrationPlayToEndSizes, AIVsAI_TerminatesWithinBound) {
  const auto [rows, cols] = GetParam();
  Board b(rows, cols);

  b.reset_board(rows, cols, /*block_initial=*/false);

  int rounds = 0;
  int moves_made = 0;
  const int max_moves = rows * cols; // safe upper bound

  while (!b.is_terminal() && moves_made < max_moves) {
    bool is_max = b.current_player_is_max(); // assuming this is public in your Board
    std::pair<int,int> chosen;
    run_ai_move(b, is_max, /*depth_override=*/1, /*rounds=*/rounds, chosen);
    ++rounds;
    ++moves_made;
  }

  EXPECT_TRUE(b.is_terminal())
      << "Game did not reach a terminal state within " << max_moves
      << " moves on " << rows << "x" << cols;
}

INSTANTIATE_TEST_SUITE_P(
  Sizes_5_7_11,
  IntegrationPlayToEndSizes,
  ::testing::Values(
    std::make_pair(5,5),
    std::make_pair(7,7),
    std::make_pair(11,11)
  )
);
