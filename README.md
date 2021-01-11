# The Game

This is an implementation of the card game "The Game" (https://pandasaurusgames.com/products/the-game-kwanchai-moriya-edition).
It is a cooperative game where the original rules are:

- There are cards from 2 to 99 and two 1s and two 100s.
- A talon is formed by shuffling all the cards from 2 to 99.
- Four piles are formed by placing the two 1s next to the two 100s.
  The piles with a 1 are _ascending_, and the piles with a 100 are _descending_.
- Each player is given 6 cards from the talon.
- At each turn, the player must get a rid of (at least) two cards
  (or only one if the talon is empty) of his hand by placing them on the piles of his choice.
  To place a card on a pile, the card must either be larger than the current card on the pile
  if the pile is ascending, or smaller if the pile is descending.
  The exception to this rule is when the card is equal to the amount on the pile - 10 (resp. + 10),
  if the pile is ascending (resp. descending).
  For instance, if the pile is ascending and the current amount is 52, then it is possible to place
  a 42 on it.
- Players are not allowed to communicate during the game, except to ask other players to avoid playing
  on a certain pile.

Note: This project is in no way affiliated with the creators of the original game.
Please buy the game if you intend to play it with friends, this project is only designed to evaluate different AI strategies.

# Building

Before building, make sure submodules have been downloaded (e.g. using `git submodule init recursive`).
Then, type the following commands:

    mkdir build
    cd build
    cmake ..
    make -j

# Running

Use `the_game --help` to display available options.
