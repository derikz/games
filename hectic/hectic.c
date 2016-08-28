
/*********************************************************************
 *
 * hectic
 *
 * 1.0	2000-06	initial version
 * 1.1  2014-12	refactored, instructions in game
 *
 * Copyright (c) 2004+2014 Derik van Zuetphen <dz@426.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Hectic is based on a program by Werner Kratz, published in BASIC
 * for the VIC 20 around 1983.
 */


#include <ncurses.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

static struct player_t {
  int x,y;			/* player position */
  int dx,dy;			/* player speed */
} player;

static struct game_t {
  int score;
  int rest;			/* when this is 0, the game is over */
  int turn;			/* number of current turn */
  int blocks;			/* number of wall segments on the board */
  int level;
  bool end;			/* boolean for end-of-game */
} game;

#define XOFS	2		/* screen offset of the board */
#define YOFS	4

/* constants for elements on the board */
enum itemtype_t {
  EMPTY,
  WALL,
  PLAYER,
  ITEM0,
  VISITED=0x80};		/* flag for not reachable */

/* the board */
#define WIDTH 37
#define HEIGHT 19
static enum itemtype_t board[WIDTH][HEIGHT];

/* the items */
struct item_t {
  int val;			/* score for this type of item */
  char *ch;			/* char to display this item */
  int num;			/* number of items of this type */
};

#define ITEMS	3
static struct item_t item[ITEMS];
static int cur_items;			/* number of items on the board */
static int count_items;		/* temporary for fill_board() */

/* colors */
enum {
  BG       = COLOR_BLACK,
  P_WALL   = 1,
  P_PLAYER,
  P_TITLE,
  P_GAMEOVER_FRAME,
  P_GAMEOVER_TEXT,
  P_ITEM0
};

/* instructions */
extern int ninst;
extern char *inst[];

/************************************************************************
 * fill the board with "visited" places (starting at player position)
 * and increment cur_item on any item found
 * thus cur_items==ITEMS, if all items are reachable
 */

static void fill_board(int x,int y) {
  /* stop criterium */
  if ((x<0)||(x>=WIDTH)||(y<0)||(y>=HEIGHT)) return;
  if ((board[x][y]&VISITED) || (board[x][y] == WALL)) return;

  /* test if item found and mark as visited */
  if ((board[x][y] >= PLAYER) && board[x][y] < ITEM0+ITEMS) count_items++;
  board[x][y] |= VISITED;

  /* visit all 4 neighbors */
  fill_board(x-1,y);
  fill_board(x+1,y);
  fill_board(x,y-1);
  fill_board(x,y+1);
}

/************************************************************************
 * initialise the board according to global variables
 */

static void place_items() {
  int i,j, x,y;
  /* repeat all the placement operations until all items are reachable */
  do {

    /* clear board */
    for (x=0;x<WIDTH;x++) {
      for (y=0;y<HEIGHT;y++) {
	board[x][y] = EMPTY;
      }
    }

    /* items */
    cur_items = 0;
    for (i=0;i<ITEMS;i++) {
      cur_items += item[i].num;
      for (j=0;j<item[i].num;j++) {
	do {
	  x = rand()%WIDTH;
	  y = rand()%HEIGHT;
	} while (board[x][y]!=EMPTY);
	board[x][y] = ITEM0+i;
      }
    }

    /* blocks */
    for (i=0;i<game.blocks;i++) {
      do {
	x = rand()%WIDTH;
	y = rand()%HEIGHT;
      } while (board[x][y]!=EMPTY);
      board[x][y] = WALL;
    }

    /* player */
    do {
      player.x = rand()%WIDTH;
      player.y = rand()%HEIGHT;
    } while (board[player.x][player.y]!=EMPTY);
    board[player.x][player.y] = PLAYER;
    player.dx = player.dy = 0;

    /* check for reachability */
    count_items=0;
    fill_board(player.x,player.y);
    /* clear not-visited-flag */
    for (x=0;x<WIDTH;x++) {
      for (y=0;y<HEIGHT;y++) {
	board[x][y] &= ~VISITED;
      }
    }
  } while(count_items!=cur_items+1);
}

/************************************************************************
 * initialise the game
 */

static void init_game() {
  /* variables */
  item[0].val = 10; item[0].ch = "<>"; item[0].num = 10;
  item[1].val = 20; item[1].ch = "::"; item[1].num = 5;
  item[2].val = 50; item[2].ch = "$$"; item[2].num = 1;

  game.turn = 0;
  game.score = 0;
  game.rest = 0;
  game.blocks = 20;
  game.level = 1;
  game.end = false;

  srand((unsigned)time(NULL));

  /* board */
  place_items();
}

static void set_getch_blocking(bool flag) {
  nodelay(stdscr,!flag);
}

static void init_curses() {
  /* curses */
  initscr();
  nonl();
  set_getch_blocking(false);
  cbreak();
  noecho();
  keypad(stdscr, true);
  curs_set(0);	/* hide cursor */

  /* colors */
  start_color();
  init_pair((short)P_WALL,           COLOR_BLUE,    BG);
  init_pair((short)P_PLAYER,         COLOR_YELLOW,  BG);
  init_pair((short)P_TITLE,          COLOR_CYAN,    BG);
  init_pair((short)P_GAMEOVER_FRAME, COLOR_YELLOW,  BG);
  init_pair((short)P_GAMEOVER_TEXT,  COLOR_WHITE,   BG);
  init_pair((short)P_ITEM0,          COLOR_GREEN,   BG);
  init_pair((short)P_ITEM0+1,        COLOR_MAGENTA, BG);
  init_pair((short)P_ITEM0+2,        COLOR_RED,     BG);
}

/************************************************************************
 * display one position of the board
 */

static void display(int x,int y) {
  int xx,yy,i;
  xx = 2*x + XOFS;
  yy = y + YOFS;

  switch (board[x][y]) {
  case EMPTY:
    mvprintw(yy,xx,"  ");
    break;
  case WALL:
    attron(A_REVERSE);
    color_set(P_WALL,NULL);
    mvprintw(yy,xx,"  ");
    attroff(A_REVERSE);
    break;
  case PLAYER:
    color_set(P_PLAYER,NULL);
    mvprintw(yy,xx,"@@");
    break;
  default:
    i=board[x][y]-ITEM0;
    if ((i>=0)&&(i<ITEMS)) {
      color_set((short)(P_ITEM0+i),NULL);
      mvprintw(yy,xx,"%s",item[i].ch);
    } else {
      mvprintw(yy,xx,"%c?",64+i);
    }
  }
  move(0,0);
  refresh();
}

/************************************************************************
 * display the whole board
 */

static void display_board() {
  int x,y;
  clear();

  /* title */
  color_set(P_TITLE,NULL);
  mvprintw(0,0,"H e c t i c");
  mvprintw(0,56,"[ ? for instructions ]");
  refresh();

  /* board */
  color_set(P_WALL,NULL);
  for (x=-1;x<=WIDTH;x++) {
    attron(A_REVERSE);
    mvprintw(YOFS-1,XOFS+2*x,"  ");
    attroff(A_REVERSE);
  }

  for (y=0;y<HEIGHT;y++) {
    attron(A_REVERSE);
    mvprintw(YOFS+y,XOFS-2,"  ");
    color_set(P_WALL,NULL);
    attroff(A_REVERSE);
    for (x=0;x<WIDTH;x++) {
      display(x,y);
    }
    attron(A_REVERSE);
    color_set(P_WALL,NULL);
    mvprintw(YOFS+y,XOFS+2*WIDTH,"  ");
    attroff(A_REVERSE);
  }

  color_set(P_WALL,NULL);
  for (x=-1;x<=WIDTH;x++) {
    attron(A_REVERSE);
    mvprintw(YOFS+HEIGHT,XOFS+2*x,"  ");
    attroff(A_REVERSE);
  }
  refresh();
}

/************************************************************************
 * show the help screen
 */

static void show_instructions() {
  int y;
  clear();
  for (y=0;y<ninst;y++) {
    mvprintw(y,0,inst[y]);
  }
  refresh();

  set_getch_blocking(true);
  getch();
  set_getch_blocking(false);
  display_board();
}


/************************************************************************
 * game loop. handle key press and events
 */

static void run() {
  int c;
  int oldx, oldy;
  bool end_wish = false;
  display_board();
  while (cur_items > 0) {
    c = getch();
    switch (c) {
    case ERR:
      break;
    case KEY_DOWN:
    case 14:
    case 'j':
      player.dy = 1;
      player.dx = 0;
      break;
    case KEY_UP:
    case 16:
    case 'k':
      player.dy = -1;
      player.dx = 0;
      break;
    case KEY_LEFT:
    case 2:
    case 'h':
      player.dy = 0;
      player.dx = -1;
      break;
    case KEY_RIGHT:
    case 6:
    case 'l':
      player.dy = 0;
      player.dx = 1;
      break;
    case 'q':
      end_wish = true;
      break;
    case '?':
      show_instructions();
      break;
    }
    oldx = player.x;
    oldy = player.y;
    player.x += player.dx;
    player.y += player.dy;

    if (player.x < 0) {
      player.x = 0;
      game.rest--;
    }
    if (player.x >= WIDTH) {
      player.x = WIDTH-1;
      game.rest--;
    }
    if (player.y < 0) {
      player.y = 0;
      game.rest--;
    }
    if (player.y >= HEIGHT) {
      player.y = HEIGHT-1;
      game.rest--;
    }

    if (board[player.x][player.y] == WALL) {
      game.rest--;
      player.x = oldx;
      player.y = oldy;
    }

    if (board[player.x][player.y] >= ITEM0) {
      game.score += item[board[player.x][player.y]-ITEM0].val;
      cur_items--;
    }

    color_set(P_TITLE,NULL);
    mvprintw(2,0,"Level %d  Blocks %d", game.level, game.blocks);
    mvprintw(2,56,"Energy %3d  Gold %5d", game.rest<0?0:game.rest, game.score);

    board[oldx][oldy] = EMPTY;
    board[player.x][player.y] = PLAYER;

    display(oldx,oldy);
    display(player.x,player.y);

    usleep(150000);

    if (game.rest < 0 || end_wish) {
      game.end=true;
      cur_items = 0;		/* force break of inner loop */
    }
  }
}

/************************************************************************
 * show the game over display and final results
 */

void game_over(struct game_t *game) {
  color_set(P_GAMEOVER_FRAME, NULL);
  attron(A_BOLD);
  mvprintw( 9,7,  "                                                                 ");
  mvprintw(10,7,  "  *************************************************************  ");
  mvprintw(11,6, "  ***                                                         ***  ");
  mvprintw(12,5,"  ***                                                           ***  ");
  mvprintw(13,6, "  ***                                                         ***  ");
  mvprintw(14,7,  "  *************************************************************  ");
  mvprintw(15,7,  "                                                                 ");

  color_set(P_GAMEOVER_TEXT, NULL);
  mvprintw(12,31,"G A M E   O V E R");
  refresh();
  set_getch_blocking(true);
  getch();

  mvprintw(12,25,"You got %d Gold in %d level%s       ",
	   game->score,
	   game->level,
	   game->level>1?"s":"");
  attroff(A_BOLD);
  refresh();
  getch();
}

int main() {
  init_game();
  init_curses();
  while(!game.end) {
    run();
    if (game.end) break;
    game.turn++;
    game.blocks += 8;
    game.rest += 10;
    game.level++;
    place_items();
  }

  game_over(&game);

  clear();
  move(0,0);
  refresh();
  endwin();

  printf("Your final score: %d Gold in %d level%s with %d blocks\n",
	 game.score, game.level, game.level>1?"s":"",game.blocks);

  return 0;
}
