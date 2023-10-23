#ifndef SMASH__SIGNALS_H_
#define SMASH__SIGNALS_H_

void ctrlZHandler(int sig_num);//SIGTSTP
void ctrlCHandler(int sig_num);//SIGINT
void alarmHandler(int sig_num);//SIGALRM

#endif //SMASH__SIGNALS_H_
