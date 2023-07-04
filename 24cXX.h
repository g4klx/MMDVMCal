/*
 * 		Modifications to code done by: Lucas Burns
 * 		Complete credit for original code goes to Stefano Barbato, details below:
 */

/***************************************************************************
    copyright            : (C) by 2003-2004 Stefano Barbato
    email                : stefano@codesink.org
    $Id: 24cXX.h,v 1.6 2004/02/29 11:05:28 tat Exp $
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _24CXX_H_
#define _24CXX_H_

extern "C" {
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <linux/fs.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <linux/i2c-dev.h>
#include <i2c/smbus.h>
}

#define EEPROM_TYPE_UNKNOWN	0
#define EEPROM_TYPE_8BIT_ADDR	1
#define EEPROM_TYPE_16BIT_ADDR 	2

class C24CXX {
public:
	C24CXX(const char *dev_fqn, int addr, int type);
	~C24CXX();
	/*
	 * opens the eeprom device at [dev_fqn] (i.e. /dev/i2c-N) whose address is
	 * [addr] and set the eeprom_24c32 [e]
	 */
	int eeprom_open(const char *dev_fqn, int addr, int type);
	/*
	 * closees the eeprom device [e]
	 */
	int eeprom_close();
	/*
	 * read and returns the eeprom byte at memory address [mem_addr]
	 * Note: eeprom must have been selected by ioctl(fd,I2C_SLAVE,address)
	 */
	int i2c_write_1b(__u8 buf);
	int i2c_write_2b(__u8 buf[2]);
	int i2c_write_3b(__u8 buf[3]);

	int eeprom_read_byte(__u16 mem_addr);
	/*
	 * read the current byte
	 * Note: eeprom must have been selected by ioctl(fd,I2C_SLAVE,address)
	 */
	int eeprom_read_current_byte();
	/*
	 * writes [data] at memory address [mem_addr]
	 * Note: eeprom must have been selected by ioctl(fd,I2C_SLAVE,address)
	 */
	int eeprom_write_byte(__u16 mem_addr, __u8 data);
	/*
	 * wait for the eeprom to accept new commands, to be called after a write
	 */
	int eeprom_wait_ready(int max_ms_to_wait);
	/*
	 * Write arbitrary number of bytes to EEPROM
	 */
	int eeprom_write(int addr, unsigned char *data, int size, int timeout);

	int getFileDescriptor();
	bool eeprom_check();

private:
	const char *m_dev;
	int m_addr;
	int m_fd;
	int m_type;
};

#endif
