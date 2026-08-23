#include "FreeRTOS.h"
#include "task.h"
#include "shell.h"
#include "console.h"

static Shell shell;
static char shell_buffer[512];

static signed short shell_writer(char *buf, unsigned short len)
{
    console_write(buf, len);
    return len;
}

void sheller_init(void)
{
    shell.read = NULL;
    shell.write = shell_writer;
    shellInit(&shell, shell_buffer, 512);

    xTaskCreate(shellTask, "shell", 1024, &shell, tskIDLE_PRIORITY + 1, NULL);
}
