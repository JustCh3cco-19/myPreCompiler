int valid_variable;
int _another_valid_one;
int var_with_number_1;

// Identificatori non validi:
int 1_invalid_start;
float my-variable; // Contiene trattino
char bad!char; // Contiene punto esclamativo
double another+bad; // Contiene segno piu'

int main() {
    int LocalVar; // Valido
    int Yet_Another_One_2; // Valido
    // Non validi locali:
    int third bad; // Spazio nel nome
    int 4th_one; // Inizia con numero

    return 0;
}