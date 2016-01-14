/*
 *   Copyright (C) 2015,2016 by Jonathan Naylor G4KLX
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "Console.h"

#include <cstdio>

#if defined(_WIN32) || defined(_WIN64)

#include <conio.h>

CConsole::CConsole()
{
}

CConsole::~CConsole()
{
}

bool CConsole::open()
{
	return true;
}

int CConsole::getChar()
{
	if (::_kbhit() == 0)
		return -1;

	return ::_getch();
}

void CConsole::close()
{
}

#else

#include <cstring>

#include <termios.h>
#include <unistd.h>

#include <sys/select.h>

CConsole::CConsole() :
m_termios()
{
	::memset(&m_termios, 0x00U, sizeof(termios));
}

CConsole::~CConsole()
{
}

bool CConsole::open()
{
	termios tios;

	int n = ::tcgetattr(STDIN_FILENO, &tios);
	if (n != 0) {
		::fprintf(stderr, "tcgetattr: returned %d\r\n", n);
		return -1;
	}

	m_termios = tios;

	::cfmakeraw(&tios);

	n = ::tcsetattr(STDIN_FILENO, TCSANOW, &tios);
	if (n != 0) {
		::fprintf(stderr, "tcsetattr: returned %d\r\n", n);
		return -1;
	}

	return true;
}

int CConsole::getChar()
{
	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(STDIN_FILENO, &fds);

	timeval tv;
	tv.tv_sec  = 0;
	tv.tv_usec = 0;               

	int n = ::select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
	if (n <= 0) {
		if (n < 0)
			::fprintf(stderr, "select: returned %d\r\n", n);
		return -1;
	}

	char c;
	n = ::read(STDIN_FILENO, &c, 1);
	if (n <= 0) {
		if (n < 0)
			::fprintf(stderr, "read: returned %d\r\n", n);
		return -1;
	}

	return c;
}

void CConsole::close()
{
	int n = ::tcsetattr(STDIN_FILENO, TCSANOW, &m_termios);
	if (n != 0)
		::fprintf(stderr, "tcsetattr: returned %d\r\n", n);
}

#endif
