/*
=====================================================================
    SISTEMA DE SIMULAÇÃO DE AEROPORTO E CONTROLE DE VOOS
=====================================================================
*/

#include <iostream>     // lib de entrada e saída de dados
#include <cstring>      // strcpy, strcmp (copiar e comparar textos) e NULL

using namespace std;

const int MAX_AERONAVES = 50;   // máximo de aeronaves cadastradas
const int MAX_VOOS = 200;       // máximo de voos cadastrados
const int NUM_PISTAS = 2;       // pistas do aeroporto (recurso limitado)


// ================= STRUCTS ===================
struct Aeronave {   // Representa uma aeronave do aeroporto.
    int id;             // posição no vetor = id - 1
    char modelo[20];    // ex: CarboX306
    int capacidade;     // quantidade de passageiros
    int autonomia;      // distância max que consegue voar (km)
    int emUso;          // 1 = já está em uma fila / 0 = livre
                        // |->(impede a mesma aeronave em duas filas)
};

struct Voo {    // Representa um voo.
    char codigo[10];    // ex: AE101 (único para cada voo)
    char origem[50];
    char destino[50];
    int hora;           // 0 a 23
    int minuto;         // 0 a 59
    char status[20];    // "Agendado", "Na fila", "Concluido" ou "Cancelado"
    int idAeronave;     // id da aeronave que faz o voo
    int distancia;      // km (a aeronave precisa ter autonomia >= distancia)
    int tipo;           // 1 = Pouso, 2 = Decolagem (decide em qual fila entra)
    int prioridade;     // 1 = Emergência, 2 = Alta, 3 = Normal (menor = mais urgente)
    int pista;          // pista usada (1 até NUM_PISTAS)
};


// ---------- FILA: lista encadeada simples ------------
struct NoFila {
    Voo* voo;           // aponta para o voo dentro do vetor voos[]
    NoFila* prox;       // próximo nó da fila (NULL se for o último)
};

struct Fila {
    NoFila* inicio;     // primeiro da fila (o próximo a ser atendido)
    NoFila* fim;        // último da fila
    int tamanho;        // quantidade de voos aguardando
};


// ----------- LISTA DUPLAMENTE ENCADEADA --------------
struct NoHistorico {
    Voo* voo;
    NoHistorico* ant;   // nó anterior (NULL se for o primeiro)
    NoHistorico* prox;  // próximo nó (NULL se for o último)
};

struct ListaHistorico {
    NoHistorico* inicio;    // voo concluído há mais tempo
    NoHistorico* fim;       // voo concluído mais recentemente
    int tamanho;
};

// ------------- PILHA (Registro de Ocorrências) --------------
struct Registro {
    int tipo;                   // 1 = Cancelamento, 2 = Alteração de rota, 3 = Incidente
    char codigo[10];            // código do voo envolvido
    char descricao[50];         // cancelamento: motivo | rota: novo destino | incidente: descrição
    char destinoAntigo[50];     // guarda o destino antes da alteração (para desfazer)
    int distanciaAntiga;        // guarda a distância antes da alteração (para desfazer)
};

struct NoPilha {
    Registro reg;
    NoPilha* prox;      // nó que está logo abaixo na pilha
};

struct Pilha {
    NoPilha* topo;      // último registro inserido
    int tamanho;
};

// -------------- ÁRVORE BINÁRIA DE BUSCA (ABB) --------------- 
// Organiza os voos pelo código.
struct NoABB {
    Voo* voo;
    NoABB* esq;
    NoABB* dir;
};


// ============= DADOS GLOBAIS ================
Aeronave aeronaves[MAX_AERONAVES];
int totalAeronaves = 0;

Voo voos[MAX_VOOS];     // TODOS os voos ficam aqui. As filas, o histórico e a
int totalVoos = 0;      // ABB guardam apenas PONTEIROS para este vetor.

Fila filaPouso = {NULL, NULL, 0};
Fila filaDecolagem = {NULL, NULL, 0};

ListaHistorico historico = {NULL, NULL, 0};
Pilha pilhaOcorrencias = {NULL, 0};
NoABB* raizABB = NULL;  // raiz da árvore (NULL = árvore vazia)


// ================= AERONAVES ================
void cadastrarAeronave() {
    if (totalAeronaves >= MAX_AERONAVES) {
        cout << "Limite de aeronaves atingido." << endl;
        return;
    }

    Aeronave a;
    a.id = totalAeronaves + 1;
    a.emUso = 0;                // toda aeronave nova começa livre
    cout << "Modelo (sem espacos, ex: CarboX306): ";
    cin >> a.modelo;
    cout << "Capacidade (passageiros): ";
    cin >> a.capacidade;
    cout << "Autonomia (km): ";
    cin >> a.autonomia;

    if (a.capacidade <= 0 || a.autonomia <= 0) {
        cout << "Capacidade e autonomia devem ser maiores que zero." << endl;
        return;
    }

    aeronaves[totalAeronaves] = a;
    totalAeronaves++;
    cout << "Aeronave cadastrada com ID " << a.id << endl;
}

void listarAeronaves() {
    if (totalAeronaves == 0) {
        cout << "Nenhuma aeronave cadastrada." << endl;
        return;
    }

    for (int i = 0; i < totalAeronaves; i++) {
        cout << "ID " << aeronaves[i].id
             << " | " << aeronaves[i].modelo
             << " | capacidade: " << aeronaves[i].capacidade
             << " | autonomia: " << aeronaves[i].autonomia << " km"
             << " | " << (aeronaves[i].emUso == 1 ? "Em uso" : "Livre") << endl;
    }
}


// ================ ÁRVORE BINÁRIA DE BUSCA (ABB) ===============

// Insere um voo na árvore e devolve a (possível nova) raiz.
// Funciona por recursão: compara o código do voo novo com o do nó atual
// e desce para a esquerda (menor) ou para a direita (maior ou igual)
// até achar um espaço vazio (NULL), onde cria o novo nó.
NoABB* inserirABB(NoABB* raiz, Voo* v) {
    if (raiz == NULL) {                 // achou o lugar: cria o nó
        NoABB* novo = new NoABB;
        novo->voo = v;
        novo->esq = NULL;
        novo->dir = NULL;
        return novo;
    }
    // strcmp devolve negativo se o 1º texto vem antes do 2º na ordem alfabética
    if (strcmp(v->codigo, raiz->voo->codigo) < 0) {
        raiz->esq = inserirABB(raiz->esq, v);
    } else {
        raiz->dir = inserirABB(raiz->dir, v);
    }
    return raiz;
}

// Procura um voo pelo código. Devolve o ponteiro do voo ou NULL se não achar.
Voo* buscarABB(NoABB* raiz, const char* codigo, int& comparacoes) {
    while (raiz != NULL) {
        comparacoes++;    //conta quantos nós foram visitados.
        int c = strcmp(codigo, raiz->voo->codigo);
        if (c == 0) return raiz->voo;               // achou
        if (c < 0) raiz = raiz->esq;                // código é menor: vai para a esquerda
        else raiz = raiz->dir;                      // código é maior: vai para a direita
    }
    return NULL;                                    // chegou ao fim sem achar
}

// Percurso EM-ORDEM: visita esquerda, depois o nó, depois a direita.
// O resultado sai em ordem alfabética de código.
void emOrdemABB(NoABB* raiz);   // (declarada aqui, definida mais abaixo)


// ============== VOOS ==============
void imprimirVoo(Voo* v) {
    cout << v->codigo << " | "
         << v->origem << " -> " << v->destino << " | "
         << v->hora << ":" << (v->minuto < 10 ? "0" : "") << v->minuto
         << " | Pista: " << v->pista
         << " | " << (v->tipo == 1 ? "Pouso" : "Decolagem")
         << " | prioridade " << v->prioridade
         << " | " << v->distancia << " km"
         << " | " << v->status
         << " | aeronave " << v->idAeronave << endl;
}

// Busca LINEAR (percorre o vetor inteiro) um voo pelo código.
// Devolve um ponteiro para o voo dentro de voos[] ou NULL se não achar.
// Usada pelas funções do menu; a busca pela ABB é a opção 14.
Voo* buscarVoo(const char* codigo) {
    for (int i = 0; i < totalVoos; i++) {
        if (strcmp(voos[i].codigo, codigo) == 0) {  // strcmp devolve 0 se forem iguais
            return &voos[i];                        // & = endereço do voo
        }
    }
    return NULL;
}

// Verifica se já existe um voo (não cancelado) na mesma pista, no mesmo horário.
// Devolve true se há conflito. É aqui que o "recurso limitado" aparece:
// uma pista não pode atender dois voos ao mesmo tempo.
bool verificarConflito(int hora, int minuto, int pista) {
    for (int i = 0; i < totalVoos; i++) {
        if (voos[i].pista == pista &&
            voos[i].hora == hora &&
            voos[i].minuto == minuto &&
            strcmp(voos[i].status, "Cancelado") != 0) {
            return true;
        }
    }
    return false;
}

// Cadastra um novo voo, validando cada dado digitado.
// Se qualquer validação falhar, o voo NÃO é cadastrado.
void cadastrarVoo() {
    if (totalAeronaves == 0) {
        cout << "Cadastre uma aeronave primeiro." << endl;
        return;
    }
    if (totalVoos >= MAX_VOOS) {
        cout << "Limite de voos atingido." << endl;
        return;
    }

    Voo v;
    cout << "Codigo do Voo (ex: AE101): ";
    cin >> v.codigo;

    if (buscarVoo(v.codigo) != NULL) {
        cout << "Ja existe um voo com esse codigo." << endl;
        return;
    }

    cout << "Origem (sem espacos): ";
    cin >> v.origem;
    cout << "Destino (sem espacos): ";
    cin >> v.destino;

    cout << "Hora (0-23): ";
    cin >> v.hora;
    if (v.hora < 0 || v.hora > 23) {
        cout << "Hora invalida." << endl;
        return;
    }

    cout << "Minuto (0-59): ";
    cin >> v.minuto;
    if (v.minuto < 0 || v.minuto > 59) {
        cout << "Minuto invalido." << endl;
        return;
    }

    cout << "Numero da Pista (1 a " << NUM_PISTAS << "): ";
    cin >> v.pista;
    if (v.pista < 1 || v.pista > NUM_PISTAS) {
        cout << "Pista invalida." << endl;
        return;
    }

    // Recurso limitado: a pista precisa estar livre naquele horário
    if (verificarConflito(v.hora, v.minuto, v.pista)) {
        cout << "Conflito: Ja existe um voo agendado para esta pista neste horario!" << endl;
        return;
    }

    cout << "Distancia (km): ";
    cin >> v.distancia;
    if (v.distancia <= 0) {
        cout << "Distancia invalida." << endl;
        return;
    }

    cout << "Tipo (1 = Pouso, 2 = Decolagem): ";
    cin >> v.tipo;
    if (v.tipo != 1 && v.tipo != 2) {
        cout << "Tipo invalido." << endl;
        return;
    }

    cout << "Prioridade (1 = Emergencia, 2 = Alta, 3 = Normal): ";
    cin >> v.prioridade;
    if (v.prioridade < 1 || v.prioridade > 3) {
        cout << "Prioridade invalida." << endl;
        return;
    }

    cout << "ID da aeronave: ";
    cin >> v.idAeronave;
    if (v.idAeronave < 1 || v.idAeronave > totalAeronaves) {
        cout << "Aeronave nao encontrada." << endl;
        return;
    }

    // A aeronave precisa aguentar a distância (id - 1 = posição no vetor)
    if (aeronaves[v.idAeronave - 1].autonomia < v.distancia) {
        cout << "Autonomia da aeronave insuficiente para esta distancia." << endl;
        return;
    }

    strcpy(v.status, "Agendado");                   // strcpy copia um texto para outro
    voos[totalVoos] = v;                            // guarda no vetor
    raizABB = inserirABB(raizABB, &voos[totalVoos]);// e também na ABB
    totalVoos++;
    cout << "Voo " << v.codigo << " cadastrado com sucesso." << endl;
}

void listarVoos() {
    if (totalVoos == 0) {
        cout << "Nenhum voo cadastrado." << endl;
        return;
    }
    for (int i = 0; i < totalVoos; i++) {
        imprimirVoo(&voos[i]);
    }
}


// =============== FILAS (pouso e decolagem) =================

// Insere um voo na fila respeitando a PRIORIDADE.
// Regras:
//   - prioridade menor (mais urgente) fica mais perto do início;
//   - em empate, quem chegou primeiro fica na frente (ordem de chegada).
// Por isso é uma "fila de prioridade" feita com lista encadeada.
void enfileirar(Fila* f, Voo* v) {
    NoFila* novo = new NoFila;      // cria o nó novo
    novo->voo = v;
    novo->prox = NULL;

    if (f->inicio == NULL) {    // Caso 1: fila vazia -> o novo é o primeiro e o último
        f->inicio = novo;
        f->fim = novo;
    }
    else if (v->prioridade < f->inicio->voo->prioridade) {
        // Caso 2: é mais urgente que o primeiro -> entra na frente de todos
        novo->prox = f->inicio;
        f->inicio = novo;
    }
    else {  // Caso 3: anda até achar a posição certa.
            // Para depois do último nó cuja prioridade seja menor ou igual à do novo.
        NoFila* atual = f->inicio;
        while (atual->prox != NULL &&
               atual->prox->voo->prioridade <= v->prioridade) {
            atual = atual->prox;
        }
        novo->prox = atual->prox;   // encaixa o novo no meio (ou no fim)
        atual->prox = novo;

        if (novo->prox == NULL) {   // se entrou no fim, atualiza o ponteiro "fim"
            f->fim = novo;
        }
    }

    f->tamanho++;
}

// Retira e devolve o voo que está no início da fila (o próximo a ser atendido).
// Atenção!!! 0_0: só chamar se a fila NÃO estiver vazia.
Voo* desenfileirar(Fila* f) {
    NoFila* no = f->inicio;
    Voo* v = no->voo;
    f->inicio = no->prox;           // o segundo passa a ser o primeiro
    if (f->inicio == NULL) {        // se a fila esvaziou, "fim" também fica vazio
        f->fim = NULL;
    }
    delete no;                      // libera a memória do nó
    f->tamanho--;
    return v;
}

// Remove um voo específico de QUALQUER posição da fila (usado no cancelamento).
void removerDaFila(Fila* f, Voo* v) {
    NoFila* atual = f->inicio;
    NoFila* anterior = NULL;

    // procura o nó, guardando também o anterior
    while (atual != NULL && atual->voo != v) {
        anterior = atual;
        atual = atual->prox;
    }
    if (atual == NULL) return;      // não estava na fila 

    if (anterior == NULL) {
        f->inicio = atual->prox;    // era o primeiro
    } else {
        anterior->prox = atual->prox;   // o anterior "pula" o nó removido
    }
    if (atual == f->fim) {          // era o último
        f->fim = anterior;
    }

    delete atual;
    f->tamanho--;
}

// Coloca um voo "Agendado" na fila de pouso ou de decolagem.
void enfileirarVoo() {
    char codigo[10];
    cout << "Codigo do voo: ";
    cin >> codigo;

    Voo* v = buscarVoo(codigo);
    if (v == NULL) {
        cout << "Voo nao encontrado." << endl;
        return;
    }

    if (strcmp(v->status, "Agendado") != 0) {
        cout << "Este voo nao esta Agendado (status: " << v->status << ")." << endl;
        return;
    }

    // A mesma aeronave não pode estar em duas filas ao mesmo tempo
    Aeronave* a = &aeronaves[v->idAeronave - 1];
    if (a->emUso == 1) {
        cout << "A aeronave deste voo ja esta em uso (em outra fila)." << endl;
        return;
    }

    a->emUso = 1;                   // aeronave ocupada
    strcpy(v->status, "Na fila");
    if (v->tipo == 1) {
        enfileirar(&filaPouso, v);
        cout << "Voo " << v->codigo << " entrou na fila de POUSO." << endl;
    } else {
        enfileirar(&filaDecolagem, v);
        cout << "Voo " << v->codigo << " entrou na fila de DECOLAGEM." << endl;
    }
}

// Mostra o conteúdo de UMA fila, da frente para trás.
void exibirFila(const char* titulo, Fila* f) {
    cout << titulo << " (" << f->tamanho << " aguardando)" << endl;
    if (f->inicio == NULL) {
        cout << "   (vazia)" << endl;
        return;
    }
    int pos = 1;
    for (NoFila* atual = f->inicio; atual != NULL; atual = atual->prox) {
        cout << "   " << pos << ". " << atual->voo->codigo
             << " | prioridade " << atual->voo->prioridade
             << " | Pista: " << atual->voo->pista
             << " | " << atual->voo->origem << " -> " << atual->voo->destino << endl;
        pos++;
    }
}

// Mostra a situação atual das DUAS filas.
void exibirFilas() {
    cout << "\n=== SITUACAO DAS FILAS ===" << endl;
    exibirFila("Fila de POUSO", &filaPouso);
    exibirFila("Fila de DECOLAGEM", &filaDecolagem);
}


// ============= HISTÓRICO (lista duplamente encadeada) ==============

// Insere um voo concluído no fim da lista (o mais recente fica por último).
void inserirHistorico(Voo* v) {
    NoHistorico* novo = new NoHistorico;
    novo->voo = v;
    novo->prox = NULL;              // será o último, então não tem próximo
    novo->ant = historico.fim;      // o anterior dele é o antigo último

    if (historico.fim == NULL) {
        historico.inicio = novo;    // lista estava vazia
    } else {
        historico.fim->prox = novo; // o antigo último passa a apontar para o novo
    }
    historico.fim = novo;
    historico.tamanho++;
}

// Mostra o histórico em um dos dois sentidos, à escolha do usuário.
void exibirHistorico() {
    if (historico.tamanho == 0) {
        cout << "Nenhum voo concluido ainda." << endl;
        return;
    }
    int sentido;
    cout << "1 = do mais antigo ao mais recente | 2 = do mais recente ao mais antigo: ";
    cin >> sentido;

    cout << "\n=== HISTORICO DE VOOS CONCLUIDOS (" << historico.tamanho << ") ===" << endl;
    if (sentido == 2) {
        // de trás para frente, usando o ponteiro "ant"
        for (NoHistorico* n = historico.fim; n != NULL; n = n->ant) {
            imprimirVoo(n->voo);
        }
    } else {
        // de frente para trás, usando o ponteiro "prox"
        for (NoHistorico* n = historico.inicio; n != NULL; n = n->prox) {
            imprimirVoo(n->voo);
        }
    }
}


// =========== PROCESSAMENTO DOS VOOS (por prioridade) =============

// Finaliza um voo: muda o status, libera a aeronave e registra no histórico.
void concluirVoo(Voo* v) {
    strcpy(v->status, "Concluido");
    aeronaves[v->idAeronave - 1].emUso = 0;     // aeronave livre de novo
    inserirHistorico(v);
}

// Atende o PRÓXIMO voo do aeroporto (só um por vez: recurso limitado).
// Compara o primeiro da fila de pouso com o primeiro da fila de decolagem
// e atende o mais urgente (menor número de prioridade).
// Em caso de empate o POUSO é atendido primeiro, porque um avião no ar
// tem combustível limitado, enquanto um avião no chão pode esperar.
void processarProximoVoo() {
    if (filaPouso.inicio == NULL && filaDecolagem.inicio == NULL) {
        cout << "Nao ha voos nas filas para processar." << endl;
        return;
    }

    Fila* origem;       // fila de onde sairá o voo
    if (filaDecolagem.inicio == NULL) {
        origem = &filaPouso;                // só há pousos esperando
    } else if (filaPouso.inicio == NULL) {
        origem = &filaDecolagem;            // só há decolagens esperando
    } else if (filaPouso.inicio->voo->prioridade <= filaDecolagem.inicio->voo->prioridade) {
        origem = &filaPouso;                // pouso é mais urgente (ou empate)
    } else {
        origem = &filaDecolagem;            // decolagem é mais urgente
    }

    Voo* v = desenfileirar(origem);
    concluirVoo(v);

    cout << "Pista " << v->pista << ": voo " << v->codigo
         << (v->tipo == 1 ? " POUSOU" : " DECOLOU")
         << " (prioridade " << v->prioridade << ")" << endl;
}


// ============= PILHA DE OCORRÊNCIAS =================
//  |-> (cancelamentos, alterações de rota, incidentes)

// Coloca um novo registro NO TOPO da pilha.
void empilhar(int tipo, const char* codigo, const char* descricao,
              const char* destinoAntigo, int distanciaAntiga) {
    NoPilha* novo = new NoPilha;
    novo->reg.tipo = tipo;
    strcpy(novo->reg.codigo, codigo);
    strcpy(novo->reg.descricao, descricao);
    strcpy(novo->reg.destinoAntigo, destinoAntigo);
    novo->reg.distanciaAntiga = distanciaAntiga;

    novo->prox = pilhaOcorrencias.topo;     // o novo aponta para o antigo topo
    pilhaOcorrencias.topo = novo;           // e passa a ser o topo
    pilhaOcorrencias.tamanho++;
}

// Retira o registro do TOPO da pilha (só chamar se não estiver vazia).
void desempilhar() {
    NoPilha* topo = pilhaOcorrencias.topo;
    pilhaOcorrencias.topo = topo->prox;     // o de baixo vira o topo
    pilhaOcorrencias.tamanho--;
    delete topo;
}

// Cancela um voo (que esteja "Agendado" ou "Na fila") e registra na pilha.
void cancelarVoo() {
    char codigo[10];
    cout << "Codigo do voo a cancelar: ";
    cin >> codigo;

    Voo* v = buscarVoo(codigo);
    if (v == NULL) {
        cout << "Voo nao encontrado." << endl;
        return;
    }

    bool naFila = (strcmp(v->status, "Na fila") == 0);
    if (!naFila && strcmp(v->status, "Agendado") != 0) {
        cout << "Nao e possivel cancelar um voo com status " << v->status << "." << endl;
        return;
    }

    char motivo[50];
    cout << "Motivo do cancelamento (sem espacos): ";
    cin >> motivo;

    if (naFila) {
        // sai da fila correta e libera a aeronave
        if (v->tipo == 1) removerDaFila(&filaPouso, v);
        else removerDaFila(&filaDecolagem, v);
        aeronaves[v->idAeronave - 1].emUso = 0;
    }

    strcpy(v->status, "Cancelado");
    empilhar(1, v->codigo, motivo, v->destino, v->distancia);
    cout << "Voo " << v->codigo << " cancelado e registrado na pilha." << endl;
}

// Muda o destino e a distância de um voo (que esteja "Agendado" ou "Na fila").
// Os dados ANTIGOS ficam guardados na pilha para poderem ser desfeitos.
void alterarRota() {
    char codigo[10];
    cout << "Codigo do voo: ";
    cin >> codigo;

    Voo* v = buscarVoo(codigo);
    if (v == NULL) {
        cout << "Voo nao encontrado." << endl;
        return;
    }
    if (strcmp(v->status, "Agendado") != 0 && strcmp(v->status, "Na fila") != 0) {
        cout << "Nao e possivel alterar a rota de um voo com status " << v->status << "." << endl;
        return;
    }

    char novoDestino[50];
    int novaDistancia;
    cout << "Novo destino (sem espacos): ";
    cin >> novoDestino;
    cout << "Nova distancia (km): ";
    cin >> novaDistancia;

    if (novaDistancia <= 0) {
        cout << "Distancia invalida." << endl;
        return;
    }
    // A aeronave precisa aguentar a nova distância
    if (aeronaves[v->idAeronave - 1].autonomia < novaDistancia) {
        cout << "Autonomia da aeronave insuficiente para a nova rota." << endl;
        return;
    }

    // Primeiro guarda o estado antigo na pilha, depois altera o voo
    empilhar(2, v->codigo, novoDestino, v->destino, v->distancia);
    strcpy(v->destino, novoDestino);
    v->distancia = novaDistancia;
    cout << "Rota alterada e registrada na pilha." << endl;
}

// Registra um incidente (ex: falha, turbulência) envolvendo um voo.
void registrarIncidente() {
    char codigo[10];
    cout << "Codigo do voo envolvido: ";
    cin >> codigo;

    Voo* v = buscarVoo(codigo);
    if (v == NULL) {
        cout << "Voo nao encontrado." << endl;
        return;
    }

    char descricao[50];
    cout << "Descricao do incidente (sem espacos): ";
    cin >> descricao;

    empilhar(3, v->codigo, descricao, v->destino, v->distancia);
    cout << "Incidente registrado na pilha." << endl;
}

// Mostra a pilha, do registro mais recente (topo) para o mais antigo.
void exibirOcorrencias() {
    if (pilhaOcorrencias.topo == NULL) {
        cout << "Nenhuma ocorrencia registrada." << endl;
        return;
    }
    cout << "\n=== OCORRENCIAS (da mais recente para a mais antiga) ===" << endl;
    int pos = 1;
    for (NoPilha* n = pilhaOcorrencias.topo; n != NULL; n = n->prox) {
        cout << pos << ". [";
        if (n->reg.tipo == 1) {
            cout << "CANCELAMENTO] voo " << n->reg.codigo << " - motivo: " << n->reg.descricao;
        } else if (n->reg.tipo == 2) {
            cout << "ALTERACAO DE ROTA] voo " << n->reg.codigo << " - destino "
                 << n->reg.destinoAntigo << " -> " << n->reg.descricao;
        } else {
            cout << "INCIDENTE] voo " << n->reg.codigo << " - " << n->reg.descricao;
        }
        if (pos == 1) cout << "   <-- TOPO";
        cout << endl;
        pos++;
    }
}

// Desfaz APENAS a ocorrência do topo (regra da pilha: último a entrar, primeiro a sair).
//   - Alteração de rota: devolve o destino e a distância antigos e remove o registro.
//   - Incidente: considera o incidente encerrado e remove o registro.
//   - Cancelamento: NÃO pode ser desfeito (o registro permanece).
void desfazerUltimaOcorrencia() {
    if (pilhaOcorrencias.topo == NULL) {
        cout << "Nenhuma ocorrencia para desfazer." << endl;
        return;
    }

    Registro* r = &pilhaOcorrencias.topo->reg;

    if (r->tipo == 1) {
        cout << "O topo e um cancelamento (voo " << r->codigo
             << "), que nao pode ser desfeito." << endl;
        return;
    }

    if (r->tipo == 2) {
        Voo* v = buscarVoo(r->codigo);
        // só restaura se o voo ainda não foi concluído nem cancelado
        if (v != NULL && (strcmp(v->status, "Agendado") == 0 || strcmp(v->status, "Na fila") == 0)) {
            strcpy(v->destino, r->destinoAntigo);
            v->distancia = r->distanciaAntiga;
            cout << "Rota do voo " << v->codigo << " restaurada para " << v->destino << "." << endl;
        } else {
            cout << "Voo ja nao pode ser alterado; registro apenas descartado." << endl;
        }
    } else {
        cout << "Incidente do voo " << r->codigo << " encerrado." << endl;
    }

    desempilhar();
}


// =============== BUSCA PELA ABB ===================

// Busca um voo pelo código usando a árvore e mostra quantas comparações foram feitas.
void buscarVooABB() {
    if (raizABB == NULL) {
        cout << "Nenhum voo cadastrado." << endl;
        return;
    }
    char codigo[10];
    cout << "Codigo do voo: ";
    cin >> codigo;

    int comparacoes = 0;
    Voo* v = buscarABB(raizABB, codigo, comparacoes);
    if (v == NULL) {
        cout << "Voo nao encontrado (" << comparacoes << " comparacoes na ABB)." << endl;
    } else {
        imprimirVoo(v);
        cout << "(encontrado com " << comparacoes << " comparacoes na ABB)" << endl;
    }
}

// Percurso em-ordem (definição da função declarada lá em cima).
void emOrdemABB(NoABB* raiz) {
    if (raiz == NULL) return;       // chegou ao fim de um galho
    emOrdemABB(raiz->esq);          // 1º: tudo que é menor
    imprimirVoo(raiz->voo);         // 2º: o próprio nó
    emOrdemABB(raiz->dir);          // 3º: tudo que é maior
}

// Lista todos os voos em ordem de código, usando o percurso em-ordem da ABB.
void listarVoosABB() {
    if (raizABB == NULL) {
        cout << "Nenhum voo cadastrado." << endl;
        return;
    }
    cout << "\n=== VOOS EM ORDEM DE CODIGO (percurso em-ordem da ABB) ===" << endl;
    emOrdemABB(raizABB);
}


// ============== ORDENAÇÃO DA PROGRAMAÇÃO (Bubble Sort) ================

// Devolve o número que representa o voo no critério escolhido.
// O voo com MENOR número aparece primeiro na programação.
//   1 = horário    -> converte para minutos desde 00:00 (ex: 10:30 vira 630)
//   2 = prioridade -> prioridade primeiro; em empate, o horário decide
//   3 = duração    -> usamos a distância como aproximação da duração
int chaveOrdenacao(Voo* v, int criterio) {
    if (criterio == 1) return v->hora * 60 + v->minuto;
    if (criterio == 2) return v->prioridade * 10000 + v->hora * 60 + v->minuto;
    return v->distancia;
}

// Mostra a programação do aeroporto ordenada pelo critério escolhido.
// Ordena uma CÓPIA do vetor: assim voos[] não muda de lugar e as filas,
// o histórico e a ABB (que apontam para voos[]) continuam corretos.
//
// Bubble Sort: compara vizinhos e troca se estiverem fora de ordem. A cada
// volta do laço de fora, o maior valor "sobe" para o final.
void ordenarProgramacao() {
    if (totalVoos == 0) {
        cout << "Nenhum voo cadastrado." << endl;
        return;
    }

    int criterio;
    cout << "Ordenar por: 1 = Horario | 2 = Prioridade | 3 = Duracao (distancia)" << endl;
    cout << "Criterio: ";
    cin >> criterio;
    if (criterio < 1 || criterio > 3) {
        cout << "Criterio invalido." << endl;
        return;
    }

    Voo copia[MAX_VOOS];
    for (int i = 0; i < totalVoos; i++) {
        copia[i] = voos[i];
    }

    for (int i = 0; i < totalVoos - 1; i++) {
        for (int j = 0; j < totalVoos - 1 - i; j++) {
            if (chaveOrdenacao(&copia[j], criterio) > chaveOrdenacao(&copia[j + 1], criterio)) {
                Voo temp = copia[j];            // troca os dois de lugar
                copia[j] = copia[j + 1];
                copia[j + 1] = temp;
            }
        }
    }

    cout << "\n=== PROGRAMACAO DO AEROPORTO ===" << endl;
    for (int i = 0; i < totalVoos; i++) {
        imprimirVoo(&copia[i]);
    }
}


// ============= LIBERAÇÃO DE MEMÓRIA ==============
// Tudo que foi criado com "new" precisa ser apagado com "delete" antes do
// programa terminar. Estas funções percorrem cada estrutura apagando os nós.
// (Os vetores aeronaves[] e voos[] são estáticos, então não precisam de delete.)

// Apaga todos os nós de uma fila, sempre removendo o primeiro.
void liberarFila(Fila* f) {
    while (f->inicio != NULL) {
        NoFila* temp = f->inicio;
        f->inicio = temp->prox;
        delete temp;
    }
    f->fim = NULL;
    f->tamanho = 0;
}

// Apaga todos os nós do histórico (lista duplamente encadeada).
void liberarHistorico() {
    while (historico.inicio != NULL) {
        NoHistorico* temp = historico.inicio;
        historico.inicio = temp->prox;
        delete temp;
    }
    historico.fim = NULL;
    historico.tamanho = 0;
}

// Apaga todos os registros da pilha (reaproveita a função desempilhar).
void liberarPilha() {
    while (pilhaOcorrencias.topo != NULL) {
        desempilhar();
    }
}

// Apaga a árvore por recursão: primeiro os filhos, depois o próprio nó.
// (Se apagasse o nó antes, perderia o acesso aos filhos.)
void liberarABB(NoABB* raiz) {
    if (raiz == NULL) return;
    liberarABB(raiz->esq);
    liberarABB(raiz->dir);
    delete raiz;
}

// Chama a liberação de todas as estruturas. Usada ao sair do programa.
void liberarMemoria() {
    liberarFila(&filaPouso);
    liberarFila(&filaDecolagem);
    liberarHistorico();
    liberarPilha();
    liberarABB(raizABB);
    raizABB = NULL;
}

// ================= MENU PRINCIPAL ==================
int main() {
    int escolha;
    do {
        cout << "\n=========================== SIMULADOR DE AEROPORTO =================================" << endl;
        cout << "====================================================================================" << endl;
        cout << "------------- Cadastros ---------------|------ Cancelamentos e incidentes ----------" << endl;
        cout << " 1 - Cadastrar aeronave                | 9 - Cancelar voo" << endl;
        cout << " 2 - Listar aeronaves                  | 10 - Alterar rota de voo" << endl;
        cout << " 3 - Cadastrar voo                     | 11 - Registrar incidente" << endl;
        cout << " 4 - Listar voos                       | 12 - Exibir ocorrencias" << endl;
        cout << "                                       | 13 - Desfazer ultima ocorrencia" << endl;
        cout << "------- Filas e pistas ----------------|--------------- Consultas ------------------" << endl;
        cout << " 5 - Enfileirar voo                    | 14 - Buscar voo por codigo (ABB)" << endl;
        cout << " 6 - Exibir filas                      | 15 - Listar voos por codigo (ABB em-ordem)" << endl;
        cout << " 7 - Processar proximo voo             | 16 - Ordenar programacao do aeroporto" << endl;
        cout << " 8 - Historico de voos concluidos      | 0 - Sair" << endl;
        cout << "------------------------------------------------------------------------------------\n\n >> ";
        cin >> escolha;

        // Se digitarem uma letra no menu, limpa o erro do cin para não travar
        if (cin.fail()) {
            cin.clear();
            cin.ignore(1000, '\n');
            escolha = -1;
        }

        switch (escolha) {
            case 1: cadastrarAeronave(); break;
            case 2: listarAeronaves(); break;
            case 3: cadastrarVoo(); break;
            case 4: listarVoos(); break;
            case 5: enfileirarVoo(); break;
            case 6: exibirFilas(); break;
            case 7: processarProximoVoo(); break;
            case 8: exibirHistorico(); break;
            case 9: cancelarVoo(); break;
            case 10: alterarRota(); break;
            case 11: registrarIncidente(); break;
            case 12: exibirOcorrencias(); break;
            case 13: desfazerUltimaOcorrencia(); break;
            case 14: buscarVooABB(); break;
            case 15: listarVoosABB(); break;
            case 16: ordenarProgramacao(); break;
            case 0: cout << "Encerrando..." << endl; break;
            default: cout << "Opcao Invalida!" << endl; break;
        }
    } while (escolha != 0);

    liberarMemoria();   // apaga todos os nós criados com "new"
    return 0;
}