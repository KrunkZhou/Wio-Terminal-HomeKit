#include <sfud.h>

#define SFUD_DEMO_TEST_BUFFER_SIZE                     1024
static uint8_t sfud_demo_test_buf[SFUD_DEMO_TEST_BUFFER_SIZE] = {0xff};
static void sfud_demo(uint32_t addr, size_t size, uint8_t *data);

#define SERIAL Serial

void setup()
{
    SERIAL.begin(115200);
    while(!SERIAL) {};
    while(!(sfud_init() == SFUD_SUCCESS));
    /* write test */
    const sfud_flash *flash = sfud_get_device_table() + 0;
    uint32_t addr = 0;
    size_t size = sizeof(sfud_demo_test_buf);
    uint8_t result = sfud_erase_write(flash, addr, size, sfud_demo_test_buf);
    if (result == SFUD_SUCCESS) {
        SERIAL.println("Write the flash data finish");
    } else {
        SERIAL.println("Write the flash data failed");
    }
}
void loop()
{   
}