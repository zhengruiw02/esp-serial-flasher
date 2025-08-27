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
#include <getopt.h>     // included for `getopt_long`
#include <libgen.h>     // included for `basename`
#include <stdlib.h>     // included for `EXIT_SUCCESS|EXIT_FAILURE`

#include <sys/param.h>
#include "linux_port.h"
#include "example_common.h"
#include "read_bin.h"

#define GPIO_BASE_KERNEL    (512)   // for Linux kernel > 6.6, need add 512 in GPIO
#define GPIO_NUM_RST        (1)
#define GPIO_NUM_IO0        (3)

#define TARGET_RST_Pin (GPIO_NUM_RST + GPIO_BASE_KERNEL)
#define TARGET_IO0_Pin (GPIO_NUM_IO0 + GPIO_BASE_KERNEL)

#define DEFAULT_BAUD_RATE 115200
#define HIGHER_BAUD_RATE  115200
#define SERIAL_DEVICE     "/dev/ttyS1"

#define VERSION "1.0"

const loader_linux_config_t config = {
    .device = SERIAL_DEVICE,
    .baudrate = DEFAULT_BAUD_RATE,
    .reset_trigger_pin = TARGET_RST_Pin,
    .gpio0_trigger_pin = TARGET_IO0_Pin,
};

static uint32_t _higher_baud_rate = HIGHER_BAUD_RATE;
static int _have_boot = 0, _have_part = 0, _have_app = 0, _have_ota = 0;
static const char *_p_path_boot = NULL, *_p_path_part = NULL, *_p_path_app = NULL, *_p_path_ota = NULL;

static const char *get_target_string(target_chip_t target)
{
    if (target >= ESP_MAX_CHIP) {
        return "INVALID_TARGET";
    }
    const char *target_to_string_mapping[ESP_MAX_CHIP] = {
        "ESP8266", "ESP32", "ESP32-S2",
        "ESP32-C3", "ESP32-S3", "ESP32-C2",
        "ESP32-C5", "ESP32-H2", "ESP32-C6", "ESP32-P4"
    };
    return target_to_string_mapping[target];
}

// For esp32, esp32s2
#define BOOTLOADER_ADDRESS_V0       0x1000
// For esp8266, esp32s3 and later chips
#define BOOTLOADER_ADDRESS_V1       0x0
// For esp32c5 and esp32p4
#define BOOTLOADER_ADDRESS_V2       0x2000
#define PARTITION_ADDRESS           0x8000
#define OTA_DATA_ADDRESS            0xd000
#define APPLICATION_ADDRESS         0x10000
static int get_target_bootloader_address(target_chip_t target, size_t *bootloader_address)
{
    if (target >= ESP_MAX_CHIP) {
        return -1;
    }
    if (target == ESP32_CHIP || target == ESP32S2_CHIP){
        *bootloader_address = BOOTLOADER_ADDRESS_V0;
    } else if (target == ESP32C5_CHIP || target == ESP32P4_CHIP){
        *bootloader_address = BOOTLOADER_ADDRESS_V2;
    } else {
        *bootloader_address = BOOTLOADER_ADDRESS_V1;
    }
    return 0;
}

static const struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    // ----------
    {"baudrate", required_argument, NULL, 'r'},
    {"reboot", no_argument, NULL, 'R'},
    {"bootloader", required_argument, NULL, 'b'},
    {"partition", required_argument, NULL, 'p'},
    {"application", required_argument, NULL, 'a'},
    {"ota", required_argument, NULL, 'o'},
    // ----------
    {NULL, 0, NULL, 0}};

    // Print help info.
static void print_help(const char *progname) {
    printf("Usage: %s [OPTION]...\n", progname);
    printf("Options:\n");
    printf("  -h, --help              display this help\n");
    printf("  -V, --version           display version information\n");
    // ----------
    printf("  -B, --baudrate=VALUE    use VALUE as the UART baudrate\n");
    printf("  -R, --reboot            reboot target\n");
    printf("  -b, --bootloader=PATH   use PATH as the path of bin\n");
    printf("  -p, --partition=PATH    use PATH as the path of bin\n");
    printf("  -a, --application=PATH  use PATH as the path of bin\n");
    printf("  -o, --ota=PATH          use PATH as the path of bin\n");
}

static void print_args(void)
{
    printf(" baudrate = %d\n", _higher_baud_rate);
    if(_have_boot){
        printf(" bootloader = %s\n", _p_path_boot);}
    if(_have_part){
        printf(" partition = %s\n", _p_path_part);}
    if(_have_app){
        printf(" application = %s\n", _p_path_app);}
    if(_have_ota){
        printf(" application = %s\n", _p_path_ota);}
}

static void print_version() {
    printf("Version %s. Compile time: %s %s\n", VERSION, __DATE__, __TIME__);
}

static void reboot_target(void)
{
    printf("Reboot target...\n");
    loader_port_linux_init(&config);                
    esp_loader_reset_target();
    loader_port_deinit();
}

static void args_handler(int argc, char *argv[])
{
    int optc;
    const char *program_name = basename(argv[0]);
    int lose = 0;

    while ((optc = getopt_long(argc, argv, "hVRB:b:p:a:o:", longopts, NULL)) != -1)
        switch (optc) {
            /* One goal here is having --help and --version exit immediately,
               per GNU coding standards.  */
            case 'h':
                print_help(program_name);
                exit(EXIT_SUCCESS);
                break;
            case 'V':
                print_version();
                exit(EXIT_SUCCESS);
                break;
            case 'R':
                reboot_target();
                exit(EXIT_SUCCESS);
                break;
            case 'B':
                _higher_baud_rate = atoi(optarg);
                break;
            case 'b':
                // bootloader
                _have_boot = 1;
                _p_path_boot = optarg;
                break;
            case 'p':
                // partition table
                _have_part = 1;
                _p_path_part = optarg;
                break;
            case 'a':
                // application
                _have_app = 1;
                _p_path_app = optarg;
                break;
            case 'o':
                // ota
                _have_ota = 1;
                _p_path_ota = optarg;
                break;
            default:
                lose = 1;
                break;
        }

    if (lose || optind < argc) {
        /* Print error message and exit.  */
        if (optind < argc)
            fprintf(stderr, "%s: extra operand: %s\n", program_name,
                    argv[optind]);
        fprintf(stderr, "Try `%s --help' for more information.\n",
                program_name);
        exit(EXIT_FAILURE);
    }

    print_args();
}

int main(int argc, char *argv[])
{

    args_handler(argc, argv);

    // Check if args have any bin location provided
    if((_have_boot || _have_part || _have_app || _have_ota) == 0){
        printf("No bin file input, exit...\n");
        exit(EXIT_SUCCESS);
    }

    // Init GPIO and serial port to target
    loader_port_linux_init(&config);    

    if (connect_to_target(_higher_baud_rate) == ESP_LOADER_SUCCESS) {
        target_chip_t target_chip;
        size_t flash_address;
        target_chip = esp_loader_get_target();        
        printf("Target chip: %s\n", get_target_string(target_chip));

        if(_have_boot){
            printf("Loading bootloader...\n");
            get_target_bootloader_address(target_chip, &flash_address);
            read_bin_and_flash(_p_path_boot, flash_address);
        }
        if(_have_part){
            printf("Loading partition table...\n");
            read_bin_and_flash(_p_path_part, PARTITION_ADDRESS);
        }
        if(_have_app){
            printf("Loading app...\n");
            read_bin_and_flash(_p_path_app, APPLICATION_ADDRESS);
        }
        if(_have_ota){
            printf("Loading ota...\n");
            read_bin_and_flash(_p_path_ota, OTA_DATA_ADDRESS);
        }
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
            usleep(100);
        }
    }

}
