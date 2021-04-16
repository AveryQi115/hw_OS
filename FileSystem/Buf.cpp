#include "Buf.h"

Buf::Buf() {
	b_flags = 0;
	forw = nullptr;
	back = nullptr;
	b_wcount = 0;
	b_addr = nullptr;
	b_blkno = -1;
	b_error = -1;
	b_resid = 0;
}

Buf::~Buf() {

}