# pali

pali is todo manager written in c++; 
it amins to be a no buldhit todo manager.
it dose only one things manage your todo list.

## Installation
```
./build.sh
```
## Features

- add task
- remove task
- support bbcode for color and style to text in termianl
- can run as a localserver to see your todo list in browser

## Usage
```
./todo-cli <action> [arguments]
Actions:
	display	Display all tasks
	add <description> <reminder>	Add a new task
	remove <index>	Remove the task at the specified index
	serve	Start a local server to use from browser
```

## Contributors
fell free to fork make your changes and contribute

### genrate information i think is imp
- i have implimeted a custom http1.1 server(which is incomplete, we will in future move to a diff repo)
- it stores its todo in .config/todo

## License
MIT
