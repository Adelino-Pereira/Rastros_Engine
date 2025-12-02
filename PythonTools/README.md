## gameRegParser.py


### Script que converte o ficheiro csv descarregado no final de uma partida na aplicação web para diferentes formatos.

-  Para carregar na aplicação na CLI no modo de jogo (csv com uma jogada por linha no formato adequado (linha,coluna))

Formato por defeito (csv) ->  gravado no diretório do input;
```
python3 csv_to_moves.py human_first_7x7_d-5_2025-09-29_17h07m34s.csv
```
Escolher o diretório de saída;
```
python3 csv_to_moves.py human_first_7x7_foo.csv -o ./exports/
```

- Para intoduzir na base de dados json da aplicação web e utilizar no modo problemas -> ficheiro json (lista de listas-[[...,...],[...,...]],...):
```
python3 csv_to_moves.py human_first_7x7_foo.csv -o ./exports/moves.json --format json
```

- Formato útil para usar em python

Formato lista de tuplos[(...,...),(...,...),...]:
```
python3 csv_to_moves.py human_first_7x7_foo.csv --format list
```
___
## debugLogParser.py

### Script que converte jogos de ficheiros logs em nível de debug 1 emitidos no modo de testes para diferentes formatos.

Pode-se converter os jogos todos de um torneio de uma só vez (--all) ou selecionar o número do jogo a converter (--game N)

Para utilizar no modo de jogo na CLI
```
python3 parse_rastros_log.py /path/to/Ad-An_7.txt --game 44
```
Para utilizar no modo de jogo na CLI -> formato json
```
python3 parse_rastros_log.py /path/to/Ad-An_7.txt --game 44 --format json -o /path/to/Ad-An_7_g44_7x7.json
```

um jogo, formato lista estilo Python, saída no mesmo dirctório do input
```
python3 parse_rastros_log.py /path/to/Ad-An_7.txt --game 44 --format list
```
converter todos os jogos da entrada
```
python3 parse_rastros_log.py /path/to/Ad-An_7.txt --all -o ./exports/
```


