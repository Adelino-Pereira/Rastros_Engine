# Rastros_Engine

## Pré-requisitos

Antes de compilar, para usar o automatismo CMake é necessário ter instalado:

- **CMake** (>= 3.14)
- **Compilador C++** com suporte para **C++17**
  - Em Linux: `g++` ou `clang`
  - Em macOS: `clang` (incluído no Xcode Command Line Tools)
  - Em Windows: Visual Studio (MSVC) ou MinGW
- Acesso à internet (necessário para o CMake fazer o download do GoogleTest, caso se deseje compilar os testes)

## Compilação e execução

Da raiz do projeto:

```bash
mkdir build
cmake -S src -B build -DCMAKE_BUILD_TYPE=Release
```

(Correr com `-DCMAKE_BUILD_TYPE=Release` para ativar otimizações no perfil do build, sem optimizações o código corre muito mais lento)

Depois da criação dos ficheiros CMake:

```bash
cd build
make Rastros
```

Executar:

```bash
./Rastros
```

## Testes Unitários (opcional)

Para compilar e correr os testes:

```bash
cd build
make RastrosTests
ctest
```

ou configurar e compilar tudo de uma só vez:

```
cmake -S src -B build && cmake --build build
```

Para compilar alterando a configuração do minimax para benchmarking (não incluído em produção):

```
#minimax sem TT
cmake -S src -B build -DRASTROS_MINIMAX_NO_TT=ON && cmake --build build

#minimax sem cortes
cmake -S src -B build -DRASTROS_MINIMAX_NO_PRUNE=ON && cmake --build build
```

NOTA: para voltar à configuração normal: ou remover a pasta build existente ou reconfigurar com opção OFF no último benchmark configurado, poe exemplo:

```
cmake -S src -B build -DRASTROS_MINIMAX_NO_PRUNE=OFF && cmake --build build
```

Para voltar a executar com o minimax padrão.

## Interface na linha de comandos (CLI)

### Modo de jogo

Ao correr a aplicação é apresentado um menu no terminal do sistema:

```
1 - modo de jogo
2 - modo de teste 1
3 - modo de teste 2
```

Escolhendo a opção 1 é possível escolher entre 4 tipos diferentes de modo de jogo:

```
1: Humano vs Humano
2: Humano (Jogador 1) vs IA (Jogador 2)
3: Humano (Jogador 2) vs IA (Jogador 1)
4: IA vs IA
```

Escolhido o modo de jogo é necesário configurar o tabuleiro de jogo:

```
1 - Novo tabuleiro
2 - Carregar de TXT
```

Em '1' é carregado um tabuleiro no seu estado inicial, ínicio de um jogo normal, configurando-se o tamanho do tabuleiro antes do ínicio do jogo:

```
Escolhe o número de linhas (mínimo 5): 7
Escolhe o número de colunas (mínimo 5): 7

Tabuleiro:
0|1 1 1 1 1 1 1
1|1 1 1 1 1 1 1
2|1 1 1 1 M 1 1
3|1 1 1 1 1 1 1
4|1 1 1 1 1 1 1
5|1 1 1 1 1 1 1
6|1 1 1 1 1 1 1
  -------------
  0 1 2 3 4 5 6
```

Em '2' é possível carregar um tabuleiro num estado específico através de um ficheiro TXT. O ficheiro TXT deverá conter uma jogada por linha com as coordenas 'linha,coluna', por exemplo:

```
3,4
2,5
3,6
4,5
3,5
2,6
1,5
0,6
```

O ficheiro pode ser fabricado manualmente ou pode-se utilizar o script python [gameRegParser.py](PythonTools/gameRegParser.py) para transformar um registo de jogo, descarregado no final de uma partida na aplicação web, num ficheiro CSV compatível. Do mesmo modo pode-se usar [debugLogParser.py](PythonTools/debugLogParser.py) para converter um ou mais jogos de um ficheiro emitido como log em nível 1 de debug.([instruções de utilização aqui](PythonTools/README.md))

Do mesmo modo pode-se utilizar o script test_log_2_cli_txt.py para converter um log de jogo(s) de teste, obtido em nível de debug 1, para o formato compatível com a CLI.

Deve ser, então, fornecido o caminho para o ficheiro TXT com o jogo a carregar e indicar o número da jogada até onde o jogo deve ser carregado:

```
Caminho para CSV: </caminho/para/jogo.csv>
Número da jogada onde o jogo deve começar: 4
```

de seguida são listadas as jogadas até à jogada escolhidae e o tabuleiro é carregado indicando que jogador deverá jogar.:

```
(3,4)
(2,5)
(3,6)
(4,5)
Jogo preparado na jogada 4.
joga jogador 1.

Tabuleiro:
0|1 1 1 1 1 1 1
1|1 1 1 1 1 1 1
2|1 1 1 1 · · 1
3|1 1 1 1 · 1 ·
4|1 1 1 1 1 M 1
5|1 1 1 1 1 1 1
6|1 1 1 1 1 1 1
  -------------
  0 1 2 3 4 5 6

🧑 Turno do Jogador (Humano)...
0: (3, 5)
1: (5, 5)
2: (4, 4)
3: (4, 6)
4: (5, 4)
5: (5, 6)
Escolhe uma jogada:
```

### Modo de teste

No modo de teste pode-se realizar torneios entre heurísticas, previamente configuradas em Heuristics1.cpp e Heuristics2.cpp, jogando IA vs IA.

O Mode de teste 2 foi criado para realizar apenas torneios de 8 jogos (um por cada movimento inicial possível) para quando a ordem dos sucessores é determínistica para ambas as IA's e todos os jogos começados de determinada maneira serão sempre iguais.

No modo de teste 3 são realizados, por defeito, torneios de 100 jogos com possibilidade de determinar a politica de ordenação de sucessores. Não sendo ambas determínisticas consegue-se maior variabilidade de jogos diferentes.

Este modo interativo de correr testes (via prompts) apenas funciona com as configurações por defeito.

Uma opção mais completa e prática consiste em correr os testes introduzindo diretamente os argumentos na linha de commandos, sem ter de passar pelo modo interativo. É sobre este modo de realizar testes que se referem as instruções seguintes.

Na realização de testes é possível optar por vários níveis de debug:

- **Nivel 0** - sem informação de debug apenas número e configurações de jogo, o vencedor e dados estatísticos.
- **Nivel 1** – Imprime as jogadas e a pontuação heurística obtida, no final de cada jogo contém informação do número de jogadas, do tempo, e dados estatísticos de cortes e da Tabela de estados.
- **Nivel 2** – Os mesmos do nível anterior mais a pontuação para cada sucessor da jogada raiz (info exclusiva do deste nível, não passa para os niveis posteriores).
- **Nivel 3** – Árvore de procura completa mais toda a informação anterior menos a do nível 2.
- **Nivel 4** - Toda a a informação anterior mais indicação de cortes.
- **Nivel 5** – Toda a informação anterior mais chaves de estados e entradas e rejeições na TT.

Exemplos de saídas de debug podem ser consultados na pasta [exemplos_debug](exemplos_debug/) neste repositório.

Para correr testes diretamente com argumentos na linha de comandos, usas-se o seguinte modelo:

```
./Rastros <MODO> <DEBUG> [ARGS_POSICIONAIS] [FLAGS]
```

Como já se viu, o modo de teste pode ser "2" ou "3" e o nível de debug entre 0 e 5.

Os argumentos como flags disponíveis são.

```
-d/--depth          //Profundidade de procura (ambos os jogadores) - default 9
-d1/--depth1        //Profundidade de procura (jogador 1) - default 9
-d2/--depth2        //Profundidade de procura (jogador 2) - default 9
-md/--maxdepth      //Profundidade máxima de procura (ambos os jogadores) - default 15
-md1/--maxdepth1    //Profundidade máxima de procura (jogador 1) - default 15
-md2/--maxdepth2    //Profundidade máxima de procura (jogador 2) - default 15
-g/--games          //Número de Jogos de um torneio - default 100
-r/--row            //Número de linhas do tabuleiro - default 7
-c/--col            //Número de colunas do tabuleiro - default 7
-h1/heur1           //Heurística usada por MAX/P1 - default G
-h2/heur2           //Heurística usada por MIN/P2 - default G
-h/--Heur           //Heurístca usada por ambos os jogadores - default G
```

Exemplos de execução de um torneio de 50 jogos com profundidade mínima de 5 e máxima de 9 com ambas as IAs com a combinação heurística C para ambas as IAs num tabuleiro 8x8:

```
./Rastros 3 -d 5 -md 9 -g 50 -h C -r 8 -c 8
#ou
./Rastros 3 --depth 5 --maxdept 9 --games 50 --heur C --row 8 --col 8
#ou
./Rastros 3 --depth=5 --maxdept=9 --games=50 --heur=C --row=8 --col=8
```

Pode-se ainda definir via argumentos posicionais a política de ordenação de sucessores para ambas as IAs.

```
<PolicyMAX> <PolicyMIN> <sigmaMAX> <sigmaMIN> <shuffleTiesOnly>
```

As políticas de ordenação são dadas por:

- `D`-> _Deterministic_ - Ordenamento determinista pela a heurística.
- `S`-> _ShuffleAll_ - Aleatoriedade no ordenamento.
- `N`-> _NoysyJitter_ - Aplicação de algum ruído na ordenação heurística segundo uma função sigma.

É ainda possível passar `1` como argumento que activa `shuffleTiesOnly` , i.e. o baralhamento dos sucessores empatados (a omissão funciona como `0`, não ativando o baralhamento). Funciona simultaneamente para MAX e MIN.

#### Exemplos de execução

Torneio de 50 jogos com profundidade fixa de 7 e com ordenação determinista para MAX [```D```] e baralhamento de sucessores para MIN [```S```]

```
./Rastros 3 1 D S -d 7 -g 50
```

Torneio de 200 jogos com profundidade mínima de 3, profundidade maxima de 7 , com ordenação com ruído para MAX e deterministica para MIN, com ambas as IAs a usar a heuristica F

```
./Rastros 3 1 N D 0.75 0 -d 3 -md 7 -g 50 -h F
```

Torneio de 100 jogos com profundidade mínima de 3, profundidade maxima de 7 e com ordenação deterministica para MAX e MIN, e baralhamento da ordenação dos sucessores empatados, em tabuleiro 5x7

```
./Rastros 3 1 D D 0 0 1 -d 3 -md 7 -g 50 -r 5
```
