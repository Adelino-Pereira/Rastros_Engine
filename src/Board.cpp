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

Board::Board() {
    // Cria um tabuleiro com dimensões padrão (rows x cols) inicializadas
    // na classe e bloqueia a casa inicial do marcador.
    grid = std::vector<std::vector<int>>(rows, std::vector<int>(cols, 1));

    // Debug: início fixo
    // grid[2][4] = 0; // bloquear casa
    // marker = {2, 4}; // colocar marcador

    // colocação dinâmica da primeira peça, consoante o tamanho do tabuleiro
    marker = {rows / 2-1, cols / 2+1};  // posição inicial
    grid[marker.first][marker.second] = 0;  // bloquear casa
    current_player = true; // definir primeiro jogador
}

Board::Board(int r, int c) : rows(r), cols(c) {
    // Construtor com dimensões explícitas
    grid = std::vector<std::vector<int>>(rows, std::vector<int>(cols, 1));
    
    int row_coord = rows / 2-1;
    int col_coord = cols % 2 == 0 ? cols / 2 : cols / 2+1;
    
    marker = {row_coord,col_coord};
     
    grid[marker.first][marker.second] = 0;  // block start
    current_player = true;
}

Board::Board(int r, int c, bool skip_initial_marker) : rows(r), cols(c) {
    // Variante Construtor que permite ignorar o bloqueio inicial (puzzles)

    grid = std::vector<std::vector<int>>(rows, std::vector<int>(cols, 1));
    if (!skip_initial_marker) {
        int row_coord = rows / 2 - 1;
        int col_coord = cols % 2 == 0 ? cols / 2 : cols / 2 + 1;
        marker = {row_coord, col_coord};
        grid[marker.first][marker.second] = 0;
    } 
    // Se skip_initial_marker = true, a posição do marcador é definida
    // posteriormente  (via set_marker_pos) e bloqueios.

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
    // Executa uma jogada: a casa atual fica bloqueada e o marcador avança.
    // INVARIANTE: assume-se que 'move' é válido. A validação deve ser feita
    // a montante (UI/AI) para evitar estados inconsistentes.
    grid[marker.first][marker.second] = 0;
    marker = move;
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
Board::ReachabilityResult Board::compute_reachability() const {

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



//Deprecated -> ainda está a ser usado no nível 5, 
//mas é para ser substituido por compute_reachability()
int Board::shortest_path_to_goal(bool for_max) const {
    // Calcula, via BFS, a distância mínima do marcador até o objetivo do
    // jogador especificado (MAX/J1 ou MIN/J2). Retorna distâncias com sinal:
    // - Para MAX (J1), retorna -dist (quanto menor a distância, melhor para MAX).
    // - Para MIN (J2), retorna +dist (quanto menor a distância, melhor para MIN).
    //
    // Se o objetivo for inalcançável (devido a bloqueios), devolve um valor
    // penalizador grande (-900 para MAX, +900 para MIN).
    // std::cout<<"for_max<<"<< for_max<<"\n";
    int goal_r = for_max ? rows - 1 : 0;
    int goal_c = for_max ? 0 : cols - 1;

   
    std::vector<std::vector<bool>> visited(rows, std::vector<bool>(cols, false));
    std::queue<std::pair<std::pair<int, int>, int>> q;
    q.push({marker, 0});
    visited[marker.first][marker.second] = true;

    int dirs[8][2] = {
        {-1, 0}, {1, 0}, {0, -1}, {0, 1},
        {-1, -1}, {-1, 1}, {1, -1}, {1, 1}
    };

    while (!q.empty()) {
        auto [pos, dist] = q.front(); q.pop();

        if (pos.first == goal_r && pos.second == goal_c)
            return for_max ? -dist : dist;

        for (auto& d : dirs) {
            int nr = pos.first + d[0];
            int nc = pos.second + d[1];

            // Use rows and cols for bounds
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols &&
                grid[nr][nc] == 1 && !visited[nr][nc]) {
                visited[nr][nc] = true;
                q.push({{nr, nc}, dist + 1});
            }
        }
    }

    // Return large penalty if unreachable
    return for_max ? -900 : 900;
}

// ============================================================================
// HELPERS (WASM/UI) PARA REINICIALIZAÇÃO E EDIÇÃO DE ESTADO
// ============================================================================

void Board::reset_board(int r, int c, bool block_initial) {
    // Reinicia o tabuleiro para novas dimensões (r x c).
    rows = r; cols = c;
    grid.assign(rows, std::vector<int>(cols, 1));
    
    // Posiciona marcador inicial (mesmo que nos construtores).
    int row_coord = rows / 2 - 1;
    int col_coord = (cols % 2 == 0) ? cols / 2 : cols / 2 + 1;
    marker = { row_coord, col_coord };

    // Opcionalmente bloqueia a célula inicial.
    if (block_initial) grid[marker.first][marker.second] = 0;

    current_player = true; // Player 1 por omissão
}

void Board::set_marker_pos(int r, int c, bool also_block_here) {
    // Coloca o marcador em (r, c). Opcionalmente bloqueia essa célula.
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        marker = { r, c };
        if (also_block_here) grid[r][c] = 0;
    }
    // Caso contrário, ignora silenciosamente — pode-se lançar exceção
}

void Board::block_cell(int r, int c) {
    // Bloqueia explicitamente uma casa (para carregar puzzles/estados).
    if (r >= 0 && r < rows && c >= 0 && c < cols) {
        grid[r][c] = 0;
    }
}

void Board::set_current_player_from_int(int player) {
    // No motor: true = Jogador 1, false = Jogador 2.
    // Este helper permite receber 1/2 da UI/JS e mapear internamente.
    current_player = (player == 1);
}

std::vector<std::vector<int>> Board::get_grid() const {
    // Devolve uma cópia da grelha (por valor) para consumo em JS/WASM.
    // NOTA: poder-se-ia otimizar com vistas (views) se necessário.
    //std::cout << "[C++] get_grid() returning size: " << grid.size() << std::endl;
    return grid;
}

//getter para iluminar as jogadas válidas no tabuleiro
std::vector<std::vector<int>> Board::get_valid_moves_js() const {
    // Adapta o formato C++ (vector<pair<int,int>>) para um array 2D amigável
    // para a ponte JS/WASM.
    auto moves = get_valid_moves();
    std::vector<std::vector<int>> result;
    for (const auto& [r, c] : moves) {
        result.push_back({r, c});
    }
    return result;
}


void Board::print() const {
    // Impressão  do tabuleiro no terminal -> testes locais.
    std::cout << "\nTabuleiro:\n";
    for (int r = 0; r < rows; ++r) {
        std::cout << r << "|";
        for (int c = 0; c < cols; ++c) {
            if (marker.first == r && marker.second == c)
                std::cout << "M "; // posição do marcador
            else if (grid[r][c] == 0)
                std::cout << "· "; // casa bloqueada/visitada
            else
                std::cout << "1 "; // casa livre
        }
        std::cout << "\n";
    }
    // Eixo das colunas
    std::cout << "  ";
    for (int c = 0; c < cols*2-1; ++c)
        std::cout << "-";
    std::cout << "\n";
    std::cout << "  ";
    for (int c = 0; c < cols; ++c)
        std::cout << c << " ";
    std::cout << "\n";
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
