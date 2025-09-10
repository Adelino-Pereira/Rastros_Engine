#pragma once
#include "Board.hpp"
#include "AI.hpp"
#include <string>
#include <vector>
#include <utility>

class GameController {
public:
    int rounds = 0;
    int start_depth = 2;
    int max_depth = start_depth;
public:
    GameController(const std::string& mode, int rows, int cols);
    GameController(const std::string& mode, int rows, int cols, const Board& board,int rounds);

    void run();

    // Accessors used by the WebAssembly bindings
    int get_rows() const { return board.rows; }
    int get_cols() const { return board.cols; }
    int get_current_player() const { return board.current_player ? 1 : 2; }
    //int get_grid_ptr() const { return reinterpret_cast<int>(&board.grid[0][0]); }
    bool is_game_over() const { return board.is_terminal(); }
    void make_move(int row, int col) { board.make_move({row, col}); }
    void run_ai_turn();
    std::vector<int> get_flat_grid();
    std::pair<int, int> get_marker();
    std::vector<std::pair<int, int>> get_valid_moves();


private:
    Board board;
    AI ai_player;
    AI ai_player_2;
    std::string mode;
    //std::vector<std::pair<int, int>> initial_moves;

    void play_ai_turn();
    void play_human_turn();
    void play_ai_vs_ai_turn();
    void handle_terminal_state();
};