#ifndef _IO_H_
#define _IO_H_

extern short nPort;   //the base address of the choosen port
extern short nAddr;   //the choosen address of the i²c module

bool io_open();
short io_tx(short);
short io_rx();
short io_sync(short reply=0);      //returns sync code

#endif
