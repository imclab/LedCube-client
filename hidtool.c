/* Name: hidtool.c
 * Project: hid-data example
 * Author: Christian Starkjohann
 * Creation Date: 2008-04-11
 * Tabsize: 4
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v2 (see License.txt), GNU GPL v3 or proprietary (CommercialLicense.txt)
 * This Revision: $Id: hidtool.c 723 2009-03-16 19:04:32Z cs $
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "hiddata.h"
#include "usbconfig.h"  /* for device VID, PID, vendor name and product name */

typedef struct _layer{
    unsigned int row0 : 5;
    unsigned int row1 : 5;
    unsigned int row2 : 5;
    unsigned int row3 : 5;
    unsigned int row4 : 5;
    unsigned int index :7;
}layer;

typedef struct _frame{
    layer layers[5];
}frame;

/* ------------------------------------------------------------------------- */

static char *usbErrorMessage(int errCode)
{
    static char buffer[80];

    switch(errCode){
    case USBOPEN_ERR_ACCESS:      return "Access to device denied";
    case USBOPEN_ERR_NOTFOUND:    return "The specified device was not found";
    case USBOPEN_ERR_IO:          return "Communication error with device";
    default:
        sprintf(buffer, "Unknown USB error %d", errCode);
        return buffer;
    }
    return NULL;    /* not reached */
}

static usbDevice_t  *openDevice(void)
{
    usbDevice_t     *dev = NULL;
    unsigned char   rawVid[2] = {USB_CFG_VENDOR_ID}, rawPid[2] = {USB_CFG_DEVICE_ID};
    char            vendorName[] = {USB_CFG_VENDOR_NAME, 0}, productName[] = {USB_CFG_DEVICE_NAME, 0};
    int             vid = rawVid[0] + 256 * rawVid[1];
    int             pid = rawPid[0] + 256 * rawPid[1];
    int             err;

    if((err = usbhidOpenDevice(&dev, vid, vendorName, pid, productName, 0)) != 0){
        fprintf(stderr, "error finding %s: %s\n", productName, usbErrorMessage(err));
        return NULL;
    }
    return dev;
}

/* ------------------------------------------------------------------------- */

static void hexdump(char *buffer, int len)
{
    int     i;
    FILE    *fp = stdout;

    for(i = 0; i < len; i++){
        if(i != 0){
            if(i % 16 == 0){
                fprintf(fp, "\n");
            }else{
                fprintf(fp, " ");
            }
        }
        fprintf(fp, "0x%02x", buffer[i] & 0xff);
    }
    if(i != 0)
        fprintf(fp, "\n");
}

static int  hexread(char *buffer, char *string, int buflen)
{
    char    *s;
    int     pos = 0;

    while((s = strtok(string, ", ")) != NULL && pos < buflen){
        string = NULL;
        buffer[pos++] = (char)strtol(s, NULL, 0);
    }
    return pos;
}

/* ------------------------------------------------------------------------- */

static void usage(char *myName)
{
    fprintf(stderr, "usage:\n");
    fprintf(stderr, "  %s read\n", myName);
    fprintf(stderr, "  %s write <listofbytes>\n", myName);
}

int msleep(unsigned long milisec)
{
    struct timespec req={0};
    time_t sec=(int)(milisec/1000);
    milisec=milisec-(sec*1000);
    req.tv_sec=sec;
    req.tv_nsec=milisec*1000000L;
    while(nanosleep(&req,&req)==-1)
        continue;
    return 1;
}

int main(int argc, char **argv)
{
    usbDevice_t *dev;
    char        buffer[21];    /* room for dummy report ID */
    int         err;

    if(argc < 2){
        usage(argv[0]);
        exit(1);
    }
    if((dev = openDevice()) == NULL)
        exit(1);
    if(strcasecmp(argv[1], "read") == 0){
        int len = sizeof(buffer);
        if((err = usbhidGetReport(dev, 0, buffer, &len)) != 0){
            fprintf(stderr, "error reading data: %s\n", usbErrorMessage(err));
        }else{
            hexdump(buffer + 1, sizeof(buffer) - 1);
        }
    }else if(strcasecmp(argv[1], "write") == 0){
        int i, pos;
        memset(buffer, 0, sizeof(buffer));
        for(pos = 1, i = 2; i < argc && pos < sizeof(buffer); i++){
            pos += hexread(buffer + pos, argv[i], sizeof(buffer) - pos);
        }
        if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0)   /* add a dummy report ID */
            fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));
    }else if(strcasecmp(argv[1], "loop") == 0){


        frame ledframe;
        int numlayer = 0;

        for(int i=0; i< 50 ;i++){

            memset(buffer,0,sizeof(buffer));
            memset(&ledframe,0,sizeof(ledframe));

            for(int j=0;j<5;j++){
                ledframe.layers[j].index=(1<<j);
            }

            ledframe.layers[numlayer].row0 = 0x1F;
            ledframe.layers[numlayer].row1 = 0x1F;
            ledframe.layers[numlayer].row2 = 0x1F;
            ledframe.layers[numlayer].row3 = 0x1F;
            ledframe.layers[numlayer].row4 = 0x1F;

            buffer[0]=0;
            memcpy(buffer+1,&ledframe,sizeof(ledframe));
            if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0)   // add a dummy report ID
                fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));

            numlayer=(numlayer +1) % 5;
            msleep(50);

        }

        numlayer = 0;

        for(int i=0; i< 50 ;i++){

            memset(buffer,0,sizeof(buffer));
            memset(&ledframe,0,sizeof(ledframe));

            for(int j=0;j<5;j++){
                ledframe.layers[j].index=(1<<j);
                switch (numlayer){
                case 0:
                    ledframe.layers[j].row0=0x1F;
                    break;
                case 1:
                    ledframe.layers[j].row1=0x1F;
                    break;
                case 2:
                    ledframe.layers[j].row2=0x1F;
                    break;
                case 3:
                    ledframe.layers[j].row3=0x1F;
                    break;
                case 4:
                    ledframe.layers[j].row4=0x1F;
                    break;
                }
            }

            buffer[0]=0;
            memcpy(buffer+1,&ledframe,sizeof(ledframe));
            if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0)   // add a dummy report ID
                fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));

            numlayer=(numlayer +1) % 5;
            msleep(80);

        }

        numlayer = 4;

        for(int i=0; i< 50 ;i++){

            memset(buffer,0,sizeof(buffer));
            memset(&ledframe,0,sizeof(ledframe));

            for(int j=0;j<5;j++){
                ledframe.layers[j].index=(1<<j);
            }

            ledframe.layers[numlayer].row0 = 0x1F;
            ledframe.layers[numlayer].row1 = 0x1F;
            ledframe.layers[numlayer].row2 = 0x1F;
            ledframe.layers[numlayer].row3 = 0x1F;
            ledframe.layers[numlayer].row4 = 0x1F;

            buffer[0]=0;
            memcpy(buffer+1,&ledframe,sizeof(ledframe));
            if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0)   // add a dummy report ID
                fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));

            numlayer--;
            if (numlayer== -1)
                numlayer = 4;
            msleep(80);

        }

        numlayer = 4;

        for(int i=0; i< 50 ;i++){

            memset(buffer,0,sizeof(buffer));
            memset(&ledframe,0,sizeof(ledframe));

            for(int j=0;j<5;j++){
                ledframe.layers[j].index=(1<<j);
                switch (numlayer){
                case 0:
                    ledframe.layers[j].row0=0x1F;
                    break;
                case 1:
                    ledframe.layers[j].row1=0x1F;
                    break;
                case 2:
                    ledframe.layers[j].row2=0x1F;
                    break;
                case 3:
                    ledframe.layers[j].row3=0x1F;
                    break;
                case 4:
                    ledframe.layers[j].row4=0x1F;
                    break;
                }
            }

            buffer[0]=0;
            memcpy(buffer+1,&ledframe,sizeof(ledframe));
            if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0)   // add a dummy report ID
                fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));

            numlayer--;
            if (numlayer== -1)
                numlayer = 4;
            msleep(80);

        }

        frame turn[4];
        memset(&turn,0,sizeof(frame)*4);
        for(int j=0;j<5;j++){
            turn[0].layers[j].index=(1<<j);
            turn[1].layers[j].index=(1<<j);
            turn[2].layers[j].index=(1<<j);
            turn[3].layers[j].index=(1<<j);
        }

        /*

                  *
                  *
                  *
                  *
                  *

                  */
        for(int j=0;j<5;j++){
            turn[0].layers[j].row0=(1<<2);
            turn[0].layers[j].row1=(1<<2);
            turn[0].layers[j].row2=(1<<2);
            turn[0].layers[j].row3=(1<<2);
            turn[0].layers[j].row4=(1<<2);
        }

        /*

                 *
                  *
                   *
                    *
                     *

                  */

        for(int j=0;j<5;j++){
            turn[1].layers[j].row0=(1<<4);
            turn[1].layers[j].row1=(1<<3);
            turn[1].layers[j].row2=(1<<2);
            turn[1].layers[j].row3=(1<<1);
            turn[1].layers[j].row4=(1<<0);
        }

        /*


                * * * * *


                  */
        for(int j=0;j<5;j++){
            turn[2].layers[j].row2=0x1F;
        }

        /*

                     *
                    *
                   *
                  *
                 *

                  */

        for(int j=0;j<5;j++){
            turn[3].layers[j].row0=(1<<0);
            turn[3].layers[j].row1=(1<<1);
            turn[3].layers[j].row2=(1<<2);
            turn[3].layers[j].row3=(1<<3);
            turn[3].layers[j].row4=(1<<4);
        }



        numlayer = 0;
        for(int i=0; i< 50 ;i++){

            memset(buffer,0,sizeof(buffer));

            buffer[0]=0;
            memcpy(buffer+1,&turn[numlayer],sizeof(frame));
            if((err = usbhidSetReport(dev, buffer, sizeof(buffer))) != 0)   // add a dummy report ID
                fprintf(stderr, "error writing data: %s\n", usbErrorMessage(err));

            numlayer=(numlayer+1)%4;
            msleep(110);

        }


    }else{
        usage(argv[0]);
        exit(1);
    }
    usbhidCloseDevice(dev);
    return 0;
}

/* ------------------------------------------------------------------------- */
