#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#define PORT 8080
#define WORD_LEN 5
#define MSG_SIZE 128

typedef enum {
    MSG_START,
    MSG_GUESS,
    MSG_FEEDBACK,
    MSG_WIN,
    MSG_ERROR,
    MSG_EXIT
} MessageType;

typedef struct {
    int type;
    int guess[5];
    int feedback[5];
    int attempts;
    int winstatus;
    char message[MSG_SIZE];
} GameMessage;

int main(void)
{
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in6 addr = {0};
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(PORT);
    inet_pton(AF_INET6, "::1", &addr.sin6_addr);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    printf("[TCP] Conectado ao servidor!\n");

    GameMessage msg;

    if (recv(sockfd, &msg, sizeof(msg), 0) <= 0 || msg.type != MSG_START) {
        printf("Falha ao iniciar o jogo.\n");
        close(sockfd);
        return 1;
    }

    printf("%s\n", msg.message);

    for (int tentativa = 1; tentativa <= 6; tentativa++) {

        char palpite[WORD_LEN + 1];

        printf("Tentativa %d/6 - digite seu palpite: ", tentativa);
        fgets(palpite, sizeof(palpite), stdin);
        palpite[strcspn(palpite, "\n")] = '\0';

        for (int i = 0; i < WORD_LEN; i++)
            palpite[i] = toupper((unsigned char)palpite[i]);

        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_GUESS;

        for (int i = 0; i < WORD_LEN; i++)
            msg.guess[i] = palpite[i];

        send(sockfd, &msg, sizeof(msg), 0);

        if (recv(sockfd, &msg, sizeof(msg), 0) <= 0) {
            printf("Servidor desconectou.\n");
            break;
        }

        if (msg.type == MSG_FEEDBACK) {

            printf("[TCP] Feedback: ");
            for (int i = 0; i < WORD_LEN; i++)
                printf("%d ", msg.feedback[i]);
            printf("\n%s\n", msg.message);

        } else if (msg.type == MSG_WIN || msg.type == MSG_EXIT) {

            printf("%s\n", msg.message);
            break;

        } else if (msg.type == MSG_ERROR) {

            printf("Erro: %s\n", msg.message);
            tentativa--;
        }
    }

    close(sockfd);
    return 0;
}
