#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "workqueue.h"
#include "app.h"
#include "ui.h"
#include "page.h"
#include "wifi.h"
#include "elog.h"

extern void board_lowlevel_init(void);
extern void board_init(void);

static void main_init(void *param)
{
	board_init();

	elog_init();
	elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_ALL);
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME | ELOG_FMT_T_INFO);
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG);
	elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TAG);
	elog_start();

	ui_init();

	welcome_page_display();

	wifi_init();
	wifi_page_display();
	wifi_wait_connect();

	main_page_display();
	app_init();

	vTaskDelete(NULL);
}

int main(void)
{
	board_lowlevel_init();
	workqueue_init();

	xTaskCreate(main_init, "init", 1024, NULL, 9, NULL);

	vTaskStartScheduler();

	while (1)
	{
		; // code should not run here
	}

}


