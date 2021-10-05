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
#include "FlyLog.h"


static volatile bool isStop = false;

static struct sigaction gOrigSigactionINT;
static struct sigaction gOrigSigactionHUP;

static void signalCatcher(int32_t signum)
{
    FLOGD("recv ctrl+c exit! signum=[%d]", signum);
    isStop = true;
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
        FLOGD("Unable to configure SIGINT handler: %s", strerror(errno));
        return err;
    }
    if (sigaction(SIGHUP, &act, &gOrigSigactionHUP) != 0) {
        int32_t err = -errno;
        FLOGD("Unable to configure SIGHUP handler: %s", strerror(errno));
        return err;
    }
    return 0;
}

int32_t main(int32_t  argc,  char*  argv[])
{
    FLOGD("main server is start.");
    isStop = false;
    if (configureSignals() != 0) {
        FLOGD("configure Signals failed!");
    }
    ServerManager *manager = new ServerManager();
    TerminalServer *terminal = new TerminalServer(manager);
    RemoteServer *remote = new RemoteServer(manager);
    while(!isStop){
        sleep(1);
    }
    delete terminal;
    delete remote;
    delete manager;
    FLOGD("main server is end.");
}

