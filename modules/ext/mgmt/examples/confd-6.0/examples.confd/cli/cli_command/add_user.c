
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include <stdio.h>

#include "confd_lib.h"
#include "confd_maapi.h"

int is_number(char *str) {
    int i;
    for(i=0 ; str[i] != 0 ; i++)
        if (str[i] < '0' || str[i] > '9')
            return 0;

    return 1;
}

int main(int argc, char **argv)
{
    int msock;
    int debuglevel = CONFD_DEBUG;
    struct sockaddr_in addr;
    int usid, thandle;
    char user[255], *pwd, home[255], sshdir[255], gid[255], uid[255];
    char buf[BUFSIZ];

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4565);

    confd_init("add_user", stderr, debuglevel);
    if ((msock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (maapi_connect(msock, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd: %s\n",
                    confd_lasterr());

    usid    = atoi(getenv("CONFD_MAAPI_USID"));
    thandle = atoi(getenv("CONFD_MAAPI_THANDLE"));

    if ((maapi_attach2(msock, 0, usid, thandle)) != CONFD_OK)
        confd_fatal("Failed to attach: %s\n", confd_lasterr());

  again0:
    printf("Adding a user\n");
    printf("Username: ");
    if ((fgets(user,255, stdin)) == NULL)
        exit(1);
    user[strlen(user)-1] = 0;
    if (maapi_cd(msock,thandle,"/aaa:aaa/authentication/users") != CONFD_OK)
        confd_fatal("cannot CD: %s", confd_lasterr());

    /* check if user exists */
    if (maapi_exists(msock, thandle,"user{%s}", user) == 1) {
        printf("user %s already exists: %s\n", user, confd_lasterr());
        goto again0;
    }

  again:
    if ((pwd = getpass("Password: ")) == NULL)
        exit(1);
    strcpy(buf, pwd);
    if ((pwd = getpass("Confirm password: ")) == NULL)
        exit(1);
    if (strcmp(pwd, buf) != 0) {
        printf("Password not confirmed\n");
        goto again;
    }

    printf("Home: ");
    if ((fgets(home,255,stdin)) == NULL)
        exit(1);
    home[strlen(home)-1] = 0;


    printf("SSH dir: ");
    if ((fgets(sshdir, 255, stdin)) == NULL)
        exit(1);
    sshdir[strlen(sshdir)-1] = 0;

 again_uid:
    printf("uid: ");
    if ((fgets(uid, 255, stdin)) == NULL)
        exit(1);
    uid[strlen(uid)-1] = 0;

    if (!is_number(uid)) {
        printf("Illegal uid\n");
        goto again_uid;
    }

 again_gid:
    printf("gid: ");
    if ((fgets(gid, 255, stdin)) == NULL)
        exit(1);
    gid[strlen(gid)-1] = 0;

    if (!is_number(gid)) {
        printf("Illegal gid\n");
        goto again_gid;
    }

    if (maapi_create(msock,thandle,"user{%s}",user) != CONFD_OK)
        confd_fatal("failed to create user %s: %s\n", user, confd_lasterr());

    if (maapi_set_elem2(msock,thandle,sshdir,"user{%s}/ssh_keydir",user) !=
        CONFD_OK)
        confd_fatal("failed to set ssh keydir: %s\n", confd_lasterr());

    if (maapi_set_elem2(msock,thandle,pwd,"user{%s}/password",user) !=
        CONFD_OK)
        confd_fatal("failed to set password: %s\n", confd_lasterr());

    if (maapi_set_elem2(msock,thandle,home,"user{%s}/homedir",user) !=
        CONFD_OK)
        confd_fatal("failed to set home: %s\n", confd_lasterr());

    if (maapi_set_elem2(msock,thandle,gid,"user{%s}/gid",user) !=
        CONFD_OK)
        confd_fatal("failed to set gid: %s\n", confd_lasterr());

    if (maapi_set_elem2(msock,thandle,uid,"user{%s}/uid",user) !=
        CONFD_OK)
        confd_fatal("failed to set uid: %s\n", confd_lasterr());

    printf("user %s added successfully \n", user);
    exit(0);
}

