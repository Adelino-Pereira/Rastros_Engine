#include "HeuristicsUtils.hpp"

int h_trap(const Board& board, bool is_max) {
    return board.get_valid_moves().size() <= 2 ? (is_max ? -5 : 5) : 0;
}

int available_choices(const Board& board, bool is_max){
    int choices = board.get_valid_moves().size();
    return is_max ? choices : -choices; 
}

int check_corners(const Board& board, std::pair<int, int> pos, bool is_max) {
    return ((pos == std::make_pair(0, 0)) || (pos == std::make_pair(board.get_rows() - 1, board.get_cols() - 1))) ?
           (is_max ? 10 : -10) : 0;
}

int quadrant_bonus(const Board& board, std::pair<int, int> pos, bool is_max) {
    int mid_row = board.get_rows() / 2;
    int mid_col = board.get_cols() / 2;
    if (is_max && pos.first > mid_row && pos.second < mid_col) return 10;
    if (!is_max && pos.first < mid_row && pos.second > mid_col) return -10;
    return 0;
}

int h_distance(const Board& board, std::pair<int, int> pos, bool is_max) {
    auto goal = is_max ? std::make_pair(board.get_rows() - 1, 0) : std::make_pair(0, board.get_cols() - 1);
    return std::max(std::abs(goal.first - pos.first), std::abs(goal.second - pos.second));
}

int count_unplayables(const Board& board) {
    int count = 0;
    for (const auto& row : board.grid_ref()) {
        for (int cell : row) {
            if (cell == 0) count++;
        }
    }
    return count;
}


int parity_heuristic(const Board& board, bool is_max,int path_val,int o_path_val) {
    if (std::abs(path_val) == 900 && std::abs(o_path_val) == 900) {
        int remaining = board.get_rows() * board.get_cols() - count_unplayables(board);
        //std::cout << "remaing: " << remaining<<"\n";
        return (remaining % 2 == 0) ? (is_max ? -200 : 200)
                                    : (is_max ? 200 : -200);

    }
    return 0;
}

int h_diag_block_goal(const Board& b) {
    if (b.is_terminal()) return 0;
    // Uses your one-BFS reachability to know distances from *current position*
    auto reach = b.compute_reachability();

    // Whose turn?
    const bool max_to_move = b.current_player_is_max();

    // “Owner can win next move?” -> then don't penalize their diagonal even if blocked
    const bool max_can_win_next =  max_to_move && (std::abs(reach.h1) == 1);
    const bool min_can_win_next = !max_to_move && (std::abs(reach.h5) == 1);

    const int R = b.get_rows(), C = b.get_cols();

    // Your goals per Board.cpp:
    //  - MAX goal at (R-1, 0)  → inboard diagonal: (R-2, 1)
    //  - MIN goal at (0,   C-1)→ inboard diagonal: (1,   C-2)
    const int rMax = R - 2, cMax = 1;
    const int rMin = 1,     cMin = C - 2;

    auto in_bounds = [&](int r, int c){
        return r >= 0 && r < R && c >= 0 && c < C;
    };

    // Tunable weight (start modest)
    const int P = 40;

    int score = 0; // MAX-perspective: + is bad for MAX, - is bad for MIN

    // MAX diagonal blocked? Penalize MAX unless MAX wins next.
    const auto& g = b.grid_ref();
    if (in_bounds(rMax, cMax) && g[rMax][cMax] == 0) {
        if (!max_can_win_next) score -= P;
    }

    // MIN diagonal blocked? Penalize MIN unless MIN wins next.
    if (in_bounds(rMin, cMin) && g[rMin][cMin] == 0) {
        if (!min_can_win_next) score += P;
    }

    return score;
}
