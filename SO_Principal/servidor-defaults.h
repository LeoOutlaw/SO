#include "medit-default.h"
#define FICHEIRO_UTILIZADORES "medit.db"

typedef struct {
    int nlinhas;
    int ncolunas;
    int nr_max_util;
    char *f_nome;
    char *nome_fifo;
    int nr_named_pipe;
}SETTINGS;

typedef struct{
    USER user;
    char fifo_a_usar[8];
    time_t data_sessao;
    bool edit;
    int lin_editar;
    float perc_edit;
}UTILIZADORES;