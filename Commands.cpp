#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <utime.h>
#include <sys/time.h>
#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";

static char* AllocNCopy(const char* const str)
{
	if (str == NULL) {
		return NULL;
	}
	char* cpy = (char*) malloc( sizeof(char) * (strlen(str)+1) );
  char placeHolder = '_';
	for (int i=0; i<(int)strlen(str)+1; i++) {
		cpy[i]=placeHolder;
	}
	strcpy(cpy,str);
	return cpy;
}
static bool stringIsNum(const std::string& str)
{
  int num=0;
  if (str.size()==0) {return false;}

  try {num= stoi(str);}
  catch (const std::invalid_argument& e) {return false;}
  if (to_string(num) != str) {return false;}
  
  return true;
}
static unsigned int getNumOfWords(const char* const cmd_line)
{
  if (cmd_line == nullptr) {return 0;}
  unsigned int num = 0;
  std::string tmp_word = "";
	for (const char* ptr = cmd_line; *ptr != '\0'; ++ptr) {
		if (*ptr==' ' || *ptr=='\t') {
			if (tmp_word != std::string("")) {
				++num;
				tmp_word=="";
			}
		} else {
			tmp_word.push_back(*ptr);
		}
	}
  if (tmp_word!="") {++num;}
  return num;
}
static bool ContainRedirectionSign(const char* cmd_line)
{
  std::string cmd_string = string(cmd_line);
  for (char c : cmd_string)
  {
    if(c == '>')
      return true;
  }
  return false;
}
static bool ContainPipeSign(const char* cmd_line)
{
  std::string cmd_string = string(cmd_line);
  for (char c : cmd_string)
  {
    if(c == '|')
      return true;
  }
  return false;
}
static std::string AddSpaces(const std::string& str)
{
  std::string spacesStr = "";
  for (unsigned int i=0; i<str.size(); ++i) {
    bool addSpaceBefore = false;
    bool addSpaceAfter = false;
    bool noWhitespaceBefore = (i!=0 && WHITESPACE.find(str[i-1])==std::string::npos);
    char c = str[i];
     
    if (c=='<') {
      addSpaceBefore= noWhitespaceBefore;
      addSpaceBefore= true;
    } else if (c=='&') {
      addSpaceBefore= false;
      addSpaceAfter= true;
    } else if (c=='>') {
      addSpaceBefore= (noWhitespaceBefore && i>0 && str[i-1]!='>');
      addSpaceAfter= (i+1!=str.size() && str[i+1]!='>');
    } else if (c=='|') {
      addSpaceBefore= noWhitespaceBefore;
      addSpaceAfter= (i+1!=str.size() && str[i+1]!='&');
    }

    if (addSpaceBefore) {spacesStr.push_back(' ');}
    spacesStr.push_back(c);
    if (addSpaceAfter) {spacesStr.push_back(' ');}
  }
  return spacesStr;
}
inline static bool signalIsValid(int signum)
{
  return signum>=0 && signum<=31;
}

static std::string _replaceTabWithSpace(const std::string& s)
{
  std::string res = s;
  for (std::string::iterator c = res.begin(); c != res.end(); ++c) {
    if (*c=='\t') {
      *c=' ';
    }
  }
  return res;
}
static std::string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}
static std::string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}
static std::string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}
static int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}
static bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}
static void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  if (idx == string::npos) {
    return;
  }
  if (cmd_line[idx] != '&') {
    return;
  }
  cmd_line[idx] = ' ';
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

Command::Command(const char* cmd_line) : cmd_line(cmd_line),pid(getpid()),background(false), child(false)
{
  string cmd_s = _trim(_replaceTabWithSpace(Command::cmd_line));
  if (cmd_s.size()==0 || cmd_s=="\n") {
    return;
  }
  
  std::string cmd_s_spaced = AddSpaces(Command::cmd_line);

  char* cmd_s_copy = AllocNCopy(cmd_s_spaced.c_str());
  _removeBackgroundSign(cmd_s_copy);
  char* args[COMMAND_MAX_ARGS];
	for (int i=0; i<COMMAND_MAX_ARGS; i++) {
		args[i]=NULL;
	}
	int numOfWords = _parseCommandLine(cmd_s_copy,args);
	
	for (int i=0; i<numOfWords; i++) {
		argv.push_back(args[i]);
	}

  free(cmd_s_copy);
	for (int i=0; i<numOfWords; i++) {
		free(args[i]);
	}
}

void JobsList::JobEntry::killjob(unsigned int signum)
{
  kill(cmd->getPid(),signum);
  // if (kill(cmd->getPid(),signum)==failed) {
  //   perror("smash error: kill failed");
  
  //way2: throw std::runtime_error("smash error: kill failed");
  // }
  
}
void JobsList::addJob(Command* cmd, bool isStopped)
{
  for (JobEntry& job : jobs) {
    if (job.isHidden()) {
      job.reveal();
      job.stop();
      return;
    }
  }
  jobs.push_back(JobEntry(cmd,isStopped,getNextJobID()));
  incNextJobID();
}
void JobsList::printJobsList() const
{
  for (const JobEntry& job : jobs) {
    job.print();
  }
}
void JobsList::killAllJobs()
{
  for (JobEntry& job : jobs) {
    job.killjob(SIGKILL);
    std::cout<<job.getPid()<<": "<<job.getCmd()->getCmdLine()<<std::endl;
  }
  jobs.clear();
}
void JobsList::removeFinishedJobs()
{
  for (std::list<JobEntry>::const_iterator it = jobs.cbegin(); it!=jobs.cend(); ) {
    if (it->isFinished()) {
      it = jobs.erase(it);
    } else {
      ++it;
    }
  }
  resetNextJobID();
}
void JobsList::JobEntry::print() const
{
  std::cout<<"["<<jobID<<"] "<<cmd->getCmdLine()<<" : "<<getPid()
    <<" "<<(unsigned long)calcPassedTime()<<" secs";
  if (isStopped()) {
    std::cout<<" (stopped)";
  }
  std::cout<<std::endl;
}
JobsList::JobEntry* JobsList::getJobById(int jobId)
{
  for (JobsList::JobEntry& job : jobs) {
    if (job.getJobID() == (unsigned int)jobId) {
      return &job;
    }
  }
  return nullptr;
}
JobsList::JobEntry* JobsList::getJobByPid(int jobPid)
{
  for (JobsList::JobEntry& job : jobs) {
    if (job.getPid() == jobPid) {
      return &job;
    }
  }
  return nullptr;
}
void JobsList::removeJobById(int jobId)
{
  for (std::list<JobEntry>::iterator it = jobs.begin(); it!=jobs.end(); ++it) {
    if (it->getJobID()==(unsigned int)jobId) {
      it->hide();
      return;
    }
  }
}
JobsList::JobEntry* JobsList::getLastJob(int* lastJobId)
{
  updateJobsStatus();
  if (jobs.size() == 0) {return nullptr;}
  JobEntry& last_job = jobs.back();
  if (lastJobId) {*lastJobId= last_job.getJobID();}
  return &last_job;
}
JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId)
{
  updateJobsStatus();
  for (std::list<JobEntry>::reverse_iterator rit=jobs.rbegin(); rit!=jobs.rend(); ++rit) {
    if (rit->isStopped()) {
      if (jobId) {*jobId= (*rit).getJobID();}
      return &(*rit);
    }
  }
  return nullptr;
}
void JobsList::updateJobsStatus()
{
  for (std::list<JobEntry>::iterator it = jobs.begin(); it!=jobs.end();++it) {
    int wstatus = 0;
    pid_t pid = waitpid(it->getPid(),&wstatus, WNOHANG|WSTOPPED|WCONTINUED);
    //cout<<"pid="<<pid<<" FIN="<<WIFEXITED(wstatus)<<" STP="<<WIFSTOPPED(wstatus)<<" CNT="<<WIFCONTINUED(wstatus)<<" SIG="<<WIFSIGNALED(wstatus)<<endl;
    if (pid==-1) {it->finish();}
    if (pid>0) {
      if (WIFEXITED(wstatus)) {
        it->finish();
      }
      else if (WIFSIGNALED(wstatus)) {
        pid_t pid_test = waitpid(it->getPid(),NULL,WNOHANG);
        if (pid_test == -1) {//killed
          it->finish(); 
        }
      }
      else if (WIFSTOPPED(wstatus)) {
        it->stop();
      }
      else if (WIFCONTINUED(wstatus)) {
        it->cont();
        if (!it->runningByBG && !it->cmd->isBackground()) {
          SmallShell& shell = SmallShell::getInstance();
          shell.bringToFG(it->getJobID());
        }
      }
    }
  }
  removeFinishedJobs();
}

void ChangePromptCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();

  if (getNumOfArgs() == 0) {
    shell.setPrompt(string("smash"));
  } else {
    shell.setPrompt(std::string(argv[1]));
  }
}
void ShowPidCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  std::cout<<"smash pid is "<<shell.getShellPid()<<std::endl;
}
void GetCurrDirCommand::execute()
{
  char* path = getcwd(nullptr,0);
  if (!path) {
    perror("smash error: getcwd failed");
  }
  std::cout<<path<<std::endl;
  free(path);
}
void ChangeDirCommand::execute()//<<<<<<<<<<<<<<<<<<<<<<<<<<<<NOOOOOOOOOOT DONE>>>>>>>>>>>>>>>>>>>>>>>>>>>>
{
  // if (getNumOfArgs() == 0) {//check in piazza
  //   //std::cerr<<"smash error: cd: no arguments"<<std::endl;
  //   return;
  // }
  if (getNumOfArgs() > 1) {
    std::cerr<<"smash error: cd: too many arguments"<<std::endl;
    return;
  }
  
  SmallShell& shell=SmallShell::getInstance();
  if (strcmp(argv[1].c_str(),"-")==0) {
    if (shell.getLastPwd()=="") {
      std::cerr<<"smash error: cd: OLDPWD not set"<<std::endl;
    } else {
      execute_aux(shell,shell.getLastPwd());
    }
  } else {
    execute_aux(shell,argv[1].c_str());
  }
}
void ChangeDirCommand::execute_aux(SmallShell& shell,const std::string& dst)
{
  std::string old_currpwd(getcwd(nullptr,0));
  if (chdir(dst.c_str()) == -1) {
    perror("smash error: chdir failed");
    return;
  }
  shell.setLastPwd(old_currpwd);
}
void JobsCommand::execute()
{
  SmallShell& shell = SmallShell::getInstance();
  JobsList& jobs = shell.getJobsList();
  jobs.updateJobsStatus();
  jobs.printJobsList();
}
void KillCommand::execute()
{
  bool invalid = false;
  SmallShell& shell = SmallShell::getInstance();
  JobsList& jobs = shell.getJobsList();
  if (getNumOfArgs() != 2) {invalid= true;}
  // else if (stringIsNum(argv[2]) && jobs.getJobById(stoi(argv[2])) == nullptr) {
  //   std::cerr<<"smash error: kill: job-id "<<argv[2]<<" does not exist"<<std::endl;
  //   return;
  // }
  else if (argv[1].size()<2 || argv[1][0]!='-') {invalid= true;}
  // else if (stringIsNum(argv[2]) && jobs.getJobById(stoi(argv[2])) == nullptr) {
  //   std::cerr<<"smash error: kill: job-id "<<argv[2]<<" does not exist"<<std::endl;
  //   return;
  // }
  else if (!stringIsNum(argv[1].substr(1)) || !stringIsNum(argv[2])) {invalid= true;}
  else if ( !signalIsValid( abs(stoi(argv[1])) ) ) {invalid= true;}

  if (invalid) {
    std::cerr<<"smash error: kill: invalid arguments"<<std::endl;
    return;
  }
  
  jobs.updateJobsStatus();
  unsigned int signum = stoi(argv[1].substr(1));
  JobsList::JobEntry* job = jobs.getJobById(stoi(argv[2]));
  if (job!=nullptr) {
    job->killjob(signum);
    std::cout<<"signal number "<<signum<<" was sent to pid "<<job->getPid()<<std::endl;
    job->setRunningByBG(false);
    jobs.updateJobsStatus();
  } else {
    std::cerr<<"smash error: kill: job-id "<<argv[2]<<" does not exist"<<std::endl;
  }
}
void ForegroundCommand::execute()
{
  SmallShell& shell = SmallShell::getInstance();
  JobsList& jobs = shell.getJobsList();
  jobs.updateJobsStatus();
  JobsList::JobEntry* job = nullptr;
  if (getNumOfArgs()==0) {
    job = jobs.getLastJob(NULL);
    if (job==nullptr) {
      std::cerr<<"smash error: fg: jobs list is empty"<<std::endl;
      return;
    }
  }
  else if (getNumOfArgs()!=1 || !stringIsNum(argv[1])) {
    std::cerr<<"smash error: fg: invalid arguments"<<std::endl;
    return;
  }
  else {
    unsigned int jobID = stoi(argv[1]);
    job = jobs.getJobById(jobID);
    if (job==nullptr) {
      std::cerr<<"smash error: fg: job-id "<<argv[1]<<" does not exist"<<std::endl;
      return;
    }
  }
  
  std::cout<<job->getCmd()->getCmdLine()<<" : "<<job->getPid()<<std::endl;
  job->setRunningByBG(false);
  shell.bringToFG(job->getJobID());
}
void BackgroundCommand::execute()
{
  SmallShell& shell = SmallShell::getInstance();
  JobsList& jobs = shell.getJobsList();
  jobs.updateJobsStatus();
  JobsList::JobEntry* job = nullptr;
  if (getNumOfArgs()==0) {
    job = jobs.getLastStoppedJob(NULL);
    if (job ==nullptr) {
      std::cerr<<"smash error: bg: there is no stopped jobs to resume"<<std::endl;
      return;
    }
  }
  else if (getNumOfArgs()!=1 || !stringIsNum(argv[1])) {
    std::cerr<<"smash error: bg: invalid arguments"<<std::endl;
    return;
  }
  else {
    unsigned int jobID = stoi(argv[1]);
    job = jobs.getJobById(jobID);
    if (job == nullptr) {
      std::cerr<<"smash error: bg: job-id "<<argv[1]<<" does not exist"<<std::endl;
      return;
    }
    if (!job->isStopped()) {
      std::cerr<<"smash error: bg: job-id "<<argv[1]<<" is already running in the background"<<std::endl;
      return;
    } 
  }

  std::cout<<job->getCmd()->getCmdLine()<<" : "<<job->getPid()<<std::endl;
  job->setRunningByBG(true);
  job->killjob(SIGCONT); 
  job->cont();
}
void QuitCommand::execute()
{
  SmallShell &smash = SmallShell::getInstance();
  if (getNumOfArgs()>0 && argv[1]=="kill") {
    JobsList& jobs = smash.getJobsList();
    jobs.updateJobsStatus();
    // jobs.updateJobsStatus();
    // jobs.updateJobsStatus();
    std::cout<<"smash: sending SIGKILL signal to "<<jobs.getNumOfJobs()<<" jobs:"<<std::endl;      
    jobs.killAllJobs();
  }
	smash.setStatus(false);
}

void HeadCommand::execute()
{  
  if(getNumOfArgs() < 1) {
    std::cerr << "smash error: head: not enough arguments\n";
    return;
  }

  int num_of_lines;
  getNumOfArgs() == 2 ? num_of_lines = abs(stoi(argv[1])) : num_of_lines = N; 

  std::string file_name = argv[getNumOfArgs()];
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd == -1) {
    std::cerr << "smash error: open failed\n";
    return;
  } 

  char buff[1]; 
  int curr_line = 0;
  while(curr_line < num_of_lines && read(fd, buff, 1)) {
    std::cout << buff;
    if(buff[0] == '\n')
      curr_line++;
  }

  if (close(fd) == -1) {
    std::cerr << "smash error: close failed\n";
    return;
  }
}

void TailCommand::execute()
{ 
  if(getNumOfArgs() < 1) {
    std::cerr << "smash error: tail: invalid arguments\n";
    return;
  }

  if(getNumOfArgs() == 2 && stoi(argv[1]) > 0) {
    std::cerr << "smash error: tail: invalid arguments\n";
    return;
  }

  int num_of_lines;
  getNumOfArgs() == 2 ? num_of_lines = abs(stoi(argv[1])) : num_of_lines = N; 

  std::string file_name = argv[getNumOfArgs()];
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd == -1) {
    std::cerr << "smash error: open failed: No such file or directory\n";
    return;
  } 

  char buff[1]; 
  int file_lines_count = 1;
  while(read(fd, buff, 1)) {
    if(buff[0] == '\n')
      file_lines_count++;
  }

  if (close(fd) == -1) {
    std::cerr << "smash error: close failed\n";
    return;
  }

  fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd == -1) {
    std::cerr << "smash error: open failed\n";
    return;
  } 
 
  if(num_of_lines >= file_lines_count) {
    int curr_line = 0;
    while(read(fd, buff, 1)) {
      std::cout << buff;
      if(buff[0] == '\n')
        curr_line++;
    }

    if (close(fd) == -1) {
      std::cerr << "smash error: close failed\n";
    }
    return;
  }

  int curr_line = 0;
  int starting_line = file_lines_count - num_of_lines;
  while(curr_line < file_lines_count && read(fd, buff, 1)) {
    if(curr_line >= starting_line) {
      std::cout << buff;
    }
    if(buff[0] == '\n')
      curr_line++;
  }

  if (close(fd) == -1) {
    std::cerr << "smash error: close failed\n";
    return;
  }
}


void TouchCommand::execute()
{ 
  if(getNumOfArgs() != 2) {
    std::cerr << "smash error: touch: invalid arguments\n";
    return;
  }

  int i = 0;
  int date[6] = {0};
  std::string temp = argv[getNumOfArgs()];
  while(int(temp.find(':')) != -1) { 
    date[i++] = atoi(temp.c_str());
    temp.erase(0, temp.find(':') + 1);
  }
  date[i++] = atoi(temp.c_str());

  std::string file_name = argv[getNumOfArgs() - 1];
  int fd = open(file_name.c_str(), O_RDONLY, 0666);
  if (fd == -1) {
    std::cerr << "smash error: utime failed: No such file or directory\n";
    return;
  } 

  // for(int i = 0; i < 6; i++)
  //   cout << "date " << i << " = " << date[i] << endl;

  struct tm time = {
     date[0], date[1], date[2], date[3], date[4]-1, date[5]-1900,0,0,0
  };
  time_t updated_time = mktime(&time);
  struct utimbuf t = {0, updated_time};
  utime(file_name.c_str(), &t);

  if (close(fd) == -1) {
    perror("smash error: close failed");
    return;
  }
}

void ExternalCommand::execute()
{
  pid_t child_pid = fork();
  if (child_pid == -1) {
    perror("smash error: fork failed");
    return;
  }
  if (child_pid!=0) {//parent
    setPid(child_pid);
    SmallShell& shell = SmallShell::getInstance();
    shell.setCurrCmd(this);
    if (isBackground()) {
      shell.getJobsList().addJob(this,false);
    } else {
      waitpid(pid,NULL,WSTOPPED);
    }
  } else {//child
    if (setpgrp()<0) {
      perror("smash error: setgrp failed");
      exit(0);
    }
    signal(SIGTSTP,SIG_DFL);
    signal(SIGINT,SIG_DFL);

    char* argv[4];
    argv[0]=AllocNCopy("/bin/bash");
    argv[1]=AllocNCopy("-c");
    argv[2]=AllocNCopy(cmd_line.c_str());
    if (isBackground()) {
      _removeBackgroundSign(argv[2]);
    }
    argv[3]=NULL;
		execv("/bin/bash",argv);
    perror("smash error: execv failed");
    exit(0);
	}
}
//next to check
void RedirectionCommand::execute()//BY Ameer //after perror is mine
{
  SmallShell& shell =  SmallShell::getInstance();
  string file_name = argv[shell.getCurrCmd()->getNumOfArgs()];
  int new_fd = dup(1);
  if (new_fd == -1) {
    perror("smash error: dup failed");
    return;
  }
  if (close(1) < 0) {
    perror("smash error: close failed");
    dup2(new_fd, 1);
    close(new_fd);
    return;
  }

  int sign_pos = 0;
  for(int i = 0; i <= shell.getCurrCmd()->getNumOfArgs(); i++) {
    if(argv[i] == ">" || argv[i] == ">>")
      sign_pos = i;
  }

  bool toAppend = argv[sign_pos] == ">>" ? true : false;
  int fd = -1;
  if (toAppend) {
    fd = open(file_name.c_str(), O_WRONLY | O_APPEND | O_CREAT, 0666);
  } else {
    fd = open(file_name.c_str(), O_WRONLY | O_TRUNC | O_CREAT, 0666);
  }
  if (fd < 0) {
    perror("smash error: open failed");
    dup2(new_fd, 1);
    close(new_fd);
    return;
  }

  std::string cmd = "";
  for(int i = 0; i < sign_pos; i++) {
    cmd+= argv[i];
    if (i != sign_pos-1) {cmd+= " ";}
  }  
  SmallShell::getInstance().executeCommand(cmd.c_str());
  // try {SmallShell::getInstance().executeCommand(cmd.c_str());}
  // catch (const Command::InvalidCommandException&){
  //   perror("Command::InvalidCommandException");
  //   dup2(new_fd, 1);
  //   close(new_fd);
  //   close(fd);
  //   return;
  // }

  if (close(fd) < 0) {
    perror("smash error: close failed");
    dup2(new_fd, 1);
    close(new_fd);
    return;
  }
  if (dup2(new_fd, 1) == -1) {
    perror("smash error: dup2 failed");
    return;
  }
  if (close(new_fd) < 0) {
    perror("smash error: close failed");
    return;
  }
}
//next to check
void PipeCommand::execute()//BY Ameer
{
  SmallShell& shell =  SmallShell::getInstance();

  int sign_pos;
  for(int i = 0; i <= shell.getCurrCmd()->getNumOfArgs(); i++) {
    if(argv[i] == "|" || argv[i] == "|&") {
      sign_pos = i;
      break;
    }
  }

  std::string cmd1 = "", cmd2 = "";
  for(int i = 0; i < sign_pos; i++) {
    cmd1 += argv[i];
    i == sign_pos - 1 ? cmd1 += "" : cmd1 += " ";
  }    
  for(int i = sign_pos + 1; i <= shell.getCurrCmd()->getNumOfArgs(); i++) {
    cmd2 += argv[i];
    i == sign_pos - 1 ? cmd2 += "" : cmd2 += " ";
  }  

  bool contain_amp;
  argv[sign_pos] == "|&" ?  contain_amp = true : contain_amp = false;
 
  int fd[2];
  if (pipe(fd) == -1) {
    perror("smash error: pipe failed");
    return;
  }

  pid_t pid1 = fork();
  if (pid1 == 0) //first child
  {
    setChild(true);
    if (!contain_amp)
    {
      if (dup2(fd[1], 1) == -1) {
        perror("smash error: dup2 failed");
        return;
      }
    }
    else {
      if (dup2(fd[1], 2) == -1) {
        perror("smash error: dup2 failed");
        return;
      }
    }
    if (close(fd[0]) == -1 || close(fd[1]) == -1) {
      perror("smash error: close failed");
      return;
    }
    SmallShell::getInstance().executeCommand(cmd1.c_str());
    if (!contain_amp)  
      close(1);
    else
      close(2);
    return;
  }

  pid_t pid2 = fork();
  if (pid2 == 0) //second child
  {
    setChild(true);
    if (dup2(fd[0], 0) == -1) {
      perror("smash error: dup2 failed");
      return;
    }
    if (close(fd[0]) == -1 || close(fd[1]) == -1) {
      perror("smash error: close failed");
      return;
    }
    SmallShell::getInstance().executeCommand(cmd2.c_str());
    close(0);
    return;
  }

  close(fd[0]);
  close(fd[1]);
  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);
}

//////////////////////////////////////////////////////////

SmallShell::SmallShell() : prompt("smash"),activated(true),
  lastPwd(""),currCmd(nullptr), shell_pid(getpid()) {}
SmallShell::~SmallShell()
{
  // cout<<"________________cd with 0 args\n";
  // cout<<"Handle Errors,check numOArgs,segmentation faults,new delete,syscalls fails";
  removeCurrCmd();
}
Command * SmallShell::CreateCommand(const char* cmd_line)
{
  if (cmd_line == nullptr) {
    return nullptr;
  }
  unsigned int numOfWords = getNumOfWords(cmd_line);
  if (numOfWords==0){
    return nullptr;
  }
  // char* cmd_line_final = AllocNCopy(cmd_line);
  // _removeBackgroundSign(cmd_line_final);
  string cmd_s = _trim(_replaceTabWithSpace(string(cmd_line)));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  //if (firstWord.back()=='&') {firstWord.pop_back();} they didnt want this

  //wanted to take "timeout -t " away and continue with the command, instead of calling shell::execute
  //, but then no nested "timeout" available

  //timeout <duration> <command>
  //way1: call smah.execute on the inner command after prep
  //ways: prep and continue with the innner command

  if (firstWord.compare("timeout") == 0) { 
    if (numOfWords<3) 
      {return nullptr;}

    cmd_s= _trim(cmd_s.substr(firstWord.size()+1));
    string secondWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (secondWord[0]=='-' || !stringIsNum(secondWord)) 
      {return nullptr;}

    // std::cout<<"smash: got an alarm"<<std::endl;
    unsigned int timer = stoi(secondWord);
    unsigned int oldTimer = alarm(timer);
    cmd_s= _trim(cmd_s.substr(secondWord.size()+1));
    // std::cout << "smash: timeout "<< secondWord <<" "<< cmd_s <<" timed out!"<<std::endl;
    executeCommand(cmd_s.c_str());
    alarm(oldTimer);
    return nullptr;
  }
  //else 
   if (ContainRedirectionSign(cmd_line)) {
    return new RedirectionCommand(cmd_line);
  }
  else if (ContainPipeSign(cmd_line)) {
    return new PipeCommand(cmd_line);
  }
  else if (firstWord.compare("head") == 0) { 
    return new HeadCommand(cmd_line);
  }
  else if (firstWord.compare("tail") == 0) {
    return new TailCommand(cmd_line);
  }
  else if (firstWord.compare("touch") == 0) {
    return new TouchCommand(cmd_line);
  }
  else if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line);
  }
  else if (firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0) {
    return new KillCommand(cmd_line);
  }
  else if (firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line);
  }
  else if (firstWord.compare("bg") == 0) {
    return new BackgroundCommand(cmd_line);
  }
  else if (firstWord.compare("quit") == 0) {
    return new QuitCommand(cmd_line);
  }
  
  return new ExternalCommand(cmd_line);
}

void SmallShell::executeCommand(const char *cmd_line)
{
  if (cmd_line == nullptr) {
    return;
  }
  if (strlen(cmd_line)==0 || strcmp(cmd_line,"\n")==0) {
    return;
  }

  bool isBackground = false;
  Command* cmd = nullptr;

  if (_isBackgroundComamnd(cmd_line)) {
    char* cmd_line_copy = AllocNCopy(cmd_line);
    _removeBackgroundSign(cmd_line_copy);
     
    isBackground = true;
  }
  cmd= CreateCommand(cmd_line);
  // printf("cmd = %d\n",);
  if (cmd != nullptr) {
    cmd->setBackground(isBackground);
    // std::cout << isBackground << std::endl;
    setCurrCmd(cmd);
  	cmd->execute();
    if(cmd->isChild()){
      setStatus(false);
    }
    removeCurrCmd();
	  delete cmd;
  }
}

void SmallShell::removeCurrCmd()
{
  if (currCmd != nullptr) {
    delete currCmd;
    currCmd= nullptr;
  }
}
void SmallShell::bringToFG(unsigned int jobID)
{
  const Command* const cmd = jobs.getJobById(jobID)->getCmd();
  pid_t pid = cmd->getPid();
  setCurrCmd(cmd);
  jobs.removeJobById(jobID);
  kill(pid,SIGCONT);
  waitpid(pid,NULL,WSTOPPED);
}
