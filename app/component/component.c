
void logger_init(void);
void sheller_init(void);

void component_init(void)
{
    logger_init();
    sheller_init();
}
