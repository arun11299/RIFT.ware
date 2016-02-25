#include "logs.h"
#include "cliLib.h"

int cliInit() {
#ifdef _WIN32
        WSADATA wsaData;
        int iResult;
#endif
#ifdef _WIN32
        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != NO_ERROR) {
                ERROR_LOG( "Error at WSAStartup()");
                return CLI_ERR;
        }
#else
        signal( SIGPIPE, SIG_IGN);      /* ignore SIGPIPE signal */
#endif
        return CLI_SUCC;
}

void cliDone() {
#ifdef _WIN32
        WSACleanup();
#endif
}

void cliClear( struct tcliClient* c) {
        memset( c, 0, sizeof( struct tcliClient));
}

int cliOpened( const struct tcliClient* c) {
        return c->sockfd != 0;
}

int cliOpen( struct tcliClient* c, const char* addr, unsigned short int tcpPort, int timeOut) {
        struct sockaddr_in serv_addr;

        if ( cliOpened( c))
                cliClose( c);

        c->buff[ 0] = '\0';
        if (( c->sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                ERROR_LOG( "Cannot create socket, port %d", tcpPort);
                return CLI_ERR;
        }

        memset((void *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family      = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr( addr);
        serv_addr.sin_port        = htons( tcpPort);

        if (connect( c->sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
                ERROR_LOG( "Cannot connect %s:%d", addr, tcpPort);
                c->sockfd = 0;
                return CLI_ERR;
        }
        FD_ZERO( &c->fdset);
        FD_SET( c->sockfd, &c->fdset);
        c->timeOut = timeOut;

        return CLI_SUCC;
}

int cliClose( struct tcliClient* c) {
        int ret = CLI_SUCC;

        if ( !cliOpened( c))
                return CLI_SUCC;

#ifdef _WIN32
        if ( closesocket( c->sockfd) < 0)
                ret = CLI_ERR;
#else
        if ( close( c->sockfd) < 0)
                ret = CLI_ERR;
#endif

        c->sockfd = 0;

        return ret;
}

int cliSelectRead( const struct tcliClient* c, unsigned int ms) {
        struct timeval tm;
        fd_set tmpSet;
        tm.tv_sec = ms / 1000;
        tm.tv_usec = ( ms % 1000) * 1000;
        tmpSet = c->fdset;
        return select( FD_SETSIZE, &tmpSet, NULL, NULL, &tm);;
}

int cliSelectWrite( const struct tcliClient* c, unsigned int ms) {
        struct timeval tm;
        fd_set tmpSet;
        tm.tv_sec = ms / 1000;
        tm.tv_usec = ( ms % 1000) * 1000;
        tmpSet = c->fdset;
        return select( FD_SETSIZE, NULL, &tmpSet, NULL, &tm);
}

static int cliSendDataRaw( const struct tcliClient* c, const char* data, int size) {
        if (( c->timeOut > 0) && ( cliSelectWrite( c, c->timeOut) <= 0)) {
                ERROR_LOG( "cliSendDataRaw write timeout %d", c->timeOut);
                return CLI_ERR; //timeout
        }
        return send( c->sockfd, (char*) data, size, 0);
}

int cliWriteStr( const struct tcliClient* c, const char* str) {
        int nBytesSent = 0;
        int nBytesThisTime;
        int     nSize = strlen( str);
        const char* pch1 = str;
        do {
                nBytesThisTime = cliSendDataRaw( c, pch1, nSize - nBytesSent);
                if ( nBytesThisTime < 0) {
                        ERROR_LOG( "cliWriteStr write error code %d", nBytesThisTime);
                        return CLI_ERR;
                }
                nBytesSent += nBytesThisTime;
                pch1 += nBytesThisTime;
        } while (nBytesSent < nSize);
        return nBytesSent;
}

static int Pos( char c, const char* Str) {
        char* ps = strchr( Str, c);
        return (int) (( ps == NULL) ? -1 : ps - Str);
}

void Delete( char* str, int from, int len) {
        int cln;
        int ln = strlen( str);
        if ( ln < from)                                 //nothing to delete
                return;
        if (( from + len) > ln) //update len if necessary
                len = ln - from;
        cln = ln - ( from + len);       //how many chart to delete
        if ( cln > 0)
                memmove( &str[ from], &str[ from + len], cln);  //copy rest of the string
        str[ ln - len] = '\0';
}

int cliRecvDataRaw( struct tcliClient* c, char* buff, int size) {
        int i;
        if ( !cliOpened( c)) {
                ERROR_LOG( "Socket is not opened!");
                return CLI_ERR;
        }
        if (( c->timeOut > 0) && ( cliSelectRead( c, c->timeOut) <= 0)) {
                ERROR_LOG( "cliRecvStrRaw read timeout %d", c->timeOut);
                return CLI_ERR; //timeout
        }
        i = recv( c->sockfd, buff, size, 0);
        buff[ i < 0 ? 0 : i] = '\0';
        if ( i <= 0)            //IO error - close socket
                cliClose( c);
        return i;
}

int cliReadedWholeLine( const struct tcliClient* c) {
        return Pos( '\n', c->buff) != -1;
}

void cliReadedAddLineText( struct tcliClient* c, const char *ss) {
        int i;
        strncat( c->buff, ss, sizeof( c->buff) - 1);
        c->buff[ sizeof( c->buff) - 1] = '\0';

        while (( i = Pos( '\r', c->buff)) != -1)                        //Delete all \r
                Delete( c->buff, i, 1);
        while (( i = Pos( '\b', c->buff)) != -1)                        //Delete all \b with one char before
                if ( i > 0)
                        Delete( c->buff, i - 1, 2);
                else
                        Delete( c->buff, i, 1);                                                                         //First position - delete just \b
}

char* Copy( char* rstr, const char* s, int from, int len) {
        int ln = strlen( s);
        if ( from > ln)         //nothing to copy
                len = 0;
        else {
                if (( from + len) > ln) //update len if necessary
                        len = ln - from;
        }
        if ( len > 0)
                memcpy( rstr, &s[ from], len);
        rstr[ len] = '\0';
        return rstr;
}

int cliReadedGetLineLength( const struct tcliClient* c) {
        int lineLen = Pos( '\n', c->buff);
        if ( lineLen == -1)                     //not the whole line - return length of string
                lineLen = strlen( c->buff);
        return lineLen;
}

void cliReadedScanLineText( const struct tcliClient* c, char *ss, int size) {
        ss[ size - 1] = '\0';
        Copy( ss, c->buff, 0, cliReadedGetLineLength( c));
}

void cliReadedDeleteLine( struct tcliClient* c) {
        Delete( c->buff, 0, cliReadedGetLineLength( c) + 1);
}

void cliReadedGetLineText( struct tcliClient* c, char *ss, int size)
{
        cliReadedScanLineText( c, ss, size);
        cliReadedDeleteLine( c);
}


int cliRecvStr( struct tcliClient* c, char* buff, int size) {
        int i;
        while ( !cliReadedWholeLine( c)) {
                buff[ 0] = '\0';
                i = cliRecvDataRaw( c, buff, size - 1);
                if ( i < 0)     //error
                        return i;
                if (i == 0) // error as well?
                  break;
                buff[ i] = '\0';
                cliReadedAddLineText( c, buff);
        }
        i = cliReadedGetLineLength( c);
        cliReadedGetLineText( c, buff, size);
        return i;
}

int strMatchRegCompile( TRex **x, const char* regExp) {
        const char* err = NULL;
        *x = trex_compile( regExp, &err);
        if ( err) {
                ERROR_LOG( "Compile error %s - %s", regExp, err);
                return CLI_ERR; //compile error
        }
        return CLI_SUCC;
}

int strMatchRegCompiled( TRex *x, const char* str) {
        const TRexChar *begin,*end;

        if ( trex_search( x, str, &begin, &end))
                return (( begin == str) && ( end == strlen(str) + str)) ? 1 : 0;

        return 0;       //not matched at all
}

int strMatchReg( const char* regExp, const char* str) {
        TRex *x = NULL;
        int match = 0;

        if ( strMatchRegCompile( &x, regExp) == CLI_ERR)
                return CLI_ERR; //compile error

        match = strMatchRegCompiled( x, str);

        trex_free(x);
        return match;
}

int cliReadedWholeLineReg( TRex *x, const struct tcliClient* c) {
        if ( c->buff[ 0] == '\0')               //empty string
                return 0;
        if ( Pos( '\n', c->buff) != -1)
                return 1;
        return strMatchRegCompiled( x, c->buff);
}

int cliRecvStrReg( TRex *x, struct tcliClient* c, char* buff, int size) {
        int i;

        while ( !cliReadedWholeLineReg( x, c)) {
                buff[ 0] = '\0';
                i = cliRecvDataRaw( c, buff, size - 1);
                if ( i < 0)     //error
                        return i;
                if (i == 0) // error as well?
                  break;
                buff[ i] = '\0';
                cliReadedAddLineText( c, buff);
        }
        i = cliReadedGetLineLength( c);
        cliReadedGetLineText( c, buff, size);
        return i;
}

int cliWriteCommand( struct tcliClient* c, const char* cmd, const char* regStrPrompt, int regErrC, const char **regErrStr, get_clifunc_str_t* read_f, void* par) {
        char line[ BUFSIZ];
        int i;
        TRex *x = NULL;
        int ret = CLI_ERR;

        if ( !cliOpened( c)) {
                ERROR_LOG( "Socket is not opened!");
                return CLI_ERR;
        }

        //DEBUG_LOG( "cliWriteCommand -> '%s'", cmd);

        if ( cmd != NULL) {
                if ( cliWriteStr( c, cmd) <= 0)
                        return CLI_ERR;
                if ( cliWriteStr( c, "\n") <= 0)
                        return CLI_ERR;
        }

        if ( strMatchRegCompile( &x, regStrPrompt) == CLI_ERR)
                return CLI_ERR; //compile error

        for (;;) {
                if ( cliRecvStrReg( x, c, line, sizeof( line)) < 0) {
                        break;
                }
                //DEBUG_LOG( "cliWriteCommand <- '%s'", line);

                if ( strMatchRegCompiled( x, line) == 1) {      //if prompt message, exit
                        //DEBUG_LOG( "cliWriteCommand MATCH '%s' with '%s'", line, regStrPrompt);
                        if ( read_f != NULL)
                                read_f( line, 1, par);
                        ret = CLI_SUCC;
                        break;
                }

                for ( i = 0; i < regErrC; i++) {
                        if ( strMatchReg( regErrStr[ i], line) == 1) {
                                ERROR_LOG( "%s: '%s'", cmd, line);
                                break;
                        }
                }
                if ( read_f != NULL)
                        read_f( line, 0, par);
        }
        trex_free(x);
        return ret;
}

int cliTest( const char* addr, unsigned short int port, const char* fileName) {
        struct tcliClient c;
        char buff[ BUFSIZ];
        int readed;
        int s;
        FILE* f;
        const char* errorStr[] = { "^%.*"};

        cliInit();

        cliOpen( &c, addr, port, 1000);

        if(( f = fopen( fileName, "rt")) == NULL) {
                printf( "The file %s was not opened\n",  fileName);
                cliClose( &c);
                return -1;
        }

        cliWriteCommand( &c, NULL, ".*Password: ", 0, NULL, NULL, NULL);

        if ( cliWriteCommand( &c, "zebra", ".*> ", 0, NULL, NULL, NULL) < 0){
                printf( "Error send password\n");
                cliClose( &c);
                fclose( f);
                return -1;
        }

        cliWriteCommand( &c, "enable", ".*# ", 0, NULL, NULL, NULL);

        cliWriteCommand( &c, "configure terminal", ".*\\(config\\)# ", 0, NULL, NULL, NULL);

        while ( !feof( f)) {
                int i;
                fgets( buff, sizeof( buff), f);
                if ( buff[ 0] == '\0')
                        continue;
                i = strlen( buff);
                if ( buff[ i - 1] == '\n')
                        buff[ i - 1] = '\0';
                if( ferror( f))      {
                        printf( "Read error");
                        break;
                }
                printf( ">%s", buff);

                if ( cliWriteCommand( &c, buff, ".*\\(.*\\)# ", 1, errorStr, NULL, NULL) < 0) {
                        break;
                }
                printf( buff);
        }

        printf( "=== rest ===\n");
        s = 0;
        while ( cliSelectRead( &c, 300) > 0) {
                readed = cliRecvDataRaw( &c, buff, sizeof( buff));
                if ( readed <= 0) {
                        printf( "Connection lost\n");
                        break;
                }
                printf( buff);
        }

        printf( "\n");
        cliClose( &c);
        fclose( f);

        cliDone();
        return 0;
}
