/*

    Read and Write the eeprom of an fx2 chip with the vend_ax.hex firmware


    1) Mount the usbfs
    mount -t usbfs usbfs /proc/bus/usb

    2) Download the fw
    fxload -I vend_ax.hex  -D /proc/bus/usb/002/011  -v -t fx2

    3) Run fx2eeprom
    3a)Read example
    ./fx2eeprom r 0x123 0x234 45 > eeprom.raw
    3b)Write example
    cat eeprom.raw | ./fxeeprom w 0x123 0x234 45 > eeprom.raw

    Copyright Ricardo Ribalda - 2012 - ricardo.ribalda@gmail.com
    Copyright Robert Sinclair - 2025 - 0wwafa@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/


#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>

#define TRANS_TYPE_READ 0xc0
#define TRANS_TYPE_WRITE 0x40
#define EEPROM 0xa2
#define TIMEOUT 4000
#define LOCATION (1<<4)
#define CHUNK_SIZE 4096

enum {READ,WRITE};

void use(char *prog){

    fprintf(stdout,"%s w/r VID PID size\n",prog);

    return;
}

int main(int argc, char *argv[]){
    struct libusb_device_handle *dev;
    int ret;
    unsigned char *buffer;
    int pid, vid,length;
    int mode;

    if (argc!=5){
        use(argv[0]);
        return -1;
    }

    if ((argv[1][0]=='W')||(argv[1][0]=='w'))
        mode=WRITE;
    else mode=READ;

    vid=strtoul(argv[2],NULL,0);
    pid=strtoul(argv[3],NULL,0);
    length=strtoul(argv[4],NULL,0);

    buffer=malloc(CHUNK_SIZE);
    if (!buffer){
        fprintf(stderr,"Unable to alloc memory\n");
        perror("malloc");
        return -1;
    }

    ret=libusb_init(NULL);
    if (ret<0){
        fprintf(stderr,"Unable to init libusb\n");
        perror("libusb_init");
        return -1;
    }

    dev=libusb_open_device_with_vid_pid(NULL,vid,pid);
    if (!dev){
        fprintf(stderr,"Unable to find device\n");
        perror("libusb_open");
        return -1;
    }

    if (libusb_kernel_driver_active(dev, 0) && libusb_detach_kernel_driver(dev, 0)){
            fprintf(stderr,"Unable to detach kernel driver\n");
            perror("libusb_detach_kernel_driver");
            return -1;
    }

    ret=libusb_claim_interface(dev,0);
    if (ret<0){
        fprintf(stderr,"Unable to claim interface\n");
        perror("libusb_claim_interface");
        return -1;
    }

    if (mode==READ){
        int total_read = 0;
        while (total_read < length) {
            int chunk = (length - total_read) > CHUNK_SIZE ? CHUNK_SIZE : (length - total_read);
            ret = libusb_control_transfer(dev, TRANS_TYPE_READ, EEPROM, total_read, LOCATION, buffer, chunk, TIMEOUT);
            if (ret != chunk) {
                fprintf(stderr,"Read error at %d: expected %d, got %d\n", total_read, chunk, ret);
                return -1;
            }
            fwrite(buffer, 1, ret, stdout);
            total_read += ret;
        }
        fprintf(stderr,"Read %d bytes\n", total_read);
    }
    else {
        int total_written = 0;
        while (total_written < length) {
            int chunk = (length - total_written) > CHUNK_SIZE ? CHUNK_SIZE : (length - total_written);
            ret = fread(buffer, 1, chunk, stdin);
            if (ret != chunk) {
                fprintf(stderr,"Wrong size from stdin, expected %d read %d\n", chunk, ret);
                return -1;
            }
            ret = libusb_control_transfer(dev, TRANS_TYPE_WRITE, EEPROM, total_written, LOCATION, buffer, chunk, TIMEOUT);
            if (ret != chunk) {
                fprintf(stderr,"Write error at %d: expected %d, got %d\n", total_written, chunk, ret);
                return -1;
            }
            total_written += chunk;
        }
        fprintf(stderr,"Written %d bytes\n", total_written);
    }

    libusb_close(dev);
    libusb_exit(NULL);

    return 0;
}
