#include <OpenDoor.h>
#include "Program.h"
#if _MSC_VER
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
#else
int main(int argc, char** argv)
{
#endif

    Program p;
    int retval = 0;
#if _MSC_VER
    od_parse_cmd_line(lpszCmdLine);
#else
    od_parse_cmd_line(argc, argv);
#endif
    od_init();

    retval = p.run();
    od_exit(retval, FALSE);

    return retval;
}