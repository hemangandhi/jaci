#include "jaci.h"

using namespace std;

static vector<string> flags;
static vector<string> include_dirs;
static vector<string> lib_dirs;
static vector<string> libs;

static string cFilename = "__jaci_committed_staged_merged__.c";
static string cRunnable = "__jaci_executable__";

static int quitsig;
void quitprog(int signo) {
  quitsig--;
  if (quitsig == 0) {
    exit(1);
  }
}

void printAbout(void) {
  printf("Jaci v0.0.1\n"
      "Type \"$().help()\" for more information.\n"
      );
}

bool isJaci(string &line) {
  return line.size() >= 4 && line.substr(0, 4) == "$().";
}

string getJaciCmd(string &line) {
  return line.substr(4);
}

void runJaciCmd(string &line, vector<string> &committed) {
  // TODO: implement features
  if (line == "help()\n") {
    printf("\nFunctions to use:\n"
        "\t$().help()\tView the help prompt\n"
        "\t$().exit()\tQuit the program\n"
        "\t$().save(f)\tSave the current program to the filename f\n"
        "\t$().pop()\tRemove the last command off the program list\n"
        "\n");
  } else if (line == "exit()\n") {
    quitsig = 0;
  }
}

int getNumBrace(string &line) {
  int numBrace = 0;
  for (char &c : line) {
    if (c == '{') {
      numBrace++;
    } else if (c == '}') {
      numBrace--;
    }
  }
  return numBrace;
}

void toggleInMain(bool &inMain, string &line, int numBrace) {
  if (numBrace <= 0) {
    inMain = false;
  } else if (line.find("main") != string::npos && (line.find("//") == string::npos ||
      (line.find("//") > line.find("main")))) {
    inMain = true;
  }
}

int runConsoleCmd(string &cmd) {
  int status;
  status = system(cmd.c_str());
  return status;
}

bool compileAndRun(bool run) {
  string compileCmd = "gcc";
  for (string &flag : flags) {
    compileCmd += " " + flag;
  }
  for (string &include_dir : include_dirs) {
    compileCmd += " " + include_dir;
  }
  compileCmd += " -o " + cRunnable;
  if (!run) {
    compileCmd += ".o";
    compileCmd += " -c " + cFilename;
  } else {
    compileCmd += " " + cFilename;
    for (string &lib_dir : lib_dirs) {
      compileCmd += " " + lib_dir;
    }
    for (string &lib : libs) {
      compileCmd += " " + lib;
    }
  }
  if (runConsoleCmd(compileCmd) != 0) { // run the make
    return false;
  }
  if (run) {
    string execCmd = "./" + cRunnable;
    int status = runConsoleCmd(execCmd);
    unlink(cRunnable.c_str());
    if (status != 0) { // run the exec
      return false;
    }
  }
  if (!run) {
    unlink((cRunnable + ".o").c_str());
  }
  return true;
}

bool fileExists(string &fname) {
  struct stat buffer;
  return (stat(fname.c_str(), &buffer) == 0);
}

bool evaluate(vector<string> &committed, vector<string> &staged, bool inMain, int numBrace) {
  // write a program file with the list of c lines
  if (fileExists(cFilename)) {
    unlink(cFilename.c_str());
  }
  ofstream programFile;
  programFile.open(cFilename);
  bool insertStaged = true;
  for (string &line : committed) {
    programFile << line;
  }
  for (string &line : staged) {
    programFile << line;
  }
  if (numBrace > 0 && inMain) {
    numBrace--; // just temporarily
    programFile << "}\n";
  }
  programFile.close();
  // try running the program that we just wrote
  bool status = compileAndRun(inMain);
  unlink(cFilename.c_str());
  if (status) {
    for (string &line : staged) {
      committed.push_back(line);
    }
  }
  staged.clear();
  return status;
}

void insertToStaged(vector<string> &staged, string &line) {
  staged.push_back(line);
}

int main(int argc, char *argv[]) {
  signal(SIGINT, quitprog);
  printAbout();
  int numBrace = 0;
  bool inMain = false;
  vector<string> committed;
  vector<string> staged;

  // start up a process?
  quitsig++;
  while (quitsig > 0) {
    if (numBrace == 0 || (numBrace == 1 && inMain)) {
      cout << ">>> ";
    }

    // read a line from the console
    string cmdline;
    getline(cin, cmdline);
    cmdline += "\n";

    // evaluate to see if it is special
    if (isJaci(cmdline)) {
      string jaciCmd = getJaciCmd(cmdline);
      runJaciCmd(jaciCmd, committed);
    } else {
      // if not special, then evaluate as so
      numBrace += getNumBrace(cmdline);
      toggleInMain(inMain, cmdline, numBrace);
      insertToStaged(staged, cmdline);
      if (numBrace == 0 || (numBrace == 1 && inMain)) {
        // this function does the interpretation
        evaluate(committed, staged, inMain, numBrace);
      }
    }
  }
  return 0;
}
