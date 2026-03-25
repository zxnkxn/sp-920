#include <stdio.h>
#include "board.h"

#include "ram_test.h"
#include "read_mac.h"
#include "rs485.h"
#include "clock_test.h"

static void print_menu(void)
{
    printf("\r\n============================\r\n");
    printf("Select task:\r\n");
    printf("1 - RAM test\r\n");
    printf("2 - Read MAC\r\n");
    printf("3 - RS485 test\r\n");
    printf("4 - Clock test\r\n");
    printf("a - Run all tasks\r\n");
    printf("============================\r\n");
    printf("> ");
}

int main(void)
{
    board_init();

    print_menu();

    while (1)
    {
        int ch = getchar();

        if (ch == '\r' || ch == '\n') {
            continue;
        }

        switch (ch)
        {
            case '1':
                ram_test_run();
                break;

            case '2':
                read_mac_run();
                break;

            case '3':
                rs485_run();
                break;

            case '4':
                clock_test_run();
                break;

            case 'a':
            case 'A':
                printf("\r\n--- RUN ALL TASKS ---\r\n");

                ram_test_run();
                read_mac_run();
                rs485_run();
                clock_test_run();

                printf("\r\nALL TASKS COMPLETED\r\n");
                break;

            default:
                printf("\r\nUnknown key '%c'\r\n", (char)ch);
                break;
        }

        print_menu();
    }
}
