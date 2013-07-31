/* Name: powerSwitch.c
 * Project: PowerSwitch based on AVR USB driver
 * Author: Christian Starkjohann
 * Creation Date: 2005-01-16
 * Tabsize: 4
 * Copyright: (c) 2005 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: Proprietary, free under certain conditions. See Documentation.
 * This Revision: $Id: uDMX.c,v 1.1.1.1 2006/02/15 17:55:06 cvs Exp $
 */

/*
General Description:
This program controls the PowerSwitch USB device from the command line.
It must be linked with libusb, a library for accessing the USB bus from
Linux, FreeBSD, Mac OS X and other Unix operating systems. Libusb can be
obtained from http://libusb.sourceforge.net/.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>    /* this is libusb, see http://libusb.sourceforge.net/ */

#define USBDEV_SHARED_VENDOR    	0x16C0  /* VOTI */
#define USBDEV_SHARED_PRODUCT   	0x05DC  /* Obdev's free shared PID for Vendor-Type devices*/
#define USBDEV_SHARED_PRODUCT_HID   0x05DF  /* Obdev's free shared PID for HID devices*/
#define USBDEV_SHARED_PRODUCT_MIDI   0x05E4  /* Obdev's free shared PID for MIDI devices*/
/* Use obdev's generic shared VID/PID pair and follow the rules outlined
 * in firmware/usbdrv/USBID-License.txt.
 */

#define GNUSB_CMD_START_BOOTLOADER 	0xf8


static int  usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
char    buffer[256];
int     rval, i;

    if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0)
        return rval;
    if(buffer[1] != USB_DT_STRING)
        return 0;
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];
    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for(i=1;i<rval;i++){
        if(i > buflen)  /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i-1;
}



char isOurVIDandPID(struct usb_device const* dev) {
	return dev->descriptor.idVendor == USBDEV_SHARED_VENDOR &&
		(dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT ||
		 dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT_HID ||
		 dev->descriptor.idProduct == USBDEV_SHARED_PRODUCT_MIDI);
}


int main(int argc, char **argv)
{
    char    string[256];
	struct usb_bus      *bus;
	struct usb_device   *dev;
	int nBytes;
	int counter = 0;
	int doStartBootloader = 0;
	usb_dev_handle      *handle = 0;

    usb_init();
  
    usb_find_busses();
    usb_find_devices();
  
  	for(bus=usb_busses; bus; bus=bus->next){
        for(dev=bus->devices; dev; dev=dev->next){
            if(isOurVIDandPID(dev)){
                int     len;
                handle = usb_open(dev); /* we need to open the device in order to query strings */
                if(!handle){
                    fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
                    continue;
                }
                /* now find out whether the device actually is obdev's Remote Sensor: */
                len = usbGetStringAscii(handle, dev->descriptor.iManufacturer, 0x0409, string, sizeof(string));
                if(len < 0){
                    fprintf(stderr, "warning: cannot query manufacturer for device: %s\n", usb_strerror());
                }
                /* fprintf(stderr, "seen device from vendor ->%s<-\n", string); */
                if(strcmp(string, "www.anyma.ch") == 0) {
                
					len = usbGetStringAscii(handle, dev->descriptor.iProduct, 0x0409, string, sizeof(string));
					if(len < 0){
						fprintf(stderr, "warning: cannot query product for device: %s\n", usb_strerror());
                	} else {
                		if (argc > 1) {
							 if(strcmp(string, argv[1]) == 0) {doStartBootloader = 1;} 
							 else {doStartBootloader = 0;} 
						} else doStartBootloader = 1;
						
						if (doStartBootloader) {
                			 nBytes = 	usb_control_msg(handle, USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
									GNUSB_CMD_START_BOOTLOADER, 0, 0, NULL, 0, 5000);
							printf("--> Started bootloader on device \"%s\" (%d)\n", string, nBytes);
							counter++;
						}
					}
                }
                usb_close(handle);
                handle = NULL;
            }
        }
    }
    usb_close(handle);
    if (!counter) {
    	if (argc > 1) {
	    	printf ("\nNo devices found with name \"%s\".\n\n",argv[1]);
    	} else {
	    	printf ("\nNo devices found with manufacturer \"anyma.ch\".\n\n");
	    }
    }
    return 0;
}

