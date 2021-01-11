#ifndef GAME_H
#define GAME_H

#include <stddef.h> 
#include <stdbool.h> 

#include "pcg_basic.h"

typedef int card_t;

struct pile {
    enum pile_type {
        UP_PILE,
        DOWN_PILE,
        TALON,
        HAND
    } type;
    card_t* cards;
    size_t card_count;
};

struct player;
struct game;

struct move {
    card_t card;
    size_t pile_index;
};

typedef void (*playfn_t)(struct player*, struct game*);

struct player {
    char name[16];
    struct pile hand;
    playfn_t play;
};

struct game {
    enum game_state {
        RUNNING, WON, LOST
    } state;
    struct pile* piles;
    size_t pile_count;
    struct pile talon;
    card_t min_card, max_card;
    struct player* players;
    size_t player_count;
    size_t cur_player;
    size_t move_count;
    size_t req_hand_size;
    size_t req_move_count_full;
    size_t req_move_count_empty;
};

void swap_cards(card_t* a, card_t* b);

struct pile new_pile(enum pile_type type, size_t max_card_count);
void free_pile(struct pile* pile);
card_t last_card(const struct pile* pile);
bool can_place_card(struct pile* pile, card_t card);
void place_card(struct pile* pile, card_t card);
bool can_take_card(struct pile* pile);
card_t take_card(struct pile* pile);
size_t find_card(const struct pile* pile, card_t card);
bool contains_card(const struct pile* pile, card_t card);
void remove_card(struct pile* pile, size_t index);

struct game new_game(
    size_t up_pile_count,
    size_t down_pile_count,
    size_t req_hand_size,
    size_t req_move_count_full,
    size_t req_move_count_empty,
    card_t min_card, card_t max_card,
    pcg32_random_t* rnd);
void free_game(struct game* game);
void reset_game(struct game* game, pcg32_random_t* rnd);
void display_game(const struct game* game);

void add_player(struct game* game, playfn_t play);
void add_default_player(struct game* game);

void apply_move(struct game* game, struct player* player, const struct move* move);
bool has_enough_moves(const struct game* game);
enum game_state play_turn(struct game* game);

#endif
