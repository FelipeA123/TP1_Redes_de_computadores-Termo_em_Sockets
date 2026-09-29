#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdlib.h>


#define WORD_LEN 5
#define MSG_SIZE 128

#define FB_CORRETA 2
#define FB_EXISTE 1
#define FB_AUSENTE 0

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

static int eh_palavra_valida(const char *s)
{
    if (strlen(s) != WORD_LEN)
        return 0;

    for (int i = 0; i < WORD_LEN; i++) {
        if (!isalpha((unsigned char)s[i]))
            return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    /* Verifica os argumentos */
    if (argc != 3) {
        printf("Uso: %s <endereco_ip> <porta>\n", argv[0]);
        return 1;
    }

    const char *ip = argv[1];
    int porta = atoi(argv[2]);
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in6 addr = {0};

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(porta);

    if (inet_pton(AF_INET6, ip, &addr.sin6_addr) <= 0) {
        printf("Endereco IPv6 invalido: %s\n", ip);
        close(sockfd);
        return 1;
    }

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
        char palpite[WORD_LEN + 2];

        printf("Tentativa %d/6 - digite seu palpite: ", tentativa);

        fgets(palpite, sizeof(palpite), stdin);

        if (strchr(palpite, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        palpite[strcspn(palpite, "\n")] = '\0';

        if (!eh_palavra_valida(palpite)) {
            printf("Erro: digite exatamente 5 letras de A a Z.\n");
            tentativa--;
            continue;
        }

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
            printf("[TCP] Palpite: ");

            for (int i = 0; i < WORD_LEN; i++)
                printf("%c", msg.guess[i]);

            printf("\n");

            printf("[TCP] Feedback: ");

            for (int i = 0; i < WORD_LEN; i++) {
                if (msg.feedback[i] == FB_CORRETA) {
                    printf("%c ", msg.guess[i]);
                } else if (msg.feedback[i] == FB_EXISTE) {
                    printf("* ");
                } else {
                    printf("_ ");
                }
            }

            printf("\n");
            printf("%s\n", msg.message);
        } else if (msg.type == MSG_WIN || msg.type == MSG_EXIT) {
            if (msg.type == MSG_WIN) {
                printf("[TCP] Palpite: ");

                for (int i = 0; i < WORD_LEN; i++)
                    printf("%c", msg.guess[i]);

                printf("\n");

                printf("[TCP] Feedback: ");

                for (int i = 0; i < WORD_LEN; i++) {
                    if (msg.feedback[i] == FB_CORRETA)
                        printf("%c ", msg.guess[i]);
                    else if (msg.feedback[i] == FB_EXISTE)
                        printf("* ");
                    else
                        printf("_ ");
                }

                printf("\n");
            }

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
