// ============================================================================
// Board.cpp — Implementação do tabuleiro do jogo
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// - A grelha (grid) é uma matriz rows x cols de inteiros:
//     1  -> célula livre (pode ser visitada)
//     0  -> célula bloqueada (já visitada ou bloqueada manualmente)
// - As coordenadas usam o formato (r, c) = (linha, coluna), com origem em (0,0).
// - O "marker" representa a posição atual da peça partilhada pelos jogadores.
// - current_player = true  -> Jogador 1 (MAX / objetivo em (rows-1, 0))
//   current_player = false -> Jogador 2 (MIN / objetivo em (0, cols-1))
// - Objetivos (por omissão):
//     Jogador 1 vence ao chegar a (rows-1, 0).
//     Jogador 2 vence ao chegar a (0, cols-1).
// - Movimento permitido: 8 direções (N, S, E, O e diagonais).
// ----------------------------------------------------------------------------

#include "Board.hpp"
#include <iostream>
#include <queue>
#include <limits>
#include <random>

// Construtor com dimensões: cria tabuleiro rows x cols com marcador na posição padrão.
Board::Board(int r, int c) : rows(r), cols(c) {
    grid = std::vector<std::vector<int>>(rows, std::vector<int>(cols, 1));
    int row_coord = rows / 2 - 1;
    int col_coord = (cols % 2 == 0) ? cols / 2 : cols / 2 + 1;
    marker = {row_coord, col_coord};
    grid[marker.first][marker.second] = 0;
    current_player = true;
    init_zobrist();
    recompute_hash();
}

// Construtor que permite saltar o posicionamento/bloqueio inicial (para estados carregados).
Board::Board(int r, int c, bool skip_initial_marker) : rows(r), cols(c) {
    grid = std::vector<std::vector<int>>(rows, std::vector<int>(cols, 1));
    if (!skip_initial_marker) {
        int row_coord = rows / 2 - 1;
        int col_coord = (cols % 2 == 0) ? cols / 2 : cols / 2 + 1;
        marker = {row_coord, col_coord};
        grid[marker.first][marker.second] = 0;
    } else {
        marker = {0, 0};
    }
    current_player = true;
    init_zobrist();
    recompute_hash();
}

// ============================================================================
// REGRAS DE MOVIMENTO E TRANSIÇÕES DE ESTADO
// ============================================================================

std::vector<std::pair<int, int>> Board::get_valid_moves() const {
    // Retorna todas as jogadas válidas a partir da posição atual do marcador.
    std::vector<std::pair<int, int>> moves;
    int r = marker.first, c = marker.second;
    // 8 direções (ortogonais + diagonais)
    int dirs[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
    for (auto& d : dirs) {
        int nr = r + d[0], nc = c + d[1];
        // Dentro dos limites e casa livre?
        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols && grid[nr][nc] == 1) {
            moves.emplace_back(nr, nc);
        }
    }
    return moves;
}


void Board::make_move(std::pair<int, int> move) {
    int old_state = grid[marker.first][marker.second] ? 1 : 0;
    hash_value ^= zobrist_table[marker.first][marker.second][old_state];
    grid[marker.first][marker.second] = 0;
    hash_value ^= zobrist_table[marker.first][marker.second][0];
    hash_value ^= hash_marker_component();
    marker = move;
    hash_value ^= hash_marker_component();
}



bool Board::is_terminal() const {
    // O jogo termina se:
    // 1) não existirem jogadas válidas (o jogador da vez fica "bloqueado"), ou
    // 2) o marcador atingir o objetivo de J1 (rows-1, 0), ou
    // 3) o marcador atingir o objetivo de J2 (0, cols-1).
    return get_valid_moves().empty() ||
           marker == std::make_pair(rows-1, 0) ||
           marker == std::make_pair(0,cols-1);
}

int Board::get_winner() const {
    // Devolve um código de vencedor:
    // 0 -> jogo não terminou
    // 1 -> vitória do Jogador 1 por alcançar objetivo(rows-1, 0)
    // 2 -> vitória do Jogador 2 por alcançar objetivo (0, cols-1)
    // 3 -> vitória do adversário porque o jogador atual ficou bloqueado
    // 6 -> (variante) idem acima, mas distinguindo tipo de vitória para análise

    if (!is_terminal()) return 0;

    if (marker == std::make_pair(rows-1, 0)) return 1;  // Player 1 wins
    if (marker == std::make_pair(0,cols-1)) return 2;  // Player 2 wins

    // Bloqueado:: jogador atual não pode jogar → adversário ganha
    return current_player ? 3 : 6; //retorna 3 e 6 para distinguir o jogador bloqueado
}

std::pair<int, int> Board::get_marker() const {
    return marker;
}

void Board::switch_player() {
    // Alterna a vez entre Jogador 1 e 2.
    current_player = !current_player;
}

bool Board::current_player_is_human() const {
    return !current_player; // humano é o Jogador 2.
}


//nota:mudar nome das variáveis h1 e h5 que perderam sentido
Board::ReachabilityResult Board::compute_distance() const {

    // Faz uma pesquisa em largura (BFS) a partir da posição do marcador
    // • h1: distância mínima até ao objetivo de MAX (canto inferior esquerdo)
    // • h5: distância mínima até ao objetivo de MIN (canto superior direito)
    // • count: nº total de casas alcançáveis a partir da posição atual -> para calcular paridade
    //
    // Se um objetivo não for alcançável, devolve 900 como flag. 
    // • h1 é devolvido como valor NEGATIVO (para facilitar perspetiva de MAX).
    // • h5 é devolvido como valor POSITIVO (para facilitar perspetiva de MIN).

    // Matriz de células visitadas
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));

    // Fila para BFS: cada elemento contém (posição, distância)
    std::queue<std::pair<std::pair<int, int>, int>> q;
    q.push({marker, 0});
    visited[marker.first][marker.second] = true;

    //objetivos
    int goal_r_max = rows - 1, goal_c_max = 0;
    int goal_r_min = 0, goal_c_min = cols - 1;

    int h1 = 900, h5 = 900;
    int count = 0;

    // Direções de movimento (8 vizinhos: ortogonais + diagonais)
    int dirs[8][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };


    // BFS
    while (!q.empty()) {
        auto [pos, dist] = q.front(); q.pop();
        int r = pos.first, c = pos.second;
        count++;
        // Se atingiu o objetivo de MAX, atualiza h1
        if (r == goal_r_max && c == goal_c_max) h1 = std::min(h1, dist);
        // Se atingiu o objetivo de MIN, atualiza h5
        if (r == goal_r_min && c == goal_c_min) h5 = std::min(h5, dist);

        // Explora vizinhos válidos
        for (auto& d : dirs) {
            int nr = r + d[0], nc = c + d[1];
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
                grid[nr][nc] == 1 && !visited[nr][nc]) {
                visited[nr][nc] = true;
                q.push({{nr, nc}, dist + 1});
            }
        }
    }

    return {
        h1 == 900 ? -900 : -h1,
        h5 == 900 ?  900 :  h5,
        count
    };
}



// ============================================================================
// HELPERS (WASM/UI) PARA REINICIALIZAÇÃO E EDIÇÃO DE ESTADO
// ============================================================================

void Board::reset_board(int r, int c, bool block_initial) {
    rows = r;
    cols = c;
    grid.assign(rows, std::vector<int>(cols, 1));

    int row_coord = rows / 2 - 1;
    int col_coord = (cols % 2 == 0) ? cols / 2 : cols / 2 + 1;
    marker = { row_coord, col_coord };

    if (block_initial) {
        grid[marker.first][marker.second] = 0;
    }

    current_player = true;
    init_zobrist();
    recompute_hash();
}

void Board::set_marker_pos(int r, int c, bool also_block_here) {
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        hash_value ^= hash_marker_component();
        marker = { r, c };
        hash_value ^= hash_marker_component();
        if (also_block_here && grid[r][c] == 1) {
            hash_value ^= zobrist_table[r][c][1];
            grid[r][c] = 0;
            hash_value ^= zobrist_table[r][c][0];
        }
    }
}

void Board::block_cell(int r, int c) {
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        if (grid[r][c] == 1) {
            hash_value ^= zobrist_table[r][c][1];
            grid[r][c] = 0;
            hash_value ^= zobrist_table[r][c][0];
        }
    }
}

void Board::set_current_player_from_int(int player) {
    current_player = (player == 1);
}

void Board::init_zobrist() {
    zobrist_table.assign(rows, std::vector<std::array<uint64_t,2>>(cols));
    std::mt19937_64 gen(0xBADC0FFEEULL ^ (static_cast<uint64_t>(rows) << 32) ^ static_cast<uint64_t>(cols));
    std::uniform_int_distribution<uint64_t> dist;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            zobrist_table[r][c][0] = dist(gen);
            zobrist_table[r][c][1] = dist(gen);
        }
    }
    hash_value = 0;
}

uint64_t Board::hash_marker_component() const {
    uint64_t h = (static_cast<uint64_t>(marker.first) << 32) ^ static_cast<uint64_t>(marker.second);
    h ^= zobrist_marker_magic;
    return h;
}

void Board::recompute_hash() {
    hash_value = 0;
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int cell = grid[r][c] ? 1 : 0;
            hash_value ^= zobrist_table[r][c][cell];
        }
    }
    hash_value ^= hash_marker_component();
}

std::vector<std::vector<int>> Board::get_grid() const {
    // Devolve uma cópia da grelha (por valor) para consumo em JS/WASM.
    // NOTA: poder-se-ia otimizar com vistas (views) se necessário.
    //std::cout << "[C++] get_grid() returning size: " << grid.size() << std::endl;
    return grid;
}

//getter para iluminar as jogadas válidas no tabuleiro
std::vector<int> Board::get_flat_valid_moves() const {
    auto moves = get_valid_moves();
    std::vector<int> flat;
    flat.reserve(static_cast<size_t>(moves.size() * 2));
    for (const auto& mv : moves) {
        flat.push_back(mv.first);
        flat.push_back(mv.second);
    }
    return flat;
}

std::vector<int> Board::get_flat_grid() const {
    std::vector<int> flat;
    flat.reserve(static_cast<size_t>(rows * cols));
    for (const auto& row : grid) {
        flat.insert(flat.end(), row.begin(), row.end());
    }
    return flat;
}

std::vector<int> Board::get_marker_flat() const {
    return {marker.first, marker.second};
}



Board::MoveUndo Board::apply_move(const Move& mv) {
    MoveUndo u;
    u.old_marker = marker;
    u.old_cell_free = (grid[marker.first][marker.second] == 1);
    u.old_current_player = current_player;
    u.old_hash = hash_value;

    make_move(mv);        // atualiza grid/marker/hash
    switch_player();      // alterna jogador

    return u;
}

void Board::undo_move(const MoveUndo& u) {
    // Restaurar jogador e marcador
    current_player = u.old_current_player;
    marker = u.old_marker;

    // Restaurar estado da célula antiga do marcador
    grid[u.old_marker.first][u.old_marker.second] = u.old_cell_free ? 1 : 0;

    // Restaurar hash pré-movimento
    hash_value = u.old_hash;
}



// ============================================================================
// NOTAS - > a fazer
// ----------------------------------------------------------------------------
// - Validar ‘make_move’ quando em modo debug (assert que o movimento é válido).
// - Considerar armazenar os objetivos como constantes ou parâmetros configuráveis.
// - Em puzzles, garantir consistência entre bloqueios pré-carregados e marker.
// - Implementar  método “undo” (stack de estados).
// - Otimizações: marcar limites e livre/bloqueado com bitsets para memória.
// ============================================================================
