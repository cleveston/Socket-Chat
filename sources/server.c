//-------------------------------------------------------------------------//
//                  Universidade Federal de Santa Maria                    //
//                   Curso de Engenharia de Computação                     //
//                  Sistemas Operacionais de Tempo Real                    //
//         								   //
//								 	   //
//   Autor: Iury Cleveston (201220748)                                     //
//   		                                                           //
//   Data: 29/05/2015                                                      //
//=========================================================================//
//                         Descrição do Programa                           //
//=========================================================================//
//   Aplicação Chat Cliente - Servidor                                     //
//                                                                         //
//-------------------------------------------------------------------------//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>	
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/sigthread.h>

#define TAMANHO_BUFFER 256

//Estrutura que guarda as informações do cliente

typedef struct {
    int id;
    int socket;
    pthread_t thread;
    struct CLIENTE *prox;
    struct CLIENTE *ant;
} CLIENTE;

//Estrutura que armazenará a fila

typedef struct fila {
    CLIENTE *inicio;
} FILA;

//Protótipos
CLIENTE * criaCliente();
void addCliente(CLIENTE* p);
void remover(CLIENTE *c);
void imprimeClientes();
void rotinaFinalizacao();
void *aguardaClientes(void *argv);
void *trataCliente(void *structCliente);
void enviaMensagem(char *buffer, CLIENTE *c);

//Definição do mutex dos clientes
pthread_mutex_t mutexClientes;

//Fila de Clientes
FILA *f;
//Número de clientes conectados
int nClientes = 0;
//Socket para conexões
int sockfd;

void main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Erro, porta não definida!\n");
        exit(1);
    }

    //Cria fila de clientes
    f = (FILA*) malloc(sizeof (FILA));

    //Inicia variavel de mutex dos clientes
    pthread_mutex_init(&mutexClientes, NULL);

    //Thread para aguardar por conexões
    pthread_t thread;

    int sig;

    sigset_t s;

    int portno = atoi(argv[1]);

    //Cria thread para aguardar cliente
    pthread_create(&thread, NULL, aguardaClientes, portno);

    //Esvazia set
    sigemptyset(&s);

    //Adiciona Sinal de Crtl-C
    sigaddset(&s, SIGINT);

    sigprocmask(SIG_BLOCK, &s, NULL);

    while (1) {

        //Aguarda Sinal
        sigwait(&s, &sig);

        printf("\n-------------------------------------\n");
        printf("Servidor Fechando Conexões!");
        printf("\n-------------------------------------\n");

        pthread_kill(thread, SIGTERM);

        //Fecha socket
        close(sockfd);

        //Executa rotina de finalizacao
        rotinaFinalizacao();

        free(f);

        //Destroi o mutex
        pthread_mutex_destroy(&mutexClientes);

        exit(0);

    }

}

//Função que aguarda por novos clientes

void* aguardaClientes(void *argv) {

    int porta = (int*) argv;

    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("Erro abrindo o socket!\n");
        exit(1);
    }
    bzero((char *) &serv_addr, sizeof (serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(porta);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Erro fazendo bind!\n");
        exit(1);
    }

    //Escuta o socket
    listen(sockfd, 5);

    printf("\n-------------------------------------\n");
    printf("Servidor Aguardando Conexões!");
    printf("\n-------------------------------------\n");

    while (1) {

        int socket;

        //Aguarda novos clientes chegarem
        socket = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        //Caso não for possível aceitar o cliente
        if (socket < 0) {
            printf("Erro na Conexão do Cliente!\n");
        } else {

            //Cria novo cliente		
            CLIENTE *c = criaCliente();
            c->socket = socket;

            //Adiciona cliente a fila
            addCliente(c);

            printf("Novo Cliente Conectou-se!\n");

            //Cria thread para tratar o novo cliente
            pthread_create(&(c->thread), NULL, trataCliente, (void*) c);

        }

    }

}


CLIENTE * criaCliente() {

    //Trava mutex
    pthread_mutex_lock(&mutexClientes);

    CLIENTE *c = (CLIENTE*) malloc(sizeof (CLIENTE));
    c->id = nClientes;
    c->socket = 0;
    c->thread = 0;
    c->ant = c;
    c->prox = c;

    //Libera mutex
    pthread_mutex_unlock(&mutexClientes);

    return c;
}

//Adiciona cliente na fila

void addCliente(CLIENTE* p) {

    char buffer[TAMANHO_BUFFER];

    //Trava mutex
    pthread_mutex_lock(&mutexClientes);

    //Verifica se a fila está vazia
    if (f->inicio != NULL) {

        CLIENTE *prox = (CLIENTE*) f->inicio;
        CLIENTE *ant = (CLIENTE*) prox->ant;

        p->prox = prox;
        p->ant = ant;
        ant->prox = p;
        prox->ant = p;

    } 

    //Atualiza início da fila
    f->inicio = p;

    //Incrementa número de clientes
    nClientes++;

    //Libera mutex
    pthread_mutex_unlock(&mutexClientes);

    enviaMensagem("entrou no chat\n", p);

}

//Retira o cliente da fila

void remover(CLIENTE *c) {

    char buffer[TAMANHO_BUFFER];

    enviaMensagem("saiu do chat\n", c);

    //Trava mutex
    pthread_mutex_lock(&mutexClientes);

    CLIENTE *prox = c->prox;
    CLIENTE *ant = c->ant;

    ant->prox = prox;
    prox->ant = ant;

    //Se o cliente a ser retirado for o fim da fila, atualiza fim da fila.
    if (c == f->inicio) {
        if (c == c->prox)
            f->inicio = NULL;
        else
            f->inicio = c->prox;

    }

    //Libera memória
    free(c);

    //Libera mutex
    pthread_mutex_unlock(&mutexClientes);

}

//Imprime os clientes 

void imprimeClientes() {

    CLIENTE *p = f->inicio;

    if (f->inicio != NULL) {

        printf("\nClientes Online\n");
        printf("-------------------------------------\n");

        do {
            printf("Cliente %d\n", p->id);

            p = p->prox;
        } while (p != f->inicio);
    }

}

//Rotina de Finalização

void rotinaFinalizacao() {

    //Trava mutex
    pthread_mutex_lock(&mutexClientes);

    CLIENTE* aux = f->inicio;
    //Percorre toda a fila
    while (aux != f->inicio) {
        CLIENTE* tmp = aux;
        aux = aux->prox;

        //Fecha socket
        close(tmp->socket);

        //Finaliza a thread
        pthread_exit(tmp->thread);

        free(tmp);
    }

    f->inicio = NULL;

    //Libera mutex
    pthread_mutex_unlock(&mutexClientes);
}

//Função que gerencia o cliente

void *trataCliente(void *structCliente) {

    CLIENTE *c = (CLIENTE*) structCliente;

    int n;

    char buffer[TAMANHO_BUFFER];

    while (1) {

        //Zera buffer
        bzero(buffer, sizeof (buffer));

        //Lê socket
        n = read(c->socket, buffer, TAMANHO_BUFFER);

        //Verifica se a leitura ocorreu com sucesso
        if (n <= 0) {
            printf("\nErro cliente %d! Fechando conexão\n", c->id);

            //Fechar conexão
            close(c->socket);

            //Remove Cliente
            remover(c);
            pthread_exit(NULL);

        } else if (strcmp(buffer, "sair\n") == 0) { //Se usuario escolheu sair

            printf("\nCliente %d fechou conexão\n", c->id);

            //Fechar conexão
            close(c->socket);

            //Remove Cliente
            remover(c);

            pthread_exit(NULL);

        } else if (strcmp(buffer, "clientes\n") == 0) {

            imprimeClientes();

        } else {
            printf("\nCliente %d enviou: %s\n", c->id, buffer);

            //Enviar mensagens
            enviaMensagem(buffer, c);

        }

    }

}

//Enviar mensagem para todos os clientes

void enviaMensagem(char *buffer, CLIENTE *c) {

    int n;

    char mensagem[TAMANHO_BUFFER];

    //Trava mutex
    pthread_mutex_lock(&mutexClientes);

    CLIENTE *aux = c->prox;

    //Repete para toda a lista
    while (aux != c) {

        //Monta mensagem
        sprintf(mensagem, "Cliente %d: %s", c->id, buffer);

        //Escreve no socket
        n = write(aux->socket, mensagem, TAMANHO_BUFFER);

        if (n < 0) {
            printf("Erro escrevendo no socket!\n");
        }

        //Atualiza próximo cliente		
        aux = aux->prox;

    }

    //Libera mutex
    pthread_mutex_unlock(&mutexClientes);
}

