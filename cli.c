#include "posix-calls.h"
#include "walkfunctions.h"

#include "posix-calls.h"
#include "walkfunctions.h"

typedef struct {
    uint current_inode;
} CLIState;

CLIState cli_state = {0}; // Initialize current_inode to 0 (root directory)


int main(int argc, char *argv[]) {
    if (argc < 2) {
        Lwrite(1, "Usage: cli <command> [args]\n", 23);
        return 1;
    }

    if (Lstrcmp(argv[1], "ls") == 0) {
        if (argc < 3) {
            Lwrite(1, "Usage: cli ls <path>\n", 20);
            return 1;
        }
        lspath(argv[2]);
    } else if (Lstrcmp(argv[1], "cd") == 0) {
        if (argc < 3) {
            Lwrite(1, "Usage: cli cd <path>\n", 21);
            return 1;
        }
        change_directory(argv[2]);
    } else if (Lstrcmp(argv[1], "pwd") == 0) {
        print_working_directory();
    } else {
        Lwrite(1, "Unknown command: ", 16);
        Lwrite(1, argv[1], Lstrlen(argv[1]));
        Lwrite(1, "\n", 1);
        return 1;
    }

    return 0;
}


void change_directory(const char *pathname) {
    uint new_inode = namei(pathname);
    if (new_inode == 0) {
        Lwrite(1, "cd: ", 4);
        Lwrite(1, pathname, Lstrlen(pathname));
        Lwrite(1, ": No such file or directory\n", 26);
        return;
    }
    struct dinode din;
    if (getinode(&din, new_inode) < 0) {
        Lwrite(1, "cd: ", 4);
        Lwrite(1, pathname, Lstrlen(pathname));
        Lwrite(1, ": Cannot access\n", 15);
        return;
    }
    if (din.type != T_DIR) {
        Lwrite(1, "cd: ", 4);
        Lwrite(1, pathname, Lstrlen(pathname));
        Lwrite(1, ": Not a directory\n", 18);
        return;
    }
    cli_state.current_inode = new_inode;
}


void print_working_directory() {
    char path[256];
    if (inode_to_path(cli_state.current_inode, path, sizeof(path)) < 0) {
        Lwrite(1, "pwd: Cannot determine current directory\n", 37);
        return;
    }
    Lwrite(1, path, Lstrlen(path));
    Lwrite(1, "\n", 1);
}
