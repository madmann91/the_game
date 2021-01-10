#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

typedef int card_t;

struct pile {
    enum pile_type {
        UP_PILE,
        DOWN_PILE,
        TALON
    } type;
    card_t* cards;
    size_t card_count;
};

struct game {
    struct pile* piles;
    size_t pile_count;
    struct pile talon;
    card_t min_card, max_card;
};

// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;
uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

static inline struct pile new_pile(enum pile_type type, card_t min_card, card_t max_card) {
    return (struct pile) {
        .type = type,
        .cards = malloc(sizeof(card_t) * (max_card - min_card)),
        .card_count = 0
    };
}

static inline void free_pile(struct pile* pile) {
    free(pile->cards);
    pile->cards = NULL;
}

static inline card_t last_card(struct pile* pile) {
    assert(pile->card_count > 0);
    return pile->cards[pile->card_count - 1];
}

static inline void place_card(struct pile* pile, card_t card) {
    assert(pile->type == UP_PILE || pile->type == DOWN_PILE);
    assert(pile->card_count == 0 || (pile->type == UP_PILE ? card > last_card(pile) : card < last_card(pile)));
    pile->cards[pile->card_count++] = card;
}

static inline struct game new_game(size_t up_pile_count, size_t down_pile_count, card_t min_card, card_t max_card) {
    struct game game;
    game.pile_count = up_pile_count + down_pile_count;
    game.piles = malloc(sizeof(struct pile) * game.pile_count);
    for (size_t i = 0; i < up_pile_count; ++i) {
        game.piles[i] = new_pile(UP_PILE, min_card, max_card);
        place_card(&game.piles[i], min_card);
    }
    for (size_t i = 0; i < down_pile_count; ++i) {
        game.piles[up_pile_count + i] = new_pile(DOWN_PILE, min_card, max_card);
        place_card(&game.piles[up_pile_count + i], max_card);
    }
    game.talon = new_pile(TALON, min_card, max_card);
    //shuffle_cards(&game.talon, min_card, max_card, seed);
    return game;
}

static inline void free_game(struct game* game) {
    for (size_t i = 0; i < game->pile_count; ++i)
        free_pile(&game->piles[i]);
    free(game->piles);
    game->piles = NULL;
    game->pile_count = 0;
    free_pile(&game->talon);
}

static inline void display_game(const struct game* game) {
    for (size_t i = 0; i < game->pile_count; ++i)
        printf("%3d ", last_card(&game->piles[i]));
    printf("\n");
    for (size_t i = 0; i < game->pile_count; ++i)
        printf(game->piles[i].type == UP_PILE ? " ^  " : " v  ");
    printf("\n");
}

int main() {
    struct game game = new_game(2, 2, 1, 100);
    display_game(&game);
    free_game(&game);
    return 0;
}
