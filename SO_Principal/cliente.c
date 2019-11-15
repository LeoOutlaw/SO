/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: leonardo
 *
 * Created on October 25, 2018, 6:04 PM
 */

#include "cliente-default.h"

int x = 0, y = 0, max_x, max_y;
int res;
int flag = 0;
int pid_servidor;
bool editar = false;
char aux[MAX_COLUNAS + 1];
char *novo;
char new_fifo[8];
char fifo_cliente[8];
TABELA tabela;
TEXTO * texto;
USER util;
MSG recv, mandar; //recebe a resposta do servidor e manda pedido ao servidor
pthread_t t_text;
WINDOW * win_consola;

void sinais(int sinal, siginfo_t *valor, void *n);
void wprinterror(WINDOW * win, char str[]);
void wprintlog(WINDOW * win, char str[]);
void editaLinha(int linha);
void lerTexto(char temp[]);
void * modoTexto();

int main(int argc, char** argv) {
    int fd_fifo, c, tecla;



    struct sigaction sinal;
    sigemptyset(&sinal.sa_mask);
    sinal.sa_sigaction = sinais;
    sinal.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR1, &sinal, NULL);
    sigaction(SIGUSR2, &sinal, NULL);

    initscr();
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLUE);
    init_pair(2, COLOR_RED, COLOR_BLUE);
    init_pair(3, COLOR_GREEN, COLOR_RED);
    clear();
    cbreak();

    getmaxyx(stdscr, max_y, max_x);
    win_consola = newwin(max_y, max_x, 0, 0);
    scrollok(win_consola, true);
    wbkgd(win_consola, COLOR_PAIR(1));
    keypad(win_consola, true);
    refresh();
    wrefresh(win_consola);



    union sigval valor_sinal_main;

    if (access(FIFO_SERVIDOR, F_OK) != 0) {
        endwin();
        printf("[ERRO] O servidor nao esta a correr!\n");
        return EXIT_SUCCESS;
    } else {
        int aux, fd_fifo, fd_int, help;
        char temp[255];
        util.logado = false;
        util.pid = getpid();
        bool flag = false; // para saber se meteu login
        bool flag2 = false;
        sprintf(fifo_cliente, "f%d", getpid());
        mkfifo(fifo_cliente, 0600);
        /* if (argc == 2){
             strcpy(nome, argv[1]);
             flag = true;
         }else if(argc == 1){

         }else{
             printf("[ERRO] Demasiados argumentos");
             delwin(win_consola);
             endwin();
             return EXIT_SUCCESS;
         }*/

        do {

            if (!util.logado) {
                if (!flag) {
                    wprintw(win_consola, "Por favor introduza o seu nome de utilizador:\n");
                    wscanw(win_consola, "%s", util.nome);
                }
                fd_fifo = open(FIFO_SERVIDOR, O_WRONLY);
                write(fd_fifo, &util, sizeof (util));
                close(fd_fifo);

                sprintf(fifo_cliente, "f%d", util.pid);
                fd_fifo = open(fifo_cliente, O_RDONLY);
                read(fd_fifo, &recv, sizeof (recv));
                help = recv.res;
                switch (help) {
                    case -1:
                        wprinterror(win_consola, "Nao consegui abrir o ficheiro!\n");
                        wrefresh(win_consola);
                        break;
                    case 0:
                        wprinterror(win_consola, "Nao encontrei o username!\n");
                        wrefresh(win_consola);
                        break;
                    case 1:
                        wprinterror(win_consola, "O username ja se encontra logado!\n");
                        wrefresh(win_consola);
                        break;
                    case 2:
                        wprintlog(win_consola, "Fez login com sucesso!\n");

                        strcpy(new_fifo, recv.fifo_iteracao);
                        util.logado = true;
                        flag = true;

                        wrefresh(win_consola);
                        break;
                    case 3:
                        wprinterror(win_consola, "O servidou chegou ao max de clientes logados!\n");
                        wrefresh(win_consola);
                        break;
                }
                close(fd_fifo);
                if (!util.logado) {
                    wprintw(win_consola, "Se quiser sair precione 'ESC' se nao precione qualquer outro botao!\n");
                    c = wgetch(win_consola);
                    switch (c) {
                        case 27:
                            delwin(win_consola);
                            endwin();
                        default:
                            break;
                    }
                }
            } else {
                fd_int = open(new_fifo, O_RDONLY);
                read(fd_int, &tabela, sizeof (tabela));
                char temp[500];
                sprintf(temp, "%s", tabela.texto);
                lerTexto(temp);
                close(fd_int);
                break;
            }
        } while (1);
        if (util.logado) {
            pthread_join(t_text, NULL);
        }
    }
    valor_sinal_main.sival_int = 0;
    sigqueue(pid_servidor, SIGUSR2, valor_sinal_main);
    free(texto);
    delwin(win_consola);
    endwin();
    return EXIT_SUCCESS;
}

void sinais(int sinal, siginfo_t *valor, void *n) {
    int c, i;
    union sigval valor_sinal_sinais;
    switch (sinal) {
        case SIGUSR1:
            pid_servidor = valor->si_pid;
            break;
        case SIGUSR2:
            if (valor->si_int == 1) {
                wprintlog(win_consola, "Ja pode editar o texto!\n");
                pthread_create(&t_text, NULL, &modoTexto, NULL);
            } else if (valor->si_int == 0) {
                wclear(win_consola);
                wprintlog(win_consola, "Servidor fechou!\n");
                pthread_cancel(t_text);
                unlink(fifo_cliente);
                pthread_join(t_text, NULL);
                free(texto);
                delwin(win_consola);
                endwin();
                exit(EXIT_SUCCESS);
            }
    }
}

void lerTexto(char temp[]) {
    int linhas = 0;
    int colunas = 0;
    int i = 0;
    int ky;
    texto = malloc(sizeof (TEXTO) * tabela.linhas);
    for (ky = 0; ky < tabela.linhas; ky++) {
        texto[ky].linha = malloc((tabela.colunas + 1) * sizeof (char));
    }

    while (temp[i] != '\0') {
        if (temp[i] == '\n') {
            linhas++;
            colunas = 0;
        } else if (colunas == tabela.colunas) {
            linhas++;
            colunas = 0;
        } else {
            texto[linhas].linha[colunas] = temp[i];
            colunas++;
        }
        if (linhas > tabela.linhas) {
            break;
        }
        i++;
    }
    if (linhas < tabela.linhas) {
        do {
            for (i = 0; i < tabela.colunas; i++) {
                texto[linhas].linha[i] = ' ';
            }
            linhas++;
            i = 0;
        } while (linhas < tabela.linhas);

    }
}

void * modoTexto() {
    int linha = 0, tecla, coluna = 8;
    int pos_y = 5, pos_x = 8;
    wclear(win_consola);
    wrefresh(win_consola);
    // pthread_create(&thread_actualizar, NULL, &actualizar, NULL);
    while (linha < tabela.linhas) {
        mvwprintw(win_consola, linha + 5, 8, texto[linha].linha);
        mvwprintw(win_consola, linha + 5, 2, "[%2d]", linha + 1);
        linha++;
    }
    wmove(win_consola, 5, 8);
    wrefresh(win_consola);
    linha = 0;
    do {
        tecla = wgetch(win_consola);
        switch (tecla) {
            case KEY_LEFT:
                coluna--;
                pos_x--;
                if (coluna == -1) {
                    coluna = 0;
                }
                if (pos_x == 7) {
                    pos_x = 8;
                }
                break;
            case KEY_RIGHT:
                coluna++;
                pos_x++;
                if (coluna >= tabela.colunas - 1) {
                    coluna = tabela.colunas - 1;
                }
                if (pos_x > tabela.colunas + 7) {
                    pos_x = tabela.colunas + 7;
                }
                break;
            case KEY_UP:
                linha--;
                pos_y--;
                if (linha == -1) {
                    linha = 0;
                }
                if (pos_y == 4) {
                    pos_y = 5;
                }
                break;
            case KEY_DOWN:
                linha++;
                pos_y++;
                if (linha >= tabela.linhas - 1) {
                    coluna = tabela.linhas - 1;
                }
                if (pos_y > (tabela.linhas + 5) - 1) {
                    pos_y = (tabela.linhas+5) - 1;
                }
                break;
            case '\n':
                editaLinha(linha);
                break;
            default:
                break;
        }
        wmove(win_consola, pos_y, pos_x);
        wrefresh(win_consola);
    } while (tecla != 27);
    pthread_exit(0);
}

void editaLinha(int linha) {
    int coluna = 0, pos_x = 8, pos_y= linha+ 5;
    int i, j;
    char aux[tabela.colunas];
    for (i = 0; i < tabela.colunas; i++) {
        aux[i] = texto[linha].linha[i];
    }
    wmove(win_consola, pos_y, pos_x);
    wrefresh(win_consola);
    do {
        res = wgetch(win_consola);

        switch (res) {
            case KEY_LEFT:
                coluna--;
                pos_x--;
                if (coluna == -1) {
                    coluna = 0;
                }
                if (pos_x == 7) {
                    pos_x = 8;
                }
                break;   free(aux);
            case KEY_RIGHT:
                coluna++;
                pos_x++;
                if (coluna >= tabela.colunas - 1) {
                    coluna = tabela.colunas - 1;
                }
                if (pos_x > tabela.colunas + 7) {
                    pos_x = tabela.colunas + 7;
                }
                break;
            case KEY_BACKSPACE:
                for (int i = coluna; i <= tabela.colunas; i++) {
                    if (texto[linha].linha[i + 1] == '\0') {
                        texto[linha].linha[i] = ' ';
                        texto[linha].linha[i - 1] = texto[linha].linha[i];
                        break;
                    } else {
                        texto[linha].linha[i - 1] = texto[linha].linha[i];
                    }
                }
                pos_x--;
                if (pos_x < 8) {
                    pos_x = 8;
                }
                coluna--;
                if (coluna < 0) {
                    coluna = 0;
                }
                break;
            case 27:
                for (i = 0; i < tabela.colunas; i++) {
                    texto[linha].linha[i] = aux[i];
                }
                break;
            case '\n':
                break;
            default:
                if (coluna == tabela.colunas - 1) {
                    texto[linha].linha[coluna] = res;
                    texto[linha].linha[coluna + 1] = '\0';
                    break;
                } else {
                    for (i = tabela.colunas - 1; i > coluna; i--) {
                        texto[linha].linha[i] = texto[linha].linha[i - 1];
                    }
                    texto[linha].linha[coluna] = res;
                    coluna++;
                    pos_x++;
                }
        }
        mvwprintw(win_consola, pos_y, 8, "%s", texto[linha].linha);
        wmove(win_consola, pos_y, pos_x);
        wrefresh(win_consola);
    } while (res != 27 && res != '\n');
}

void wprinterror(WINDOW * win, char str[]) {

    wattron(win, A_BOLD | COLOR_PAIR(2));
    wprintw(win, "[ERRO] ");
    wattroff(win, A_BOLD | COLOR_PAIR(2));
    wattron(win, A_BOLD);
    wprintw(win, "%s", str);
    wattroff(win, A_BOLD);
}

void wprintlog(WINDOW * win, char str[]) {
    wattron(win, A_BOLD | COLOR_PAIR(3));
    wprintw(win, "[->]");
    wattroff(win, A_BOLD | COLOR_PAIR(3));
    wattron(win, A_BOLD);
    wprintw(win, "%s", str);
    wattroff(win, A_BOLD);
}

