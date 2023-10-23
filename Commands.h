#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>
#include <list>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using std::string;
using std::vector;

class Command;
class BuiltInCommand;
class ExternalCommand;
class PipeCommand;
class RedirectionCommand;
class ChangePromptCommand;
class ShowPidCommand;
class GetCurrDirCommand;
class ChangeDirCommand;
class QuitCommand;
class JobsList; 
class JobsCommand;
class KillCommand;
class ForegroundCommand;
class BackgroundCommand;
class HeadCommand;
class TailCommand;
class TouchCommand;
class SmallShell;

class Command {
 protected:
	vector<string> argv;
  std::string cmd_line;
  pid_t pid;
  bool background;
  bool child;
 public:
  Command(const char* cmd_line);
  virtual ~Command() = default;
  virtual void execute() = 0;
  virtual Command* clone() const = 0;
  const std::string& getCmdLine() const {return cmd_line;}
  int getNumOfArgs() const {return argv.size()-1;}
  const pid_t& getPid() const {return pid;}
  void setPid(pid_t pid) {Command::pid= pid;}
  bool isBackground() const {return background;}
  void setBackground(const bool& background) {Command::background= background;}
  bool isChild() const {return child;}
  void setChild(const bool& child) {Command::child= child;}
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line): Command(cmd_line) {}
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~ExternalCommand() {}
  void execute() override;
  Command* clone() const override {return new ExternalCommand(*this);}
};

class PipeCommand : public Command {
 public:
  PipeCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~PipeCommand() {}
  void execute() override;
  Command* clone() const override {return new PipeCommand(*this);}
};

class RedirectionCommand : public Command {
 public:
  explicit RedirectionCommand(const char* cmd_line) : Command(cmd_line) {}
  virtual ~RedirectionCommand() {}
  void execute() override;
  Command* clone() const override {return new RedirectionCommand(*this);}
};

class ChangePromptCommand : public BuiltInCommand {
public:
  ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
  virtual ~ChangePromptCommand() {}
  void execute() override;
  Command* clone() const override {return new ChangePromptCommand(*this);}
};
class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line){}
  virtual ~ShowPidCommand() {}
  void execute() override;
  Command* clone() const override {return new ShowPidCommand(*this);}
};
class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~GetCurrDirCommand() {}
  void execute() override;
  Command* clone() const override {return new GetCurrDirCommand(*this);}
};
class ChangeDirCommand : public BuiltInCommand { 
public:
  ChangeDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ChangeDirCommand() {}
  void execute() override;
  void execute_aux(SmallShell& shell, const std::string& dst);
  Command* clone() const override {return new ChangeDirCommand(*this);}
};
class QuitCommand : public BuiltInCommand {
public:
  QuitCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~QuitCommand() {}
  void execute() override;
  Command* clone() const override {return new QuitCommand(*this);}
};

class JobsList {
 public:
  class JobEntry {
    Command* cmd;
    bool stopped;
    bool finished;
    unsigned int jobID;
    time_t start_time;
    bool hidden;
    bool runningByBG;
    JobEntry(Command* cmd, bool stopped, unsigned int jobID) : cmd(nullptr),stopped(stopped),
     finished(false),jobID(jobID),start_time(time(NULL)),hidden(false),runningByBG(false) {
      if (cmd != nullptr) {JobEntry::cmd = cmd->clone();}
    }
  public:
    ~JobEntry() {delete cmd;}
    JobEntry(const JobEntry& other) : JobEntry(nullptr,other.stopped,other.jobID) {
      if (other.cmd != nullptr) {cmd = other.cmd->clone();}
      finished= other.finished;
    }
    void print() const;
    bool isFinished() const {return finished;}
    bool isStopped() const {return stopped;}
    void finish() {stopped= false; finished= true;}
    void stop() {stopped= true;}
    void cont() {stopped= false;}
    pid_t getPid() const {return cmd->getPid();}
    void setPid(pid_t pid) {cmd->setPid(pid);}
    const Command* const getCmd() const {return cmd;}
    double calcPassedTime() const {return difftime(time(NULL),start_time);}
    unsigned int getJobID() const {return jobID;}
    void killjob(unsigned int signum);
    void hide() {hidden= true;}
    bool isHidden() const {return hidden;}
    void reveal() {if (hidden) {hidden= false;start_time= time(NULL);}}
    void setRunningByBG(bool running) {runningByBG= running;}
    friend JobsList;
  };
 private:
  std::list<JobEntry> jobs;
  unsigned int nextJobID;
  time_t lastOpTime;
 public:
  JobsList() : nextJobID(1) {}
  ~JobsList() = default;
  void addJob(Command* cmd, bool isStopped = false);
  void printJobsList() const;
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry* getJobById(int jobId);
  JobEntry* getJobByPid(int jobPid);
  void removeJobById(int jobId);
  JobEntry* getLastJob(int* lastJobId);
  JobEntry* getLastStoppedJob(int *jobId);
  unsigned int getNextJobID() const {return nextJobID;}
  void incNextJobID() {++nextJobID;}
  void resetNextJobID() {nextJobID= jobs.size()==0 ? 1 : jobs.back().jobID+1;}
  void updateJobsStatus();
  unsigned int getNumOfJobs() const {return jobs.size();}
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~JobsCommand() {}
  void execute() override;
  Command* clone() const override {return new JobsCommand(*this);}
};

class KillCommand : public BuiltInCommand {
 public:
  KillCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~KillCommand() {}
  void execute() override;
  Command* clone() const override {return new KillCommand(*this);}
};

class ForegroundCommand : public BuiltInCommand {
 public:
  ForegroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~ForegroundCommand() {}
  void execute() override;
  Command* clone() const override {return new ForegroundCommand(*this);}
};

class BackgroundCommand : public BuiltInCommand {
 public:
  BackgroundCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~BackgroundCommand() {}
  void execute() override;
  Command* clone() const override {return new BackgroundCommand(*this);}
};

class HeadCommand : public BuiltInCommand {
  const int N = 10;
 public:
  HeadCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~HeadCommand() {}
  void execute() override;
  Command* clone() const override {return new HeadCommand(*this);}
};

class TailCommand : public BuiltInCommand {
  const int N = 10;
 public:
  TailCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~TailCommand() {}
  void execute() override;
  Command* clone() const override {return new TailCommand(*this);}
};

class TouchCommand : public BuiltInCommand {
 public:
  TouchCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
  virtual ~TouchCommand() {}
  void execute() override;
  Command* clone() const override {return new TouchCommand(*this);}
};

class SmallShell {
 private:
  std::string prompt;
  bool activated;
  std::string lastPwd;
  JobsList jobs;
  Command* currCmd;
  pid_t shell_pid;
  SmallShell();
 public:
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
  void setPrompt(const std::string& new_prompt = "smash") {prompt= new_prompt;}
  std::string getPrompt() const {return prompt;}
  void setStatus(const bool& status) {activated = status;}
  bool isRunning() const {return activated;}
  void setLastPwd(const std::string& new_lastPwd) {lastPwd= new_lastPwd;}
  std::string getLastPwd() const {return lastPwd;}
  JobsList& getJobsList() {return jobs;}
  void setCurrCmd(const Command* const cmd) {if (cmd) {removeCurrCmd(); currCmd= cmd->clone();}}
  Command* getCurrCmd() const {return currCmd;}
  void removeCurrCmd();
  void bringToFG(unsigned int jobID);
  typename std::list<JobsList::JobEntry>::iterator bringToFG(JobsList* jobslist, unsigned int jobID, const typename std::list<JobsList::JobEntry>::iterator& start);
  const pid_t& getShellPid() const {return shell_pid;}
};

#endif //SMASH_COMMAND_H_