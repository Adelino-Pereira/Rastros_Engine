#pragma once
#include "Board.hpp"
#include "AI.hpp"
#include <string>
#include <vector>
#include <utility>

class GameController {
public:
    GameController(const std::string& mode, int rows, int cols);
    GameController(const std::string& mode, int rows, int cols, const Board& board,int rounds);

    void run();

    int get_rows() const { return board.get_rows(); }
    int get_cols() const { return board.get_cols(); }
    int get_current_player() const { return board.current_player_is_max() ? 1 : 2; }
    bool is_game_over() const { return board.is_terminal(); }
    void make_move(int row, int col) { board.make_move({row, col}); }
    void run_ai_turn();
    std::pair<int, int> get_marker();
    std::vector<std::pair<int, int>> get_valid_moves();


private:
    int rounds = 0;
    int start_depth = 2;
    int max_depth = 2;
    Board board;
    AI ai_player;
    AI ai_player_2;
    std::string mode;

    void print_board() const;
    void play_ai_turn();
    void play_human_turn();
    void play_ai_vs_ai_turn();
    void handle_terminal_state();
};
