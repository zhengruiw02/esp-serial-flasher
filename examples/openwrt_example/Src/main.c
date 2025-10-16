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

typedef struct {
    size_t address;
    const char* filepath;
} flash_item_t;

static flash_item_t* flash_items = NULL;
static int flash_item_count = 0;

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

static const struct option longopts[] = {
    {"help", no_argument, NULL, 'h'},
    {"version", no_argument, NULL, 'V'},
    // ----------
    {"baudrate", required_argument, NULL, 'b'},
    {"reboot", no_argument, NULL, 'R'},
    // ----------
    {NULL, 0, NULL, 0}};

    // Print help info.
static void print_help(const char *progname) {
    printf("Usage: %s [OPTION]...\n", progname);
    printf("Options:\n");
    printf("  -h, --help              display this help\n");
    printf("  -V, --version           display version information\n");
    // ----------
    printf("  -b, --baudrate=VALUE    use VALUE as the UART baudrate\n");
    printf("  -R, --reboot            reboot target\n");
}

static void print_args(void)
{
    printf(" baudrate = %d\n", _higher_baud_rate);
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

    while ((optc = getopt_long(argc, argv, "hVRb:", longopts, NULL)) != -1)
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
            case 'b':
                _higher_baud_rate = atoi(optarg);
                break;
            default:
                lose = 1;
                break;
        }

    print_args();

    // try to use extra args to parse for flasher address and binary file
    int extra_opts_cnt = argc - optind;
    // printf("extra_opts_cnt = %d\n",extra_opts_cnt);
    if (lose || extra_opts_cnt % 2) {
        /* Print error message and exit.  */
        // printf("optind = %d, argc = %d \n", optind, argc);
        if (optind < argc){
            for(int i = optind; i < argc; i++){
                fprintf(stderr, "%s: extra operand: %s\n", program_name,
                    argv[i]);
            }
        }
        fprintf(stderr, "Try `%s --help' for more information.\n",
                program_name);
        exit(EXIT_FAILURE);
    }else{
        // Process address-file pairs from command line
        flash_item_count = extra_opts_cnt / 2;
        flash_items = malloc(sizeof(flash_item_t) * flash_item_count);
        
        // Check memory allocation result
        if (flash_items == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
        
        // printf("optind = %d, argc = %d \n", optind, argc);
        for(int i = optind; i < argc; i += 2) {
            int bin_index = (i - optind) / 2;
            // Validate address format using strtoul for hexadecimal parsing
            char* endptr;
            flash_items[bin_index].address = strtoul(argv[i], &endptr, 16);
            
            // Check if address parsing was successful
            if (*endptr != '\0') {
                fprintf(stderr, "Invalid address format: %s\n", argv[i]);
                free(flash_items);
                exit(EXIT_FAILURE);
            }
            
            // Store the file path
            flash_items[bin_index].filepath = argv[i+1];
            
            // Print parsed information for debugging
            printf("bin_index = %d, bin_address = 0x%08X, path_bin = %s \n", 
                bin_index, (unsigned int)flash_items[bin_index].address, flash_items[bin_index].filepath);
        }
        // exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[])
{
    args_handler(argc, argv);

    // Check if args have any bin location provided
    if(flash_item_count == 0){
        printf("No bin file input, exit...\n");
        exit(EXIT_SUCCESS);
    }

    // Init GPIO and serial port to target
    loader_port_linux_init(&config);    

    if (connect_to_target(_higher_baud_rate) == ESP_LOADER_SUCCESS) {
        target_chip_t target_chip;
        target_chip = esp_loader_get_target();        
        printf("Target chip: %s\n", get_target_string(target_chip));
        
        // Flash additional address-file pairs
        for (int i = 0; i < flash_item_count; i++) {
            printf("Loading custom binary to address 0x%08X...\n", (unsigned int)flash_items[i].address);
            read_bin_and_flash(flash_items[i].filepath, flash_items[i].address);
        }
            
        printf("Done!\n");
        esp_loader_reset_target();
        loader_port_deinit();

        // Free memory allocated for flash items
        if (flash_items) {
            free(flash_items);
        }

        exit(EXIT_SUCCESS); // exit program when done. Below code is to open UART monitor

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
    
    // Free memory even if connection to target failed
    if (flash_items) {
        free(flash_items);
    }
}
