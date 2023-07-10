/*
O código fornecido é um programa em C que implementa um jogo estilo Flappy Bird usando a biblioteca ncurses para manipulação da tela de terminal.
Aqui está uma descrição comentada do código:
*/

#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <limits.h>

typedef struct vpipe {
	float opening_height;
	int center;
} vpipe;

typedef struct flappy {
	int h0;
	int t;
} flappy;

// Constantes do jogo
const float GRAV = 0.05;
const float V0 = -0.5;
const int NUM_ROWS = 24;
const int NUM_COLS = 80;
const int PIPE_RADIUS = 3;
const int OPENING_WIDTH = 7;
const int FLAPPY_COL = 10;
const float TARGET_FPS = 24;
const float START_TIME_SEC = 3;
const int PROG_BAR_LEN = 76;
const int PROG_BAR_ROW = 22;
const int SCORE_START_COL = 62;

// Variáveis globais
int frame = 0;
int score = 0;
int sdigs = 1;
int best_score = 0;
int bdigs = 1;
vpipe p1, p2;

// Função auxiliar para converter um caractere em uma string
void chtostr(char ch, char *str) {
	str[0] = ch;
	str[1] = '\0';
}

// Função para desenhar o teto e o chão na tela
void draw_floor_and_ceiling(int ceiling_row, int floor_row,
		char ch, int spacing, int col_start) {
	char c[2];
	chtostr(ch, c);
	int i;
	for (i = col_start; i < NUM_COLS - 1; i += spacing) {
		if (i < SCORE_START_COL - sdigs - bdigs)
			mvprintw(ceiling_row, i, c);
		mvprintw(floor_row, i, c);
	}
}

// Função para atualizar a posição do cano
void pipe_refresh(vpipe *p) {
	if(p->center + PIPE_RADIUS < 0) {
		p->center = NUM_COLS + PIPE_RADIUS;
		p->opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
		score++;
		if(sdigs == 1 && score > 9)
			sdigs++;
		else if(sdigs == 2 && score > 99)
			sdigs++;
	}
	p->center--;
}

// Função para obter a posição da linha correspondente a um cano
int get_orow(vpipe p, int top) {
	return p.opening_height * (NUM_ROWS - 1) -
			(top ? 1 : -1) * OPENING_WIDTH / 2;
}

// Função para desenhar um cano na tela
void draw_pipe(vpipe p, char vch, char hcht, char hchb,
		int ceiling_row, int floor_row) {
	int i, upper_terminus, lower_terminus;
	char c[2];
	for(i = ceiling_row + 1; i < get_orow(p, 1); i++) {
		if ((p.center - PIPE_RADIUS) >= 0 &&
				(p.center - PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center - PIPE_RADIUS, c);
		}
		if ((p.center + PIPE_RADIUS) >= 0 &&
				(p.center + PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center + PIPE_RADIUS, c);
		}
	}
	upper_terminus = i;
	for (i = -PIPE_RADIUS; i <= PIPE_RADIUS; i++) {
		if ((p.center + i) >= 0 &&
				(p.center + i) < NUM_COLS - 1) {
			chtostr(hcht, c);
			mvprintw(upper_terminus, p.center + i, c);
		}
	}
	for(i = floor_row - 1; i > get_orow(p, 0); i--) {
		if ((p.center - PIPE_RADIUS) >= 0 &&
				(p.center - PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center - PIPE_RADIUS, c);
		}
		if ((p.center + PIPE_RADIUS) >= 0 &&
				(p.center + PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center + PIPE_RADIUS, c);
		}
	}
	lower_terminus = i;

	// Desenhar a parte horizontal inferior do cano.
	for (i = -PIPE_RADIUS; i <= PIPE_RADIUS; i++) {
		if ((p.center + i) >= 0 &&
				(p.center + i) < NUM_COLS - 1) {
			chtostr(hchb, c);
			mvprintw(lower_terminus, p.center + i, c);
		}
	}
}

// Função para obter a posição vertical do Flappy Bird no momento t
int get_flappy_position(flappy f) {
	return f.h0 + V0 * f.t + 0.5 * GRAV * f.t * f.t;
}

// Função para verificar se o Flappy Bird colidiu com um cano
int crashed_into_pipe(flappy f, vpipe p) {
	if (FLAPPY_COL >= p.center - PIPE_RADIUS - 1 &&
			FLAPPY_COL <= p.center + PIPE_RADIUS + 1) {

		if (get_flappy_position(f) >= get_orow(p, 1)  + 1 &&
				get_flappy_position(f) <= get_orow(p, 0) - 1) {
			return 0;
		}
		else {
			return 1;
		}
	}
	return 0;
}

// Função para exibir a tela de falha (game over)
int failure_screen() {
	char ch;
	clear();
	mvprintw(NUM_ROWS / 2 - 1, NUM_COLS / 2 - 22,
			"Flappy died :-(. <Enter> to flap, 'q' to quit.\n");
	refresh();
	timeout(-1);
	ch = getch();
	switch(ch) {
	case 'q':
		endwin();
		exit(0);
		break;
	default:
		if (score > best_score)
			best_score = score;
		if (bdigs == 1 && best_score > 9)
			bdigs++;
		else if(bdigs == 2 && best_score > 99)
			bdigs++;
		score = 0;
		sdigs = 1;
		return 1;
	}
	endwin();
	exit(0);
}

// Função para desenhar o Flappy Bird na tela
int draw_flappy(flappy f) {
	char c[2];
	int h = get_flappy_position(f);

	// Se o Flappy Bird colidiu como teto ou o chão...
	if (h <= 0 || h >= NUM_ROWS - 1)
		return failure_screen();

	// Se o Flappy Bird colidiu com um cano...
	if (crashed_into_pipe(f, p1) || crashed_into_pipe(f, p2)) {
		return failure_screen();
	}
	if (GRAV * f.t + V0 > 0) {
		chtostr('\\', c);
		mvprintw(h, FLAPPY_COL - 1, c);
		mvprintw(h - 1, FLAPPY_COL - 2, c);
		chtostr('0', c);
		mvprintw(h, FLAPPY_COL, c);
		chtostr('/', c);
		mvprintw(h, FLAPPY_COL + 1, c);
		mvprintw(h - 1, FLAPPY_COL + 2, c);
	}
	else {
		// Asa esquerda
		if (frame % 6 < 3) {
			chtostr('/', c);
			mvprintw(h, FLAPPY_COL - 1, c);
			mvprintw(h + 1, FLAPPY_COL - 2, c);
		}
		else {
			chtostr('\\', c);
			mvprintw(h, FLAPPY_COL - 1, c);
			mvprintw(h - 1, FLAPPY_COL - 2, c);
		}

		chtostr('0', c);
		mvprintw(h, FLAPPY_COL, c);
		if (frame % 6 < 3) {
			chtostr('\\', c);
			mvprintw(h, FLAPPY_COL + 1, c);
			mvprintw(h + 1, FLAPPY_COL + 2, c);
		}
		else {
			chtostr('/', c);
			mvprintw(h, FLAPPY_COL + 1, c);
			mvprintw(h - 1, FLAPPY_COL + 2, c);
		}
	}
	return 0;
}

// Função para exibir a tela inicial do jogo
void splash_screen() {
	int i;
	int r = NUM_ROWS / 2 - 6;
	int c = NUM_COLS / 2 - 22;

	// Imprimir o título.
	mvprintw(r, c,     " ___ _                       ___ _        _ ");
	mvprintw(r + 1, c, "| __| |__ _ _ __ _ __ _  _  | _ |_)_ _ __| |");
	mvprintw(r + 2, c, "| _|| / _` | '_ \\ '_ \\ || | | _ \\ | '_/ _` |");
	mvprintw(r + 3, c, "|_| |_\\__,_| .__/ .__/\\_, | |___/_|_| \\__,_|");
	mvprintw(r + 4, c, "           |_|  |_|   |__/                  ");
	mvprintw(NUM_ROWS / 2 + 1, NUM_COLS / 2 - 10,
			"Press <up> to flap!");

	// Imprimir a barra de progresso.
	mvprintw(PROG_BAR_ROW, NUM_COLS / 2 - PROG_BAR_LEN / 2 - 1, "[");
	mvprintw(PROG_BAR_ROW, NUM_COLS / 2 + PROG_BAR_LEN / 2, "]");
	refresh();
	for(i = 0; i < PROG_BAR_LEN; i++) {
		usleep(1000000 * START_TIME_SEC / (float) PROG_BAR_LEN);
		mvprintw(PROG_BAR_ROW, NUM_COLS / 2 - PROG_BAR_LEN / 2 + i, "=");
		refresh();
	}
	usleep(1000000 * 0.5);
}

int main() {
	int leave_loop = 0;
	int ch;
	flappy f;
	int restart = 1;

	srand(time(NULL));

	// Inicializar o ncurses
	initscr();
	raw();					// Desativar o buffer de linha
	keypad(stdscr, TRUE);
	noecho();				// Não exibir caracteres digitados
	curs_set(0);
	timeout(0);
	splash_screen();
	while(!leave_loop) {
		if (restart) {
			timeout(0); // Não bloquear na entrada.
			p1.center = (int)(1.2 * (NUM_COLS - 1));
			p1.opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
			p2.center = (int)(1.75 * (NUM_COLS - 1));
			p2.opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
			f.h0 = NUM_ROWS / 2;
			f.t = 0;
			restart = 0;
		}

		usleep((unsigned int) (1000000 / TARGET_FPS));

		// Processar as teclas pressionadas.
		ch = -1;
		ch = getch();
		switch (ch) {
		case 'q': // Sair.
			endwin();
			exit(0);
			break;
		case KEY_UP: // Dar um impulso ao Flappy Bird!
			f.h0 = get_flappy_position(f);
			f.t = 0;
			break;
		default: // Deixar o Flappy Bird cair ao longo da parábola.
			f.t++;
		}
		clear();
		draw_floor_and_ceiling(0, NUM_ROWS - 1, '/', 2, frame % 2);
		draw_pipe(p1, '|', '=', '=', 0, NUM_ROWS - 1);
		draw_pipe(p2, '|', '=', '=', 0, NUM_ROWS - 1);
		pipe_refresh(&p1);
		pipe_refresh(&p2);
		if(draw_flappy(f)) {
			restart = 1;
			continue;
		}
		mvprintw(0, SCORE_START_COL - bdigs - sdigs,
				" Score: %d  Best: %d", score, best_score);
		refresh();
		frame++;
	}
	endwin();
	return 0;
}

/*
Este código implementa um jogo Flappy Bird em um terminal de texto usando a biblioteca ncurses. O jogador controla um pássaro chamado Flappy, que está voando entre canos. O objetivo do jogo é evitar que o Flappy colida com os canos e o chão.
A função main() é o ponto de entrada do programa. Ela inicializa a biblioteca ncurses, configura algumas opções de tela e exibe uma tela de splash. Em seguida, o programa entra em um loop principal onde o jogo é executado. O loop principal processa as entradas do usuário, atualiza o estado do jogo e desenha os elementos na tela. Se o Flappy colidir com um cano ou o chão, a função failure_screen() é chamada para exibir a tela de falha.
Outras funções auxiliares são usadas para desenhar os elementos do jogo, como os canos, o Flappy Bird e o teto e o chão. A estrutura vpipe representa um cano e aestrutura flappy representa o Flappy Bird. O código também contém constantes e variáveis globais para armazenar informações sobre o jogo, como a pontuação e a posição dos elementos.
Em resumo, o código implementa a mecânica básica do jogo Flappy Bird usando a biblioteca ncurses para exibir os elementos na tela do terminal. Ele lida com as entradas do jogador, atualiza o estado do jogo e verifica colisões.
*/
