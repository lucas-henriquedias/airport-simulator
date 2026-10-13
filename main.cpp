
#include <iostream>

typedef enum { 
    AGENDADO, 
    NA_FILA, 
    EM_OPERACAO, 
    CONCLUIDO, 
    CANCELADO 
} Status;

typedef enum {
    POUSO,
    DECOLAGEM
} TipoOperacao;

typedef struct {
    int id;
    char modelo[30];
    int capacidade;
    int autonomia;
} Aeronave;

typedef struct {
    int codigo[10];
    char origem[30];
    char destino[30];
    int horario;
    Status status;
    TipoOperacao tipo;
    int prioridade;
} Voo;


typedef struct {
    int valor;
    Voo *voo;
} Pista;

/*====================================*/

/*====================================*/


int main () {

    return 0;
}


void exibirMenu() {
    printf("\n=========== AEROPORTO ==========\n");
    printf(" 1 Cadastrar aeronave");
}