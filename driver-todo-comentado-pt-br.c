/**
 * @file
 * @autor Hamik Mukelyan
 *
 * Dirige uma versão baseada em texto do Flappy Bird, que foi projetada para ser executada em um console 80 x 24.
 */

#include <ncurses.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <limits.h>

//-------------------------------- Definições --------------------------------

/**
 * Representa um cano vertical pelo qual o Flappy Bird deve voar.
 */
typedef struct vpipe {

	/*
	 * A altura da abertura do cano como uma fração da altura da janela do console.
	 */
	float opening_height;

	/*
	 * O centro do cano está neste número de coluna (por exemplo, em algum lugar entre [0, 79]).
	 * Quando o centro + raio é negativo, o centro do cano é enrolado para algum lugar > do número de colunas e a altura da abertura é alterada.
	 */
	int center;
} vpipe;

/** Representa o Flappy Bird. */
typedef struct flappy {
	/* Altura do Flappy Bird no último pressionamento da seta para cima. */
	int h0;

	/* Tempo desde o último pressionamento da seta para cima. */
	int t;
} flappy;

//------------------------------ Constantes Globais -----------------------------

/** Constante de aceleração gravitacional */
const float GRAV = 0.05;

/** Velocidade inicial com o pressionamento da seta para cima */
const float V0 = -0.5;

/** Número de linhas na janela do console. */
const int NUM_ROWS = 24;

/** Número de colunas na janela do console. */
const int NUM_COLS = 80;

/** Raio de cada cano vertical. */
const int PIPE_RADIUS = 3;

/** Largura da abertura em cada cano. */
const int OPENING_WIDTH = 7;

/** Flappy Bird permanece nesta coluna. */
const int FLAPPY_COL = 10;

/** Mirando essa quantidade de quadros por segundo. */
const float TARGET_FPS = 24;

/** Tempo que a tela de splash é exibida. */
const float START_TIME_SEC = 3;

/** Comprimento da "barra de progresso" na tela de status. */
const int PROG_BAR_LEN = 76;

/** Número da linha em que a barra de progresso será exibida. */
const int PROG_BAR_ROW = 22;

const int SCORE_START_COL = 62;

//------------------------------ Variáveis Globais -----------------------------

/** Número do quadro. */
int frame = 0;

/** Número de canos que foram ultrapassados. */
int score = 0;

/** Número de dígitos na pontuação. */
int sdigs = 1;

/** Melhor pontuação até agora. */
int best_score = 0;

/** Número de dígitos na melhor pontuação. */
int bdigs = 1;

/** Os obstáculos de cano vertical. */
vpipe p1, p2;

//---------------------------------- Funções --------------------------------

/**
 * Converte o char fornecido em uma string.
 *
 * @param ch Char para converter em uma string.
 * @param[out] str Recebe 'ch' em uma string C terminada em nulo. Assume que str tem 2 bytes alocados.
 */
void chtostr(char ch, char *str) {
	str[0] = ch;
	str[1] = '\0';
}

/**
 * "Movendo" o chão e o teto são escritos na matriz da janela.
 *
 * @param ceiling_row
 * @param floor_row
 * @param ch Char a ser usado para o teto e o chão.
 * @param spacing Espaçamento entre os caracteres do chão e do teto
 * @param col_start Atrasa o início dos caracteres do chão e do teto em quantia definida
 */
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

/**
 * Atualiza o centro do cano e a altura da abertura para cada novo quadro. Se o cano
 * estiver suficientemente fora da tela à esquerda, o centro é retornado para a direita,
 * momento em que a altura da abertura é alterada.
 */
void pipe_refresh(vpipe *p) {

	// Se o cano sair da tela à esquerda, mova-o para o lado direito da tela.
	if(p->center + PIPE_RADIUS < 0) {
		p->center = NUM_COLS + PIPE_RADIUS;

		// Obtenha uma fração de altura de abertura.
		p->opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
		score++;
		if(sdigs == 1 && score > 9)
			sdigs++;
		else if(sdigs == 2 && score > 99)
			sdigs++;
	}
	p->center--;
}

/**
 * Obtém o número da linha do topo ou da base da abertura no cano fornecido.
 *
 * @param p O obstáculo de cano.
 * @param top Deve ser 1 para o topo, 0 para a base.
 *
 * @return Número da linha.
 */
int get_orow(vpipe p, int top) {
	return p.opening_height * (NUM_ROWS - 1) -
			(top ? 1 : -1) * OPENING_WIDTH / 2;
}

/**
 * Desenha o cano fornecido na janela usando 'vch' como caractere para a
 * parte vertical do cano e 'hch' como caractere para a parte horizontal.
 *
 * @param p
 * @param vch Caractere para a parte vertical do cano
 * @param hcht Caractere para a parte horizontal do cano superior
 * @param hchb Caractere para a parte horizontal do cano inferior
 * @param ceiling_row Começa o cano logo abaixo disso
 * @param floor_row Começa o cano logo acima disso
 */
void draw_pipe(vpipe p, char vch, char hcht, char hchb,
		int ceiling_row, int floor_row) {
	int i, upper_terminus, lower_terminus;
	char c[2];

	// Desenhe a parte vertical da metade superior do cano.
	for(i = ceiling_row + 1; i < get_orow(p, 1); i++) {
		if ((p.center - PIPE_RADIUS) >= 0 &&
				(p.center - PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch

, c);
			mvprintw(i, p.center - PIPE_RADIUS, c);
		}
		if ((p.center + PIPE_RADIUS) >= 0 &&
				(p.center + PIPE_RADIUS) < NUM_COLS - 1) {
			chtostr(vch, c);
			mvprintw(i, p.center + PIPE_RADIUS, c);
		}
	}
	upper_terminus = i;

	// Desenhe a parte horizontal da metade superior do cano.
	for (i = -PIPE_RADIUS; i <= PIPE_RADIUS; i++) {
		if ((p.center + i) >= 0 &&
				(p.center + i) < NUM_COLS - 1) {
			chtostr(hcht, c);
			mvprintw(upper_terminus, p.center + i, c);
		}
	}

	// Desenhe a parte vertical da metade inferior do cano.
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

	// Desenhe a parte horizontal da metade inferior do cano.
	for (i = -PIPE_RADIUS; i <= PIPE_RADIUS; i++) {
		if ((p.center + i) >= 0 &&
				(p.center + i) < NUM_COLS - 1) {
			chtostr(hchb, c);
			mvprintw(lower_terminus, p.center + i, c);
		}
	}
}

/**
 * Obtém a altura do Flappy Bird ao longo de sua trajetória parabólica.
 *
 * @param f Flappy Bird!
 *
 * @return altura como contagem de linhas
 */
int get_flappy_position(flappy f) {
	return f.h0 + V0 * f.t + 0.5 * GRAV * f.t * f.t;
}

/**
 * Retorna verdadeiro se o Flappy Bird colidir com um cano.
 *
 * @param f Flappy Bird!
 * @param p O obstáculo de cano vertical.
 *
 * @return 1 se o Flappy Bird colidir, 0 caso contrário.
 */
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

/**
 * Imprime uma tela de falha pedindo ao usuário para jogar novamente ou sair.
 *
 * @return 1 se o usuário quiser jogar novamente. Encerra o programa caso contrário.
 */
int failure_screen() {
	char ch;
	clear();
	mvprintw(NUM_ROWS / 2 - 1, NUM_COLS / 2 - 22,
			"Flappy Bird morreu :-(. <Enter> para voar, 'q' para sair.\n");
	refresh();
	timeout(-1); // Bloquear até o usuário digitar algo.
	ch = getch();
	switch(ch) {
	case 'q': // Sair.
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
		return 1; // Reiniciar o jogo.
	}
	endwin();
	exit(0);
}

/**
 * Desenha o Flappy Bird na tela e mostra a mensagem de morte se o Flappy Bird colidir com
 * o teto ou o chão. O usuário pode continuar jogando ou sair se o Flappy Bird morrer.
 *
 * @param f Flappy Bird!
 *
 * @return 0 se o Flappy Bird foi desenhado corretamente, 1 se o jogo deve ser reiniciado.
 */
int draw_flappy(flappy f) {
	char c[2];
	int h = get_flappy_position(f);

	// Se o Flappy Bird colidir com o teto ou o chão...
	if (h <= 0 || h >= NUM_ROWS - 1)
		return failure_screen();

	// Se o Flappy Bird colidir com um cano...
	if (crashed_into_pipe(f, p1) || crashed_into_pipe(f, p2)) {
		return failure_screen();
	}

	// Se estiver descendo, não bata asas.
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

	// Se estiver subindo, bata asas!
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

		// Corpo
		chtostr('0', c);
		mvprintw(h, FLAPPY_COL, c);

		// Asa direita
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

/**
 * Imprime uma tela de splash e mostra uma barra de progresso. NB arte ASCII foi gerada por patorjk.com.
 */
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
			"Pressione <up> para voar!");

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

//------------------------------------ Main -----------------------------------

int main()
{
	int leave_loop = 0;
	int ch;
	flappy f;
	int restart = 1;

	srand(time(NULL));

	// Inicializar ncurses
	initscr();
	raw();					// Desativar o buffer de linha
	keypad(stdscr, TRUE);
	noecho();				// Não ecoar para getch
	curs_set(0);
	timeout(0);

	splash_screen();

	while(!leave_loop) {

		// Se estivermos apenas iniciando um jogo, faça algumas inicializações.
		if (restart) {
			timeout(0); // Não bloquear na entrada.

			// Inicie os canos logo fora da vista à direita.
			p1.center = (int)(1.2 * (NUM_COLS - 1));
			p1.opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;
			p2.center = (int)(1.75 * (NUM_COLS - 1));
			p2.opening_height = rand() / ((float) INT_MAX) * 0.5 + 0.25;

			// Inicializar Flappy Bird
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
		default: // Deixar o Flappy Bird cair ao longo de sua trajetória parabólica.
			f.t++;
		}

		clear();

		// Imprimir chão e teto "em movimento"
		draw_floor_and_ceiling(0, NUM_ROWS - 1, '/', 2, frame % 2);

		// Atualizar as posições dos canos e desenhá-los.
		draw_pipe(p1, '|', '=', '=', 0, NUM_ROWS - 1);
		draw_pipe(p2, '|', '=', '=', 0, NUM_ROWS - 1);
		pipe_refresh(&p1);
		pipe_refresh(&p2);

		// Desenhar o Flappy Bird. Se o Flappy Bird colidir e o usuário quiser reiniciar...
		if(draw_flappy(f)) {
			restart = 1;
			continue; // ...então reinicie o jogo.
		}

		mvprintw(0, SCORE_START_COL - bdigs - sdigs,
				" Pontuação: %d  Melhor: %d", score, best_score);

		// Exibir todos os caracteres para este quadro.
		refresh();
		frame++;
	}

	endwin();

	return 0;
}"
