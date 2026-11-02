#include <iostream>
#include <cstring>

using namespace std;

// ------ STRUCTS -------
struct Aeronave {
    int id;
    char modelo[20];
    int capacidade;
    int autonomia;      // km
};

struct Voo {
    char codigo[10];
    char origem[50];
    char destino[50];
    int hora;           // 0 a 23
    int minuto;         // 0 a 59
    char status[20];
    int idAeronave;
};

// -------- DADOS ---------
Aeronave aeronaves[50];
int totalAeronaves = 0;

Voo voos[200];
int totalVoos = 0;

// ------- AERONAVES -------
void cadastrarAeronave() {
    Aeronave a;
    a.id = totalAeronaves + 1;
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
             << " | autonomia: " << aeronaves[i].autonomia << " km" << endl;
    }
}

// -------- VOOS ---------
void cadastrarVoo() {
    if (totalAeronaves == 0) {
        cout << "Cadastre uma aeronave primeiro." << endl;
        return;
    }

    Voo v;
    cout << "Codigo do Voo (ex: AE101): ";
    cin >> v.codigo;
    cout << "Origem (sem espacos): ";
    cin >> v.origem;
    cout << "Destino (sem espacos): ";
    cin >> v.destino;
    cout << "Hora (0-23): ";
    cin >> v.hora;
    cout << "Minuto (0-59): ";
    cin >> v.minuto;
    cout << "ID da aeronave: ";
    cin >> v.idAeronave;

    if (v.idAeronave < 1 || v.idAeronave > totalAeronaves) {
        cout << "Aeronave nao encontrada." << endl;
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
             << " | " << voos[i].status
             << " | aeronave " << voos[i].idAeronave << endl;
    }
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
        cout << "0 - Sair" << endl;
        cout << "Opcao: ";
        cin >> escolha;
 
        switch(escolha) {
            case 1: cadastrarAeronave(); break;
            case 2: listarAeronaves(); break;
            case 3: cadastrarVoo(); break;
            case 4: listarVoos(); break;
            case 0: cout << "Encerrando..." << endl; break;
            default: cout << "Opção Invalida!" << endl; break;
        }
    } while (escolha != 0);
 
    return 0;
}


