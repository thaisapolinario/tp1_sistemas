#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_ARGS 100  // Número máximo de argumentos 

// Trata o comando interno "cd"
void criar_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "Esperado argumento para \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            perror("cd");  // Se falhar, mostra o erro
        }
    }
}

// Mensagem de ajuda 
void mostrar_ajuda() {
    printf("Comandos internos disponíveis:\n");
    printf("  cd [diretório] - muda o diretório atual\n");
    printf("  exit - encerra o shell\n");
    printf("  help - mostra esta mensagem de ajuda\n");
}

// Verifica se o comando digitado é um comando interno 
int verifica_interno(char **args) {
    if (args[0] == NULL) return 0; // Se nada foi digitado, não faz nada

    if (strcmp(args[0], "cd") == 0) {
        criar_cd(args);
        return 1;
    }

    if (strcmp(args[0], "exit") == 0) {
        exit(0);  // Encerra o shell
    }

    if (strcmp(args[0], "help") == 0) {
        mostrar_ajuda();
        return 1;
    }

    return 0; // Não é um comando interno
}

// Executa comandos externos com fork + exec
void executar_comando(char **args, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Erro ao criar processo com fork");
        return;
    }

    if (pid == 0) {
        // Processo filho: tenta executar o comando
        if (execvp(args[0], args) == -1) {
            perror("Erro ao executar comando");
        }
        exit(1); // Se execvp falhar, encerra o filho
    } else {
        // Processo pai: espera o filho terminar, a menos que seja em background
        if (!background) {
            waitpid(pid, NULL, 0);
        }
    }
}

// Executa comandos encadeados com pipe (ex: ls | grep .c | sort)
void executar_pipe(char *command) {
    char *commands[MAX_ARGS];
    char *args[MAX_ARGS];
    int pipe_fd[2];
    int fd_in = 0;  // Descritor de entrada padrão para o próximo comando

    // Divide a linha com base no símbolo "|"
    char *cmd = strtok(command, "|");
    int i = 0;
    while (cmd != NULL) {
        commands[i++] = cmd;
        cmd = strtok(NULL, "|");
    }

    // Para cada comando entre os pipes
    for (int j = 0; j < i; j++) {
        pipe(pipe_fd);  // Cria um novo pipe
        pid_t pid = fork();

        if (pid == 0) { // Processo filho
            // Se não for o último comando, redireciona a saída para o pipe
            if (j < i - 1) {
                dup2(pipe_fd[1], STDOUT_FILENO);
            }
            // Se não for o primeiro, redireciona a entrada para o pipe anterior
            if (j > 0) {
                dup2(fd_in, STDIN_FILENO);
            }

            // Fecha descritores de pipe desnecessários
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            // Quebra o comando atual em argumentos
            int k = 0;
            char *arg = strtok(commands[j], " ");
            while (arg != NULL) {
                args[k++] = arg;
                arg = strtok(NULL, " ");
            }
            args[k] = NULL;

            // Tenta executar o comando
            if (execvp(args[0], args) == -1) {
                perror("Erro ao executar comando no pipe");
                exit(1);
            }
        } else {
            // Processo pai: espera o filho e prepara o próximo pipe
            wait(NULL);
            close(pipe_fd[1]);
            fd_in = pipe_fd[0];  // Entrada para o próximo comando
        }
    }
}

// Loop principal do shell
int main() {
    char *line;
    char *args[MAX_ARGS];
    int background;

    while (1) {
        // Mostra o prompt do shell
        line = readline("novo_shell> ");
        if (line == NULL) {
            printf("\n");
            exit(0); // Ctrl+D encerra o shell
        }

        // Adiciona comando ao histórico
        add_history(line);

        // Verifica se comando será executado em background
        background = 0;
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '&') {
            background = 1;
            line[len - 1] = '\0';  // Remove '&' do final
        }

        // Cópia da linha para verificar pipes sem corromper a original
        char *line_copy = strdup(line);
        if (strchr(line_copy, '|') != NULL) {
            executar_pipe(line_copy);
            free(line_copy);
            free(line);
            continue;
        }
        free(line_copy);

        // Divide a linha em argumentos com strtok
        int i = 0;
        char *arg = strtok(line, " ");
        while (arg != NULL && i < MAX_ARGS - 1) {
            args[i++] = arg;
            arg = strtok(NULL, " ");
        }
        args[i] = NULL;

        // Verifica e executa comandos internos
        if (verifica_interno(args)) {
            free(line);
            continue;
        }

        // Executa comando externo (em foreground ou background)
        executar_comando(args, background);

        free(line);
    }

    return 0;
}

