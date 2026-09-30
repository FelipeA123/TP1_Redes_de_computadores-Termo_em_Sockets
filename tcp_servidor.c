#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#define MAX_ATTEMPTS 6
#define WORD_LEN 5
#define MSG_SIZE 128

#define FB_CORRETA 2
#define FB_EXISTE  1
#define FB_AUSENTE 0

/* Tipos de mensagem */
typedef enum {
    MSG_START,
    MSG_GUESS,
    MSG_FEEDBACK,
    MSG_WIN,
    MSG_ERROR,
    MSG_EXIT
} MessageType;

/* Estrutura das mensagens */
typedef struct {
    int type;
    int guess[5];
    int feedback[5];
    int attempts;
    int winstatus;
    char message[MSG_SIZE];
} GameMessage;

int PORT = 0;
char *palavra = "";
int protocolo = 0;

static void para_maiusculas(char *s)
{
    for (int i = 0; s[i] != '\0'; i++)
        s[i] = (char)toupper((unsigned char)s[i]);
}

static int eh_palavra_valida(const char *s)
{
    int len = (int)strlen(s);

    if (len != WORD_LEN)
        return 0;

    for (int i = 0; i < len; i++) {
        if (!isalpha((unsigned char)s[i]))
            return 0;
    }

    return 1;
}

static void calcula_feedback(const char *secreta, const char *palpite, int *feedback)
{
    int contagem[26] = {0};

    /* Conta as letras da palavra para tratar corretamente letras repetidas. */
    for (int i = 0; i < WORD_LEN; i++)
        contagem[secreta[i] - 'A']++;

    /* Marca primeiro as letras que estão na posição correta. */
    for (int i = 0; i < WORD_LEN; i++) {
        if (palpite[i] == secreta[i]) {
            feedback[i] = FB_CORRETA;
            contagem[palpite[i] - 'A']--;
        } else {
            feedback[i] = -1;
        }
    }

    /* Usa as letras restantes para identificar as que estão em outra posição. */
    for (int i = 0; i < WORD_LEN; i++) {
        if (feedback[i] == FB_CORRETA)
            continue;

        int idx = palpite[i] - 'A';

        if (contagem[idx] > 0) {
            feedback[i] = FB_EXISTE;
            contagem[idx]--;
        } else {
            feedback[i] = FB_AUSENTE;
        }
    }
}

int main(int argc, char *argv[])
{
    /* argv[0]: programa; argv[1]: protocolo; argv[2]: porta; argv[3]: palavra secreta. */
    if (argc != 4) {
        printf("Uso: %s <v4|v6> <porta> <palavra>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "v4") != 0 && strcmp(argv[1], "v6") != 0) {
        printf("Protocolo invalido. Use: v4 ou v6\n");
        return 1;
    }

    PORT = atoi(argv[2]);

    if (PORT <= 0 || PORT > 65535) {
        printf("Porta invalida.\n");
        return 1;
    }

    palavra = argv[3];
    para_maiusculas(palavra);

    if (!eh_palavra_valida(palavra)) {
        printf("A palavra secreta deve possuir 5 letras.\n");
        return 1;
    }

    if (strcmp(argv[1], "v4") == 0)
        protocolo = AF_INET;
    else
        protocolo = AF_INET6;

    int server_fd = socket(protocolo, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Estrutura genérica que comporta endereços IPv4 e IPv6. */
    struct sockaddr_storage addr;
    memset(&addr, 0, sizeof(addr));
    socklen_t addr_len = sizeof(addr);

    if (protocolo == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;

        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = htons(PORT);
    } else {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;

        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = htons(PORT);
    }

    if (bind(server_fd, (struct sockaddr *)&addr, addr_len) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Servidor iniciado em modo IP%s na porta %d.\n", argv[1], PORT);

    int client_fd = accept(server_fd, NULL, NULL);

    if (client_fd < 0) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    printf("Cliente Conectado\n");

    GameMessage msg;
    memset(&msg, 0, sizeof(msg));

    /* Envia ao cliente a mensagem inicial antes de receber palpites. */
    msg.type = MSG_START;
    msg.attempts = 0;
    msg.winstatus = 0;
    strcpy(msg.message, "Jogo iniciado. Envie uma palavra de 5 letras.");

    send(client_fd, &msg, sizeof(msg), 0);

    int venceu = 0;

    for (int tentativa = 1; tentativa <= MAX_ATTEMPTS; tentativa++) {
        memset(&msg, 0, sizeof(msg));

        /* Aguarda a mensagem do cliente; recv <= 0 indica desconexão ou erro. */
        ssize_t n = recv(client_fd, &msg, sizeof(msg), 0);

        if (n <= 0) {
            printf("Cliente Desconectado\n");
            break;
        }

        if (msg.type != MSG_GUESS) {
            printf("Tipo de mensagem invalido.\n");

            memset(&msg, 0, sizeof(msg));
            msg.type = MSG_ERROR;
            msg.winstatus = -1;
            strcpy(msg.message, "Tipo de mensagem invalido.");

            send(client_fd, &msg, sizeof(msg), 0);
            tentativa--;
            continue;
        }

        /* Converte os caracteres recebidos em uma string terminada em '\0'. */
        char buf[WORD_LEN + 1];

        for (int i = 0; i < WORD_LEN; i++)
            buf[i] = (char)msg.guess[i];

        buf[WORD_LEN] = '\0';
        para_maiusculas(buf);

        if (!eh_palavra_valida(buf)) {
            memset(&msg, 0, sizeof(msg));
            msg.type = MSG_ERROR;
            msg.attempts = tentativa - 1;
            msg.winstatus = -1;
            strcpy(msg.message, "ERRO: digite exatamente 5 letras de A a Z.");

            send(client_fd, &msg, sizeof(msg), 0);
            tentativa--;
            continue;
        }

        int feedback[WORD_LEN];
        calcula_feedback(palavra, buf, feedback);

        for (int i = 0; i < WORD_LEN; i++)
            msg.guess[i] = buf[i];

        for (int i = 0; i < WORD_LEN; i++)
            msg.feedback[i] = feedback[i];

        msg.attempts = tentativa;

        int acertou_tudo = 1;

        for (int i = 0; i < WORD_LEN; i++) {
            if (feedback[i] != FB_CORRETA) {
                acertou_tudo = 0;
                break;
            }
        }

        if (acertou_tudo) {
            msg.type = MSG_WIN;
            msg.winstatus = 1;
            strcpy(msg.message, "Parabéns! Você venceu!");
            printf("Cliente Desconectado");
            venceu = 1;
        } else {
            msg.type = MSG_FEEDBACK;
            msg.winstatus = 0;
            strcpy(msg.message, "Feedback recebido.");
        }

        send(client_fd, &msg, sizeof(msg), 0);

        if (venceu)
            break;
    }

    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_EXIT;
    msg.attempts = venceu ? 0 : MAX_ATTEMPTS;
    msg.winstatus = venceu ? 1 : 0;

    if (venceu) {
        strcpy(msg.message, "Conexao encerrada.");
    } else {
        snprintf(msg.message, MSG_SIZE, "Voce perdeu. A palavra era: %s", palavra);
    }

    send(client_fd, &msg, sizeof(msg), 0);

    close(client_fd);
    close(server_fd);

    return 0;
}
