#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

typedef struct
{
	int sur_bombs;
	bool has_bomb;
	bool revealed;
} g_tile;

typedef struct
{
	int r;
	int c;
	int bombs;
	g_tile *tiles;
} g_board;

static bool parse_int(const char *s, int *out)
{
	char *end;
	long v = strtol(s, &end, 10);
	if (end == s || *end != '\0' || v <= 0 || v > 1000)
	{
		return false;
	}
	*out = (int)v;
	return true;
}

static g_tile *board_tile(g_board *b, int r, int c)
{
	return &b->tiles[r * b->c + c];
}

static void place_bombs(g_board *b)
{
	int total = b->r * b->c;
	int placed = 0;

	while (placed < b->bombs)
	{
		int i = rand() % total;
		if (!b->tiles[i].has_bomb)
		{
			b->tiles[i].has_bomb = true;
			placed++;
		}
	}
}

static void calc_sur_bombs(g_board *b)
{
	for (int row = 0; row < b->r; row++)
	{
		for (int col = 0; col < b->c; col++)
		{
			int count = 0;

			for (int dr = -1; dr <= 1; dr++)
			{
				for (int dc = -1; dc <= 1; dc++)
				{
					if (dr == 0 && dc == 0)
					{
						continue;
					}

					int nr = row + dr, nc = col + dc;
					if (nr < 0 || nr >= b->r || nc < 0 || nc >= b->c)
					{
						continue;
					}

					if (board_tile(b, nr, nc)->has_bomb)
					{
						count++;
					}
				}
			}

			board_tile(b, row, col)->sur_bombs = count;
		}
	}
}

static bool reveal(g_board *b, int row, int col)
{
	g_tile *start = board_tile(b, row, col);
	if (start->revealed)
	{
		return false;
	}
	start->revealed = true;
	if (start->has_bomb)
	{
		return true;
	}

	int *stack = malloc(sizeof(int) * (size_t)b->r * b->c);
	if (!stack)
	{
		return false;
	}
	int top = 0;
	stack[top++] = row * b->c + col;

	while (top > 0)
	{
		int i = stack[--top];
		int tr = i / b->c, tc = i % b->c;
		if (b->tiles[i].sur_bombs != 0)
		{
			continue;
		}

		for (int dr = -1; dr <= 1; dr++)
		{
			for (int dc = -1; dc <= 1; dc++)
			{
				int nr = tr + dr, nc = tc + dc;
				if (nr < 0 || nr >= b->r || nc < 0 || nc >= b->c)
				{
					continue;
				}
				g_tile *n = board_tile(b, nr, nc);
				if (!n->revealed && !n->has_bomb)
				{
					n->revealed = true;
					stack[top++] = nr * b->c + nc;
				}
			}
		}
	}

	free(stack);
	return false;
}

static int count_hidden_safe(g_board *b)
{
	int n = 0;
	for (int i = 0; i < b->r * b->c; i++)
	{
		if (!b->tiles[i].revealed && !b->tiles[i].has_bomb)
		{
			n++;
		}
	}
	return n;
}

static void print_board(g_board *b, bool show_all)
{
	printf("    ");
	for (int col = 0; col < b->c; col++)
	{
		printf("%2d", col);
	}
	printf("\n");

	for (int row = 0; row < b->r; row++)
	{
		printf("%2d  ", row);
		for (int col = 0; col < b->c; col++)
		{
			g_tile *t = board_tile(b, row, col);
			putchar(' ');
			if (!t->revealed && !show_all)
			{
				putchar('#');
			}
			else if (t->has_bomb)
			{
				putchar('*');
			}
			else if (t->sur_bombs == 0)
			{
				putchar('.');
			}
			else
			{
				putchar('0' + t->sur_bombs);
			}
		}
		putchar('\n');
	}
}

typedef struct
{
	int r;
	int c;
} g_move;

/**
 * Write a game record in BGN (Bomb Game Notation):
 *
 *   [Date "2026.11.23"]
 *   [Rows "8"]
 *   [Cols "10"]
 *   [Bombs "10"]
 *   [Layout "1,0 3,2 3,4 ..."]     bomb positions as row,col
 *   [Result "Win"]                 Win | Loss | Abandoned
 *
 *   1. 0,0 2. 3,4 3. 7,1 Loss
 *
 * Tags come first, one per line; then a blank line; then numbered moves
 * (row,col), ending with the result word. Lines starting with ';' are comments.
 */
static bool write_bgn(const char *path, g_board *b, g_move *moves, int nmoves, const char *result)
{
	FILE *f = fopen(path, "w");
	if (!f)
	{
		return false;
	}

	time_t now = time(NULL);
	struct tm *tm = localtime(&now);
	fprintf(f, "[Date \"%04d.%02d.%02d\"]\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
	fprintf(f, "[Rows \"%d\"]\n", b->r);
	fprintf(f, "[Cols \"%d\"]\n", b->c);
	fprintf(f, "[Bombs \"%d\"]\n", b->bombs);

	fprintf(f, "[Layout \"");
	bool first = true;
	for (int i = 0; i < b->r * b->c; i++)
	{
		if (b->tiles[i].has_bomb)
		{
			fprintf(f, "%s%d,%d", first ? "" : " ", i / b->c, i % b->c);
			first = false;
		}
	}
	fprintf(f, "\"]\n");
	fprintf(f, "[Result \"%s\"]\n\n", result);

	for (int i = 0; i < nmoves; i++)
	{
		fprintf(f, "%d. %d,%d ", i + 1, moves[i].r, moves[i].c);
		if ((i + 1) % 8 == 0)
		{
			fputc('\n', f);
		}
	}
	fprintf(f, "%s\n", result);

	return fclose(f) == 0;
}

g_board init_board(int r, int c, int bombs)
{
	g_board b = {
		.r = r,
		.c = c,
		.bombs = bombs,
		.tiles = calloc((size_t)r * c, sizeof(g_tile))};
	return b;
}

void free_board(g_board *b)
{
	free(b->tiles);
	b->tiles = NULL;
}

int main(int argc, char const *argv[])
{
	int r = 8, c = 10, bombs = 10;

	if (argc == 4)
	{
		if (!parse_int(argv[1], &r) || !parse_int(argv[2], &c) || !parse_int(argv[3], &bombs))
		{
			fprintf(stderr, "error: arguments must be positive integers\n");
			return 1;
		}
	}
	else if (argc != 1)
	{
		fprintf(stderr, "usage: %s [rows cols bombs]\n", argv[0]);
		return 1;
	}

	if (bombs >= r * c)
	{
		fprintf(stderr, "error: too many bombs for a %dx%d board\n", r, c);
		return 1;
	}

	g_board board = init_board(r, c, bombs);
	if (!board.tiles)
	{
		fprintf(stderr, "error: out of memory\n");
		return 1;
	}

	srand((unsigned)time(NULL));
	place_bombs(&board);
	calc_sur_bombs(&board);

	g_move *moves = NULL;
	int nmoves = 0, cap = 0;
	const char *result = "Abandoned";

	while (true)
	{
		print_board(&board, false);
		printf("reveal (row col): ");
		fflush(stdout);

		int row, col;
		int n = scanf("%d %d", &row, &col);
		if (n == EOF)
		{
			printf("\n");
			break;
		}
		if (n != 2)
		{
			int ch;
			while ((ch = getchar()) != '\n' && ch != EOF)
				;
			printf("enter two numbers, e.g. 3 4\n");
			continue;
		}
		if (row < 0 || row >= board.r || col < 0 || col >= board.c)
		{
			printf("out of range: rows 0-%d, cols 0-%d\n", board.r - 1, board.c - 1);
			continue;
		}

		if (!board_tile(&board, row, col)->revealed)
		{
			if (nmoves == cap)
			{
				cap = cap ? cap * 2 : 32;
				g_move *grown = realloc(moves, sizeof *moves * (size_t)cap);
				if (!grown)
				{
					fprintf(stderr, "error: out of memory\n");
					break;
				}
				moves = grown;
			}
			moves[nmoves++] = (g_move){row, col};
		}

		if (reveal(&board, row, col))
		{
			print_board(&board, true);
			printf("BOOM! You hit a bomb.\n");
			result = "Loss";
			break;
		}
		if (count_hidden_safe(&board) == 0)
		{
			print_board(&board, true);
			printf("You win!\n");
			result = "Win";
			break;
		}
	}

	if (write_bgn("game.bgn", &board, moves, nmoves, result))
	{
		printf("game saved to game.bgn\n");
	}
	else
	{
		perror("game.bgn");
	}

	free(moves);
	free_board(&board);
	return 0;
}
