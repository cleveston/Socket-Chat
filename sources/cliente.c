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
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>
#include <pthread.h>

#define TAMANHO_BUFFER 256

//Protótipos
void *leitura();
void *escrita();
void rotinaFinalizacao();

//Socket para leitura
int sockfd;

//Threads para lidar com o cliente
pthread_t thread_escrita, thread_leitura;

void main(int argc, char *argv[]) {
    int portno;
    struct sockaddr_in serv_addr;

    int sig;

    sigset_t s;

    if (argc < 3) {
        printf("Uso: %s nomehost porta\n", argv[0]);
        exit(1);
    }

    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
        printf("Erro criando socket!\n");
        exit(1);
    }

    bzero((char *) &serv_addr, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;

    inet_aton(argv[1], &serv_addr.sin_addr);

    serv_addr.sin_port = htons(portno);

    //Conecta no servidor
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Erro conectando!\n");
        exit(1);
    }

    printf("\n-------------------------------------\n");
    printf("Cliente Conectado!");
    printf("\n-------------------------------------\n");

    //Thread que irá ler os dados
    pthread_create(&thread_leitura, NULL, leitura, NULL);

    //Thread que irá escrevcer os dados
    pthread_create(&thread_escrita, NULL, escrita, NULL);

    //Esvazia set
    sigemptyset(&s);

    //Adiciona Sinal de Crtl-C
    sigaddset(&s, SIGINT);

    sigprocmask(SIG_BLOCK, &s, NULL);

    while (1) {

        //Aguarda Sinal
        sigwait(&s, &sig);

        //Executa rotina de finalização
        rotinaFinalizacao();

    }

}

//Função que lê os dados do servidor

void *leitura() {

    char buffer[TAMANHO_BUFFER];
    int n;

    while (1) {
        //Zera buffer
        bzero(buffer, sizeof (buffer));

        //Recebe dados do servidor
        n = recv(sockfd, buffer, TAMANHO_BUFFER, 0);

        //Se houve erro
        if (n <= 0) {
            printf("Erro lendo do socket!\n");
            //Executa rotina de finalização
            rotinaFinalizacao();

        } else {
            printf("\r                               ");
            printf("\r%s", buffer);
            printf("\nDigite a mensagem (ou sair):");

            fflush(stdout);
        }
    }
}

//Função que escreve os dados para o servidor

void *escrita() {

    int n;

    char buffer[TAMANHO_BUFFER];

    while (1) {
        //Zera buffer
        bzero(buffer, sizeof (buffer));

        printf("\nDigite a mensagem (ou sair):");

        fgets(buffer, TAMANHO_BUFFER, stdin);

        //Escreve no socket
        n = send(sockfd, buffer, TAMANHO_BUFFER, 0);

        //Se houve erro
        if (n <= 0) {
            printf("Erro escrevendo no socket!\n");
            //Executa rotina de finalização
            rotinaFinalizacao();
        } else if (strcmp(buffer, "sair\n") == 0) { //Se o cliente desejou sair
            //Executa rotina de finalização
            rotinaFinalizacao();
        }
    }

}

//Rotina de Finalização

void rotinaFinalizacao() {

    printf("\n-------------------------------------\n");
    printf("Cliente Fechando Conexão!");
    printf("\n-------------------------------------\n");

    //Fecha socket
    close(sockfd);

    //Mata thread de escrita
    pthread_kill(thread_escrita, SIGTERM);

    //Mata thread de leitura
    pthread_kill(thread_leitura, SIGTERM);

    exit(0);
}

