#include <xinu.h>

typedef enum { INATIVO, RODANDO, PAUSADO, RESFRIANDO } Estado;

typedef struct {
    char *nome;
    int duracao;   
    int potencia;  
    int curvaTemp; 
} CicloAquecimento;

struct Microondas {
    Estado estado;
    int tempoRestante;
    bool portaAberta;
    bool emergenciaAtivada;
    bool agendamentoAtivo;
    int tempoAgendado; 
    CicloAquecimento cicloAtual;
    sid32 mutex; 
};

CicloAquecimento ciclos[] = {
    {"Carnes", 600, 100, 1},
    {"Peixe", 450, 80, 2},
    {"Frango", 500, 90, 1},
    {"Lasanha", 700, 100, 2},
    {"Pipoca", 300, 70, 1}
};
void controle_klystron(struct Microondas *microondas) {
    while (1) {
        wait(microondas->mutex);
        if (microondas->estado == RODANDO) {
            if (microondas->cicloAtual.curvaTemp == 1) {
                printf("Klystron: Potencia constante em %d%%\n", microondas->cicloAtual.potencia);
            } else if (microondas->cicloAtual.curvaTemp == 2) {
                printf("Klystron: Potencia ajustada (curva exponencial)\n");
            }
        } else if (microondas->estado == RESFRIANDO) {
            printf("Klystron: Ciclo de resfriamento ativo.\n");
        }
        signal(microondas->mutex);
        sleep(1); 
    }
}
void anunciador_bip(struct Microondas *microondas) {
    while (1) {
        wait(microondas->mutex);
        if (microondas->estado == INATIVO && microondas->tempoRestante == 0) {
            printf("Beep! Ciclo concluido!\n");
        } else if (microondas->emergenciaAtivada) {
            printf("Beep! Emergencia ativada!\n");
        }
        signal(microondas->mutex);
        sleep(1);
    }
}
void botao_emergencia(struct Microondas *microondas) {
    while (1) {
        wait(microondas->mutex);
        if (microondas->emergenciaAtivada) {
            microondas->estado = INATIVO;
            microondas->tempoRestante = 0;
            printf("Emergencia ativada! Ciclo cancelado.\n");
            microondas->emergenciaAtivada = false;
        }
        signal(microondas->mutex);
        sleepms(100);
    }
}
void resfriamento(struct Microondas *microondas) {
    while (1) {
        wait(microondas->mutex);
        if (microondas->estado == RESFRIANDO) {
            printf("Ventilação ativada para resfriamento.\n");
            sleep(5);
            microondas->estado = INATIVO;
            printf("Micro-ondas resfriado e pronto.\n");
        }
        signal(microondas->mutex);
        sleep(1);
    }
}
void programacao_futura(struct Microondas *microondas) {
    while (1) {
        wait(microondas->mutex);
        if (microondas->agendamentoAtivo && microondas->tempoAgendado > 0) {
            microondas->tempoAgendado--;
            if (microondas->tempoAgendado == 0) {
                microondas->estado = RODANDO;
                microondas->agendamentoAtivo = false;
                printf("Ciclo agendado iniciado: %s\n", microondas->cicloAtual.nome);
            }
        }
        signal(microondas->mutex);
        sleep(1);
    }
}
void main(void) {
    struct Microondas microondas;
    microondas.estado = INATIVO;
    microondas.tempoRestante = 0;
    microondas.portaAberta = false;
    microondas.emergenciaAtivada = false;
    microondas.agendamentoAtivo = false;
    microondas.mutex = semcreate(1);

    resume(create(controle_klystron, 1024, 20, "Klystron", 1, &microondas));
    resume(create(anunciador_bip, 1024, 20, "Bip", 1, &microondas));
    resume(create(botao_emergencia, 1024, 20, "Emergencia", 1, &microondas));
    resume(create(resfriamento, 1024, 20, "Resfriamento", 1, &microondas));
    resume(create(programacao_futura, 1024, 20, "Agendamento", 1, &microondas));

    while (1) {
        printf("\nMenu:\n");
        printf("1. Abrir porta\n");
        printf("2. Fechar porta\n");
        printf("3. Selecionar ciclo\n");
        printf("4. Iniciar\n");
        printf("5. Programar inicio futuro\n");
        printf("6. Acionar emergencia\n");
        printf("7. Sair\n");

        int opcao;
        scanf("%d", &opcao);

        wait(microondas.mutex);
        switch (opcao) {
            case 1:
                microondas.portaAberta = true;
                printf("Porta aberta.\n");
                break;
            case 2:
                microondas.portaAberta = false;
                printf("Porta fechada.\n");
                break;
            case 3: {
                printf("Selecione o ciclo (0-Carnes, 1-Peixe, ...): ");
                int escolha;
                scanf("%d", &escolha);
                if (escolha >= 0 && escolha < 5) {
                    microondas.cicloAtual = ciclos[escolha];
                    microondas.tempoRestante = ciclos[escolha].duracao;
                    printf("Ciclo '%s' selecionado.\n", ciclos[escolha].nome);
                } else {
                    printf("Ciclo invalido.\n");
                }
                break;
            }
            case 4:
                if (!microondas.portaAberta && microondas.tempoRestante > 0) {
                    microondas.estado = RODANDO;
                    printf("Ciclo iniciado.\n");
                } else {
                    printf("Não eh possivel iniciar o ciclo.\n");
                }
                break;
            case 5:
                printf("Informe o tempo de agendamento (em segundos): ");
                int tempo;
                scanf("%d", &tempo);
                microondas.tempoAgendado = tempo;
                microondas.agendamentoAtivo = true;
                printf("Ciclo agendado para comecar em %d segundos.\n", tempo);
                break;
            case 6:
                microondas.emergenciaAtivada = true;
                break;
            case 7:
                signal(microondas.mutex);
                printf("Encerrando micro-ondas...\n");
                return;
            default:
        }
    }
};
