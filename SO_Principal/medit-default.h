#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <stdbool.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

#define MAX 25
#define MAX_LINHAS 15
#define MAX_COLUNAS 45
#define MAX_USERS 3
#define FIFO_SERVIDOR "fifoServ"
#define TIMEOUT 2000

typedef struct{
    char nome[8];
    bool logado;  
    int pid;
}USER;

typedef struct{
    int id;
    char nome_fifo[8];
    int num_users;
}FIFO;

typedef struct{
    int res;
    char fifo_iteracao[8];
}MSG;

typedef struct{
    char * linha;
}TEXTO;

typedef struct{
    int linhas;
    int colunas;
    char texto[600];
}TABELA;