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
#ifndef BOARD_HPP
#define BOARD_HPP

#pragma once
#include <vector>
#include <utility>
#include <array>
#include <cstdint>

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

    struct ReachabilityResult {
        int h1;              // distância (com sinal) para objetivo de MAX
        int h5;              // distância (com sinal) para objetivo de MIN
        int reachable_count; // nº de casas alcançáveis
    };

    struct MoveUndo {
        int old_r, old_c;
        int new_r, new_c;
        bool old_cell_free;
        bool old_current_player;
        std::uint64_t old_hash;
    };

    MoveUndo apply_move(const std::pair<int,int>& mv);
    void undo_move(const MoveUndo& u);

    /**
     * Construtor por omissão.
     * Inicializa um tabuleiro com dimensões padrão e posiciona o marcador
     * numa célula inicial, bloqueando-a conforme a regra do jogo.
     */
    Board();
    /** 
     * Construtor com dimensões explícitas.
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

    //Lista as jogadas válidas (até 8) a partir da posição atual.
    std::vector<std::pair<int, int>> get_valid_moves() const;

    void make_move(std::pair<int, int> move);
    bool is_terminal() const;
    int get_winner() const;

    std::pair<int, int> get_marker() const;
    bool current_player_is_human() const;
    ReachabilityResult compute_reachability() const;

    void switch_player();
   
    bool current_player_is_max() const { return current_player; }

    uint64_t get_hash() const { return hash_value; }
    
    std::vector<std::vector<int>> get_grid() const;
    int get_rows() const { return rows; }
    int get_cols() const { return cols; }
    //const std::vector<std::vector<int>>& grid_ref() const { return grid; }

    bool is_cell_free(int r, int c) const;


    // // Helpers WASM/UI
    // ------------------------------------------------------------------------
    //métodos para auxilia a UI a reconfigurar rapidamente o tabuleiro
    
    //Reinicia o tabuleiro para dimensões (r x c).
    //Se true, bloqueia também a célula inicial do marcador.
    void reset_board(int r, int c, bool block_initial = true);

    // Define a posição do marcador.
    // Se true, bloqueia também essa célula.
    // Útil para reconstruir estados ou injetar posições de puzzle.

    void set_marker_pos(int r, int c, bool also_block_here = false);
    
    // Bloqueia explicitamente a célula (r,c).
    // Útil para carregar estados/puzzles em que certas casas já estão
    // ocupadas/visitadas.
    void block_cell(int r, int c);

    // Define o jogador atual a partir de inteiro vindo da UI.
    // @param player Valor 1 (Jogador 1/MAX) ou 2 (Jogador 2/MIN).
    // Internamente converte para bool conforme a convenção do motor.
    void set_current_player_from_int(int player); // 1 ou 2

    //getters para WASM/JS - devolvem arrays "flat" (1D) para facilitar o consumo na UI
    
    std::vector<int> get_flat_valid_moves() const;
    std::vector<int> get_flat_grid() const;
    std::vector<int> get_marker_flat() const; // [r,c]


private:
    int rows = 7;
    int cols = 7;
    std::pair<int, int> marker;
    bool current_player = true; // true para J1, false para J2
    std::vector<std::vector<int>> grid;
    using Move = std::pair<int,int>;


    // Representação compacta da grelha:
    //   vector<uint64_t> com bits:
    //     bit = 1 → célula livre
    //     bit = 0 → célula bloqueada
    //
    // - A grelha é armazenada linha a linha.
    //   Cada linha usa "words_per_row" elementos de 64 bits.
    int words_per_row = 0;
    std::vector<std::uint64_t> grid_bits;

    // Helpers para índice/bit
    inline bool is_inside(int r, int c) const {
        return (r >= 0 && r < rows && c >= 0 && c < cols);
    }

    inline std::size_t bit_index(int r, int c) const {
        // use size_t to avoid overflow and sign issues
        return static_cast<std::size_t>(r) * static_cast<std::size_t>(words_per_row)
             + static_cast<std::size_t>(c >> 6);   // c / 64
    }

    inline std::uint64_t bit_mask(int c) const {
        return std::uint64_t{1} << (c & 63); // c % 64
    }

    inline bool is_free(int r, int c) const;
    inline void set_free(int r, int c, bool free);

    // -----------------------------------------------------------------
    // Zobrist hashing
    // -----------------------------------------------------------------
    std::vector<std::vector<std::array<std::uint64_t, 2>>> zobrist_table;
    std::uint64_t hash_value = 0;
    static constexpr std::uint64_t zobrist_marker_magic =
        0x9E3779B97F4A7C15ULL; // constante arbitrária

    void init_zobrist();
    std::uint64_t hash_marker_component() const;
    void recompute_hash();

};

#endif // BOARD_HPP