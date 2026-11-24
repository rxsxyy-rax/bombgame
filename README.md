# bombgame

bombgame is a VERY small terminal bomb sweeping game written in C, it saves every game it plays to a plain text record

## building

### linux / macos

```
$ cc -std=c99 -Wall -Wextra bombgame.c -o bombgame
```

### windows

with mingw-w64 (gcc):

```
> gcc -std=c99 -Wall -Wextra bombgame.c -o bombgame.exe
```

with msvc, from a developer command prompt:

```
> cl /W4 /D_CRT_SECURE_NO_WARNINGS bombgame.c /Fe:bombgame.exe
```

## usage

```
$ ./bombgame [ROWS COLS BOMBS]
```

with no arguments the board is 8 rows by 10 columns with 10 bombs, each value can go up to 1000 and there must be fewer bombs than tiles

type a row and a column to reveal a tile (`3 4`), revealing an empty tile also opens its empty neighbours

```
#      hidden tile
.      empty tile
1-8    number of bombs around the tile
*      bomb
```

reveal every safe tile to win, hit a bomb and you lose, press ctrl+d (ctrl+z then enter on windows) to give up

### examples

```
$ ./bombgame
$ ./bombgame 16 16 40
> bombgame.exe 16 30 99
```

## game records

when a game ends it is written to `game.bgn` in the current directory, overwriting the last one

```
[Date "2026.11.23"]
[Rows "4"]
[Cols "4"]
[Bombs "2"]
[Layout "1,2 2,2"]
[Result "Loss"]

1. 0,0 2. 1,2 Loss
```

tags come first, one per line, then a blank line, then the numbered moves as `row,col` ending with the result (`Win`, `Loss` or `Abandoned`), lines starting with `;` are comments
