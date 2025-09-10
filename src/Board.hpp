// ============================================================================
// Board.hpp — Interface do tabuleiro do jogo
// ----------------------------------------------------------------------------
// expõe a API principal do motor C++ que será compilado para
// WebAssembly e consumido por uma UI em React/Vite.
// 
// - Grelha (grid) de tamanho rows x cols com inteiros:
//     1 -> célula livre; 0 -> célula bloqueada/visitada.
// - Coordenadas: (r, c) com origem em (0,0) no canto superior esquerdo.
// - O "marker" é a posição atual compartilhada pelos dois jogadores.
// - Jogador 1 (MAX) vence ao alcançar (rows-1, 0).
//   Jogador 2 (MIN) vence ao alcançar (0, cols-1).
// - Movimento permitido em 8 direções (ortogonais e diagonais).
// - Em regra, a célula é bloqueada quando se sai dela (não quando se entra).
// ============================================================================

#pragma once
#include <vector>
#include <utility>

/** 
 * @class Board
 * Representa o estado do tabuleiro e regras básicas do jogo.
 *
 * Responsabilidades:
 * - Manter a grelha (livre/bloqueado), posição do marcador e jogador atual.
 * - Gerar jogadas válidas e aplicar transições de estado (make_move/switch).
 * - Detetar estados terminais e determinar vencedor/resultado.
 * - Fornecer métricas para heurísticas (shortest_path).
 *
 * Integração WASM/JS:
 * - Métodos auxiliares expõem estruturas para JS (arrays 2D, etc.).
 * - Os helpers de "reset" e "set_marker_pos" úteis para puzzles.
 */
class Board {

public:
    /**
     * Construtor por omissão.
     * Inicializa um tabuleiro com dimensões padrão e posiciona o marcador
     * numa célula inicial, bloqueando-a conforme a regra do jogo.
     */
    Board();
    /** 
     * Construtor com dimensões explícitas.
     * @param rows Número de linhas.
     * @param cols Número de colunas.
     * Gera uma grelha rows x cols e posiciona o marcador no arranque.
     */
    Board(int rows, int cols);
    /**
     * Construtor que permite ignorar o bloqueio/posicionamento inicial.
     * @param r Linhas; @param c Colunas.
     * @param skip_initial_marker Se true, o chamador deverá definir o marcador
     *        e bloqueios manualmente (útil para carregar puzzles/estados).
     */
    Board(int r, int c, bool skip_initial_marker);

    /**
     * Imprime o tabuleiro no terminal (texto), útil para debugging local.
     */
    void print() const;

    /**
     * Lista as jogadas válidas (até 8) a partir da posição atual.
     * @return Vetor de pares (r,c) alcançáveis numa jogada.
     */
    std::vector<std::pair<int, int>> get_valid_moves() const;

    void make_move(std::pair<int, int> move);
    bool is_terminal() const;
    void switch_player();
    bool current_player_is_human() const;

    int shortest_path_to_goal(bool for_max) const;

    std::vector<std::vector<int>> grid;
    std::pair<int, int> marker;
    bool current_player; // true para J1, false para J2

    std::pair<int, int> get_marker() const; // return marker;

    //added for wasm
    std::vector<std::vector<int>> get_grid() const;
    std::vector<std::vector<int>> get_valid_moves_js() const;

    int get_winner() const;

    int rows = 7;
    int cols = 7;

    struct ReachabilityResult {
        int h1;
        int h5;
        int reachable_count;
    };

    ReachabilityResult compute_reachability() const;


    // Helpers para lidar com posições a partir do JS
    // ------------------------------------------------------------------------
    // Estes métodos auxiliam a UI em React/Vite (via WASM) para reconfigurar
    // rapidamente o tabuleiro (ex.: trocar dimensões, carregar puzzles, etc.).
    
    /**
     * Reinicia o tabuleiro para dimensões (r x c).
     * @param r Linhas; @param c Colunas.
     * @param block_initial Se true, bloqueia também a célula inicial do marcador.
     */
    void reset_board(int r, int c, bool block_initial = true);
    /**
     * Define a posição do marcador.
     * @param r Linha; @param c Coluna.
     * @param also_block_here Se true, bloqueia também essa célula.
     * Útil para reconstruir estados ou injetar posições de puzzle.
     */
    void set_marker_pos(int r, int c, bool also_block_here = false);
    /**
     * Bloqueia explicitamente a célula (r,c).
     * Útil para carregar estados/puzzles em que certas casas já estão
     *       ocupadas/visitadas.
     */
    void block_cell(int r, int c);
        /**
     * Define o jogador atual a partir de inteiro vindo da UI.
     * @param player Valor 1 (Jogador 1/MAX) ou 2 (Jogador 2/MIN).
     * Internamente converte para bool conforme a convenção do motor.
     */
    void set_current_player_from_int(int player); // 1 ou 2



};