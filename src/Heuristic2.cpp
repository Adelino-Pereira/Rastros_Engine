#include "Heuristic2.hpp"
#include "HeuristicsUtils.hpp"
#include <iostream>
#include "AI.hpp" 

int heuristic2(const Board& board, bool is_max) {
    auto pos = board.get_marker();
    int r = AI::rounds();
    // std::cout << "rounds:"<< r<<"\n";
    // int h1 = board.shortest_path_to_goal( true);
    //  int h5 = board.shortest_path_to_goal(false);
     // std::cout <<is_max <<"("<<pos.first<<","<<pos.second<<") "<<h1<<"+"<<h5<<"\n";

    // int h3 = h_trap(board, is_max);
    // int h4 = check_corners(board,pos, is_max);
    
    // int h7 = quadrant_bonus(board,pos, is_max);
    // int h8a = parity_heuristic(board, is_max,h1,h5);

    
    // auto reach = board.compute_reachability();
    Board::ReachabilityResult reach = board.compute_reachability();

    int h1 = reach.h1;
    int h5 = reach.h5;
    // std::cout <<is_max <<"("<<pos.first<<","<<pos.second<<") "<<h1b<<"+"<<h5b<<"\n";


    int h8 = 0;
    if (std::abs(h1) == 900 && std::abs(h5) == 900) {
        h8 = (reach.reachable_count % 2 == 0)
             ? (is_max ? 200 : -200)
             : (is_max ? -200 : 200);
       // std::cout <<is_max<<" h1: "<<h1 << " h5: "<< h5<<" h8-> "<<h8<< " ("<<reach.reachable_count<<")\n";

    }

    // if (h8a > 0 || h8b > 0){
    //     std::cout << is_max <<"\n";
    //     std::cout << "h8(a): "<<h8a<<"\n";
    //     std::cout << "h8(b): "<<h8b<<"\n";
    // }
    
    
    int h9 = available_choices(board,is_max);
    //std::cout << "p2-h9: "<<h9<<"\n";
    //std::cout << "p1 -h1: "<<h1<<"-h5: "<<h5<<"\n";

    int dist = h_distance(board, pos, is_max);
    int o_dist = h_distance(board, pos, !is_max);

    // int bonus = 0;
    // if (dist == 1) bonus = is_max ? 800 : -800;

    int penalty = 0;
    if (dist + 2 < std::abs(h1) && std::abs(h1) != 900)
        penalty += -(std::abs(h1) - dist);

    if (o_dist + 2 < std::abs(h5) && std::abs(h5) != 900)
        penalty += (std::abs(h5) - o_dist);

   //  // dist = is_max ? dist : -dist;
   //  // o_dist = is_max? -o_dist: o_dist;
   //  // int hDiag = r <= 7? h_diag_block_goal(board) : 0;
   //  int hDiag = h_diag_block_goal(board);

   // std::cout <<"penalty "<<penalty<<"\n";
    //std::cout <<"("<<pos.first<<","<<pos.second<<") "<<h1 <<" + "<<h4<<" + "<< h5<<" + "<< h7<<" + "<< penalty<<" + "<< h8<<"\n";
    //std::cout <<is_max <<"("<<pos.first<<","<<pos.second<<") "<<h1<<"+"<<h5<<"+"<<h8<<"\n";
    //std::cout <<"("<<pos.first<<","<<pos.second<<") "<<h1<<"+"<<h5<<"+"<<penalty<<"+"<<bonus<<"\n";
    // std::cout <<is_max <<"("<<pos.first<<","<<pos.second<<") "<<h1<<"+"<<h5<<"\n";
    //std::cout <<is_max <<"("<<pos.first<<","<<pos.second<<") "<<h1<<"+"<<h5<<"+"<<h8<<"+"<<h9<<"\n";

    // return h1; // A
    // return h5; // B
    // return h1+h5; // C
    return h1+h5+h8; // D
    // return h1+h5+h; // E
    // return h1+h5+h8+h9; //F
    // return h1+h5+h8+hDiag; // G
    // return h1+h5+h9+hDiag; // H
    //return h1+h5+h8+h9+hDiag; //I
    // return h1+h5+hDiag; // J

    // return h5+h8+h9+hDiag; // N




}
