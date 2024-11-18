#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <libusb-1.0/libusb.h>

#define VENDOR_ID      0x1209
#define PRODUCT_ID     0xFABD

#define ACM_CTRL_DTR   0x01
#define ACM_CTRL_RTS   0x02

// static int ep_in_addr  = 0x83;
static int ep_out_addr = 0x04;

static struct libusb_device_handle *devh = NULL;

void write_char(unsigned char c)
{
    int actual_length;
    int result = libusb_bulk_transfer(devh, ep_out_addr, &c, 1,
                             &actual_length, 0);
    if (result < 0) {
        fprintf(stderr, "Error while sending char: %s\n", libusb_strerror(result));
    }
    // fprintf(stderr, "Sent: %i\n", c);
}

// int read_chars(unsigned char * data, int size)
// {
//     /* To receive characters from the device initiate a bulk_transfer to the
//      * Endpoint with address ep_in_addr.
//      */
//     int actual_length;
//     int rc = libusb_bulk_transfer(devh, ep_in_addr, data, size, &actual_length,
//                                   1000);
//     if (rc == LIBUSB_ERROR_TIMEOUT) {
//         printf("timeout (%d)\n", actual_length);
//         return -1;
//     } else if (rc < 0) {
//         fprintf(stderr, "Error while waiting for char\n");
//         return -1;
//     }

//     return actual_length;
// }

void setButton(uint8_t index, uint8_t state, uint8_t color_index) {
    write_char(0xCC);
    write_char(0xBF);
    write_char(index);
    write_char(state);
    write_char(color_index);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
}

void setBacklight(uint8_t state, uint8_t color_index) {
    write_char(0xCC);
    write_char(0xB0);
    write_char(state);
    write_char(color_index);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
}

void setEffect(uint8_t state, uint8_t effect_index, uint8_t color_index, uint8_t speed) {
    write_char(0xCC);
    write_char(0xF0);
    write_char(state);
    write_char(effect_index);
    write_char(color_index);
    write_char(speed);
    write_char(0x00);
    write_char(0x00);
}

void reset() {
    write_char(0xCC);
    write_char(0x99);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
    write_char(0x00);
}

char* getArg(uint8_t index, int argc, char **argv){
    if (index >= argc)
        return "";

    return argv[index];
}

uint8_t isNumeric(char* input) {
    if (strcmp(input, "0") == 0)
        return 1;
    if (atoi(input) > 0)
        return 1;

    return 0;
}

int main(int argc, char **argv)
{
    int rc;

    /* Initialize libusb
     */
    rc = libusb_init(NULL);
    if (rc < 0) {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(rc));
        exit(1);
    }

    /* Set debugging output to max level.
     */
    // libusb_set_debug(NULL, 3);

    devh = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (!devh) {
        fprintf(stderr, "Error finding USB device\n");
        goto out;
    }

    /* As we are dealing with a CDC-ACM device, it's highly probable that
     * Linux already attached the cdc-acm driver to this device.
     * We need to detach the drivers from all the USB interfaces. The CDC-ACM
     * Class defines two interfaces: the Control interface and the
     * Data interface.
     */
    for (int if_num = 0; if_num < 2; if_num++) {
        if (libusb_kernel_driver_active(devh, if_num)) {
            libusb_detach_kernel_driver(devh, if_num);
        }
        rc = libusb_claim_interface(devh, if_num);
        if (rc < 0) {
            fprintf(stderr, "Error claiming interface: %s\n",
                    libusb_error_name(rc));
            goto out;
        }
    }

    /* Start configuring the device:
     * - set line state
     */
    rc = libusb_control_transfer(devh, 0x21, 0x22, ACM_CTRL_DTR | ACM_CTRL_RTS,
                                0, NULL, 0, 0);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
        goto out;
    }

    /* - set line encoding: here 9600 8N1
     * 9600 = 0x2580 ~> 0x80, 0x25 in little endian
     */
    unsigned char encoding[] = { 0x80, 0x25, 0x00, 0x00, 0x00, 0x00, 0x08 };
    rc = libusb_control_transfer(devh, 0x21, 0x20, 0, 0, encoding,
                                sizeof(encoding), 0);
    if (rc < 0) {
        fprintf(stderr, "Error during control transfer: %s\n",
                libusb_error_name(rc));
        goto out;
    }


    uint8_t state;
    uint8_t index;
    uint8_t color;
    uint8_t speed;
    char* nextArg = "";

    for (int i=0; i<argc; i++) {
        switch (argv[i][0]) {
            case 'r':
                reset();
                break;

            case 'e':
                state = 2;
                index = 0;
                color = 0;
                speed = 0;

                nextArg = getArg(i + 1, argc, argv);
                if (isNumeric(nextArg)) {
                    state = atoi(nextArg);
                    i++;

                    nextArg = getArg(i + 1, argc, argv);
                    if (isNumeric(nextArg)) {
                        index = atoi(nextArg);
                        i++;

                        nextArg = getArg(i + 1, argc, argv);
                        if (isNumeric(nextArg)) {
                            color = atoi(nextArg);
                            i++;

                            nextArg = getArg(i + 1, argc, argv);
                            if (isNumeric(nextArg)) {
                                speed = atoi(nextArg);
                                i++;
                            }
                        }
                    }
                }

                setEffect(state, index, color, speed);
                break;

            case 'b':
                index = 0;
                state = 2;
                color = 0;

                nextArg = getArg(i + 1, argc, argv);
                if (isNumeric(nextArg)) {
                    index = atoi(nextArg);
                    i++;

                    nextArg = getArg(i + 1, argc, argv);
                    if (isNumeric(nextArg)) {
                        state = atoi(nextArg);
                        i++;

                        nextArg = getArg(i + 1, argc, argv);
                        if (isNumeric(nextArg)) {
                            color = atoi(nextArg);
                            i++;
                        }
                    }
                }

                setButton(index, state, color);
                break;

            case 's':
                nextArg = getArg(i + 1, argc, argv);
                // fprintf(stdout, "sleep \"%s\"\n", nextArg);
                if (atoi(nextArg) > 0) {
                    sleep(atoi(nextArg));
                    i++;
                }
                break;
        }
    }
    libusb_release_interface(devh, 0);

out:
    if (devh)
            libusb_close(devh);
    libusb_exit(NULL);
    return rc;
}
