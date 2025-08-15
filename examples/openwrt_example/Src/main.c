/* Flash multiple partitions example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/param.h>
#include "linux_port.h"
#include "esp_loader.h"
#include "example_common.h"

#define GPIO_BASE_KERNEL    (512)   // for Linux kernel > 6.6, need add 512 in GPIO
#define GPIO_NUM_RST        (1)
#define GPIO_NUM_IO0        (3)

#define TARGET_RST_Pin (GPIO_NUM_RST + GPIO_BASE_KERNEL)
#define TARGET_IO0_Pin (GPIO_NUM_IO0 + GPIO_BASE_KERNEL)

#define DEFAULT_BAUD_RATE 115200
#define HIGHER_BAUD_RATE  115200
#define SERIAL_DEVICE     "/dev/ttyS1"



int main(void)
{
    example_binaries_t bin;

    const loader_linux_config_t config = {
        .device = SERIAL_DEVICE,
        .baudrate = DEFAULT_BAUD_RATE,
        .reset_trigger_pin = TARGET_RST_Pin,
        .gpio0_trigger_pin = TARGET_IO0_Pin,
    };

    loader_port_linux_init(&config);

    if (connect_to_target(HIGHER_BAUD_RATE) == ESP_LOADER_SUCCESS) {

        get_example_binaries(esp_loader_get_target(), &bin);

        printf("Loading bootloader...\n");
        flash_binary(bin.boot.data, bin.boot.size, bin.boot.addr);
        printf("Loading partition table...\n");
        flash_binary(bin.part.data, bin.part.size, bin.part.addr);
        printf("Loading app...\n");
        flash_binary(bin.app.data,  bin.app.size,  bin.app.addr);
        printf("Done!\n");
        esp_loader_reset_target();
        loader_port_deinit();


        int serial = serialOpen(SERIAL_DEVICE, DEFAULT_BAUD_RATE);
        if (serial < 0) {
            printf("Serial port could not be opened!\n");
        }

        printf("********************************************\n");
        printf("*** Logs below are print from slave .... ***\n");
        printf("********************************************\n");

        // Delay for skipping the boot message of the targets
        usleep(500000);
        while (1) {
            char ch;
            int byte = read(serial, &ch, 1);
            if (byte == 1) {
                printf("%c", ch);
            }
        }
    }

}
