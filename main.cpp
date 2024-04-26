#include "http.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <regex>
#include <string>
#include <sys/inotify.h>
#include <unistd.h>
#include <vector>

const std::string CONFIG_DIR =
    std::string(std::getenv("HOME")) + "/.config/todo/";
const std::string TODO_FILE = CONFIG_DIR + "tasks.csv";

struct Task {
  std::string description;
  std::time_t reminder;
};

std::vector<Task> tasks;
bool shouldExit = false; // Flag to signal exit

void sendNotification(const std::string &message) {
  std::string notificationCommand =
      "notify-send \"Todo Reminder\" \"" + message + "\"";
  system(notificationCommand.c_str());
}

void monitorReminders() {
  int fd = inotify_init();
  if (fd == -1) {
    std::cerr << "Failed to initialize inotify\n";
    return;
  }

  int wd = inotify_add_watch(fd, CONFIG_DIR.c_str(), IN_CLOSE_WRITE);
  if (wd == -1) {
    std::cerr << "Failed to watch directory: " << CONFIG_DIR << "\n";
    return;
  }

  while (!shouldExit) { // Check the exit flag
    char buffer[1024];
    int length = read(fd, buffer, sizeof(buffer));
    if (length < 0) {
      std::cerr << "Failed to read inotify event\n";
      break;
    }

    std::time_t currentTime = std::time(nullptr);
    for (const auto &task : tasks) {
      if (task.reminder != 0 && task.reminder - currentTime <= 300) {
        sendNotification(task.description);
      }
    }
  }

  inotify_rm_watch(fd, wd);
  close(fd);
}

void loadTasks() {
  std::ifstream file(TODO_FILE);
  tasks.clear();
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      std::string description;
      std::time_t reminder = 0;
      size_t delimiterPos = line.find("|");
      if (delimiterPos != std::string::npos) {
        description = line.substr(0, delimiterPos);
        reminder = std::stoll(line.substr(delimiterPos + 1));
      } else {
        description = line;
      }
      tasks.push_back({description, reminder});
    }
    file.close();
  }
}

void saveTasks() {
  std::ofstream file(TODO_FILE);
  if (file.is_open()) {
    for (const auto &task : tasks) {
      file << task.description << "|" << task.reminder << "\n";
    }
    file.close();
  }
}

std::string formatHTML(const std::string& text) {
    std::string formattedText = text;

    static const std::regex boldRegex(R"(\[b\](.*?)\[/b\])");
    formattedText = std::regex_replace(formattedText, boldRegex, "<b>$1</b>");

    static const std::regex italicRegex(R"(\[i\](.*?)\[/i\])");
    formattedText = std::regex_replace(formattedText, italicRegex, "<i>$1</i>");

    static const std::regex underlineRegex(R"(\[u\](.*?)\[/u\])");
    formattedText = std::regex_replace(formattedText, underlineRegex, "<u>$1</u>");

    static const std::regex colorRegex(R"(\[color=([a-zA-Z]+)\](.*?)\[/color\])");

    static const std::unordered_map<std::string, std::string> colorMap = {
        {"red", "red"},
        {"green", "green"},
        {"blue", "blue"},
        {"yellow", "yellow"},
        {"magenta", "magenta"},
        {"cyan", "cyan"},
        {"black", "black"},
        {"white", "white"},
        {"gray", "gray"},
        {"orange", "orange"},
        {"purple", "purple"},
        {"brown", "brown"},
        {"pink", "pink"},
    };

    std::smatch colorMatch;
    while (std::regex_search(formattedText, colorMatch, colorRegex)) {
        const std::string& colorName = colorMatch[1].str();
        const auto it = colorMap.find(colorName);
        if (it != colorMap.end()) {
            formattedText.replace(colorMatch[0].first, colorMatch[0].second,
                                   "<span style=\"color:" + it->second + "\">" + colorMatch[2].str() + "</span>");
        } else {
            // Skip unrecognized color
            formattedText.replace(colorMatch[0].first, colorMatch[0].second, colorMatch[2].str());
        }
    }

    return formattedText;
}

std::string formatBBCode(const std::string &task) {
  std::string formattedTask = task;

  static const std::regex boldRegex(R"(\[b\](.*?)\[/b\])");
  formattedTask =
      std::regex_replace(formattedTask, boldRegex, "\033[1m$1\033[0m");

  static const std::regex italicRegex(R"(\[i\](.*?)\[/i\])");
  formattedTask =
      std::regex_replace(formattedTask, italicRegex, "\033[3m$1\033[0m");

  static const std::regex underlineRegex(R"(\[u\](.*?)\[/u\])");
  formattedTask =
      std::regex_replace(formattedTask, underlineRegex, "\033[4m$1\033[0m");

  static const std::regex redRegex(R"(\[color=red\](.*?)\[/color\])");
  formattedTask =
      std::regex_replace(formattedTask, redRegex, "\033[31m$1\033[0m");

  static const std::regex greenRegex(R"(\[color=green\](.*?)\[/color\])");
  formattedTask =
      std::regex_replace(formattedTask, greenRegex, "\033[32m$1\033[0m");

  static const std::regex yellowRegex(R"(\[color=yellow\](.*?)\[/color\])");
  formattedTask =
      std::regex_replace(formattedTask, yellowRegex, "\033[33m$1\033[0m");

  static const std::regex blueRegex(R"(\[color=blue\](.*?)\[/color\])");
  formattedTask =
      std::regex_replace(formattedTask, blueRegex, "\033[34m$1\033[0m");

  static const std::regex magentaRegex(R"(\[color=magenta\](.*?)\[/color\])");
  formattedTask =
      std::regex_replace(formattedTask, magentaRegex, "\033[35m$1\033[0m");

  static const std::regex cyanRegex(R"(\[color=cyan\](.*?)\[/color\])");
  formattedTask =
      std::regex_replace(formattedTask, cyanRegex, "\033[36m$1\033[0m");

  return formattedTask;
}

void displayTasks() {
  std::cout << "Tasks:\n";
  for (int i = 0; i < (int)tasks.size(); i++) {
    std::cout << i + 1 << ". " << formatBBCode(tasks[i].description) << "\n";
  }
  std::cout << "\n";
}

void addTask(const std::string &description, const std::string &reminder) {
  std::time_t reminderTime = 0;
  if (!reminder.empty()) {
    std::tm reminder_tm = {};
    std::stringstream reminderStream(reminder);
    reminderStream >> std::get_time(&reminder_tm, "%Y-%m-%d %H:%M");
    reminderTime = std::mktime(&reminder_tm);
  }

  Task newTask = {description, reminderTime};
  tasks.push_back(newTask);
  std::cout << "Task added successfully.\n\n";
  saveTasks();
}

void removeTask(int index) {
  if (index >= 0 && index < (int)tasks.size()) {
    tasks.erase(tasks.begin() + index);
    std::cout << "Task removed successfully.\n\n";
    saveTasks();
  } else {
    std::cout << "Invalid task number.\n\n";
  }
}

int main(int argc, char *argv[]) {
  std::string command = "mkdir -p " + CONFIG_DIR;
  system(command.c_str());

  loadTasks();

  if (argc >= 2) {
    std::string action = argv[1];
    if (action == "display") {
      displayTasks();
    } else if (action == "add" || (action == "a" && argc >= 4)) {
      addTask(argv[2], argv[3]);
    } else if (action == "remove" || (action == "r" && argc == 3)) {
      int index = std::stoi(argv[2]) - 1;
      removeTask(index);
    } else if (action == "serve") {
      Application app(8080);
      Router router;
      router.get("/", [](HTTPRequest &req, HTTPResponse &res) {
        loadTasks();
        res.setHeader("Content-Type", "text/html");
        res.send("<html><body><h1>Task Manager</h1><ul>");
        for (int i = 0; i < (int)tasks.size(); i++) {
          res.send("<li>" + formatHTML(tasks[i].description) + "</li>");
        }
        res.send("</ul></body></html>");
      });
      app.useRouter(router);
      app.run();
    } else {
      std::cerr << "Invalid command.\n";
      return 1;
    }
  } else {
    std::cerr << "Usage:\t" << argv[0] << " <action> [arguments]\n";
    std::cerr << "Actions:\n";
    std::cerr << "\tdisplay\tDisplay all tasks\n";
    std::cerr << "\tadd <description> <reminder>\tAdd a new task\n";
    std::cerr << "\tremove <index>\tRemove the task at the specified index\n";
    std::cerr << "\tserve\tStart a local server to use from browser\n";
    return 1;
  }

  return 0;
}
