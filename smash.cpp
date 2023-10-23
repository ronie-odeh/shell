#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

using namespace std;

int main(int argc, char* argv[]) {
    if (signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
        return 0;
    }
    if (signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
        return 0;
    }
    
    struct sigaction sigact;
    sigact.sa_handler = alarmHandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = SA_RESTART|SA_SIGINFO;
    if (sigaction(SIGALRM, &sigact, NULL)==-1) {
        perror("smash error: failed to set alarm handler");
        return 0;
    }

    SmallShell& smash = SmallShell::getInstance();
    
    while(smash.isRunning()) { 
        std::cout<<smash.getPrompt()<<"> "; 
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}
