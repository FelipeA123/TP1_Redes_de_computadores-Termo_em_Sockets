/*
 * tcp_servidor.c
 *
 * Exemplo didático de servidor TCP (SOCK_STREAM).
 * Aceita uma conexão, recebe uma mensagem e responde.
 *
 * Uso: ./tcp_servidor
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int PORT = 0;
char *palavra = "";
int variavelbunda = 0;
#define BUF_SIZE 256

int main(int argc, char *argv[]){
    if (argc != 4) {
        return 1;
    }

    if (strcmp(argv[1], "v4") != 0 && strcmp(argv[1], "v6") != 0) {
        printf("Protocolo inválido. Use: v4 ou v6\n");
        return 1;
    }
    if (strlen(argv[2]) != 5 || strlen(argv[3]) != 5 ) {
        return 1;
    }
    PORT = atoi(argv[2]);
    palavra = argv[3];

    if(strcmp(argv[1], "v4") == 0) {
        variavelbunda = AF_INET;
    } else {
        variavelbunda = AF_INET6;
    }

    int server_fd = socket(variavelbunda, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    /* Permite reiniciar o servidor rapidamente sem erro de "endereço em uso" */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Servidor iniciado em modo %d na porta %d\n", variavelbunda, PORT);

    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }
    printf("[TCP] Cliente conectado.\n");

    char buf[BUF_SIZE] = {0};
    ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        printf("[TCP] Recebido: %s\n", buf);
        const char *resp = "Mensagem recebida pelo servidor TCP!";
        send(client_fd, resp, strlen(resp), 0);
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
