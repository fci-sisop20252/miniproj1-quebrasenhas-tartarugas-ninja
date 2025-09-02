#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include "hash_utils.h"

/**
 * PROCESSO COORDENADOR - Mini-Projeto 1: Quebra de Senhas Paralelo
 * 
 * Este programa coordena múltiplos workers para quebrar senhas MD5 em paralelo.
 * O MD5 JÁ ESTÁ IMPLEMENTADO - você deve focar na paralelização (fork/exec/wait).
 * 
 * Uso: ./coordinator <hash_md5> <tamanho> <charset> <num_workers>
 * 
 * Exemplo: ./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 4
 * 
 * SEU TRABALHO: Implementar os TODOs marcados abaixo
 */

#define MAX_WORKERS 16
#define RESULT_FILE "password_found.txt"

/**
 * Calcula o tamanho total do espaço de busca
 * 
 * @param charset_len Tamanho do conjunto de caracteres
 * @param password_len Comprimento da senha
 * @return Número total de combinações possíveis
 */
long long calculate_search_space(int charset_len, int password_len) {
    long long total = 1;
    for (int i = 0; i < password_len; i++) {
        total *= charset_len;
    }
    return total;
}

/**
 * Converte um índice numérico para uma senha
 * Usado para definir os limites de cada worker
 * 
 * @param index Índice numérico da senha
 * @param charset Conjunto de caracteres
 * @param charset_len Tamanho do conjunto
 * @param password_len Comprimento da senha
 * @param output Buffer para armazenar a senha gerada
 */
void index_to_password(long long index, const char *charset, int charset_len, 
                       int password_len, char *output) {
    for (int i = password_len - 1; i >= 0; i--) {
        output[i] = charset[index % charset_len];
        index /= charset_len;
    }
    output[password_len] = '\0';
}

/**
 * Função principal do coordenador
 */
int main(int argc, char *argv[]) {
    // TODO 1: Validar argumentos de entrada
    // Verificar se argc == 5 (programa + 4 argumentos)
    // Se não, imprimir mensagem de uso e sair com código 1
    
    // IMPLEMENTE AQUI: verificação de argc e mensagem de erro
    // Parsing dos argumentos (após validação)
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <hash> <tamanho> <charset> <workers>\n", argv[0]);
        return 1;
    }

    const char *target_hash = argv[1];
    int password_len = atoi(argv[2]);
    const char *charset = argv[3];
    int num_workers = atoi(argv[4]);
    int charset_len = strlen(charset);
    
    // TODO: Adicionar validações dos parâmetros
    // - password_len deve estar entre 1 e 10
    if (password_len < 1 || password_len>10) {
        fprintf(stderr, "Erro: tamanho da senha deve estar entre 1 e 10.\n");
        return 1;
    }
    // - num_workers deve estar entre 1 e MAX_WORKERS
    if (num_workers<1|| num_workers>MAX_WORKERS) {
        fprintf(stderr, "Erro: número de workers deve estar entre 1 e %d\n", MAX_WORKERS);
        return 1;
    }
    // - charset não pode ser vazio
    if (charset_len == 0) {
        fprintf(stderr, "Erro: charset não pode ser vazio.\n");
        return 1;
    }
    
    printf("=== Mini-Projeto 1: Quebra de Senhas Paralelo ===\n");
    printf("Hash MD5 alvo: %s\n", target_hash);
    printf("Tamanho da senha: %d\n", password_len);
    printf("Charset: %s (tamanho: %d)\n", charset, charset_len);
    printf("Número de workers: %d\n", num_workers);
    
    // Calcular espaço de busca total
    long long total_space = calculate_search_space(charset_len, password_len);
    printf("Espaço de busca total: %lld combinações\n\n", total_space);
    
    // Remover arquivo de resultado anterior se existir
    unlink(RESULT_FILE);
    
    // Registrar tempo de início
    time_t start_time = time(NULL);
    
    // TODO 2: Dividir o espaço de busca entre os workers
    // Calcular quantas senhas cada worker deve verificar
    // DICA: Use divisão inteira e distribua o resto entre os primeiros workers
    
    // IMPLEMENTE AQUI:
    long long passwords_per_worker = total_space / num_workers;
    long long remaining = total_space % num_workers;
    
    // Arrays para armazenar PIDs dos workers
    pid_t workers[MAX_WORKERS];
    
    // TODO 3: Criar os processos workers usando fork()
    printf("Iniciando workers...\n");
    
    // IMPLEMENTE AQUI: Loop para criar workers
    for (int i = 0; i < num_workers; i++) {
        // TODO: Calcular intervalo de senhas para este worker 
        // TODO: Converter indices para senhas de inicio e fim
        long long start_index = (total_space * i) / num_workers;
        long long end_index   = (total_space * (i + 1)) / num_workers;

        char start_str[32], end_str[32];
        sprintf(start_str, "%lld", start_index);
        sprintf(end_str, "%lld", end_index);
        // TODO 4: Usar fork() para criar processo filho
        pid_t pid = fork();
        if(pid > 0){
        // TODO 5: No processo pai: armazenar PID
        workers[i] = pid;
        }
        else if(pid == 0){
        // TODO 6: No processo filho: usar execl() para executar worker
        execl("./worker", "worker", target_hash, start_str, end_str, charset, 
          argv[2], worker_id_str, NULL);
        printf("DEBUG: Se chegou aqui, execl falhou!\n");
        }
        else{
        // TODO 7: Tratar erros de fork() e execl()
        perror("Erro ao criar processo (fork)");
        exit(1);
        }
    }
    
    printf("\nTodos os workers foram iniciados. Aguardando conclusão...\n");
    
    // TODO 8: Aguardar todos os workers terminarem usando wait()
    // IMPORTANTE: O pai deve aguardar TODOS os filhos para evitar zumbis
    
    // IMPLEMENTE AQUI:
    // - Loop para aguardar cada worker terminar
    // - Usar wait() para capturar status de saída
    // - Identificar qual worker terminou
    // - Verificar se terminou normalmente ou com erro
    // - Contar quantos workers terminaram
    for (int i = 0; i < num_workers; i++) {
        int status;
        pid_t child_pid = wait(&status);

        if (child_pid > 0) {
            printf("Filho %d terminou\n", child_pid);
            
            // Analisar o status de saída
            if (WIFEXITED(status)) {
                int exit_code = WEXITSTATUS(status);
                printf("Código de saída: %d\n", exit_code);
            }
            else{
                printf("Worker com PID %d terminou de forma anormal.\n", child_pid);
            }
        }
    }
    // Registrar tempo de fim
    time_t end_time = time(NULL);
    double elapsed_time = difftime(end_time, start_time);
    
    printf("\n=== Resultado ===\n");
    
    // TODO 9: Verificar se algum worker encontrou a senha
    // Ler o arquivo password_found.txt se existir
    
    // IMPLEMENTE AQUI:
    // - Abrir arquivo RESULT_FILE para leitura
    // - Ler conteúdo do arquivo
    // - Fazer parse do formato "worker_id:password"
    // - Verificar o hash usando md5_string()
    // - Exibir resultado encontrado
    
    // Estatísticas finais (opcional)
    // TODO: Calcular e exibir estatísticas de performance
    FILE *fp = fopen(RESULT_FILE, "r");
    if (fp != NULL) {
        char line[256];
        if (fgets(line, sizeof(line), fp)) {
            int worker_id;
            char password[64];

            if (sscanf(line, "%d:%63s", &worker_id, password) == 2) {
                char hash_check[33];
                md5_string(password, hash_check);

                if (strcmp(hash_check, target_hash) == 0) {
                    printf("Senha encontrada pelo worker %d: %s\n", worker_id, password);
                } else {
                    printf("Senha lida no arquivo não corresponde ao hash.\n");
                }
            } else {
                printf("Formato inválido no arquivo de resultado.\n");
            }
        } else {
            printf("Arquivo de resultado vazio.\n");
        }
        fclose(fp);
    } else {
        printf("Senha não foi encontrada por nenhum worker.\n");
    }
    
    
    return 0;
}