#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "game.h"

struct pile new_pile(enum pile_type type, size_t max_card_count) {
    return (struct pile) {
        .type = type,
        .cards = malloc(sizeof(card_t) * max_card_count),
        .card_count = 0
    };
}

void free_pile(struct pile* pile) {
    free(pile->cards);
    pile->cards = NULL;
}

card_t last_card(const struct pile* pile) {
    assert(pile->card_count > 0);
    return pile->cards[pile->card_count - 1];
}

bool can_place_card(struct pile* pile, card_t card) {
    switch (pile->type) {
        case UP_PILE:
            return last_card(pile) == card + 10 || last_card(pile) < card;
        case DOWN_PILE:
            return last_card(pile) == card - 10 || last_card(pile) > card;
        default:
            return true;
    }
}

void place_card(struct pile* pile, card_t card) {
    assert(pile->card_count == 0 || can_place_card(pile, card));
    pile->cards[pile->card_count++] = card;
}

bool can_take_card(struct pile* pile) {
    return pile->card_count > 0;
}

card_t take_card(struct pile* pile) {
    assert(can_take_card(pile));
    return pile->cards[--pile->card_count];
}

void swap_cards(card_t* a, card_t* b) {
    card_t tmp = *a;
    *a = *b;
    *b = tmp;
}

size_t find_card(const struct pile* pile, card_t card) {
    for (size_t i = 0; i < pile->card_count; ++i) {
        if (pile->cards[i] == card)
            return i;
    }
    return SIZE_MAX;
}

bool contains_card(const struct pile* pile, card_t card) {
    return find_card(pile, card) != SIZE_MAX;
}

void remove_card(struct pile* pile, size_t index) {
    assert(index < pile->card_count);
    pile->cards[index] = pile->cards[--pile->card_count];
}

static void shuffle_cards(struct pile* pile, card_t min_card, card_t max_card, pcg32_random_t* rnd) {
    pile->card_count = 0;
    for (card_t card = min_card + 1; card < max_card; ++card)
        pile->cards[pile->card_count++] = card;
    for (size_t i = 0; i < pile->card_count; ++i)
        swap_cards(&pile->cards[i], &pile->cards[pcg32_boundedrand_r(rnd, pile->card_count)]);
}

static inline struct player new_player(size_t hand_size, playfn_t play) {
    return (struct player) {
        .hand = new_pile(HAND, hand_size),
        .play = play
    };
}

static inline void free_player(struct player* player) {
    free_pile(&player->hand);
}

static inline void refill_hand(struct player* player, struct game* game) {
    while (player->hand.card_count < game->req_hand_size && can_take_card(&game->talon))
        place_card(&player->hand, take_card(&game->talon));
}

struct game new_game(
    size_t up_pile_count,
    size_t down_pile_count,
    size_t req_hand_size,
    size_t req_move_count_full,
    size_t req_move_count_empty,
    card_t min_card, card_t max_card,
    pcg32_random_t* rnd)
{
    assert(up_pile_count > 0);
    assert(down_pile_count > 0);
    assert(min_card < max_card);
    assert(req_hand_size > 0);
    assert(req_move_count_full > 0);
    assert(req_move_count_empty > 0);

    struct game game;
    game.pile_count = up_pile_count + down_pile_count;
    game.piles = malloc(sizeof(struct pile) * game.pile_count);
    for (size_t i = 0; i < up_pile_count; ++i) {
        game.piles[i] = new_pile(UP_PILE, max_card - min_card);
        place_card(&game.piles[i], min_card);
    }
    for (size_t i = 0; i < down_pile_count; ++i) {
        game.piles[up_pile_count + i] = new_pile(DOWN_PILE, max_card - min_card);
        place_card(&game.piles[up_pile_count + i], max_card);
    }
    game.talon = new_pile(TALON, max_card - min_card);
    shuffle_cards(&game.talon, min_card, max_card, rnd);
    game.players = NULL;
    game.player_count = 0;
    game.cur_player = 0;
    game.req_hand_size = req_hand_size;
    game.req_move_count_full = req_move_count_full;
    game.req_move_count_empty = req_move_count_empty;
    game.state = RUNNING;
    game.min_card = min_card;
    game.max_card = max_card;
    return game;
}

void free_game(struct game* game) {
    for (size_t i = 0; i < game->pile_count; ++i)
        free_pile(&game->piles[i]);
    free(game->piles);
    game->piles = NULL;
    game->pile_count = 0;
    free_pile(&game->talon);
    for (size_t i = 0; i < game->player_count; ++i)
        free_player(&game->players[i]);
    free(game->players);
    game->players = NULL;
}

void reset_game(struct game* game, pcg32_random_t* rnd) {
    for (size_t i = 0; i < game->pile_count; ++i)
        game->piles[i].card_count = 1;
    shuffle_cards(&game->talon, game->min_card, game->max_card, rnd);
    for (size_t i = 0; i < game->player_count; ++i) {
        game->players[i].hand.card_count = 0;
        refill_hand(&game->players[i], game);
    }
    game->cur_player = 0;
    game->state = RUNNING;
}

void display_game(const struct game* game) {
    for (size_t i = 0; i < game->pile_count; ++i)
        printf("%3d ", last_card(&game->piles[i]));
    printf("\n");
    for (size_t i = 0; i < game->pile_count; ++i)
        printf(game->piles[i].type == UP_PILE ? " ^  " : " v  ");
    printf("\n");
    printf("Player %zu's card(s): [", game->cur_player);
    for (size_t i = 0, n = game->players[game->cur_player].hand.card_count; i < n; ++i)
        printf(i != n - 1 ? "%d " : "%d", game->players[game->cur_player].hand.cards[i]);
    printf("]\n");
}

void add_player(struct game* game, playfn_t play) {
    game->players = realloc(game->players, sizeof(struct player) * ++game->player_count);
    game->players[game->player_count - 1] = new_player(game->req_hand_size, play);
    refill_hand(&game->players[game->player_count - 1], game);
}

static inline int move_cost(const struct game* game, const struct move* move) {
    const struct pile* pile = &game->piles[move->pile_index];
    switch (pile->type) {
        case UP_PILE:   return move->card - last_card(pile);
        case DOWN_PILE: return last_card(pile) - move->card;
        default:
            assert(false);
            return INT_MAX;
    }
}

static inline int find_best_move(const struct game* game, const struct player* player, struct move* move) {
    int best_cost = INT_MAX;
    for (size_t i = 0; i < game->pile_count; ++i) {
        for (size_t j = 0; j < player->hand.card_count; ++j) {
            if (can_place_card(&game->piles[i], player->hand.cards[j])) {
                struct move candidate = { .pile_index = i, .card = player->hand.cards[j] };
                int cost = move_cost(game, &candidate);
                if (cost < best_cost) {
                    best_cost = cost;
                    *move = candidate;
                }
            }
        }
    }
    return best_cost;
}

static void default_play(struct player* player, struct game* game) {
    struct move move;
    while (true) {
        while (find_best_move(game, player, &move) <= 0)
            apply_move(game, player, &move);
        if (has_enough_moves(game))
            return;
        if (find_best_move(game, player, &move) != INT_MAX)
            apply_move(game, player, &move);
        else
            return;
    }
}

void add_default_player(struct game* game) {
    add_player(game, default_play);
}

void apply_move(struct game* game, struct player* player, const struct move* move) {
    assert(player == &game->players[game->cur_player]);
    assert(move->pile_index < game->pile_count);
    assert(contains_card(&player->hand, move->card));
    remove_card(&player->hand, find_card(&player->hand, move->card));
    place_card(&game->piles[move->pile_index], move->card);
    game->move_count++;
}

bool has_enough_moves(const struct game* game) {
    return game->move_count >= (game->talon.card_count > 0 ? game->req_move_count_full : game->req_move_count_empty);
}

enum game_state play_turn(struct game* game) {
    assert(game->state == RUNNING);
    assert(game->player_count > 0);
    size_t first_player = game->cur_player;
    while (true) {
        struct player* player = &game->players[game->cur_player];
        assert(player->hand.card_count == game->req_hand_size || game->talon.card_count == 0);
        if (player->hand.card_count != 0) {
            game->move_count = 0;
            player->play(player, game);
            if (player->hand.card_count > 0 && !has_enough_moves(game))
                return game->state = LOST;
            refill_hand(player, game);
        }
        game->cur_player = (game->cur_player + 1) % game->player_count;
        if (player->hand.card_count == 0) {
            if (game->cur_player == first_player)
                return game->state = WON;
            continue;
        }
        return RUNNING;
    }
}
