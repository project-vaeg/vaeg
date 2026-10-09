/*
 * i8255.c: i8255
 *
 */

#include "compiler.h"
#include "i8255.h"

#define CTRL_PORTADIR 0x10
#define CTRL_PORTBDIR 0x02
#define CTRL_PORTCHDIR 0x08
#define CTRL_PORTCLDIR 0x01

// ----

void i8255_init(I8255CFG p, I8255 s) {
	ZeroMemory(p, sizeof(_I8255CFG));
	ZeroMemory(s, sizeof(_I8255));
	p->s = s;
}

void i8255_reset(I8255CFG p) {
	ZeroMemory(p->s, sizeof(_I8255));
	i8255_outctrl(p, 0x80);
}

// ---- CPU との I/F

void i8255_outporta(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (!(s->mode & CTRL_PORTADIR)) {
		s->porta = dat;
		if (p->busoutporta)
			p->busoutporta(dat);
	}
}

BYTE i8255_inporta(I8255CFG p) {
	return p->s->porta;
}

void i8255_outportb(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (!(s->mode & CTRL_PORTBDIR)) {
		s->portb = dat;
		if (p->busoutportb)
			p->busoutportb(dat);
	}
}

BYTE i8255_inportb(I8255CFG p) {
	return p->s->portb;
}

void i8255_outportc(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	s->portc = (s->portc & s->portcinmask) | (dat & ~s->portcinmask);
	if (s->portcinmask != 0xff) {
		if (p->busoutportc)
			p->busoutportc(dat);
	}
}

BYTE i8255_inportc(I8255CFG p) {
	return p->s->portc;
}

void i8255_outctrl(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (dat & 0x80) {
		s->mode = dat;
		s->portcinmask = 0;
		if (s->mode & CTRL_PORTCHDIR)
			s->portcinmask |= 0xf0;
		if (s->mode & CTRL_PORTCLDIR)
			s->portcinmask |= 0x0f;
	} else {
		int bit = (dat >> 1) & 7;
		BYTE pat = 1 << bit;
		pat &= ~s->portcinmask;
		if (dat & 1) {
			// set
			s->portc |= pat;
		} else {
			// reset
			s->portc &= ~pat;
		}
		if (s->portcinmask != 0xff) {
			if (p->busoutportc)
				p->busoutportc(s->portc);
		}
	}
}

// ---- 外部装置との I/F

void i8255_businporta(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (s->mode & CTRL_PORTADIR) {
		s->porta = dat;
	}
}

void i8255_businportb(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	if (s->mode & CTRL_PORTBDIR) {
		s->portb = dat;
	}
}

void i8255_businportc(I8255CFG p, BYTE dat) {
	I8255 s = p->s;

	s->portc = (s->portc & ~s->portcinmask) | (dat & s->portcinmask);
}
