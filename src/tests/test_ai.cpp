#include <gtest/gtest.h>
#include "AI.hpp"
#include "Board.hpp"
#include <algorithm>


// Função auxiliar para distância de Chebyshev
static int cheb(int r1, int c1, int r2, int c2) {
  return std::max(std::abs(r1 - r2), std::abs(c1 - c2));
}

/* ---------------------------------
   1) Vitória imediata para MAX (profundidade 1)
   --------------------------------- */
TEST(AIChoice, PrefersImmediateWinForMax) {
  const int rows = 7, cols = 7;
  Board b(rows, cols);

  // Reinicia e coloca o marcador adjacente ao objetivo de MAX (rows-1,0) para forçar jogada vencedora.
  b.reset_board(rows, cols, /*block_initial=*/false);
  // Coloca o marcador em (rows-2,1), diagonal a (rows-1,0)
  b.set_marker_pos(rows-2, 1, /*also_block_here=*/true);

  // Verificação: a jogada vencedora deve estar disponível
  auto moves = b.get_valid_moves();
  bool has_goal = false;
  for (auto& m : moves) if (m.first==rows-1 && m.second==0) { has_goal = true; break; }
  ASSERT_TRUE(has_goal) << "Expected diagonal to MAX goal as a valid move";

  AI ai(/*is_max=*/true, /*max_depth=*/1);
  // depth_override=1, rounds=1 (não é o caso especial da primeira jogada)
  auto chosen = ai.choose_move(b, /*depth_override=*/1, /*rounds=*/1);

  EXPECT_EQ(chosen.first, rows-1);
  EXPECT_EQ(chosen.second, 0);
}

/* ---------------------------------
   2) Validação do auxiliar de distância (público)
   --------------------------------- */
TEST(AIHeuristic, DistanceNonNegativeForBothPlayers) {
  Board b(7,7);
  b.reset_board(7,7, /*block_initial=*/false);
  b.set_marker_pos(5,1, /*also_block_here=*/true);

  int d_max = AI::h_distance(b, b.get_marker(), /*is_max=*/true);
  int d_min = AI::h_distance(b, b.get_marker(), /*is_max=*/false);

  EXPECT_GE(d_max, 0);
  EXPECT_GE(d_min, 0);
}

/* ---------------------------------
   3) Escolha greedy em profundidade 0 reduz distância de MAX
   --------------------------------- */
TEST(AIChoice, GreedyPickReducesMaxDistanceAtDepth0) {
  Board b(7,7);
  b.reset_board(7,7, /*block_initial=*/false);
  b.set_marker_pos(4,2, /*also_block_here=*/true); // assimetria em relação a (6,0)

  auto moves = b.get_valid_moves();
  ASSERT_FALSE(moves.empty());

  // Procura a jogada com menor h_distance ao objetivo de MAX
  std::pair<int,int> argmin = moves.front();
  int best = AI::h_distance(b, argmin, /*is_max=*/true);
  for (auto& m : moves) {
    int d = AI::h_distance(b, m, /*is_max=*/true);
    if (d < best) { best = d; argmin = m; }
  }

  AI ai(/*is_max=*/true, /*max_depth=*/0);
  auto chosen = ai.choose_move(b, /*depth_override=*/0, /*rounds=*/1);

  EXPECT_EQ(chosen, argmin) << "Em profundidade 0, a IA deve escolher a jogada mais próxima do objetivo de MAX";
}

/* ---------------------------------
   4) Política parametrizada da primeira jogada para vários tamanhos
   --------------------------------- */
class AIFirstMoveSizes : public ::testing::TestWithParam<std::pair<int,int>> {};

TEST_P(AIFirstMoveSizes, FirstMove_NotAdjacentToOppGoal) {
  const auto [rows, cols] = GetParam();

  Board b(rows, cols);
  b.reset_board(rows, cols, /*block_initial=*/false);

  // Coloca o marcador perto do objetivo de MIN em (0, cols-1) para criar jogadas adjacentes.
  int start_r = std::min(1, rows - 2);
  int start_c = std::max(1, cols - 2);
  b.set_marker_pos(start_r, start_c, /*also_block_here=*/true);

  AI ai(/*is_max=*/true, /*max_depth=*/1);

  // Ativa a seleção especial da primeira jogada passando rounds=0
  auto chosen = ai.choose_move(b, /*depth_override=*/1, /*rounds=*/0);

  auto valids = b.get_valid_moves();
  ASSERT_FALSE(valids.empty());
  ASSERT_TRUE(std::find(valids.begin(), valids.end(), chosen) != valids.end())
      << "AI chose an invalid move on " << rows << "x" << cols;

  // Não pode ser adjacente (distância de Chebyshev > 1) ao objetivo do adversário (0, cols-1)
  const int opp_goal_r = 0, opp_goal_c = cols - 1;
  int d = cheb(chosen.first, chosen.second, opp_goal_r, opp_goal_c);
  EXPECT_GT(d, 1) << "AI first move is adjacent to opponent goal on "
                  << rows << "x" << cols << " from start (" << start_r << "," << start_c << ")";
}

INSTANTIATE_TEST_SUITE_P(
  Sizes_Extremes_And_Common,
  AIFirstMoveSizes,
  ::testing::Values(
    std::make_pair(5,5),
    std::make_pair(5,11),
    std::make_pair(11,5),
    std::make_pair(7,7),
    std::make_pair(9,9),
    std::make_pair(11,11)
  )
);

/* ---------------------------------
   5) Teste rápido do tabuleiro em vários tamanhos
   --------------------------------- */
class BoardSizesSmoke : public ::testing::TestWithParam<std::pair<int,int>> {};

TEST_P(BoardSizesSmoke, StartHasValidMoves) {
  const auto [rows, cols] = GetParam();
  Board b(rows, cols);
  auto moves = b.get_valid_moves();
  EXPECT_FALSE(moves.empty()) << "Sem jogadas válidas no estado inicial em " << rows << "x" << cols;
}

INSTANTIATE_TEST_SUITE_P(
  Sizes5to11,
  BoardSizesSmoke,
  ::testing::Values(
    std::make_pair(5,5),
    std::make_pair(6,6),
    std::make_pair(7,7),
    std::make_pair(8,8),
    std::make_pair(9,9),
    std::make_pair(10,10),
    std::make_pair(11,11)
  )
);
