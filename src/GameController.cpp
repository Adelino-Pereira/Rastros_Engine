#include "GameController.hpp"
#include <iostream>
#include <random>


GameController::GameController(const std::string& mode, int rows, int cols)
    : ai_player(true, max_depth),
      ai_player_2(false, max_depth),
      mode(mode),
      rounds(0),
      board(rows, cols) // initialize directly in the member initializer list
{
    if (mode == "ai_first") {
        ai_player = AI(true, max_depth);
    } else if (mode == "human_first") {
        ai_player_2 = AI(false, max_depth);
    } else if (mode == "ai_vs_ai") {
        ai_player = AI(true, max_depth);
        ai_player_2 = AI(false, max_depth);
    }
}


GameController::GameController(const std::string& mode, int rows, int cols, const Board& board, int rounds)
    : ai_player(true, max_depth),
      ai_player_2(false, max_depth),
      mode(mode),
      rounds(rounds),
      board(board)
{
    if (mode == "ai_first") {
        ai_player = AI(true, max_depth);
    } else if (mode == "human_first") {
        ai_player_2 = AI(false, max_depth);
    } else if (mode == "ai_vs_ai") {
        ai_player = AI(true, max_depth);
        ai_player_2 = AI(false, max_depth);
    }
}



void GameController::run() {
    while (!board.is_terminal()) {
        print_board();

        if (mode == "human_vs_human") {
            play_human_turn();
        } else if (mode == "human_first") {
            if (board.current_player_is_max()) play_human_turn();
            else play_ai_turn();
        } else if (mode == "ai_first") {
            if (board.current_player_is_max()) play_ai_turn();
            else play_human_turn();
        } else if (mode == "ai_vs_ai") {
            play_ai_vs_ai_turn();
        }

        board.switch_player();
        rounds++;
    }

    print_board();
    handle_terminal_state();
}

void GameController::play_ai_turn() {
    std::cout << "\n🤖 Turno do Jogador " << (board.current_player_is_max() ? "1" : "2") << " (IA)...\n";

    std::pair<int, int> move;

    auto& ai = board.current_player_is_max() ? ai_player : ai_player_2;
    int depth = std::min(start_depth + rounds / 5, max_depth);
    depth = (depth % 2 == 0) ? depth - 1 : depth;
    depth = std::max(depth, start_depth);
    move = ai.choose_move(board,depth,rounds);
    board.make_move(move);
}

void GameController::play_human_turn() {
    std::cout << "\n🧑 Turno do Jogador (Humano)...\n";
    auto moves = board.get_valid_moves();
    if (moves.empty()) {
        std::cout << "Sem jogadas válidas disponíveis.\n";
        return;
    }
    for (size_t i = 0; i < moves.size(); ++i) {
        std::cout << i << ": (" << moves[i].first << ", " << moves[i].second << ")\n";
    }
    int choice;
    std::cout << "Escolhe uma jogada: ";
    std::cin >> choice;
    if (choice >= 0 && static_cast<size_t>(choice) < moves.size()) {
        board.make_move(moves[choice]);
    }
}

void GameController::play_ai_vs_ai_turn() {
    std::cout << "\nTurno do ";
    std::cout << (board.current_player_is_max() ? "Jogador 1 (IA)" : "Jogador 2 (IA)") << "...\n";
    auto& ai = board.current_player_is_max() ? ai_player : ai_player_2;
    int depth = std::min(start_depth + rounds / 5, max_depth);
    depth = (depth % 2 == 0) ? depth - 1 : depth;
    depth = std::max(depth, start_depth);
    auto move = ai.choose_move(board, depth,rounds);

    board.make_move(move);
}

void GameController::print_board() const {
    std::cout << "\nTabuleiro:\n";
    for (int r = 0; r < board.get_rows(); ++r) {
        std::cout << r << "|";
        for (int c = 0; c < board.get_cols(); ++c) {
            auto mk = board.get_marker();
            if (mk.first == r && mk.second == c) {
                std::cout << "M "; // posição do marcador
            }
            // antes: else if (board.grid_ref()[r][c] == 0)
            else if (!board.is_cell_free(r, c)) {
                std::cout << "· "; // casa bloqueada/visitada
            } else {
                std::cout << "1 "; // casa livre
            }
        }
        std::cout << "\n";
    }

    std::cout << "  ";
    for (int c = 0; c < board.get_cols()*2-1; ++c)
        std::cout << "-";
    std::cout << "\n";

    std::cout << "  ";
    for (int c = 0; c < board.get_cols(); ++c)
        std::cout << c << " ";
    std::cout << "\n";
}


void GameController::handle_terminal_state() {
    auto mk = board.get_marker();
    if (mk == std::make_pair(board.get_rows()-1, 0)) {
        std::cout << "🎉 Vitória do Jogador 1!\n";
    } else if (mk == std::make_pair(0, board.get_cols()-1)) {
        std::cout << "🎉 Vitória do Jogador 2!\n";
    } else {
        std::cout << "⚠️ Sem jogadas válidas. Fim de jogo!\n";
    }
}

void GameController::run_ai_turn() {
    std::cout << "mode ->" << mode <<  std::endl;
    if ((mode == "ai_first" && board.current_player_is_max()) ||
        (mode == "human_first" && !board.current_player_is_max()) ||
        (mode == "ai_vs_ai")) {
        play_ai_turn();
        board.switch_player();
    } else {
        std::cout << "[Warning] Attempted AI turn when it's not AI's turn.\n";
    }
}


std::pair<int, int> GameController::get_marker() {
    return board.get_marker();
}

std::vector<std::pair<int, int>> GameController::get_valid_moves() {
    return board.get_valid_moves();
}
