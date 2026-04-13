# Implementacion de First y Follow

## Descripcion
Actividad Practica: First, Follow, tabla de analisis sintactico

## Video de Youtube
https://youtu.be/Km6jVp3_L6E

## Uso
Compilar el programa con el siguiente comando:

MacOS
```bash
clang++ -std=c++26 parser_table.cpp -o parser_table
```
Linux
```bash
g++ -std=c++26 parser_table.cpp -o parser_table
```

Luego, ejecutar el programa con el siguiente comando:

```bash
./parser_table
```

## Gramaticas:

1. Gramática 1:
E  -> T E'
E' -> + T E' | ε
T  -> F T'
T' -> * F T' | ε
F  -> ( E ) | id

2. Gramática 2:
S -> D S'
S' -> . D S' | ε
D -> 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

- Nota: Escogi esta gramatica porque es una gramatica muy similar a la representacion de numeros int y flotantes, 
lo cual es algo que se encuentra comunmente en los lenguajes de programacion, ademas de que es una gramatica sencilla y facil de entender.

3. Gramática 3:
S -> NP VP
NP -> the N | a N
N -> cat | dog
VP -> V NP
V -> chases | sees

- Nota: Escogi esta gramatica porque es una gramatica muy sencilla y facil de entender, ademas de que es una gramatica comunmente utilizada en ejemplos de gramatica y analisis sintactico.

4. Gramática 4:
S -> A a | b A c | d c | b d
A -> d

- Nota: Escogi esta gramatica porque es una gramatica que no es LL(1), para probar el programa y ver como maneja los casos en los que la gramatica no es LL(1).