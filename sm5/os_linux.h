/* 
 *  os_linux.h
 * 
 * Home page of code is: http://smartmontools.sourceforge.net
 *
 * Copyright (C) 2003-4 Bruce Allen <smartmontools-support@lists.sourceforge.net>
 *
 * Derived from code that was
 *
 *  Written By: Adam Radford <linux@3ware.com>
 *  Modifications By: Joel Jacobson <linux@3ware.com>
 *                   Arnaldo Carvalho de Melo <acme@conectiva.com.br>
 *                    Brad Strand <linux@3ware.com>
 *
 *  Copyright (C) 1999-2003 3ware Inc.
 *
 *  Kernel compatablity By:     Andre Hedrick <andre@suse.com>
 *  Non-Copyright (C) 2000      Andre Hedrick <andre@suse.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * You should have received a copy of the GNU General Public License
 * (for example COPYING); if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * This code was originally developed as a Senior Thesis by Michael Cornwell
 * at the Concurrent Systems Laboratory (now part of the Storage Systems
 * Research Center), Jack Baskin School of Engineering, University of
 * California, Santa Cruz. http://ssrc.soe.ucsc.edu/
 * 
 */


#ifndef OS_LINUX_H_
#define OS_LINUX_H_

#define OS_XXXX_H_CVSID "$Id: os_linux.h,v 1.14 2004/07/09 15:32:03 ballen4705 Exp $\n"

#define u32 unsigned int

/* Misc defines */
#define TW_IOCTL                 0x80
#define TW_ATA_PASSTHRU          0x1e
#define TW_ATA_PASS_SGL_MAX      60
#define TW_MAX_SGL_LENGTH_9000   61
#define TW_MAX_SGL_LENGTH        62
#define TW_CMD_PACKET_WITH_DATA 0x1f
#define TW_OP_ATA_PASSTHRU 0x11


#define TW_IOCTL_FIRMWARE_PASS_THROUGH        0x108

/* Scatter gather list entry -- same for all three cases */
typedef struct TAG_TW_SG_Entry
{
  unsigned int address;
  unsigned int length;
} TW_SG_Entry;

/* Command header for ATA pass-thru.  Note that for different
   drivers/interfaces the length of sg_list is different.  But it can
   be taken as same for all three cases because it's never used to
   define any other structures, and we never use anything in the
   sg_list or beyond! */
typedef struct TAG_TW_Passthru {
  struct {
    unsigned char opcode:5;
    unsigned char sgloff:3;
  } byte0;
  unsigned char size;
  unsigned char request_id;
  struct { 
    unsigned char aport:4;
    unsigned char host_id:4;
  } byte3;
  unsigned char status;
  unsigned char flags;
  unsigned short param;
  unsigned short features;
  unsigned short sector_count;
  unsigned short sector_num;
  unsigned short cylinder_lo;
  unsigned short cylinder_hi;
  unsigned char drive_head;
  unsigned char command;
  TW_SG_Entry sg_list[TW_ATA_PASS_SGL_MAX];
  unsigned char padding[12];
} TW_Passthru;

/* Ioctl buffer -- SCSI interface only! */
// Note that this defn has changed in kernel tree...
// Total size is 1041 bytes -- this is really weird
typedef struct TAG_TW_Ioctl { 
  int input_length;
  int output_length;
  unsigned char cdb[16];
  unsigned char opcode;
  unsigned short table_id;
  unsigned char parameter_id;
  unsigned char parameter_size_bytes;
  unsigned char unit_index;
  // Size up to here is 30 bytes
  // CHECK ME -- is this RIGHT??
  unsigned char input_data[499];
  // Reserve lots of extra space for commands that set Sector Count
  // register to large values
  unsigned char output_data[512];
} TW_Ioctl;

/* Ioctl buffer output -- SCSI interface only! */
typedef struct TAG_TW_Output {
  // CHECKME - is padding right on machines with 8-byte INTEGERS??
  int padding[2];
  char output_data[512];
} TW_Output; 



// needed for 9000 char only
typedef struct TAG_TW_Ioctl_Driver_Command_9000 {
	unsigned int control_code;
	unsigned int status;
	unsigned int unique_id;
	unsigned int sequence_id;
	unsigned int os_specific;
	unsigned int buffer_length;
} TW_Ioctl_Driver_Command_9000;

// needed for 9000 char only
/* Command Packet */
typedef struct TW_Command_9000 {
	/* First DWORD */
	struct {
		unsigned char opcode:5;
		unsigned char sgl_offset:3;
	} byte0;
	unsigned char size;
	unsigned char request_id;
	struct {
		unsigned char unit:4;
		unsigned char host_id:4;
	} byte3;
	/* Second DWORD */
	unsigned char status;
	unsigned char flags;
	union {
		unsigned short block_count;
		unsigned short parameter_count;
		unsigned short message_credits;
	} byte6;
	union {
		struct {
			u32 lba;
			TW_SG_Entry sgl[TW_MAX_SGL_LENGTH_9000];
			u32 padding;	/* pad to 512 bytes */
		} io;
		struct {
			TW_SG_Entry sgl[TW_MAX_SGL_LENGTH_9000];
			u32 padding[2];
		} param;
		struct {
			u32 response_queue_pointer;
			u32 padding[125];
		} init_connection;
		struct {
			char version[504];
		} ioctl_miniport_version;
	} byte8;
} TW_Command_9000;


// needed for 9000 char interface only
/* Command Packet for 9000+ controllers */
typedef struct TAG_TW_Command_Apache {
	struct {
		unsigned char opcode:5;
		unsigned char reserved:3;
	} command;
	unsigned char unit;
	unsigned short request_id;
	unsigned char sense_length;
	unsigned char sgl_offset;
	unsigned short sgl_entries;
	unsigned char cdb[16];
	TW_SG_Entry sg_list[TW_MAX_SGL_LENGTH_9000];
} TW_Command_Apache;

// needed for 9000 char interface only
/* New command packet header */
typedef struct TAG_TW_Command_Apache_Header {
	unsigned char sense_data[18];
	struct {
		char reserved[4];
		unsigned short error;
		unsigned char status;
		struct {
			unsigned char severity:3;
			unsigned char reserved:5;
		} substatus_block;
	} status_block;
	unsigned char err_specific_desc[102];
} TW_Command_Apache_Header;

// needed for 9000 char interface only
/* This struct is a union of the 2 command packets */
typedef struct TAG_TW_Command_Full_9000 {
	TW_Command_Apache_Header header;
	union {
		TW_Command_9000 oldcommand;
		TW_Command_Apache newcommand;
	} command;
	unsigned char padding[384]; /* Pad to 1024 bytes */
} TW_Command_Full_9000;


// Needed for 9000 char interface only!
typedef struct TAG_TW_Ioctl_Apache {
	TW_Ioctl_Driver_Command_9000 driver_command;
	char padding[488];
	TW_Command_Full_9000 firmware_command;
	char data_buffer[1];
} TW_Ioctl_Buf_Apache;



// START OF DEFINITIONS FOR THE CHARACTER INTERFACE TO THE
// 6000/7000/8000 drivers


/* Command Packet */
typedef struct TW_Command {
	/* First DWORD */
	struct {
		unsigned char opcode:5;
		unsigned char sgl_offset:3;
	} byte0;
	unsigned char size;
	unsigned char request_id;
	struct {
		unsigned char unit:4;
		unsigned char host_id:4;
	} byte3;
	/* Second DWORD */
	unsigned char status;
	unsigned char flags;
	union {
		unsigned short block_count;
		unsigned short parameter_count;
		unsigned short message_credits;
	} byte6;
	union {
		struct {
			u32 lba;
			TW_SG_Entry sgl[TW_MAX_SGL_LENGTH];
			u32 padding;	/* pad to 512 bytes */
		} io;
		struct {
			TW_SG_Entry sgl[TW_MAX_SGL_LENGTH];
			u32 padding[2];
		} param;
		struct {
			u32 response_queue_pointer;
			u32 padding[125];
		} init_connection;
		struct {
			char version[504];
		} ioctl_miniport_version;
	} byte8;
} TW_Command;


typedef struct TAG_TW_New_Ioctl {
	unsigned int data_buffer_length;
	unsigned char padding [508];
	TW_Command firmware_command;
	char data_buffer[1];
} TW_New_Ioctl;


#define MAX(x,y) ( (x)>(y)?(x):(y) )


// The following definitions are from hdreg.h in the kernel source
// tree.  They don't carry any Copyright statements, but I think they
// are primarily from Mark Lord and Andre Hedrick.
typedef unsigned char task_ioreg_t;

typedef struct hd_drive_task_hdr {
	task_ioreg_t data;
	task_ioreg_t feature;
	task_ioreg_t sector_count;
	task_ioreg_t sector_number;
	task_ioreg_t low_cylinder;
	task_ioreg_t high_cylinder;
	task_ioreg_t device_head;
	task_ioreg_t command;
} task_struct_t;

typedef union ide_reg_valid_s {
	unsigned all				: 16;
	struct {
		unsigned data			: 1;
		unsigned error_feature		: 1;
		unsigned sector			: 1;
		unsigned nsector		: 1;
		unsigned lcyl			: 1;
		unsigned hcyl			: 1;
		unsigned select			: 1;
		unsigned status_command		: 1;

		unsigned data_hob		: 1;
		unsigned error_feature_hob	: 1;
		unsigned sector_hob		: 1;
		unsigned nsector_hob		: 1;
		unsigned lcyl_hob		: 1;
		unsigned hcyl_hob		: 1;
		unsigned select_hob		: 1;
		unsigned control_hob		: 1;
	} b;
} ide_reg_valid_t;

typedef struct ide_task_request_s {
	task_ioreg_t	io_ports[8];
	task_ioreg_t	hob_ports[8];
	ide_reg_valid_t	out_flags;
	ide_reg_valid_t	in_flags;
	int		data_phase;
	int		req_cmd;
	unsigned long	out_size;
	unsigned long	in_size;
} ide_task_request_t;

#define TASKFILE_NO_DATA		0x0000
#define TASKFILE_IN			0x0001
#define TASKFILE_OUT			0x0004

#define HDIO_DRIVE_TASK_HDR_SIZE	8*sizeof(task_ioreg_t)

#define IDE_DRIVE_TASK_NO_DATA		0
#define IDE_DRIVE_TASK_IN		2
#define IDE_DRIVE_TASK_OUT		3

#define HDIO_DRIVE_CMD       0x031f
#define HDIO_DRIVE_TASK      0x031e
#define HDIO_DRIVE_TASKFILE  0x031d
#define HDIO_GET_IDENTITY    0x030d

#endif /* OS_LINUX_H_ */
