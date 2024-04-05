//
//
//	term.h
//
//	(C) R.P.Bellis 1994
//

#pragma once

#include <cstdio>
#include "usim.h"
#include "mc6850.h"

#ifdef _POSIX_SOURCE
#include <termios.h>
#endif

class Terminal :  virtual public mc6850_impl {

protected:
	USim&				sys;
	Byte				read_data;
	bool				read_data_available = false;
	bool				insert_data_available = false;
	int					tilde_escape_phase = 0;

	void				tilde_escape_help();
	virtual void		tilde_escape_help_other();
	virtual void 		tilde_escape_do_other(char ch);
	bool				real_poll_read();
	Byte				real_read();
	bool				open_insert_file();

#ifdef _POSIX_SOURCE
	FILE*				input;
	FILE*				output;
	FILE*				insert;
	int			 		input_fd;
	int					insert_fd;
	struct termios		oattr, nattr;
#endif // _POSIX_SOURCE

public:
	virtual bool		poll_read();
	virtual void		write(Byte);
	virtual Byte		read();

public:
	virtual void		setup();
	virtual void		reset();

// Public constructor and destructor
public:
						 Terminal(USim& sys);
	virtual				~Terminal();

};
