#include "test/test_include_header.h"

int main() {
    // La variabile definita nell'header dovrebbe essere disponibile qui
    int result = header_value;
    return 0;
}