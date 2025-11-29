#include <gtest/gtest.h>
#include "AI.hpp"
#include "Board.hpp"
#include <algorithm>
#include <utility>
#include <vector>

// --- helpers -------------------------------------------------------------

// Read (rows, cols) from the board grid
static inline std::pair<int,int> dims(const Board& b) {
  const auto grid = b.get_grid();
  const int rows = static_cast<int>(grid.size());
  const int cols = rows ? static_cast<int>(grid[0].size()) : 0;
  return {rows, cols};
}

static inline std::pair<int,int> max_goal(int rows) { return {rows - 1, 0}; }
static inline std::pair<int,int> min_goal(int cols) { return {0, cols - 1}; }

static bool contains(const std::vector<std::pair<int,int>>& mv,
                     std::pair<int,int> x) {
  return std::find(mv.begin(), mv.end(), x) != mv.end();
}

// From a position where it's MIN's turn, can MIN win immediately?
static bool min_has_win_in_one(Board& b) {
  auto [rows, cols] = dims(b);
  (void)rows; // unused
  auto mv = b.get_valid_moves();  // assumes current_player == MIN
  return contains(mv, min_goal(cols));
}

// From a position where it's MAX's turn, can MAX win immediately?
static bool max_has_win_in_one(Board& b) {
  auto [rows, cols] = dims(b);
  (void)cols; // unused
  auto mv = b.get_valid_moves();  // assumes current_player == MAX
  return contains(mv, max_goal(rows));
}

// MAX moves that allow MIN to win on the very next ply (blunders)
static std::vector<std::pair<int,int>> dangerous_for_max(const Board& start) {
  std::vector<std::pair<int,int>> out;
  auto max_moves = start.get_valid_moves();
  for (auto m : max_moves) {
    Board b2 = start;
    b2.make_move(m);          // MIN to move
    if (min_has_win_in_one(b2)) out.push_back(m);
  }
  return out;
}

// MAX moves that guarantee a win in 2 (∀ MIN replies, MAX wins in 1)
static std::vector<std::pair<int,int>> win_in_two_for_max(const Board& start) {
  std::vector<std::pair<int,int>> out;
  auto max_moves = start.get_valid_moves();
  for (auto m : max_moves) {
    Board b2 = start;
    b2.make_move(m);               // MIN to move
    auto min_moves = b2.get_valid_moves();
    if (min_moves.empty()) continue;

    bool all_replies_lose = true;
    for (auto r : min_moves) {
      Board b3 = b2;               // copy
      b3.make_move(r);             // MAX to move
      if (!max_has_win_in_one(b3)) {
        all_replies_lose = false;
        break;
      }
    }
    if (all_replies_lose) out.push_back(m);
  }
  return out;
}

// --- tests ---------------------------------------------------------------

// Depth-2 should avoid handing MIN an immediate win
TEST(Minimax, Avoids_OpponentImmediateWin_Depth2) {
  const int rows = 7, cols = 7;
  Board b(rows, cols);
  b.reset_board(rows, cols, /*block_initial=*/false);

  // Place near MIN goal to create plausible blunders
  const int sr = std::min(2, rows - 2);
  const int sc = std::max(2, cols - 2);
  b.set_marker_pos(sr, sc, /*also_block_here=*/true);

  auto bad = dangerous_for_max(b);
  if (bad.empty()) GTEST_SKIP() << "No blunder that allows MIN to win in 1 from this setup.";

  AI ai(/*is_max=*/true, /*max_depth=*/2);
  auto chosen = ai.choose_move(b, /*depth_override=*/2, /*rounds=*/5);

  EXPECT_TRUE(std::find(bad.begin(), bad.end(), chosen) == bad.end())
      << "Depth-2 minimax selected a move that lets MIN win immediately.";
}

// // If a win-in-2 exists, depth-2 minimax should pick one of those moves
// TEST(Minimax, Takes_WinInTwo_IfExists_Depth2) {
//   const int rows = 7, cols = 7;
//   Board b(rows, cols);
//   b.reset_board(rows, cols, /*block_initial=*/false);

//   // Try a few starts near MAX goal to increase chance of a win-in-2
//   std::vector<std::pair<int,int>> starts = {
//     {rows-3, 2}, {rows-3, 1}, {rows-2, 2}
//   };

//   bool found_case = false;
//   for (auto [r,c] : starts) {
//     Board trial = b;
//     trial.set_marker_pos(r, c, /*also_block_here=*/true);
//     auto good = win_in_two_for_max(trial);
//     if (!good.empty()) {
//       AI ai(/*is_max=*/true, /*max_depth=*/2);
//       auto chosen = ai.choose_move(trial, /*depth_override=*/2, /*rounds=*/3);
//       EXPECT_TRUE(std::find(good.begin(), good.end(), chosen) != good.end())
//           << "Depth-2 minimax did not choose a known 'win in 2' move.";
//       found_case = true;
//       break;
//     }
//   }

//   if (!found_case) {
//     GTEST_SKIP() << "No guaranteed 'win in 2' found from tried starts; position-dependent.";
//   }
// }

// If a win-in-2 exists, depth-2 minimax should pick one of those moves.
// Scan a few sizes/starts so it rarely skips.
TEST(Minimax, Takes_WinInTwo_IfExists_Depth2) {
  // Try both 5x5 and 7x7 — 5x5 tends to produce short tactics more often.
  for (auto sz : {std::make_pair(5,5), std::make_pair(7,7)}) {
    const int rows = sz.first, cols = sz.second;
    Board base(rows, cols);
    base.reset_board(rows, cols, /*block_initial=*/false);

    // Scan a small window near MAX's goal (rows-1,0)
    // e.g., rows-4..rows-2 for r, and 0..2 for c.
    for (int r = std::max(1, rows - 4); r <= rows - 2; ++r) {
      for (int c = 0; c <= std::min(2, cols - 1); ++c) {
        Board trial = base;
        trial.set_marker_pos(r, c, /*also_block_here=*/true);

        auto good = win_in_two_for_max(trial);  // helper defined above
        if (!good.empty()) {
          AI ai(/*is_max=*/true, /*max_depth=*/2);
          auto chosen = ai.choose_move(trial, /*depth_override=*/2, /*rounds=*/3);
          EXPECT_TRUE(std::find(good.begin(), good.end(), chosen) != good.end())
              << "Depth-2 minimax did not choose a known 'win in 2' move on "
              << rows << "x" << cols << " from start (" << r << "," << c << ").";
          return;  // found a case and asserted it
        }
      }
    }
  }

  // If we get here, none of the scanned starts had a guaranteed win-in-2.
  GTEST_SKIP() << "No guaranteed 'win in 2' found in scanned windows on 5x5/7x7.";
}


