//
// Created by FlyZebra on 2021/9/15 0015.
//
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include "RemoteManager.h"
#include "TerminalServer.h"


static volatile bool is_stop = false;

static struct sigaction gOrigSigactionINT;
static struct sigaction gOrigSigactionHUP;

static void signalCatcher(int signum)
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

static int configureSignals() {
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = signalCatcher;
    if (sigaction(SIGINT, &act, &gOrigSigactionINT) != 0) {
        int err = -errno;
        printf("Unable to configure SIGINT handler: %s\n", strerror(errno));
        return err;
    }
    if (sigaction(SIGHUP, &act, &gOrigSigactionHUP) != 0) {
        int err = -errno;
        printf("Unable to configure SIGHUP handler: %s\n", strerror(errno));
        return err;
    }
    return 0;
}

int main(int  argc,  char*  argv[])
{
    printf("remote server is start.\n");

    is_stop = false;
    if (configureSignals() != 0) {
        printf("configureSignals failed!\n");
    }
    RemoteManager *manager = new RemoteManager();
    TerminalServer *terminal = new TerminalServer(manager);
    terminal->start();
    while(!is_stop){
        sleep(1);
    }
    terminal->stop();
    delete terminal;
    delete manager;
    printf("remote server is end.\n");
}

