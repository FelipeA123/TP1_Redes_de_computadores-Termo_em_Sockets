#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

#define MAX_ATTEMPTS 6
#define WORD_LEN 5
#define BUF_SIZE 256

#define FB_CORRETA 2
#define FB_EXISTE  1
#define FB_AUSENTE 0

int PORT = 0;
char *palavra = "";
int variavelbunda = 0;


/* Remove \n e \r do final da string */
static void remove_quebra_linha(char *s)
{
    int len = (int)strlen(s);

    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r'))
        s[--len] = '\0';
}


/* Converte a string para maiúsculas */
static void para_maiusculas(char *s)
{
    for (int i = 0; s[i] != '\0'; i++)
        s[i] = (char)toupper((unsigned char)s[i]);
}


/* Verifica se o palpite possui exatamente 5 letras */
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


/* Calcula o feedback do Termo */
static void calcula_feedback(
    const char *secreta,
    const char *palpite,
    int *feedback)
{
    int contagem[26] = {0};

    /* Conta as letras da palavra secreta */
    for (int i = 0; i < WORD_LEN; i++)
        contagem[secreta[i] - 'A']++;

    /* Primeiro verifica letras na posição correta */
    for (int i = 0; i < WORD_LEN; i++) {

        if (palpite[i] == secreta[i]) {
            feedback[i] = FB_CORRETA;
            contagem[palpite[i] - 'A']--;
        } else {
            feedback[i] = -1;
        }
    }

    /* Depois verifica letras existentes em posição errada */
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


/* Monta a resposta que será enviada para o cliente */
static void monta_resposta(
    const char *palpite,
    const int *feedback,
    char *resposta)
{
    resposta[0] = '\0';

    strcat(resposta, "Palpite: ");

    for (int i = 0; i < WORD_LEN; i++) {
        char temp[2];

        temp[0] = palpite[i];
        temp[1] = '\0';

        strcat(resposta, temp);
    }

    strcat(resposta, "\nFeedback: ");

    for (int i = 0; i < WORD_LEN; i++) {

        if (feedback[i] == FB_CORRETA) {
            strcat(resposta, "C ");
        }
        else if (feedback[i] == FB_EXISTE) {
            strcat(resposta, "E ");
        }
        else {
            strcat(resposta, "A ");
        }
    }

    strcat(resposta,
           "\nC = correta | E = existe em outra posicao | A = ausente\n");
}


int main(int argc, char *argv[])
{
    if (argc != 4) {
        printf("Uso: %s <v4|v6> <porta> <palavra>\n", argv[0]);
        return 1;
    }


    /* Verifica IPv4 ou IPv6 */
    if (strcmp(argv[1], "v4") != 0 &&
        strcmp(argv[1], "v6") != 0) {

        printf("Protocolo invalido. Use: v4 ou v6\n");
        return 1;
    }


    /* Pega a porta */
    PORT = atoi(argv[2]);

    if (PORT <= 0 || PORT > 65535) {
        printf("Porta invalida.\n");
        return 1;
    }


    /* Pega a palavra secreta */
    palavra = argv[3];

    para_maiusculas(palavra);

    if (!eh_palavra_valida(palavra)) {
        printf("A palavra secreta deve possuir 5 letras.\n");
        return 1;
    }


    /* Define IPv4 ou IPv6 */
    if (strcmp(argv[1], "v4") == 0)
        variavelbunda = AF_INET;
    else
        variavelbunda = AF_INET6;


    printf("Familia do socket: %d\n", variavelbunda);


    /* Cria socket */
    int server_fd = socket(
        variavelbunda,
        SOCK_STREAM,
        0
    );

    if (server_fd < 0) {
        perror("socket");
        return 1;
    }


    /* Permite reutilizar a porta */
    int opt = 1;

    setsockopt(
        server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &opt,
        sizeof(opt)
    );


    /* Estrutura genérica para endereço */
    struct sockaddr_storage addr;

    memset(&addr, 0, sizeof(addr));

    socklen_t addr_len = sizeof(addr);


    /* Configuração IPv4 */
    if (variavelbunda == AF_INET) {

        struct sockaddr_in *addr4 =
            (struct sockaddr_in *)&addr;

        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = htons(PORT);

    }

    /* Configuração IPv6 */
    else {

        struct sockaddr_in6 *addr6 =
            (struct sockaddr_in6 *)&addr;

        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = htons(PORT);
    }


    /* Faz bind */
    if (bind(
        server_fd,
        (struct sockaddr *)&addr,
        addr_len) < 0) {

        perror("bind");
        close(server_fd);
        return 1;
    }


    /* Coloca socket em modo de escuta */
    if (listen(server_fd, 5) < 0) {

        perror("listen");
        close(server_fd);
        return 1;
    }


    printf(
        "Servidor iniciado em modo %s na porta %d\n",
        argv[1],
        PORT
    );

    printf("Palavra secreta: %s\n", palavra);


    /* Aceita cliente */
    int client_fd = accept(
        server_fd,
        NULL,
        NULL
    );

    if (client_fd < 0) {

        perror("accept");
        close(server_fd);
        return 1;
    }


    printf("[TCP] Cliente conectado.\n");


    /*
     * Jogo começa aqui
     */

    int venceu = 0;

    for (int tentativa = 1;
         tentativa <= MAX_ATTEMPTS;
         tentativa++) {

        char buf[BUF_SIZE] = {0};

        ssize_t n = recv(
            client_fd,
            buf,
            sizeof(buf) - 1,
            0
        );

        if (n <= 0) {

            printf("[TCP] Cliente desconectado.\n");
            break;
        }


        buf[n] = '\0';

        remove_quebra_linha(buf);
        para_maiusculas(buf);


        printf(
            "[TCP] Tentativa %d/%d: %s\n",
            tentativa,
            MAX_ATTEMPTS,
            buf
        );


        /*
         * Verifica se o palpite possui
         * exatamente 5 letras
         */

        if (!eh_palavra_valida(buf)) {

            const char *erro =
                "ERRO: digite exatamente 5 letras de A a Z.\n";

            send(
                client_fd,
                erro,
                strlen(erro),
                0
            );

            /*
             * Como a tentativa foi invalida,
             * nao incrementamos a tentativa.
             */

            tentativa--;

            continue;
        }


        /*
         * Calcula feedback
         */

        int feedback[WORD_LEN];

        calcula_feedback(
            palavra,
            buf,
            feedback
        );


        /*
         * Monta resposta
         */

        char resposta[BUF_SIZE];

        monta_resposta(
            buf,
            feedback,
            resposta
        );


        /*
         * Verifica vitória
         */

        int acertou_tudo = 1;

        for (int i = 0; i < WORD_LEN; i++) {

            if (feedback[i] != FB_CORRETA) {

                acertou_tudo = 0;
                break;
            }
        }


        if (acertou_tudo) {

            strcat(
                resposta,
                "\nParabens! Voce venceu!\n"
            );

            venceu = 1;
        }


        /*
         * Se perdeu na sexta tentativa
         */

        if (!venceu &&
            tentativa == MAX_ATTEMPTS) {

            strcat(
                resposta,
                "\nVoce perdeu!\n"
            );

            strcat(
                resposta,
                "A palavra era: "
            );

            strcat(
                resposta,
                palavra
            );

            strcat(
                resposta,
                "\n"
            );
        }


        /*
         * Envia resultado para o cliente
         */

        send(
            client_fd,
            resposta,
            strlen(resposta),
            0
        );


        /*
         * Se venceu, termina o jogo
         */

        if (venceu)
            break;
    }


    close(client_fd);
    close(server_fd);

    return 0;
}
