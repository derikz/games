
/*********************************************************************
 *
 * mathematico
 *
 * 1.0    dz  2000-04-15	initial version
 * 1.1    dz  2000-04-30	linted, colors
 * 1.2    dz  2015-02-22        refactored, instructions in game
 * 1.2.1  dz  2015-11-03        score bug fixed
 *
 * Copyright (c) 2000+2015 Derik van Zuetphen <dz@426.ch>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 */

#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define VERSION "1.2.1"

/* board */
#define ROWS	5
#define COLS	5

int board[COLS][ROWS];
int score[COLS+ROWS+2];         /* order: columns, rows, diag[x][x], diag[x][5-x] */

int card;			/* current card */
int xpos,ypos;			/* current cursor */
int drawn_cards[13];	        /* number of drawn cards */

/* colors */
#define BG	COLOR_BLACK

/* palette indexes (pair in curses speech) */
#define P_LINE	1
#define P_POINT 2
#define P_SIDE	3
#define P_TITLE	4
#define P_NUM	5
#define P_NUM_HL 6
#define P_HELP   7
#define P_GAMEOVER_FRAME 8
#define P_GAMEOVER_TEXT  9

/* instructions */
extern int ninst;
extern char *inst[];

void display_board() {
  const int xofs = 17;
  const int yofs = 4;

  clear();
  color_set(P_TITLE,NULL);
  mvprintw(0,56,"[ ? for instructions ]");
  //mvprintw(1,70,"V.");
  mvprintw(1,73,VERSION);
  mvprintw(1,24,"M a t h e m a t i c o");
  color_set(P_LINE,NULL);
  mvprintw(yofs,xofs,"+-----+-----+-----+-----+-----+");
  for (int y=0;y<ROWS;y++) {
    mvprintw(yofs+3*y+1,xofs,"|     |     |     |     |     |");
    mvprintw(yofs+3*y+2,xofs,"|     |     |     |     |     |");
    mvprintw(yofs+3*y+3,xofs,"+-----+-----+-----+-----+-----+");
  }
  refresh();
}

void init_board() {
  for (int x=0;x<COLS;x++) {
    for (int y=0;y<ROWS;y++) {
      board[x][y] = 0;
    }
  }
  for (int x=0;x<COLS+ROWS+2;x++) score[x] = 0;
  for (int x=0;x<13;x++) drawn_cards[x]=0;
}

int total_score() {
  int total = 0;
  for (int i=0;i<COLS+ROWS+2;i++)
    total += score[i];
  return total;
}

void print_score() {
  color_set(P_POINT,NULL);
  for (int i=0;i<COLS;i++) {
    mvprintw(20,18+6*i,"%3d",score[i]);
  }
  for (int i=COLS;i<COLS+ROWS;i++) {
    mvprintw(6+3*(i-COLS),48,"%3d",score[i]);
  }
  mvprintw(20,48,"%3d",score[COLS+ROWS]);
  mvprintw(3,48,"%3d",score[COLS+ROWS+1]);

  color_set(P_SIDE,NULL);

  mvprintw(15,64,"total score");
  mvprintw(17,64,"%5d",total_score());
  refresh();
}

void display_next_card() {
  color_set(P_SIDE,NULL);
  mvprintw(8,64,"next card");
  mvprintw(10,67,"%2d",card);
  refresh();
}

void get_card() {
  do {
    card = rand()%13+1;
  } while (drawn_cards[card]>=4);
  drawn_cards[card]++;
  display_next_card();
}

/* if 'n' contains the digit '1' */
int highlight_number(int n) {
  return (n==1 || n >=10);
}

void print_card(int x, int y) {
  color_set(P_NUM,NULL);
  if (board[x][y]!=0) {
    if (highlight_number(board[x][y]))
      color_set(P_NUM_HL,NULL);
    else
      color_set(P_NUM,NULL);
    mvprintw(5+3*y,18+6*x,"     ");
    mvprintw(6+3*y,18+6*x," %2d  ",board[x][y]);
  } else {
    mvprintw(5+3*y,18+6*x,"     ");
    mvprintw(6+3*y,18+6*x,"     ");
  }
  move(23,0);
  refresh();
}

void cursor (bool on) {
  if (on)
    attron(A_REVERSE);
  print_card(xpos,ypos);
  if(on)
    attroff(A_REVERSE);
  refresh();
}

static void show_instructions() {
  clear();
  color_set(P_HELP,NULL);
  for (int y=0;y<ninst;y++) {
    mvprintw(y,0,inst[y]);
  }
  refresh();

  getch();

  /* rebuild screen */
  display_board();
  for (int x=0;x<COLS;x++) {
    for (int y=0;y<ROWS;y++) {
      print_card(x,y);
    }
  }
  print_score();
  display_next_card();
  cursor(true);
}

bool place_card() {
  bool end = false;			/* end of input loop */
  bool quit = false;			/* end of game requested */
  cursor(true);
  while (!end) {
    int c = getch();
    switch(c) {
    case KEY_DOWN:
    case 14:
    case 'j':
      cursor(false);
      ypos=ypos>=ROWS-1?ROWS-1:ypos+1;
      cursor(true);
      break;
    case KEY_UP:
    case 16:
    case 'k':
      cursor(false);
      ypos=ypos<=0?0:ypos-1;
      cursor(true);
      break;
    case KEY_LEFT:
    case 2:
    case 'h':
      cursor(false);
      xpos=xpos<=0?0:xpos-1;
      cursor(true);
      break;
    case KEY_RIGHT:
    case 6:
    case 'l':
      cursor(false);
      xpos=xpos>=COLS-1?COLS-1:xpos+1;
      cursor(true);
      break;
    case KEY_ENTER:
    case 13:
    case ' ':
      if (board[xpos][ypos]==0) {
	cursor(false);
	board[xpos][ypos]=card;
	print_card(xpos,ypos);
	end = true;
      }
      break;
    case '?':
      show_instructions();
      break;
    case 'q':
      end = true;
      quit = true;
      break;
    }
  }
  return quit;
}

int eval_five(int a, int b, int c, int d, int e) {
  int res = 0;
  int x[5];

  x[0]=a; x[1]=b; x[2]=c; x[3]=d; x[4]=e;

  /* sort x[] */
  for (int i=0;i<4;i++)
    for (int j=i+1;j<5;j++)
      if (x[i]>x[j]) {
	int tmp=x[i];
	x[i]=x[j];
	x[j]=tmp;
      }

  /* fill zeros in x[] with numbers that don't gain score */
  int value = 20;
  for (int i=0;i<5;i++) {
    if (x[i]==0) x[i]=value;
    value+=2;
  }

  /* one pair */
  if ((x[0]==x[1])||(x[1]==x[2])||(x[2]==x[3])||(x[3]==x[4]))
    res=10;

  /* two pairs */
  if (((x[0]==x[1])&&((x[2]==x[3])||(x[3]==x[4])))
      ||((x[1]==x[2])&&(x[3]==x[4])))
    res=20;

  /* 3x same */
  if (((x[1]==x[2])&&((x[0]==x[1])||(x[2]==x[3])))
      ||((x[2]==x[3])&&(x[3]==x[4])))
    res=40;

  /* full house */
  if ((x[0]==x[1])&&(x[3]==x[4])&&((x[2]==x[1])||(x[2]==x[3])))
    res=80;

  /* full house out of 1 and 13 */
  if ((x[0]==1)&&(x[1]==1)&&(x[2]==1)&&(x[3]==13)&&(x[4]==13))
    res=100;

  /* 4x same */
  if ((x[1]==x[2])&&(x[2]==x[3])&&((x[0]==x[1])||(x[3]==x[4])))
    res=160;

  /* 4x one */
  if ((x[0]==1)&&(x[1]==1)&&(x[2]==1)&&(x[3]==1))
    res=200;

  /* street */
  if ((x[0]+1==x[1])&&(x[1]+1==x[2])&&(x[2]+1==x[3])&&(x[3]+1==x[4]))
    res=50;

  /* 1,10,11,12,13 */
  if ((x[0]==1)&&(x[1]==10)&&(x[2]==11)&&(x[3]==12)&&(x[4]==13))
    res=150;

  return res;
}

bool eval_board() {
  for (int i=0;i<COLS;i++) {
    score[i] = eval_five(board[i][0],board[i][1],board[i][2],board[i][3],board[i][4]);
  }
  for (int i=0;i<ROWS;i++) {
    score[COLS+i] = eval_five(board[0][i],board[1][i],board[2][i],board[3][i],board[4][i]);
  }

  int diag_score;
  diag_score = eval_five(board[0][0],board[1][1],board[2][2],board[3][3],board[4][4]);
  score[COLS+ROWS] = diag_score>0? 10+diag_score : 0;

  diag_score = eval_five(board[4][0],board[3][1],board[2][2],board[1][3],board[0][4]);
  score[COLS+ROWS+1] = diag_score>0? 10+diag_score : 0;

  int cnt = 0;
  for (int i=0;i<ROWS;i++)
    for (int j=0;j<COLS;j++)
      if (board[j][i]!=0) cnt++;

  return (cnt==ROWS*COLS);	/* true, if end of game */
}

void game_over() {
  color_set(P_GAMEOVER_FRAME, NULL);
  attron(A_BOLD);
  mvprintw(21,7,  " *************************************************************** ");
  mvprintw(22,5,"  ***                                                           ***  ");
  mvprintw(23,7,  " *************************************************************** ");
  mvprintw(24,7,  "                                                                 ");

  color_set(P_GAMEOVER_TEXT, NULL);
  mvprintw(22,31,"G A M E   O V E R");
  refresh();
  getch();

  mvprintw(22,25,"Your final score is %d points.", total_score());
  attroff(A_BOLD);
  refresh();
  getch();
}

/*ARGSUSED 1*/
int main(int argc,char **argv) {
  /* show help */
  if (argc>1) {
    for (int y=0;y<ninst;y++) {
      printf("%s\n",inst[y]);
    }
    exit(0);
  }

  /* init curses */
  initscr();
  nonl();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  start_color();
  clear();

  init_pair(P_LINE,COLOR_GREEN,BG);
  init_pair(P_NUM,COLOR_WHITE,BG);
  init_pair(P_NUM_HL,COLOR_RED,BG);
  init_pair(P_POINT,COLOR_YELLOW,BG);
  init_pair(P_SIDE,COLOR_CYAN,BG);
  init_pair(P_TITLE,COLOR_RED,BG);
  init_pair(P_HELP,COLOR_YELLOW,BG);
  init_pair(P_GAMEOVER_FRAME,COLOR_YELLOW,BG);
  init_pair(P_GAMEOVER_TEXT,COLOR_WHITE,BG);

  /* init game */
  srand((unsigned)time(NULL));
  xpos=ypos=0;
  init_board();
  display_board();
  print_score();

  /* game loop */
  bool endofgame = false;
  while (!endofgame) {
    get_card();
    endofgame = place_card();
    endofgame |= eval_board();
    print_score();
  }
  game_over();

  /* finish curses  */
  curs_set(1);
  move(23,0);
  refresh();
  endwin();

  return 0;
}
