#include <iostream>
#include <string>
#include <cstring>

using namespace std;

// ------ STRUCTS -------
struct Aeronave { 
    int id;
    char modelo[20];
    int capacidade;
    int autonomia;      // km
    int emUso;          // variavel para impedir q a mesma aeronave esteja em duas filas.
};

struct Voo {
    char codigo[10];
    char origem[50];
    char destino[50];
    int hora;           // 0 a 23
    int minuto;         // 0 a 59
    char status[20];
    int idAeronave;
    int distancia;      // checa se a aeronave tem autonomia o suficiente.
    int tipo;           // decide para qual fila o voo vai.
    int prioridade;     // definir quem pousa ou decola primeiro.
};

struct NoFila {
    Voo* voo;       // aponta para o voo do vetor voo[]
    NoFila* prox;
};

struct Fila {       // definir a fila em si.
    NoFila* inicio;
    NoFila* fim;
    int tamanho;
};

// -------- DADOS ---------
Aeronave aeronaves[50];
int totalAeronaves = 0;

Voo voos[200];
int totalVoos = 0;

Fila filaPouso = {NULL, NULL, 0};
Fila filaDecolagem = {NULL, NULL, 0};
// Os nós n copiam o voo, só guardam o endereco, pois vetor voos é
// | global e enderecos n mudam.

// ------- AERONAVES -------
void cadastrarAeronave() {
    Aeronave a;
    a.id = totalAeronaves + 1;
    a.emUso = 0;
    cout << "Modelo (sem espacos, ex: CarboX306): ";
    cin >> a.modelo;
    cout << "Capacidade (passageiros): ";
    cin >> a.capacidade;
    cout << "Autonomia (km): ";
    cin >> a.autonomia;

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

// -------- VOOS ---------
Voo* buscarVoo(const char* codigo) {        // função q devolve um ponteiro Voo* para o voo
    for (int i = 0; i < totalVoos; i++) {   // | dentro do vetor, ou NULL se não achar.
        if (strcmp(voos[i].codigo, codigo) == 0) {  // compara os textos e devolve 0 quando são iguais.
            return &voos[i];
        }
    }

    return NULL;
}

void cadastrarVoo() {
    if (totalAeronaves == 0) {
        cout << "Cadastre uma aeronave primeiro." << endl;
        return;
    }

    Voo v;
    cout << "Codigo do Voo (ex: AE101): ";
    cin >> v.codigo;

    if (buscarVoo(v.codigo) != NULL) {
        cout << "Já existe um voo com esse código." << endl;
        return;
    }

    cout << "Origem (sem espacos): ";
    cin >> v.origem;
    cout << "Destino (sem espacos): ";
    cin >> v.destino;
    cout << "Hora (0-23): ";
    cin >> v.hora;
    cout << "Minuto (0-59): ";
    cin >> v.minuto;
    cout << "Distancia (km): ";
    cin >> v.distancia;
    cout << "Tipo (1 = Pouso, 2 = Decolagem): ";
    cin >> v.tipo;
    cout << "Prioridade (1 = Emergencia, 2 = Alta, 3 = Normal): ";
    cin >> v.prioridade;
    cout << "ID da aeronave: ";
    cin >> v.idAeronave;

    if (v.idAeronave < 1 || v.idAeronave > totalAeronaves) {
        cout << "Aeronave nao encontrada." << endl;
        return;
    }

    // Ficou idAeronave - 1 pq vetor começa com indice 0, logo ID 1 está em 0 no vetor.
    if (aeronaves[v.idAeronave - 1].autonomia < v.distancia) {
        cout << "Autonomia da aeronave insuficiente para esta distancia." << endl;
        return;
    }

    strcpy(v.status, "Agendado");
    voos[totalVoos] = v;
    totalVoos++;
    cout << "Voo " << v.codigo << " cadastrado." << endl;
}

void listarVoos() {
    if (totalVoos == 0) {
        cout << "Nenhum voo cadastrado." << endl;
        return;
    }

    for (int i = 0; i < totalVoos; i++) {
        cout << voos[i].codigo << " | "
             << voos[i].origem << " -> " << voos[i].destino << " | "
             << voos[i].hora << ":" << (voos[i].minuto < 10 ? "0" : "") << voos[i].minuto
             << " | " << (voos[i].tipo == 1 ? "Pouso" : "Decolagem")
             << " | prioridade " << voos[i].prioridade
             << " | " << voos[i].status
             << " | aeronave " << voos[i].idAeronave << endl;
    }
}
// ------- ENFILEIRAMENTO ----------

void enfileirar (Fila* f, Voo* v) {
    NoFila* novo = new NoFila;
    novo->voo = v;
    novo->prox = NULL;

    if (f->fim == NULL) {   // fila vazia
        f->inicio = novo;
    } else {                    // o antigo ultimo aponta para o novo
        f->fim->prox = novo;
    }

    f->fim = novo;
    f->tamanho++;
}

void enfileirarVoo () {
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

    Aeronave* a = &aeronaves[v->idAeronave - 1];
    if (a->emUso == 1) {
        cout << "A aeronave deste voo ja esta em uso (em outra fila)." << endl;
        return;
    }

    a->emUso = 1;
    strcpy(v->status, "Na fila");
    if (v->tipo == 1) {
        enfileirar(&filaPouso, v);
        cout << "Voo " << v->codigo << " entrou na fila de POUSO." << endl;
    } else {
        enfileirar(&filaDecolagem, v);
        cout << "Voo " << v->codigo << " entrou na fila de DECOLAGEM." << endl;
    }
}

void exibirFila(const char* titulo, Fila* f) {          // Função para agrupar uma fila
    cout << titulo << " (" << f->tamanho << " aguardando)" << endl;
    if (f->inicio == NULL) {
        cout << "   (vazia)" << endl;
        return;
    }
    int pos = 1;
    for (NoFila* atual = f->inicio; atual != NULL; atual = atual->prox) {
        cout << "   " << pos << ". " << atual->voo->codigo
             << " | prioridade " << atual->voo->prioridade
             << " | " << atual->voo->origem << " -> " << atual->voo->destino << endl;
        pos++;
    }
}

void exibirFilas() {            // função para apresentar as filas agrupadas por exibirFila();
    cout << "\n=== SITUACAO DAS FILAS ===" << endl;
    exibirFila("Fila de POUSO", &filaPouso);
    exibirFila("Fila de DECOLAGEM", &filaDecolagem);
}


// -------- MENU ----------
int main() {
    int escolha;
    do {
        cout << "\n===== AEROPORTO =====" << endl;
        cout << "1 - Cadastrar aeronave" << endl;
        cout << "2 - Listar aeronaves" << endl;
        cout << "3 - Cadastrar voo" << endl;
        cout << "4 - Listar voos" << endl;
        cout << "5 - Enfileirar voo" << endl;
        cout << "6 - Exibir filas" << endl;
        cout << "0 - Sair" << endl;
        cout << "Opcao: ";
        cin >> escolha;
 
        switch(escolha) {
            case 1: cadastrarAeronave(); break;
            case 2: listarAeronaves(); break;
            case 3: cadastrarVoo(); break;
            case 4: listarVoos(); break;
            case 5: enfileirarVoo(); break;
            case 6: exibirFilas(); break;
            case 0: cout << "Encerrando..." << endl; break;
            default: cout << "Opcao Invalida!" << endl; break;
        }
    } while (escolha != 0);
 
    return 0;
}


