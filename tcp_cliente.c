#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>
#include <stdlib.h>
#include <netdb.h>


#define WORD_LEN 5
#define MSG_SIZE 128

#define FB_CORRETA 2  /* Letra correta na posição certa. */
#define FB_EXISTE 1   /* Letra presente em outra posição. */
#define FB_AUSENTE 0  /* Letra que não aparece na palavra. */

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

    struct addrinfo hints, *res, *p;
    int sockfd = -1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;      // IPv4 ou IPv6
    hints.ai_socktype = SOCK_STREAM;

    char porta_str[10];
    sprintf(porta_str, "%d", porta);

    if (getaddrinfo(ip, porta_str, &hints, &res) != 0) {
        printf("Endereco IP invalido: %s\n", ip);
        return 1;
    }

    for (p = res; p != NULL; p = p->ai_next) {

        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (sockfd < 0)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
            break;

        close(sockfd);
        sockfd = -1;
    }

    freeaddrinfo(res);

    if (sockfd < 0) {
        perror("connect");
        return 1;
    }
    GameMessage msg;

    if (recv(sockfd, &msg, sizeof(msg), 0) <= 0 || msg.type != MSG_START) {
        printf("Falha ao iniciar o jogo.\n");
        close(sockfd);
        return 1;
    }

    for (int tentativa = 1; tentativa <= 6; tentativa++) {
        // +2 para ter espaço para as 5 letras, '\n' e '\0'.
        char palpite[WORD_LEN + 2];

        printf("Insira seu palpite:\n");
        printf("> ");
        fgets(palpite, sizeof(palpite), stdin);

        /* Descarta o restante da entrada se o palpite exceder o buffer. */
        if (strchr(palpite, '\n') == NULL) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        palpite[strcspn(palpite, "\n")] = '\0';

        if (!eh_palavra_valida(palpite)) {
            printf("Erro: Insira uma sequência de 5 caracteres de A a Z!\n");
            tentativa--; /* Entrada inválida não consome uma tentativa. */
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
            printf("Dica: ");
            for (int i = 0; i < WORD_LEN; i++) {
                /* Imprime o separador antes das letras, sem espaço final. */
                if (i > 0)
                    printf(" ");

                if (msg.feedback[i] == FB_CORRETA)
                    printf("%c", msg.guess[i]);
                else if (msg.feedback[i] == FB_EXISTE)
                    printf("*");
                else
                    printf("_");
            }
            printf("\n");
            printf("Tentativas realizadas: %d\n", tentativa);

        } else if (msg.type == MSG_WIN || msg.type == MSG_EXIT) {
            printf("%s\n", msg.message);
            break;
        } else if (msg.type == MSG_ERROR) {
            printf("Erro: %s\n", msg.message);
            tentativa--; /* Erro informado pelo servidor não consome tentativa. */
        }
    }

    close(sockfd);
    return 0;
}
