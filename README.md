# fackstone

## Install
make

## Clean
make clean

## Play
1. open server on a computer.
$ ./server
2. connect server (There must be 2 players(i.e. 2 clients connected)).
$ ./client -s server-ip -c card-lib
The default card-lib is specified by the card file in the client file's dir.

## How to play
Main commands are: s, u, a, e, h, q

>>> s
Hero >>> HP:30 CanATK:0 ATK:0 POW:1 <<< <--- // This is our hero info. "<---" stands for our turn.
Hero >>> HP:30 CanATK:0 ATK:0 POW:0 <<<      // This is opponent hero info.

// This is battlefiled. ATK: attack-force; CATK, can attack.
// There are 7 position in the battlefield. Position 0 stands for hero.
===========================================================
|  HP  |  ATK | CATK | ---> NO. <--- |  HP  |  ATK | CATK |
|    0 |    0 |   0  | --->  1  <--- |    0 |    0 |   0  |
|    0 |    0 |   0  | --->  2  <--- |    0 |    0 |   0  |
|    0 |    0 |   0  | --->  3  <--- |    0 |    0 |   0  |
|    0 |    0 |   0  | --->  4  <--- |    0 |    0 |   0  |
|    0 |    0 |   0  | --->  5  <--- |    0 |    0 |   0  |
|    0 |    0 |   0  | --->  6  <--- |    0 |    0 |   0  |
|    0 |    0 |   0  | --->  7  <--- |    0 |    0 |   0  |
===========================================================

// This is our hand card. POWER is stand for the power to consume to put the card to the battlefiled.
====================================
|  NO. | TYPE |  HP  |  ATK | POWER|
|   1  |   0  |    4 |    3 |   3  |
|   2  |   0  |    8 |    7 |   7  |
|   3  |   0  |    7 |    6 |   6  |
|   4  |   0  |   10 |    9 |   9  |
====================================

>>> u 1 2
"u 1 2" stands for put the card NO. 1 from our hand to Position 2 in battlefield.

>>> a 1 0
"a 1 0" stands for make the soldier in position 1 attack the opponent's hero.
"a 2 5" stands for make the soldier in position 2 attack the opponent's soldier at positon 5.

>>> e
end our turn.
Our turn will be auto end after 90 seconds.

>>> h
help

## About card file
One line stand for a card.
The 3 column is the POW, HP, ATK of the card. They are sepreate by a space.
There must be 30 lines in the file. i.e. 30 cards.
Each card should abide the formula "HP + ATK <= 2*POW + 1".
