//
// Created by FlyZebra on 2021/9/15 0015.
//
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "ServerManager.h"
#include "TerminalServer.h"
#include "RemoteServer.h"


static volatile bool is_stop = false;

static struct sigaction gOrigSigactionINT;
static struct sigaction gOrigSigactionHUP;

static void signalCatcher(int32_t signum)
{
    printf("recv ctrl+c signal [%d]\n", signum);
    is_stop = true;
    switch (signum) {
    case SIGINT:
    case SIGHUP:
        sigaction(SIGINT, &gOrigSigactionINT, NULL);
        sigaction(SIGHUP, &gOrigSigactionHUP, NULL);
        break;
    default:
        break;
    }
}

static int32_t configureSignals() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalCatcher;
    if (sigaction(SIGINT, &act, &gOrigSigactionINT) != 0) {
        int32_t err = -errno;
        printf("Unable to configure SIGINT handler: %s\n", strerror(errno));
        return err;
    }
    if (sigaction(SIGHUP, &act, &gOrigSigactionHUP) != 0) {
        int32_t err = -errno;
        printf("Unable to configure SIGHUP handler: %s\n", strerror(errno));
        return err;
    }
    return 0;
}

int32_t main(int32_t  argc,  char*  argv[])
{
    printf("main server is start.\n");

    is_stop = false;
    if (configureSignals() != 0) {
        printf("configure Signals failed!\n");
    }
    ServerManager *manager = new ServerManager();
    TerminalServer *terminal = new TerminalServer(manager);
    RemoteServer *remote = new RemoteServer(manager);
    while(!is_stop){
        sleep(1);
    }
    delete terminal;
    delete manager;
    printf("main server is end.\n");
}

