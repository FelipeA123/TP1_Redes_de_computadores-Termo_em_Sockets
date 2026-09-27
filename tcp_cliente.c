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
    int sockfd = socket(
        AF_INET6,
        SOCK_STREAM,
        0
    );

    if (sockfd < 0) {
        perror("socket");
        return 1;
    }


    struct sockaddr_in6 addr;

    memset(
        &addr,
        0,
        sizeof(addr)
    );

    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(PORT);

    inet_pton(
        AF_INET6,
        "::1",
        &addr.sin6_addr
    );


    if (connect(
        sockfd,
        (struct sockaddr *)&addr,
        sizeof(addr)) < 0) {

        perror("connect");

        close(sockfd);

        return 1;
    }


    printf("[TCP] Conectado ao servidor!\n");


    /*
     * =====================================================
     * Recebe MSG_START
     * =====================================================
     */

    GameMessage msg;

    memset(
        &msg,
        0,
        sizeof(msg)
    );

    ssize_t n = recv(
        sockfd,
        &msg,
        sizeof(msg),
        0
    );

    if (n <= 0) {

        printf("Servidor desconectou.\n");

        close(sockfd);

        return 1;
    }


    if (msg.type != MSG_START) {

        printf(
            "[TCP] Mensagem inicial invalida.\n"
        );

        close(sockfd);

        return 1;
    }


    printf(
        "[TCP] %s\n",
        msg.message
    );


    /*
     * =====================================================
     * JOGO
     * =====================================================
     */

    for (int tentativa = 1;
         tentativa <= 6;
         tentativa++) {

        char palpite[WORD_LEN + 1];


        printf(
            "Tentativa %d/6 - digite seu palpite: ",
            tentativa
        );


        if (fgets(
            palpite,
            sizeof(palpite),
            stdin) == NULL) {

            break;
        }


        palpite[strcspn(
            palpite,
            "\n"
        )] = '\0';


        /*
         * Converte para maiúsculas
         */

        for (int i = 0; i < WORD_LEN; i++) {

            palpite[i] = (char)toupper(
                (unsigned char)palpite[i]
            );
        }


        /*
         * Monta MSG_GUESS
         */

        memset(
            &msg,
            0,
            sizeof(msg)
        );

        msg.type = MSG_GUESS;


        /*
         * Coloca cada letra no vetor guess
         */

        for (int i = 0; i < WORD_LEN; i++) {

            msg.guess[i] = palpite[i];
        }


        /*
         * Envia a estrutura completa
         */

        if (send(
            sockfd,
            &msg,
            sizeof(msg),
            0) < 0) {

            perror("send");

            break;
        }


        /*
         * =================================================
         * Recebe resposta do servidor
         * =================================================
         */

        memset(
            &msg,
            0,
            sizeof(msg)
        );


        n = recv(
            sockfd,
            &msg,
            sizeof(msg),
            0
        );


        if (n <= 0) {

            printf(
                "Servidor desconectou.\n"
            );

            break;
        }


        /*
         * MSG_FEEDBACK
         */

        if (msg.type == MSG_FEEDBACK) {

            printf(
                "[TCP] Feedback: "
            );

            for (int i = 0; i < WORD_LEN; i++) {

                printf(
                    "%d ",
                    msg.feedback[i]
                );
            }

            printf("\n");

            printf(
                "[TCP] %s\n",
                msg.message
            );
        }


        /*
         * MSG_WIN
         */

        else if (msg.type == MSG_WIN) {

            printf(
                "[TCP] %s\n",
                msg.message
            );

            break;
        }


        /*
         * MSG_ERROR
         */

        else if (msg.type == MSG_ERROR) {

            printf(
                "[TCP] Erro: %s\n",
                msg.message
            );

            tentativa--;

            continue;
        }


        /*
         * MSG_EXIT
         */

        else if (msg.type == MSG_EXIT) {

            printf(
                "[TCP] %s\n",
                msg.message
            );

            break;
        }


        /*
         * Tipo desconhecido
         */

        else {

            printf(
                "[TCP] Tipo de mensagem desconhecido.\n"
            );
        }
    }


    /*
     * =====================================================
     * Fecha conexão
     * =====================================================
     */

    close(sockfd);

    return 0;
}
