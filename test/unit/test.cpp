#define DOCTEST_CONFIG_IMPLEMENT
#include "../../lib/doctest.h"
#include "../../engine/config/config.h"
#include "../../engine/config/social.h"
#include "../../engine/definitions/definitions.h"
#include "../../engine/objective.h"

int main(int argc, char** argv) {
    doctest::Context context;
    context.setOption("abort-after", 5);              // stop test execution after 5 failed assertions

    context.applyCommandLine(argc, argv);

    // overrides
    context.setOption("no-breaks", false);             // don't break in the debugger when assertions fail

    // Config must be loaded before definitions.
    Config::load();
    Config::Social::load();
    definitions::load();
    ObjectiveType::load();

    int res = context.run(); // run

    if(context.shouldExit()) // important - query flags (and --exit) rely on the user doing this
        return res;          // propagate the result of the tests

    int client_stuff_return_code = 0;
    // your program - if the testing framework is integrated in your production code

    return res + client_stuff_return_code; // the result from doctest is propagated here as well
}
