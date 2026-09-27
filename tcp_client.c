/*
 * tcp_cliente.c
 *
 * Exemplo didático de cliente TCP (SOCK_STREAM).
 * Conecta ao servidor, envia uma mensagem e imprime a resposta.
 *
 * Uso: ./tcp_cliente
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 256

int main(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    const char *msg = "Ola, servidor TCP!";
    send(sockfd, msg, strlen(msg), 0);
    printf("[TCP] Mensagem enviada: %s\n", msg);

    char buf[BUF_SIZE] = {0};
    ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
    if (n > 0) {
        printf("[TCP] Resposta do servidor: %s\n", buf);
    }

    close(sockfd);
    return 0;
}
