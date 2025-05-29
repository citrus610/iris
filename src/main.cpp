#include "chess/chess.h"
#include "test/test.h"

int main()
{
    chess::init();

    test::quiet::test();

    return 0;
};