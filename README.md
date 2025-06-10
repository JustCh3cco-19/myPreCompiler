# myPreCompiler

### Come compilare

gcc src/main.c src/preprocessor.c src/utils.c -Iinclude -o myPreCompiler.out

### Eseguire i test (esempio)

cd test

../myPreCompiler.out -i test2.c -v -o test2_processed.c