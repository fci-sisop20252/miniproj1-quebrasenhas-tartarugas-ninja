# Relatório: Mini-Projeto 1 - Quebra-Senhas Paralelo

**Aluno(s):** 
- Arthur Rodrigues Lourenço Soares (10434424)
- Luiz Alberto Silva Mota (10436776)
- Gustavo Melo Silva (10438415)
- Marcus Ruiz Nishimura Baena (10426044)
---

## 1. Estratégia de Paralelização


**Como você dividiu o espaço de busca entre os workers?**

- O algoritmo se inicia calculando o total de combinações (*total_space*) e o divide igualmente entre o *num_workers*, resultando na carga base (*passwords_per_worker*) e no *remaining*. No loop de criação dos workers, cada um recebe a carga base, mais uma senha extra se seu índice for menor que o remaining. O coordenador rastreia o ponto de início (*current_start*), calcula o *start_index* e o *end_index*, converte esses índices para senhas (*start_str, end_str*) usando *index_to_password()*, e as passa via *execl()* para o worker iniciar a busca em seu bloco exclusivo.

**Código relevante:** Cole aqui a parte do coordinator.c onde você calcula a divisão:
```c
long long total_space = calculate_search_space(charset_len, password_len);
printf("Espaço de busca total: %lld combinações\n\n", total_space);
.
.
.
long long passwords_per_worker = total_space / num_workers;
long long remaining = total_space % num_workers;

// Arrays para armazenar PIDs dos workers
pid_t workers[MAX_WORKERS];

// TODO 3: Criar os processos workers usando fork()
printf("Iniciando workers...\n");
long long current_start = 0;
// IMPLEMENTE AQUI: Loop para criar workers
for (int i = 0; i < num_workers; i++) {
    // TODO: Calcular intervalo de senhas para este worker 
    // TODO: Converter indices para senhas de inicio e fim
    long long block_size = passwords_per_worker + (i < remaining ? 1 : 0);

    if (block_size == 0) {
        workers[i] = -1;
        continue;
    }
    long long start_index = current_start;
    long long end_index   = start_index + block_size;

    if (start_index < 0) start_index = 0;
    if (end_index >= total_space) end_index = total_space - 1;

    char start_str[32], end_str[32];
    index_to_password(start_index, charset, charset_len, password_len, start_str);
    index_to_password(end_index, charset, charset_len, password_len, end_str);
    // TODO 4: Usar fork() para criar processo filho
    pid_t pid = fork();

    if(pid > 0){
        // TODO 5: No processo pai: armazenar PID
        workers[i] = pid;
    }else if(pid == 0){
        char worker_id_str[16];
        sprintf(worker_id_str, "%d", i);
    // TODO 6: No processo filho: usar execl() para executar worker
        execl("./worker", "worker", target_hash, start_str, end_str, charset, 
        argv[2], worker_id_str, NULL);
        printf("DEBUG: Se chegou aqui, execl falhou!\n");
    }
    else{
    // TODO 7: Tratar erros de fork() e execl()
        perror("Erro ao criar processo (fork)");
        for (int j = 0; j < i; j++) {
            if (workers[j] > 0) waitpid(workers[j], NULL, 0);
        }
        exit(1);
    }
    current_start = end_index + 1;
}
```

---

## 2. Implementação das System Calls

**Descreva como você usou fork(), execl() e wait() no coordinator:**

- Primeiro o **coordinator** criou os processos usando *fork()*. No processo processo filho, usamos o *execl()* para substituir a imagem do processo pelo programa **worker** com os paramentros: *hash, intervalo de senhas, charset e o ID do worker*. O processo pai armazenou os PIDs e no final utilizou o *wait()* para gerenciar e esperar a finalização dos processos workers que foram criados via fork() e execl().

**Código do fork/exec:**
```c
pid_t pid = fork();
if(pid > 0){
    // TODO 5: No processo pai: armazenar PID
    workers[i] = pid;
}else if(pid == 0){
    char worker_id_str[16];
    sprintf(worker_id_str, "%d", i);
    // TODO 6: No processo filho: usar execl() para executar worker
    execl("./worker", "worker", target_hash, start_str, end_str, charset, 
    argv[2], worker_id_str, NULL);
    printf("DEBUG: Se chegou aqui, execl falhou!\n");
}
```

---

## 3. Comunicação Entre Processos

**Como você garantiu que apenas um worker escrevesse o resultado?**


- A escrita atômica foi usada no arquivo *password_found.txt*. Cada worker tentam criar o arquivo com o comando *open()* com os parametros **O_CREAT | O_EXCL** e caso já exista, worker falha na criação. Assim, apenas o primeiro worker que encontrar a senha consegue gravar o resultado.

Leia sobre condições de corrida (aqui)[https://pt.stackoverflow.com/questions/159342/o-que-%C3%A9-uma-condi%C3%A7%C3%A3o-de-corrida]

**Como o coordinator consegue ler o resultado?**

No final da execução, o coordinator abre o arquivo *password_found.txt*, lê a linha no formato *"worker_id:senha"*, e calcula novamente o hash da senha para verificar se corresponde ao hash alvo antes de exibir o resultado.

---

## 4. Análise de Performance
Complete a tabela com tempos reais de execução:
O speedup é o tempo do teste com 1 worker dividido pelo tempo com 4 workers.

| Teste | 1 Worker | 2 Workers | 4 Workers | Speedup (4w) |
|-------|----------|-----------|-----------|--------------|
| Hash: 202cb962ac59075b964b07152d234b70<br>Charset: "0123456789"<br>Tamanho: 3<br>Senha: "123" | 0.00s | 0.00s | 0.00s | 0.00s|
| Hash: 5d41402abc4b2a76b9719d911017c592<br>Charset: "abcdefghijklmnopqrstuvwxyz"<br>Tamanho: 5<br>Senha: "hello" | 4s | 7s | 2s | 2s |

*O speedup foi linear? Por quê?*

O speedup não foi completamente linear. Embora a utilização de mais workers tenha reduzido o tempo de execução, o ganho não foi proporcional ao número de processos criados. Isso ocorre devido ao overhead de criação e gerenciamento dos processos, como à forma como a carga de trabalho é distribuída: em alguns casos, a senha pode ser encontrada rapidamente por um dos workers, fazendo com que os outos não explorem todo o espaço de busca. Dessa forma, a paralelização trouxe um ganho de desempenho, mas não foi perfeitamente linear.
---

## 5. Desafios e Aprendizados
*Qual foi o maior desafio técnico que você enfrentou?*

No começo percebemos que dois workers estavam falhando, depois descobrimos que o password_checked estava sendo incrementada duas vezes, e que no coordinator o current_start não avançava corretamente então como resultado, os workers recebiam o mesmo intervalo de busca, gerando sobreposição total entre eles.

---

## Comandos de Teste Utilizados

bash
# Teste básico
./coordinator "900150983cd24fb0d6963f7d28e17f72" 3 "abc" 2

# Teste de performance
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 1
time ./coordinator "202cb962ac59075b964b07152d234b70" 3 "0123456789" 4

# Teste com senha maior
time ./coordinator "5d41402abc4b2a76b9719d911017c592" 5 "abcdefghijklmnopqrstuvwxyz" 4

---

*Checklist de Entrega:*
- [x] Código compila sem erros
- [x] Todos os TODOs foram implementados
- [x] Testes passam no ./tests/simple_test.sh
- [x] Relatório preenchido
