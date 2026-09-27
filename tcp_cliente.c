#include <stdio.h>
#include <string.h>
#include <unistd.h>
#define PORT 8080
#define BUF_SIZE 256

int main(void)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    printf("[TCP] Conectado ao servidor!\n");

    for (int tentativa = 1; tentativa <= 6; tentativa++) {
        char msg[BUF_SIZE];
        printf("Tentativa %d/6 - digite seu palpite: ", tentativa);
        fgets(msg, sizeof(msg), stdin);
        msg[strcspn(msg, "\n")] = '\0';

        send(
            sockfd,
            msg,
            strlen(msg),
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


        /*
         * Se o servidor mandar "Parabens"
         * ou "Voce perdeu", podemos encerrar.
         */

        if (strstr(buf, "Parabens") != NULL ||
            strstr(buf, "Voce perdeu") != NULL) {

            break;
        }
    }

    close(sockfd);
    return 0;
}
