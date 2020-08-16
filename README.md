#  Introdução e uso

Este é o primeiro Exercício-Programa (EP1) da matéria **MAC0219/5742 – Introdução à Computação Concorrente, Paralela e Distribuída**, onde construímos uma versão do utilitário Unix/Linux _Grep_ para uso de múltiplas _threads_ em paralelo e com acesso recursivo de diretórios, o **pgrep**

Para compilá-lo, simplesmente execute o comando _make_ em um terminal no diretório onde está localizado o arquivo _Makefile_. Para rodá-lo, execute o comando:

``bash

./pgrep MAX_THREADS REGEX_PESQUISA CAMINHO_DO_DIRETÓRIO

``

onde MAX_THREADS é o número máximo de threads que podem ser criadas durante a execução do programa (deve ser maior ou igual a 2), REGEX_PESQUISA é uma sentença em REGEX na especificação BRE (conjunto básico de expressões regulares) e CAMINHO_DO_DIRETÓRIO é o caminho do diretório onde se inicia a procura recursiva.

A saída do programa é um conjunto de linhas de formato

``text

caminho_relativo/nome_do_arquivo: numero_da_linha_correspondida

``

É importante ressaltar que **o número mínimo de _threads_ para rodar o programa é 2**. Isso foi estabelecido pois a parte do programa que faz a procura recursiva espera uma das _threads_ terminar quando o número máximo de threads é atingido. Como o programa necessariamente cria uma outra _thread_ para fazer a procura em arquivos, se permitíssemos que o usuário estabelecesse somente uma _thread_ (a mesma que procura recursivamente pelos diretórios), haveria uma espera indefinida.

## Funcionamento

O nosso Grep paralelo funciona da seguinte forma:

-   Após tratar os argumentos de entrada, ajustar variáveis globais e
    criar variáveis da biblioteca *pthreads*, a *thread* inicial começa
    a realizar a procura por arquivos.

-   A função de busca recursiva abre um diretório de caminho
    especificado e lê sequencialmente cada arquivo no diretório (lembrar
    que diretórios em Linux também são arquivos), de modo a identificar
    quais são arquivos e quais são outros diretórios. Se uma entrada for
    um arquivo, criamos uma nova *thread* e ela será responsável por
    procurar o padrão REGEX nele. Se for um diretório, realizamos uma
    chamada recursiva para esse diretório.\
    Esse trecho possui alguns detalhes importantes de implementação:

    -   Os diretórios \".\" e \"..\" são ignorados, visto que
        representam o diretório atual e o anterior, respectivamente, e
        não precisamos da informação (principalmente porque a procura é
        recursiva, então não precisamos voltar diretórios através do
        \"..\")

    -   A leitura de entradas do diretório retorna apenas o nome delas,
        e não o seu caminho. Porém, para que possamos manipular os
        arquivos encontrados, precisamos de seu caminho total ou
        relativo (tanto os diretórios para que possamos abri-los e
        percorrê-los, quanto os arquivos para serem abertos e analisados
        pelas *threads* paralelas). Portanto, ao ler uma entrada de
        diretório, concatenamos o caminho do diretório passado como
        argumento com o nome da entrada atual.

-   Após ser crida, uma *thread* vai identificar as linhas onde há
    ocorrências do padrão passado para o programa. Para isso, primeiro
    conseguimos a informação de quantas linhas o arquivo tem. Após isso,
    lemos o arquivo linha-a-linha e checamos se o padrão está nela. Se
    estiver, salvamos o número dessa linha num vetor. Após isso, são
    impressos os resultados e a *thread*, extinta. É importante notar
    que o programa funciona procurando padrões em uma linha de cada vez,
    assim como o *grep* original (item 1 dos requesitos desse EP
    menciona que ele deve \"testar correspondência com a expressão
    regular fornecida em cada linha dos arquivos \[\...\]\")

-   O funcionamento geral do paralelismo está implementado da seguinte
    forma:

    -   Criamos *threads* que leem os arquivos em paralelo e, como os
        resultados da análise de um arquivo devem ser impressos em
        sequência, há uma seção crítica que só permite que uma *thread*
        imprima de cada vez.

    -   O problema de limitar o número máximo de *threads* e de saber
        quando podemos liberar o espaço de memória das variáveis da
        biblioteca *pthreads* foi resolvido ao possuirmos uma variável
        global que indicam o número atual de *threads* e ajustar seções
        críticas de forma a permitir a mudança correta dessa variável.
        Ela é responsável por impedir a criação de *threads* adicionais
        quando atingimos o número máximo estipulado e impedir a
        destruição das variáveis *mutex* e condicionais de *pthreads*
        até que não haja mais *threads* de leitura de arquivos. Talvez
        possa ser comparado com um sistema de \"produtor-consumidor\"