#include "posix-calls.h"
#include "Llibc.h"
#include "Llibc.c"
#include "Lcli.h"
#include "walkfunctions.h"

#define LINESIZE 1024   /* Line buffer size */
#define NTOKS 128       /* Max number of tokens in a line */
//#define BSIZE 512       /* Assuming a block size of 512 bytes for simplicity */
//#define NDIRECT 12      /* Number of direct blocks in an inode */
//#define T_DIR 2         /* Assuming T_DIR is 2 for directories */

/* From Lbio.c */
void binit(void);
struct buf* bread(uint, uint);
void brelse(struct buf*);

/* Our globals */
int DEVFD;
struct superblock SB;
struct {
    uint inum;
    char name[MAXPATH];
} CWD;

void help(void);

void devfd_init(const char *devpath) {
    if ((DEVFD = Lopen(devpath, O_RDONLY)) < 0 ) {
        Lfprintf(2, "Could not open %s\n", devpath);
        Lexit(2);
    }
}

void superblock_init(int devfd) {
    struct buf *b;
    if ((b = bread(DEVFD, 1)) == 0) {
        Lfprintf(2, "Could not read superblock\n");
        Lexit(3);
    }
    Lmemcpy(&SB, &b->data[0], sizeof(struct superblock));
    brelse(b);

    // Print superblock details
    Lprintf("Superblock magic = %08x\n", SB.magic);
    Lprintf("FS device size = %d\n", SB.size);
    Lprintf("Number of data blocks = %d\n", SB.nblocks);
    Lprintf("Number of inodes = %d\n", SB.ninodes);
    Lprintf("Block number for the first inode block = %d\n", SB.inodestart);
    Lprintf("Block number for the first bitmap block = %d\n", SB.bmapstart);
    Lprintf("Done with superblock for now!\n");
}

void cwd_init(void) {
    CWD.inum = 1;
    Lstrcpy(CWD.name, "/");
}

void dump_inodes(int devfd) {
    uint inodesize = sizeof(struct dinode);
    uint inodes_per_block = BSIZE / inodesize;
    Lprintf("Size of each inode = %d\n", inodesize);
    Lprintf("Number of inodes per block = %d\n", inodes_per_block);
    Lprintf("Dumping the first few inodes ...\n");

    struct buf *b;
    b = bread(DEVFD, SB.inodestart);
    struct dinode *inode;
    for (int k = 0; k < 4; k++) { // Dump the first 4 inodes
        inode = (struct dinode *) &b->data[k*inodesize];
        Lprintf("inode %d:\n", k);
        if (inode->type == 0) {
            Lprintf(" UNUSED (file type = 0)\n");
            continue;
        }
        Lprintf(" file type = %d\n", inode->type);
        Lprintf(" number of links = %d\n", inode->nlink);
        Lprintf(" file size = %u (bytes)\n", inode->size);
        Lprintf(" block map:\n");
        for (uint j = 0; j < NDIRECT && inode->addrs[j] != 0; j++)
            Lprintf("    direct block decimal: %d, hexadecimal: 0x%08x\n", inode->addrs[j], inode->addrs[j]);
        if (inode->size > NDIRECT * BSIZE && inode->addrs[NDIRECT - 1] != 0)
            Lprintf("      indirect block 0x%08x\n", inode->addrs[NDIRECT]);
    }
    brelse(b);
}

int Lmain(int argc, char *argv[]) {
    if (argc < 2) {
        Lprintf("Usage: %s fs_img_path\n", argv[0]);
        return 1;
    }

    devfd_init(argv[1]);
    binit();
    superblock_init(DEVFD);
    cwd_init();
    dump_inodes(DEVFD);

    char buf[LINESIZE];
    Lprintf("fscli> ");
    while (Lread(0, buf, LINESIZE) > 0) {
        buf[Lstrlen(buf) - 1] = '\0'; // Remove newline character
        char *tokens[NTOKS];
        int ntoks = Ltokenize(buf, tokens, NTOKS);

        if (ntoks == 0) {
            Lprintf("fscli> ");
            continue;
        }

        if (Lstrcmp(tokens[0], "pwd") == 0) {
            Lprintf("%s\n", CWD.name);
        } else if (Lstrcmp(tokens[0], "cd") == 0) {
            if (ntoks == 1) {
                CWD.inum = 1;
                Lstrcpy(CWD.name, "/");
            } else {
                uint in = namei(tokens[1]);
                struct dinode inode;
                if (in != 0 && getinode(&inode, in) == 0 && inode.type == T_DIR) {
                    CWD.inum = in;
                    Lstrcpy(CWD.name, tokens[1]);
                } else {
                    Lprintf("Could not cd to %s (not a directory)\n", tokens[1]);
                }
            }
        } else if (Lstrcmp(tokens[0], "ls") == 0) {
              if (ntoks == 1) {
              lspath(CWD.name);
              } else {
                  for (int i = 1; i < ntoks; i++) {
                  char path[MAXPATH]; // Assuming MAXPATH is defined somewhere as the maximum path length
                  Lprintf(path, sizeof(path), "%s/%s", CWD.name, tokens[i]); // Append the argument to the current working directory
                  uint in = namei(path);
                  if (in != 0) {
                  lspath(path);
                  } else {
                  Lprintf("ls: cannot access '%s': No such file or directory\n", path);
                  }
              }
            }
        } else if (Lstrcmp(tokens[0], "help") == 0) {
            help();
        } else if (Lstrcmp(tokens[0], "mkdir") == 0) {
               if (ntoks < 2) {
               Lprintf("mkdir: missing operand\n");
               } else {
                      for (int i = 1; i < ntoks; i++) {
                      Lprintf("Creating directory: %s\n", tokens[i]);
                      int status = Lmkdir(tokens[i]);
                      if (status != 0) {
                      Lprintf("mkdir: cannot create directory '%s'\n", tokens[i]);
                   }
               }
           }
        }

        
          else if (Lstrcmp(tokens[0], "quit") == 0) {
            Lprintf("Exiting CLI...\n");
            break; // Exits the while loop, effectively quitting the CLI
        } else {
            Lprintf("Invalid command. Type 'help' for a list of commands.\n");
        }

        Lprintf("fscli> ");
    }
    Lprintf("\n");
    return 0;
}

void help() {
    Lwrite(1,"+-----------------------+--------------------------------------------------------+\n",84);
    Lwrite(1,"| Command               | Description                                            |\n",84);
    Lwrite(1,"+-----------------------+--------------------------------------------------------+\n",84);
    Lwrite(1,"| help                  | Print this table                                       |\n",84);
    Lwrite(1,"| pwd                   | Show current directory (path and inode)                |\n",84);
    Lwrite(1,"| cd [path]             | Change directory to path (or to /)                     |\n",84);
    Lwrite(1,"| ls [-d] [-R] [path]   | List path as in ls -ail                                |\n",84);
    Lwrite(1,"| creat path            | Create file at path (like touch)                       |\n",84);
    Lwrite(1,"| mkdir path            | Create directory at path                               |\n",84);
    Lwrite(1,"| unlink path           | Like rm and rmdir                                      |\n",84);
    Lwrite(1,"| link oldpath newpath  | Like ln                                                |\n",84);
    Lwrite(1,"| sync                  | Write all cached dirty buffers to device blocks        |\n",84);
    Lwrite(1,"| quit                  | Exit CLI (should also sync                             |\n",84);
    Lwrite(1,"+-----------+--------------------------------------------------------------------+\n",84);
}
