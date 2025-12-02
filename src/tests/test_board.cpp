#include <gtest/gtest.h>
#include "Board.hpp"

// Helper: place marker safely
static void place_marker(Board& b, int r, int c, bool block_here=true) {
  b.set_marker_pos(r, c, block_here);
}

TEST(BoardBasics, StartsWithSomeValidMoves) {
  Board b(7,7);
  auto moves = b.get_valid_moves();
  EXPECT_FALSE(moves.empty()) << "Marker should have valid moves at start";
}

TEST(BoardBasics, MakeMoveBlocksPreviousAndUpdatesMarker) {
  Board b(7,7);
  auto start = b.get_marker();
  auto moves = b.get_valid_moves();
  ASSERT_FALSE(moves.empty());
  auto mv = moves.front();
  b.make_move(mv);

  // previous cell should be blocked now
  auto grid = b.get_grid();
  EXPECT_EQ(grid[start.first][start.second], 0);
  EXPECT_EQ(b.get_marker(), mv);
}

TEST(BoardTerminal, ReachingEitherGoalEndsGame) {
  Board b(7,7);
  // Max player's goal is (rows-1, 0)
  place_marker(b, 6, 0, /*block_here=*/true);
  EXPECT_TRUE(b.is_terminal());

  // Min player's goal is (0, cols-1)
  Board b2(7,7);
  place_marker(b2, 0, 6, /*block_here=*/true);
  EXPECT_TRUE(b2.is_terminal());
}

TEST(BoardHeuristics, ShortestPathSignsPerSpec) {
  Board b(7,7);
  // Put near MAX goal to get small negative distance for for_max=true
  place_marker(b, 5, 1, true); // diagonal to (6,0) is possible
  // shortest_path_to_goal removido; usa compute_distance para obter distâncias mínimas
  auto reach = b.compute_distance();
  int h_max = reach.h1;
  int h_min = reach.h5;
  // For max, function returns -dist; for min, +dist
  EXPECT_LT(h_max, 0);
  EXPECT_GT(h_min, 0);
}

TEST(BoardReachability, ComputesBothGoalDistancesAndCount) {
  Board b(7,7);
  auto res = b.compute_distance();
  // We can't assert exact numbers without duplicating BFS here;
  // just verify signs and that count >= 1 (at least the marker cell).
  // res.h1 (for max) uses negative distance or -900 if unreachable,
  // res.h5 (for min) uses positive distance or +900 if unreachable.
  // We'll assume start position makes both reachable in empty board.
  // So both should be finite and have opposite signs.
  EXPECT_GT(res.reachable_count, 0);
  EXPECT_LT(res.h1, 0);
  EXPECT_GT(res.h5, 0);
}
