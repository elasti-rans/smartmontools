// Bartlomiej Zolnierkiewicz figured this out.  Only
// HDIO_DRIVE_TASKFILE can send data to the disk.  In fact this is
// obvious if you search for copy_from_user() in the driver and look
// for variable length arguments.  I guess I'll have to learn to use
// another ioctl().
// 
//  BUG REPORT FOR 2.4.20 SMART_WRITE_LOG_SECTOR ioctl()
//
// The ATA-7 specifications define a new type of SMART self-test for
// ATA disks.  This is called the "Selective" self test. I have been
// trying to add support for this to smartmontools.
//
// In order to carry out a selective self-test, one of the SMART logs
// (#9) must be written.  I have been unable to get
// SMART_WRITE_LOG_SECTOR to work correctly with a stock 2.4.20
// kernel.  I have tried both the HDIO_DRIVE_CMD and HDIO_DRIVE_TASK
// ioctl()s.
//
// Note that this bug report is NOT related to the ATA-7 General
// Purpose Logging feature set.  The SMART Selective Self-test log at
// address 0x09 is readable/writable with the SMART READ/WRITE LOG
// commands, NOT with the READ/WRITE LOG EXT commands.
//
// This code demonstrates this failure. The first #define below can be
// used to choose to test the HDIO_DRIVE_CMD or HDIO_DRIVE_TASK ioctl.
//
// To reproduce this problem, you MUST use a disk that supports SMART
// LOG page #9 (the only writable SMART log page).  The only disks I
// have seen that support this are recent Maxtor ATA-7 disks (example:
// Maxtor 4R080L0 Firmware RAMC1TU0) though are probably others.  To
// see if a disk supports this page, use smartmontools 'smartctl -c
// /dev/hd?'.  If it says "Selective self-test supported" then the
// disk DOES support this page, and you can then use this code to
// demonstrate the problem.
//
// In order for this code to work, you must have SMART enabled on the
// drive.  This can be done with 'smartctl -s on /dev/hd?'.
//
// The code below does the following:
// - reads SMART LOG page #9
// - tries to write the exact same data back to SMART LOG page #9
//
// compile: cc -Wall -o writelog thisfile.c
// run:     ./writelog /dev/hda
//
// Kernel error messages are reproduced below after the code
//
// Bruce Allen, ballen at gravity dot phys dot uwm dot edu
//
// $Id: writelog.c,v 1.4 2003/09/04 21:24:25 ballen4705 Exp $

// set to zero to try using HDIO_DRIVE_CMD ioctl()
#define USE_HDIO_DRIVE_TASK 1

#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/hdreg.h>
#include <string.h>
#include <stdlib.h>

#define WHICHLOG 0x09
#define ONEPAGE 1
#define LENGTH (512+8)

int main(int argc, char **argv){
  int fd;
  unsigned char buff[LENGTH],buff2[LENGTH];
  
  memset(buff, 0, LENGTH);
  memset(buff2,0, LENGTH);
  
  if (argc!=2){
    fprintf(stderr, "Correct syntax is: %s /dev/hd?\n", argv[0]);
    exit(1);
  }
  
  if ((fd=open(argv[1], O_RDWR | O_NONBLOCK))<0){
    perror("Open Device");
    fprintf(stderr, "Unable to open device %s\n", argv[1]);
    exit(2);
  }
  printf("Device %s opened\n", argv[1]);
  
  // See struct hd_drive_cmd_hdr in hdreg.h
  // buff[0]: ATA COMMAND CODE REGISTER
  // buff[1]: ATA SECTOR NUMBER REGISTER == LBA LOW REGISTER
  // buff[2]: ATA FEATURES REGISTER
  // buff[3]: ATA SECTOR COUNT REGISTER
  // First read selective self-test log at address 9
  buff[0]=WIN_SMART;
  buff[1]=WHICHLOG;
  buff[2]=SMART_READ_LOG_SECTOR;
  buff[3]=ONEPAGE;
  
  if (ioctl(fd, HDIO_DRIVE_CMD, buff)){
    perror("SMART READ SELECTIVE SELF-TEST LOG");
    fprintf(stderr, 
	    "Unable to read selective self test log.\n"
	    "This is NOT the kernel problem I am reporting!\n"
	    "To reproduce the problem, device %s must support\n"
	    "the selective self-test log page, defined in the ATA-7\n"
	    "specifications.  Example: Maxtor 4R080L0 Firmware RAMC1TU0\n"
	    "Use smartmontools 'smartctl -c' to see if the disk suports it.\n", argv[1]);
    exit(3);
  }
  printf("Sucessfully read selective self-test log\n");
  
#if USE_HDIO_DRIVE_TASK
  // See struct hd_drive_task_hdr in /usr/src/linux/include/linux/hdreg.h
  // buff2[0]: DATA: ATA COMMAND CODE REGISTER
  // buff2[1]: ATA FEATURES REGISTER
  // buff2[2]: ATA SECTOR_COUNT
  // buff2[3]: ATA SECTOR NUMBER
  // buff2[4]: ATA CYL LO REGISTER
  // buff2[5]: ATA CYL HI REGISTER
  // buff2[6]: ATA DEVICE HEAD
  // buff2[7]: COMMAND
  // Now WRITE selective self-test log at same address.
  buff2[0]=WIN_SMART;
  buff2[1]=SMART_WRITE_LOG_SECTOR;
  buff2[2]=ONEPAGE;
  buff2[3]=WHICHLOG;
  buff2[4]=0x4f;
  buff2[5]=0xc2;
  buff2[6]=0;
  buff2[7]=0;
  
  // copy log into new buffer
  memcpy(buff2+8, buff+4, 512);
  
  if (ioctl(fd, HDIO_DRIVE_TASK, buff2)){
    perror("SMART WRITE SELECTIVE SELF-TEST LOG");
    fprintf(stderr, "Unable to write selective self test log using HDIO_DRIVE_TASK\n");
    exit(5);
  }
#else
  // buff[0]: ATA COMMAND CODE REGISTER
  // buff[1]: ATA SECTOR NUMBER REGISTER == LBA LOW REGISTER
  // buff[2]: ATA FEATURES REGISTER
  // buff[3]: ATA SECTOR COUNT REGISTER
  // Now WRITE selective self-test log at same address.
  buff[0]=WIN_SMART;
  buff[1]=WHICHLOG;
  buff[2]=SMART_WRITE_LOG_SECTOR;
  buff[3]=ONEPAGE;
  
  if (ioctl(fd, HDIO_DRIVE_CMD, buff)){
    perror("SMART WRITE SELECTIVE SELF-TEST LOG");
    fprintf(stderr, "Unable to write selective self test log using HDIO_DRIVE_CMD\n");
    exit(4);
  }
#endif
  printf("SUCCESS writing selective self-test log -- BUG FIXED!\n");
  exit(0);
}

// What follow are the run-time messages and syslog output for the
// code run with both possible choices of ioctl() type.

#if 0

[root@medusa-slave301 root]# uname -a
Linux medusa-slave301.medusa.phys.uwm.edu 2.4.20-1medusa #1 Wed Feb 12 17:20:41 CST 2003 i686 unknown


With HDIO_DRIVE_TASK, the code segvs in the write log ioctl():

[root@medusa-slave301 root]# ./writelog /dev/hdb
Device /dev/hdb opened
Sucessfully read selective self-test log
Segmentation fault

  Syslog says:

Sep  4 14:10:42 medusa-slave301 kernel:  hdb: lost interrupt
Sep  4 14:10:42 medusa-slave301 kernel: hdb: drive_cmd: status=0x58 { DriveReady SeekComplete DataRequest }
Sep  4 14:10:42 medusa-slave301 kernel: Unable to handle kernel NULL pointer dereference at virtual address 00000000
Sep  4 14:10:42 medusa-slave301 kernel:  printing eip:
Sep  4 14:10:42 medusa-slave301 kernel: 00000000
Sep  4 14:10:42 medusa-slave301 kernel: *pde = 00000000
Sep  4 14:10:42 medusa-slave301 kernel: Oops: 0000
Sep  4 14:10:42 medusa-slave301 kernel: CPU:    0
Sep  4 14:10:42 medusa-slave301 kernel: EIP:    0010:[<00000000>]    Not tainted
Sep  4 14:10:42 medusa-slave301 kernel: EFLAGS: 00010296
Sep  4 14:10:42 medusa-slave301 kernel: eax: fffffffb   ebx: 00000000   ecx: 00000000   edx: fffffffb
Sep  4 14:10:42 medusa-slave301 kernel: esi: 00000000   edi: 00000000   ebp: 00000000   esp: d5addf8c
Sep  4 14:10:42 medusa-slave301 kernel: ds: 0018   es: 0018   ss: 0018
Sep  4 14:10:42 medusa-slave301 kernel: Process writelog (pid: 5004, stackpage=d5add000)
Sep  4 14:10:42 medusa-slave301 kernel: Stack: 00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
Sep  4 14:10:42 medusa-slave301 kernel:        00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
Sep  4 14:10:42 medusa-slave301 kernel:        00000000 00000000 00000000 00000000 00000000 00000000 00000000 00000000 
Sep  4 14:10:42 medusa-slave301 kernel: Call Trace:   
Sep  4 14:10:42 medusa-slave301 kernel: 
Sep  4 14:10:42 medusa-slave301 kernel: Code:  Bad EIP value.


With HDIO_DRIVE_CMD, the code returns unsucessfully from the HDIO_DRIVE_CMD ioctl():

[root@medusa-slave301 root]# ./writelog /dev/hdb
Device /dev/hdb opened
Sucessfully read selective self-test log
SMART WRITE SELECTIVE SELF-TEST LOG: Input/output error
Unable to write selective self test log using HDIO_DRIVE_CMD

  Syslog says:

Sep  4 14:13:35 medusa-slave301 kernel:  hdb: lost interrupt
Sep  4 14:13:35 medusa-slave301 kernel: hdb: drive_cmd: status=0x58 { DriveReady SeekComplete DataRequest }

#endif
