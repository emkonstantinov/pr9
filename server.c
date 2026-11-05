#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

int send_all(int sockfd, const char *data, int length)
{
    int sent = 0;

    while (sent < length)
    {
        int result = send(sockfd, data + sent, length - sent, 0);

        if (result <= 0)
        {
            return -1;
        }

        sent = sent + result;
    }

    return 0;
}

int receive_line(int sockfd, char *buffer, int size)
{
    int i = 0;
    char symbol;
    int result;

    while (i < size - 1)
    {
        result = recv(sockfd, &symbol, 1, 0);

        if (result == 0)
        {
            return 0;
        }

        if (result < 0)
        {
            return -1;
        }

        if (symbol == '\n')
        {
            break;
        }

        buffer[i] = symbol;
        i++;
    }

    buffer[i] = '\0';
    return 1;
}

int main(int argc, char *argv[])
{
    int server_fd;
    int client_fd;
    int port;
    int reuse = 1;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_len;
    char *client_ip;
    char guess_text[128];
    int secret_number;
    int guess;
    int result;
    char *end;
    long port_number;

    if (argc != 2)
    {
        printf("Использование: ./server порт\n");
        return 1;
    }

    port_number = strtol(argv[1], &end, 10);

    if (*end != '\0' || port_number < 1 || port_number > 65535)
    {
        printf("Ошибка: порт должен быть числом от 1 до 65535.\n");
        return 1;
    }

    port = (int)port_number;
    srand((unsigned int)time(NULL));

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd == -1)
    {
        perror("Ошибка socket");
        return 1;
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Ошибка bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1)
    {
        perror("Ошибка listen");
        close(server_fd);
        return 1;
    }

    printf("Сервер запущен. Порт: %d\n", port);
    printf("Ожидание подключений...\n");
    fflush(stdout);

    while (1)
    {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd == -1)
        {
            perror("Ошибка accept");
            continue;
        }

        client_ip = inet_ntoa(client_addr.sin_addr);

        if (client_ip == NULL)
        {
            client_ip = "unknown";
        }

        printf("%s:%d: клиент подключился\n", client_ip, ntohs(client_addr.sin_port));
        fflush(stdout);

        secret_number = rand() % 100 + 1;

         while (1)
        {
            result = receive_line(client_fd, guess_text, sizeof(guess_text));

            if (result == 0)
            {
                break;
            }

            if (result < 0)
            {
                perror("Ошибка recv");
                break;
            }

            guess = atoi(guess_text);
            printf("%s:%d: попытка: %s\n", client_ip, ntohs(client_addr.sin_port), guess_text);
            fflush(stdout);

            if (guess < 1 || guess > 100)
            {
                send_all(client_fd, "ERROR\n", 6);
                printf("%s:%d: ответ: введите число от 1 до 100\n", client_ip, ntohs(client_addr.sin_port));
            }
            else if (guess < secret_number)
            {
                send_all(client_fd, "MORE\n", 5);
                printf("%s:%d: ответ: загаданное число больше\n", client_ip, ntohs(client_addr.sin_port));
            }
            else if (guess > secret_number)
            {
                send_all(client_fd, "LESS\n", 5);
                printf("%s:%d: ответ: загаданное число меньше\n", client_ip, ntohs(client_addr.sin_port));
            }
            else
            {
                send_all(client_fd, "WIN\n", 4);
                printf("%s:%d: число угадано: %d\n", client_ip, ntohs(client_addr.sin_port), secret_number);
                break;
            }

            fflush(stdout);
        }

        printf("%s:%d: клиент отключился\n", client_ip, ntohs(client_addr.sin_port));
        fflush(stdout);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
