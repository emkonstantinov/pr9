#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
    int sockfd;
    int port = 5000;
    struct sockaddr_in server_addr;
    char input[128];
    char response[128];
    int result;
    char *end;
    long port_number;
    size_t length;

    if (argc != 2 && argc != 3)
    {
        printf("Использование: ./client IP-адрес [порт]\n");
        printf("Если порт не указан, используется 5000.\n");
        return 1;
    }

    if (argc == 3)
    {
        port_number = strtol(argv[2], &end, 10);

        if (*end != '\0' || port_number < 1 || port_number > 65535)
        {
            printf("Ошибка: порт должен быть числом от 1 до 65535.\n");
            return 1;
        }

        port = (int)port_number;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

     if (sockfd == -1)
    {
        perror("Ошибка socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (server_addr.sin_addr.s_addr == INADDR_NONE)
    {
        printf("Ошибка: неправильный IPv4-адрес.\n");
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Ошибка connect");
        close(sockfd);
        return 1;
    }

    printf("Соединение с сервером установлено.\n");
    printf("Игра началась. Нужно угадать число от 1 до 100.\n");

    while (1)
    {
        printf("Введите число: ");

        if (fgets(input, sizeof(input) - 1, stdin) == NULL)
        {
            printf("\nВвод завершен.\n");
            break;
        }

        length = strlen(input);

        if (length > 0 && input[length - 1] != '\n')
        {
            input[length] = '\n';
            input[length + 1] = '\0';
        }

        if (send_all(sockfd, input, (int)strlen(input)) == -1)
        {
            perror("Ошибка send");
            break;
        }

        result = receive_line(sockfd, response, sizeof(response));

        if (result <= 0)
        {
            printf("Сервер закрыл соединение.\n");
            break;
        }

        if (strcmp(response, "MORE") == 0)
        {
            printf("Загаданное число БОЛЬШЕ.\n");
        }
        else if (strcmp(response, "LESS") == 0)
        {
            printf("Загаданное число МЕНЬШЕ.\n");
        }
        else if (strcmp(response, "WIN") == 0)
        {
            printf("Поздравляем! Вы угадали число!\n");
            break;
        }
        else if (strcmp(response, "ERROR") == 0)
        {
            printf("Нужно ввести целое число от 1 до 100.\n");
        }
        else
        {
            printf("Получен неизвестный ответ сервера: %s\n", response);
        }
    }

    close(sockfd);
    return 0;
}

