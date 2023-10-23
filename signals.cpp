#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num)
{
  SmallShell& shell = SmallShell::getInstance();  
  std::cout<<"smash: got ctrl-Z"<<std::endl;
  Command* currCmd = shell.getCurrCmd();
  if (currCmd != nullptr) {
    kill(currCmd->getPid(),SIGSTOP);
    shell.getJobsList().addJob(currCmd,true);
    std::cout<<"smash: process "<<currCmd->getPid()<<" was stopped"<<std::endl;
  }
}

void ctrlCHandler(int sig_num)
{
  SmallShell& shell = SmallShell::getInstance();
  std::cout<<"smash: got ctrl-C"<<std::endl;
  Command* currCmd = shell.getCurrCmd();
  if (currCmd != nullptr) {
    kill(currCmd->getPid(),SIGKILL);
    std::cout<<"smash: process "<<currCmd->getPid()<<" was killed"<<std::endl;
  }
}

void alarmHandler(int sig_num)
{
  // TODO: Add your implementation
  SmallShell& shell = SmallShell::getInstance();
  // std::cout<<"alarmHandler"<<std::endl;
  pid_t shell_pid = shell.getShellPid();
  pid_t cmd_pid = shell.getCurrCmd()->getPid();
  // cout<<"shell_pid="<<shell_pid<<" cmd_pid="<<cmd_pid<<endl;
  if (cmd_pid != shell_pid) {
    kill(cmd_pid,SIGKILL);
    std::cout<<"smash: got an alarm"<<std::endl; 
    std::cout << "smash: timeout "<< 3 <<" "<< shell.getCurrCmd()->getCmdLine() <<" timed out!"<<std::endl;
  }
}
