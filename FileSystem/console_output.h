//console_output.h
#ifndef DIAGNOSE_H
#define DIAGNOSE_H

class Diagnose
{
public:
	Diagnose();
	~Diagnose();

	static void Write(const char* msg);
};

#endif
