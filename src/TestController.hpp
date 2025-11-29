#pragma once
#include "Board.hpp"
#include "AI.hpp"
#include "Heuristic1.hpp"
#include <string>
#include <vector>
#include <utility>

class TestController {
public:
    void configure_ordering(OrderingPolicy pMax,
                            OrderingPolicy pMin,
                            double sigmaMax = 0.75,
                            double sigmaMin = 0.75,
                            bool shuffleTiesOnly = false);

    TestController(const std::string& mode, int rows, int cols, std::pair<int, int>  first_move,int debug,
                   HeuristicCombo combo_p1 = HeuristicCombo::A,
                   HeuristicCombo combo_p2 = HeuristicCombo::C);
    TestController(const std::string& mode, int rows, int cols, int debug,
                   HeuristicCombo combo_p1 = HeuristicCombo::A,
                   HeuristicCombo combo_p2 = HeuristicCombo::C);
    bool run(int mode);

    int get_rows() const { return board.get_rows(); }
    int get_cols() const { return board.get_cols(); }
    int get_current_player() const { return board.current_player_is_max() ? 1 : 2; }
    bool is_game_over() const { return board.is_terminal(); }
    void make_move(int row, int col) { board.make_move({row, col}); }
    void run_ai_turn();
    std::pair<int, int> get_marker();
    std::vector<std::pair<int, int>> get_valid_moves();

    void configure_quiescence(bool max_on, bool min_on,
                          int max_plies = 4, int swing_delta = 2, int low_mob = 2);

    void set_depth_limits(int start, int max);
    int get_start_depth() const { return start_depth; }
    int get_max_depth() const { return max_depth; }
    int get_rounds() const { return rounds; }
    bool get_winner() const { return winner; }

private:
    int rounds = 0;
    int start_depth = 9;
    int max_depth = 15;
    bool winner = false;
    std::pair<int, int>  first_move;
    HeuristicCombo combo_p1;
    HeuristicCombo combo_p2;

    Board board;
    AI ai_player;
    AI ai_player_2;
    std::string mode;

    void play_ai_turn();
    void play_human_turn();
    void play_ai_vs_ai_turn_mode1();
    void play_ai_vs_ai_turn_mode2();
    int compute_depth() const;
    std::pair<int, int> select_ai_move(bool allow_first_override);
    bool handle_terminal_state();
};
