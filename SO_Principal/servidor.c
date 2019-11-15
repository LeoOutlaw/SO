#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "servidor-defaults.h"

int num_utilizadores_on = 0; //saber os numero de clientes online
char comando_separado[2][25]; //separar comandos
char nomeFicheiro[10]; //users recebido pelo servidor
char nome_ficheiro[10]; // ficheiro de texto para editar
char fifos[8]; //string para nome dos fifos de iteracao
char a[500];
bool editar = false;
pthread_t thread_respondeLogin, t_resp;
SETTINGS settings;
char fifo_cliente[8];
WINDOW *win_consola;
TABELA tabela;
USER util;
UTILIZADORES * users;
FIFO * fifo;
TEXTO * text, *original;
pthread_t t_controla;

void *controlaTexto(void * dadosCliente);
void preencheTabela(TABELA * tab);
void sinais(int sinal, siginfo_t * valor, void *n);
void wprintlog(WINDOW * win, char str[]);
void wprinterror(WINDOW * win, char str[]);
void separaComandos(char str[]);
int validaUsername(char utilizadorInput[]);
bool confirmaVariaveisAmbiente(SETTINGS * set);
bool verificaUserOnline(char login[]);
void *respLogin();
void criarFifosIteracao();
bool adicionaUser(USER * user);
bool abreTexto(char * ficheiroTxt);
void terminarServidor();
void *editaCliente(void * dadosCliente);
void lerText();
void malloc_erro();
TEXTO * iniciarTexto(char tex[]);
void terminarLigacaoClientes();

int main(int argc, char** argv) {
    int max_x, max_y, i;
    char comando[50];
    bool flag;
    a[0] = '\0';

    union sigval valor_sinal_main;

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

    if (access(FIFO_SERVIDOR, F_OK) == 0) {
        endwin();
        printf("[ERRO] Ja existe um servidor a correr!\n");
        return EXIT_SUCCESS;
    } else {
        if (argc == 2) {
            strcpy(nomeFicheiro, argv[1]);
        } else if (argc == 1) {
            strcpy(nomeFicheiro, FICHEIRO_UTILIZADORES);
        } else {
            printf("[ERRO] Demasiados argumentos!\n");
            delwin(win_consola);
            endwin();
            return EXIT_SUCCESS;
        }

        flag = confirmaVariaveisAmbiente(&settings);
        if (flag == false) {
            wgetch(win_consola);
            delwin(win_consola);
            endwin();
            return EXIT_SUCCESS;
        }
        mkfifo(FIFO_SERVIDOR, 0600);
        if ((original = malloc(sizeof (TEXTO) * settings.nlinhas)) == NULL) {
            malloc_erro();
        }
        for (i = 0; i < settings.nlinhas; i++) {
            original[i].linha = malloc((settings.ncolunas + 1) * sizeof (char));
        }
        if ((text = malloc(sizeof (TEXTO) * settings.nlinhas)) == NULL) {
            malloc_erro();
        }
        for (i = 0; i < settings.nlinhas; i++) {
            text[i].linha = malloc((settings.ncolunas + 1) * sizeof (char));
        }
        text = iniciarTexto(a);
        original = iniciarTexto(a);
        users = malloc(sizeof (UTILIZADORES) * settings.nr_max_util);
        for (i = 0; i < settings.nr_max_util; i++) {
            users[i].user.logado = false;
            users[i].user.pid = 0;
        }
        criarFifosIteracao();
        pthread_create(&thread_respondeLogin, NULL, &respLogin, NULL);
        // pthread_create(&t_controla, NULL, &controlaTexto, NULL);
        do {
            wscanw(win_consola, "%49[^\n]", comando);
            separaComandos(comando);
            if (!strcmp(comando_separado[0], "settings")) {
                wprintlog(win_consola, "Numero de Linhas: ");
                wprintw(win_consola, "%i\n", settings.nlinhas);
                wprintlog(win_consola, "Numero de Colunas: ");
                wprintw(win_consola, "%i\n", settings.ncolunas);
                wprintlog(win_consola, "Numero de Users: ");
                wprintw(win_consola, "%i\n", settings.nr_max_util);
                wprintlog(win_consola, "Nome do ficheiro: ");
                wprintw(win_consola, "%s\n", nomeFicheiro);
                wprintlog(win_consola, "Nome do FIFO principal: ");
                wprintw(win_consola, "%s\n", FIFO_SERVIDOR);
                wprintlog(win_consola, "Numero de utilizadores online: ");
                wprintw(win_consola, "%d\n", num_utilizadores_on);
            } else if (!strcmp(comando_separado[0], "load")) {
                wprintlog(win_consola, "Comando ");
                wprintw(win_consola, "[%s]\n", comando_separado[0]);
                strcpy(nome_ficheiro, comando_separado[1]);
                lerText(nome_ficheiro);
                editar = true;
                text = iniciarTexto(a);
                original = iniciarTexto(a);
            } else if (!strcmp(comando_separado[0], "save")) {
                wprintlog(win_consola, "Comando ");
                wprintw(win_consola, "[%s]\n", comando_separado[0]);
            } else if (!strcmp(comando_separado[0], "free")) {
                wprintlog(win_consola, "Comando ");
                wprintw(win_consola, "[%s]\n", comando_separado[0]);
            } else if (!strcmp(comando_separado[0], "statistics")) {
                wprintlog(win_consola, "Comando ");
                wprintw(win_consola, "[%s]\n", comando_separado[0]);
            } else if (!strcmp(comando_separado[0], "users")) {
                wprintlog(win_consola, "Comando ");
                wprintw(win_consola, "[%s]\n", comando_separado[0]);
                for (i = 0; i < settings.nr_max_util; i++) {
                    if (users[i].user.pid != 0) {
                        wprintw(win_consola, "%s\n", users[i].user.nome);
                    }
                }
            } else if (!strcmp(comando_separado[0], "text")) {
                wprintlog(win_consola, "Comando ");
                wprintw(win_consola, "[%s]\n", comando_separado[0]);
            } else if (!strcmp(comando_separado[0], "shutdown")) {
                break;
            }
        } while (strcmp(comando_separado[0], "shutdown"));

        terminarServidor();
        return (EXIT_SUCCESS);
    }
}

void *controlaTexto(void * dadosCliente) {
    int i, fd_val, b, inteiro;

    while (true) {
        b = open("fifo1", O_RDONLY);
        read(b, &inteiro, sizeof (inteiro));
        wprintw(win_consola, "%d\n", inteiro);

        close(b);

    }

}

void terminarServidor() {
    union sigval valor;
    int i;
    editar = false;
    terminarLigacaoClientes();
    pthread_cancel(thread_respondeLogin);
    pthread_cancel(t_resp);
    for (i = 1; i <= settings.nr_max_util; i++) {
        sprintf(fifos, "fifo%d", i);
        unlink(fifos);
    }
    for(i=0; i<= settings.nr_max_util;i++){
        sprintf(fifos, "f%d", users[i].user.pid);
        unlink(fifos);
    }
    free(text);
    free(original);
    free(fifo);
    free(users);
    delwin(win_consola);
    endwin();
    unlink(FIFO_SERVIDOR);
}

void terminarLigacaoClientes() {
    int i;
    union sigval sinal_termina;
    sinal_termina.sival_int = 0;
    for (i = 0; i < settings.nr_max_util; i++) {
        if (users[i].user.logado) {
            sigqueue(users[i].user.pid, SIGUSR2, sinal_termina);
        }
    }
}

void sinais(int sinal, siginfo_t * valor, void *n) {
    union sigval valor_sinal;
    int i;
    switch (sinal) {
        case SIGUSR1:
            terminarServidor();
            exit(EXIT_SUCCESS);
        case SIGUSR2:
            if (valor->si_int == 0) {
                for (i = 0; i < settings.nr_max_util; i++) {
                    if (users[i].user.pid == valor->si_pid) {
                        users[i].edit = false;
                        users[i].user.logado = false;
                        strcpy(users[i].user.nome, "");
                        users[i].user.pid = 0;
                    }
                }
            }

    }
}

void criarFifosIteracao() {
    int i;
    fifo = malloc(sizeof (FIFO) * settings.nr_max_util);
    for (i = 1; i <= settings.nr_max_util; i++) {
        sprintf(fifo[i - 1].nome_fifo, "fifo%d", i);
        fifo[i - 1].id = i;
        fifo[i - 1].num_users = 0;
        mkfifo(fifo[i - 1].nome_fifo, 0600);
    }
}

TEXTO * iniciarTexto(char tex[]) {
    int i;
    int linha = 0, coluna = 0;
    TEXTO * aux;
    aux = malloc(sizeof (TEXTO) * settings.nlinhas);
    for (i = 0; i < settings.nlinhas; i++) {
        aux[i].linha = malloc((settings.ncolunas + 1) * sizeof (char));
    }
    if (strlen(tex) == 0) {
        for (linha = 0; linha < settings.nlinhas; linha++) {
            for (coluna = 0; coluna < settings.ncolunas; coluna++) {
                aux[linha].linha[coluna] = ' ';
            }
        }
    } else {
        int linha = 0, coluna = 0, i = 0;
        while (tex[i] != '\0') {
            if (tex[i] == '\n') {
                linha++;
                coluna = 0;
            } else if (coluna == settings.ncolunas) {
                linha++;
                coluna = 0;
            } else {
                aux[linha].linha[coluna] = tex[i];
                coluna++;
            }
            if (linha > settings.nlinhas) {
                break;
            }
            i++;
        }
        if (linha < settings.nlinhas) {
            do {

                aux[linha].linha[0] = ' ';
                linha++;

            } while (linha != settings.nlinhas);

        }
    }
    return aux;
}

bool verificaUserOnline(char login[]) {
    int i = 0;
    for (i = 0; i < settings.nr_max_util; i++) {
        if (strcmp(users[i].user.nome, login) == 0) {
            return true;
        }
    }
    return false;
}

bool adicionaUser(USER * a) {
    int i;
    if (num_utilizadores_on == settings.nr_max_util) {
        return false;
    }
    for (i = 0; i < settings.nr_max_util; i++) {
        if (users[i].user.logado == false) {
            users[i].user.logado = true;
            users[i].user.pid = a->pid;
            strcpy(users[i].user.nome, a->nome);
            users[i].edit = false;
            users[i].data_sessao = time(NULL);
            break;
        }
    }
    return true;
}

void * respLogin() {
    int fd_fifo, aux, i, fd_val, flag, res_fifo;
    union sigval sinal_editar;
    char temp[255];
    MSG resposta_login;
    while (true) {
        fd_fifo = open(FIFO_SERVIDOR, O_RDONLY);
        aux = read(fd_fifo, &util, sizeof (util));
        if (aux == sizeof (util)) {
            sprintf(temp, "[%s] Esta a tentar fazer login\n", util.nome);
            sprintf(fifo_cliente, "f%d", util.pid);
            wprintlog(win_consola, temp);
            wrefresh(win_consola);
            if (validaUsername(util.nome) == 0) {
                sprintf(temp, "Username [%s],logo nao fez login!\n", util.nome);
                wprintlog(win_consola, temp);
                wrefresh(win_consola);
                flag = 0;
            } else if (validaUsername(util.nome) == 1) {
                if (verificaUserOnline(util.nome)) {
                    sprintf(temp, "[%s] ja se encontra logado!\n", util.nome);
                    flag = 1;
                    wprintlog(win_consola, temp);
                    wrefresh(win_consola);
                } else {
                    if (adicionaUser(&util)) {
                        num_utilizadores_on++; //funcao para saber se existe no ficheiro
                        sinal_editar.sival_int = 1;
                        if (sigqueue(util.pid, SIGUSR2, sinal_editar) != 0) {
                            wprinterror(win_consola, "Sinal nao enviado!\n");
                        } else {
                            wprintlog(win_consola, "Sinal enviado!!\n");
                        }
                        sprintf(temp, "[%s] Fez login!\n", util.nome);
                        flag = 2;
                        wprintlog(win_consola, temp);
                        wrefresh(win_consola);
                        for (i = 0; i < settings.nr_max_util; i++) {  // ter de ser criada nova variavel para comparar
                            if (fifo[i].num_users == 0) {
                                sprintf(resposta_login.fifo_iteracao, "fifo%d", i + 1);
                                break;
                            }
                        }
                        // Adicionar o users ao array de users online
                        // E mandar sinal a users recentemente logados se podem editar ou nao

                        for (i = 0; i < settings.nr_max_util; i++) {
                            if (users[i].user.pid == util.pid) {
                                strcpy(users[i].fifo_a_usar, resposta_login.fifo_iteracao);
                                preencheTabela(&tabela);
                                break;
                            }
                        }
                    } else {
                        sprintf(temp, "Nao e possivel fazer login(demasiados users online)!\n");
                        wprintlog(win_consola, temp);
                        wrefresh(win_consola);
                        flag = 3;
                    }
                }
            } else {

                flag = -1;
            }
            resposta_login.res = flag;
            res_fifo = open(fifo_cliente, O_WRONLY);
            write(res_fifo, &resposta_login, sizeof (resposta_login));
            close(res_fifo);
            fd_val = open(resposta_login.fifo_iteracao, O_WRONLY);
            write(fd_val, &tabela, sizeof (tabela));
            close(fd_val);
        }
        close(fd_fifo);
    }

}

void preencheTabela(TABELA * tab) {

    tab->colunas = settings.ncolunas;
    tab->linhas = settings.nlinhas;
    strcpy(tab->texto, a);
}

void separaComandos(char str[]) {
    char *token;
    const char s[] = " ";
    int i = 0;
    token = strtok(str, s);
    for (i = 0; i < 2; i++) {
        if (token == NULL) {
            strcpy(comando_separado[i], "");
        } else {

            strcpy(comando_separado[i], token);
        }
        token = strtok(NULL, s);
    }
}

int validaUsername(char utilizadorInput[]) {
    FILE *f;
    char linha[25];
    f = fopen(nomeFicheiro, "r");

    if (f != NULL) {
        do {
            fscanf(f, "%s", linha);
            if (strcmp(linha, utilizadorInput) == 0) {
                return 1;
            }
        } while (!feof(f));
    } else {

        return -1;
        wprintw(win_consola, "ERRO a abrir o ficheiro %s\n", nomeFicheiro);
    }
    fclose(f);

    return 0;
}

bool confirmaVariaveisAmbiente(SETTINGS * set) {
    if (getenv("MEDIT_MAXLINES") == NULL) {
        set->nlinhas = MAX_LINHAS;
    } else if (atoi(getenv("MEDIT_MAXLINES")) < 0 || atoi(getenv("MEDIT_MAXLINES")) > 15) {
        wprintw(win_consola, "ERRO nas variaveis de ambiente (linhas)!\n");
        return false;
    } else {
        set->nlinhas = atoi(getenv("MEDIT_MAXLINES"));
    }

    if (getenv("MEDIT_MAXCOLUMNS") == NULL) {
        //set->ncolunas = MAX_COLUNAS;
        set->ncolunas = MAX_COLUNAS;
    } else if (atoi(getenv("MEDIT_MAXCOLUMNS")) < 0 || atoi(getenv("MEDIT_MAXCOLUMNS")) > 45) {
        wprintw(win_consola, "ERRO nas variaveis de ambiente (colunas)!\n");
        return false;
    } else {
        set->ncolunas = atoi(getenv("MEDIT_MAXCOLUMNS"));
    }

    if (getenv("MEDIT_MAXUSERS") == NULL) {
        set->nr_max_util = MAX_USERS;
    } else if (atoi(getenv("MEDIT_MAXUSERS")) < 0 || atoi(getenv("MEDIT_MAXUSERS")) > set->nlinhas) {
        wprintw(win_consola, "ERRO nas variaveis de ambiente (users)!\n");
        return false;
    } else {

        set->nr_max_util = atoi(getenv("MEDIT_MAXUSERS"));
    }
    return true;
}

void wprintlog(WINDOW * win, char str[]) {

    wattron(win, A_BOLD | COLOR_PAIR(3));
    wprintw(win, "[->]");
    wattroff(win, A_BOLD | COLOR_PAIR(3));
    wattron(win, A_BOLD);
    wprintw(win, "%s", str);
    wattroff(win, A_BOLD);
}

void wprinterror(WINDOW * win, char str[]) {

    wattron(win, A_BOLD | COLOR_PAIR(2));
    wprintw(win, "[ERRO] ");
    wattroff(win, A_BOLD | COLOR_PAIR(2));
    wattron(win, A_BOLD);
    wprintw(win, "%s", str);
    wattron(win, A_BOLD);
}

void malloc_erro() {

    endwin();
    fprintf(stderr, "malloc error\n");
    exit(EXIT_FAILURE);
}

void lerText(char * ficheiroTxt) {
    FILE * f;
    int c;
    int i = 0;
    int linhas = 1, colunas = 1;
    f = fopen(ficheiroTxt, "r");
    if (f != NULL) {
        while ((c = fgetc(f)) != EOF) {
            if (colunas > settings.ncolunas) {
                a[i] = '\n';
                colunas = 0;
                linhas++;
            } else if (c == '\n') {
                a[i] = c;
                colunas = 0;
                linhas++;
            } else {
                a[i] = c;
                colunas++;
            }
            if (linhas > settings.nlinhas) {
                break;
            }
            i++;
        }
        if (linhas < settings.nlinhas) {
            do {
                i++;
                a[i] = '\n';
                linhas++;
            } while (linhas != settings.nlinhas);

        }
        a[i] = '\0';
    } else {
        wprinterror(win_consola, "A abrir o ficherio!\n");
    }
    fclose(f);

}