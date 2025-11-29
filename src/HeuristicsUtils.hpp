#ifndef HEURISTICS_UTILS_HPP
#define HEURISTICS_UTILS_HPP

#include "Board.hpp"


int h_trap(const Board& board, bool is_max);
int check_corners(const Board& board, std::pair<int, int> pos, bool is_max);
int quadrant_bonus(const Board& board, std::pair<int, int> pos, bool is_max);
int h_distance(const Board& board, std::pair<int, int> pos, bool is_max);
int parity_heuristic(const Board& board, bool is_max,int path_val, int o_path_val);
int count_unplayables(const Board& board);

int available_choices(const Board& board, bool is_max);

int h_diag_block_goal(const Board& b);

#endif
