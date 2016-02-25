#ifndef __CLI_LIB_H__
#define __CLI_LIB_H__

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include <signal.h>
#include "trex.h"

#ifdef _WIN32
#       include <Winsock2.h>
#define snprintf _snprintf
#else
#       include <sys/socket.h>
#       include <sys/select.h>
#       include <netinet/in.h>
#       include <arpa/inet.h>
#       include <netdb.h>
#       include <unistd.h>
typedef int SOCKET;
#endif

struct tcliClient {
        SOCKET sockfd;
        fd_set fdset;
        char buff[ BUFSIZ];
        int timeOut;
};

#define CLI_ERR (-1)
#define CLI_SUCC (0)

typedef int get_clifunc_str_t( const char* str, int prompt, void* par);

void cliClear( struct tcliClient* c);
int cliOpened( const struct tcliClient* c);
int cliInit();
void cliDone();
int cliOpen( struct tcliClient* c, const char* addr, unsigned short int tcpPort, int timeOut);
int cliClose( struct tcliClient* c);

int cliSelectRead( const struct tcliClient* c, unsigned int ms);
int cliRecvStr( struct tcliClient* c, char* buff, int size);
int cliWriteStr( const struct tcliClient* c, const char* str);
int cliWriteCommand( struct tcliClient* c, const char* cmd, const char* regStrPrompt, int regErrC, const char **regErrStr, get_clifunc_str_t* read_f, void* par); //c == NULL means write it to all active clients

int cliTest( const char* addr, unsigned short int port, const char* fileName);


int cliRecvStrReg( TRex *x, struct tcliClient* c, char* buff, int size);        //read string with <CR> or part of string matched with regular expression
int strMatchRegCompile( TRex **x, const char* regExp); //compile regular expression
int strMatchRegCompiled( TRex *x, const char* str);     //Check str with compiled x

#endif
