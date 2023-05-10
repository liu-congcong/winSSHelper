# winSSHelper

SSH helper for Windows11.

## Usage

```cmd
git clone https://github.com/liu-congcong/winSSHelper
```

```cmd
winSSHelper.exe --help

SSH helper for Windows11 v0.1 (https://github.com/liu-congcong/winSSHelper)
Usage:
List profiles:
    winSSHelper.exe --list
Add new profiles:
    winSSHelper.exe --add <name>:<user>@<host>:<port> ...
    winSSHelper.exe --add node1:congcong@192.168.1.2:22 node2:congcong@192.168.1.3:666
Remove profiles:
    winSSHelper.exe --delete <name> ...
    winSSHelper.exe --delete node1 node2
```
