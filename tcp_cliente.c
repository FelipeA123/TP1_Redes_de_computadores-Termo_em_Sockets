#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUF_SIZE 256

typedef enum {
    MSG_START,
    MSG_GUESS,
    MSG_FEEDBACK,
    MSG_END
} MsgType;

typedef struct {
    MsgType tipo;
    char palavra[BUF_SIZE];
} Mensagem;

int main(void)
{
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

    printf("[TCP] Conectado ao servidor!\n");

    for (int tentativa = 1; tentativa <= 6; tentativa++) {

        Mensagem mensagem;

        memset(&mensagem, 0, sizeof(mensagem));

        mensagem.tipo = MSG_GUESS;

        printf("Tentativa %d/6 - digite seu palpite: ", tentativa);

        fgets(mensagem.palavra, sizeof(mensagem.palavra), stdin);

        mensagem.palavra[strcspn(mensagem.palavra, "\n")] = '\0';

        send(
            sockfd,
            &mensagem,
            sizeof(mensagem),
            0
        );

        char buf[BUF_SIZE] = {0};

        ssize_t n = recv(
            sockfd,
            buf,
            sizeof(buf) - 1,
            0
        );

        if (n <= 0) {
            printf("Servidor desconectou.\n");
            break;
        }

        buf[n] = '\0';

        printf("\n[TCP] Resposta do servidor:\n");
        printf("%s\n", buf);

        if (strstr(buf, "Parabens") != NULL ||
            strstr(buf, "Voce perdeu") != NULL) {

            break;
        }
    }

    close(sockfd);

    return 0;
}
