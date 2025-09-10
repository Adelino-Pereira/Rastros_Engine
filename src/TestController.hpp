#pragma once
#include "Board.hpp"
#include "AI.hpp"
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


    int rounds = 0;
    int start_depth =9;
    int max_depth =15 ;
    bool winner;
    std::pair<int, int>  first_move;


    TestController(const std::string& mode, int rows, int cols, std::pair<int, int>  first_move,int debug);
    TestController(const std::string& mode, int rows, int cols, int debug);
    bool run(int mode);

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


    void configure_quiescence(bool max_on, bool min_on,
                          int max_plies = 4, int swing_delta = 2, int low_mob = 2);



private:
    Board board;
    AI ai_player;
    AI ai_player_2;
    std::string mode;
    //std::vector<std::pair<int, int>> initial_moves;

    void play_ai_turn();
    void play_human_turn();
    void play_ai_vs_ai_turn_mode1();
    void play_ai_vs_ai_turn_mode2();

    bool handle_terminal_state();
    std::pair<int, int> get_random_move(const std::vector<std::pair<int, int>>& moves);
};