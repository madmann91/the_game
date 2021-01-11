#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

#include "game.h"

struct options {
    size_t up_pile_count, down_pile_count;
    size_t req_hand_size;
    size_t req_move_count_full, req_move_count_empty;
    card_t min_card, max_card;
    bool interactive, verbose;
    size_t run_count;
};

static const struct options default_options = {
    .up_pile_count        = 2,
    .down_pile_count      = 2,
    .req_hand_size        = 6,
    .req_move_count_full  = 2,
    .req_move_count_empty = 1,
    .min_card             = 1,
    .max_card             = 100,
    .interactive          = false,
    .verbose              = false,
    .run_count            = 1
};

static inline void usage(const struct options* defaults) {
    printf(
        "Usage: the_game [options]\n"
        "Options:\n"
        "  -h      --help                Shows this message\n"
        "  -s      --seed <state> <seq>  Sets the seed for the random generator (defaults to a random seed)\n"
        "  -i      --interactive         Enables interactive mode (disabled by default, enables verbose mode automatically)\n"
        "  -v      --verbose             Show every turn in detail in the output\n"
        "  -r <n>  --runs <n>            Runs the given number of games and return the ratio of wins/losses\n"
        "          --up-piles <n>        Sets the number of piles going up (defaults to %zu)\n"
        "          --down-piles <n>      Sets the number of piles going down (defaults to %zu)\n"
        "          --hand-size <n>       Sets the required player hand size (the number of cards given to players, defaults to %zu)\n"
        "          --moves-full <n>      Sets the required number of cards that must be played when the talon is not empty (defaults to %zu)\n"
        "          --moves-empty <n>     Sets the required number of cards that must be played when the talon is empty (defaults to %zu)\n"
        "          --min-card <n>        Sets the minimum card (defaults to %d)\n"
        "          --max-card <n>        Sets the maximum card (defaults to %d)\n",
        defaults->up_pile_count,
        defaults->down_pile_count,
        defaults->req_hand_size,
        defaults->req_move_count_full,
        defaults->req_move_count_empty,
        defaults->min_card,
        defaults->max_card);
}

static inline bool check_arg(int i, int n, int argc, char** argv) {
    if (i + n >= argc) {
        fprintf(stderr, "Need %d argument(s) for '%s'\n", n, argv[i]);
        return false;
    }
    return true;
}

static inline void play_game(struct game* game, const struct options* options, pcg32_random_t* rnd) {
    size_t wins = 0, jumps = 0;
    for (size_t i = 0; i < options->run_count; ++i) {
        while (game->state == RUNNING) {
            if (options->verbose)
                display_game(game);
            if (options->interactive)
                getchar();
            play_turn(game);
        }
        wins += game->state == WON;
        jumps += game->jump_count;
        if (i != options->run_count - 1)
            reset_game(game, rnd);
    }
    if (options->run_count == 1) {
        display_game(game);
        if (wins > 0)
            printf("WON!\n");
        else {
            printf("LOST :(\n");
            printf("\n%zu card(s) remained in the talon\n", game->talon.card_count);
            for (size_t i = 0; i < game->player_count; ++i) {
                printf("Player %zu had %zu card(s) left: [", i, game->players[i].hand.card_count);
                for (size_t j = 0, n = game->players[i].hand.card_count; j < n; ++j)
                    printf(j != n - 1 ? "%d " : "%d", game->players[i].hand.cards[j]);
                printf("]\n");
            }
        }
    } else {
        size_t losses = options->run_count - wins;
        printf("%zu wins, %zu losses, %.2g%% success\n", wins, losses, (double)wins*100.0/(double)options->run_count);
    }
    printf("There %s %zu jump(s) during the game(s)\n", jumps > 1 ? "were" : "was", jumps);
}

int main(int argc, char** argv) {
    pcg32_random_t rnd;
    uint64_t state = clock(), seq = state;
    pcg32_srandom_r(&rnd, state, seq);
    seq   = ((uint64_t)pcg32_random_r(&rnd)) << 32 | (uint64_t)pcg32_random_r(&rnd);
    state = ((uint64_t)pcg32_random_r(&rnd)) << 32 | (uint64_t)pcg32_random_r(&rnd);

    struct options options = default_options;
    for (int i = 1; i < argc; ++i) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
                usage(&default_options);
                return EXIT_SUCCESS;
            } else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--seed")) {
                if (!check_arg(i, 2, argc, argv))
                    return EXIT_FAILURE;
                state = strtoumax(argv[++i], NULL, 10);
                seq = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--interactive")) {
                options.interactive = true;
                options.verbose = true;
            } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
                options.verbose = true;
            } else if (!strcmp(argv[i], "-r") || !strcmp(argv[i], "--runs")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.run_count = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--up-piles")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.up_pile_count = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--down-piles")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.down_pile_count = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--hand-size")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.req_hand_size = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--moves-full")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.req_move_count_full = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--moves-empty")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.req_move_count_empty = strtoumax(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--min-card")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.min_card = strtol(argv[++i], NULL, 10);
            } else if (!strcmp(argv[i], "--max-card")) {
                if (!check_arg(i, 1, argc, argv))
                    return EXIT_FAILURE;
                options.max_card = strtol(argv[++i], NULL, 10);
            } else {
                fprintf(stderr, "Unknown option '%s'\n", argv[i]);
                return EXIT_FAILURE;
            }
        } else {
            fprintf(stderr, "Unknown argument '%s'\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    if (options.min_card >= options.max_card) {
        fprintf(stderr, "Invalid minimum and maximum cards\n");
        return EXIT_FAILURE;
    }
    if (options.up_pile_count == 0 || options.down_pile_count == 0) {
        fprintf(stderr, "There must be at least one card pile going up, and one going down\n");
        return EXIT_FAILURE;
    }
    if (options.req_hand_size == 0) {
        fprintf(stderr, "Players should at least have to hold one card\n");
        return EXIT_FAILURE;
    }
    if (options.req_move_count_full == 0 || options.req_move_count_empty == 0) {
        fprintf(stderr, "Players should at least have to do one move\n");
        return EXIT_FAILURE;
    }

    pcg32_srandom_r(&rnd, state, seq);
    struct game game = new_game(
        options.up_pile_count,
        options.down_pile_count,
        options.req_hand_size,
        options.req_move_count_full,
        options.req_move_count_empty,
        options.min_card,
        options.max_card,
        &rnd);

    add_default_player(&game);
    add_default_player(&game);
    add_default_player(&game);

    play_game(&game, &options, &rnd);
    printf("This game's seed was %"PRIu64" %"PRIu64"\n", state, seq);
    free_game(&game);
    return EXIT_SUCCESS;
}
