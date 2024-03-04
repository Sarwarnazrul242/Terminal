#include "posix-calls.h"
#include "Llibc.h"
#include "Lcli.h"
#include "walkfunctions.h"

struct buf* bread(uint, uint);
extern struct superblock SB;
extern  int DEVFD;

void brelse(struct buf*);
void Lprintf(const char *format, ...);

int getinode(struct dinode *inode, uint inodenum) {
    uint bn = IBLOCK(inodenum, SB);
    struct buf *bp = bread(DEVFD, bn);
    struct dinode *dip = (struct dinode*)bp->data + inodenum % IPB;
    Lmemmove(inode, dip, sizeof(*dip));
    brelse(bp);
    return 0;
}

uint find_name_in_dirblock(uint blockptr, const char *name) {
    struct buf *bp = bread(DEVFD, blockptr);
    struct dirent *de = (struct dirent*)bp->data;
    for (int i = 0; i < NDIRECT; i++) {
        if (Lstrcmp(de[i].name, name) == 0) {
            uint inum = de[i].inum;
            brelse(bp);
            return inum;
        }
    }
    brelse(bp);
    return 0;
}

uint find_dent(uint inum, const char *name) {
    struct dinode din;
    if (getinode(&din, inum) < 0)
        return 0;
    if (din.type != T_DIR)
        return 0;
    //uint bn = bmap(inum, 0);
	uint bn = 0;
    return find_name_in_dirblock(bn, name);
}

/*

uint namei(const char *pathname) {
    char name[DIRSIZ+1];
    uint inum = namei(pathname);

    if (inum == 0)
        return 0;
    return find_dent(inum, name);
}
*/

uint namei(const char *pathname) {
    if (pathname[0] != '/') {
        // Only absolute paths are supported in this example
        return 0;
    }

    uint inum = 1; // Start from the root inode
    char component[DIRSIZ+1]; // Temporary storage for each path component
    const char *path = pathname + 1; // Skip the leading '/'

    while (*path != '\0') {
        // Clear previous path component
        Lmemset(component, 0, sizeof(component));

        // Extract the next component from the path
        int i = 0;
        while (*path != '/' && *path != '\0' && i < DIRSIZ) {
            component[i++] = *path++;
        }

        // Skip all consecutive slashes
        while (*path == '/') path++;

        // If we have an empty component (i.e., we reached the end of the path), break
        if (i == 0) break;

        // Now, find the directory entry for this component
        struct dinode din;
        if (getinode(&din, inum) < 0 || din.type != T_DIR) {
            // Failed to read inode or inode is not a directory
            return 0;
        }

        uint next_inum = find_dent(inum, component);
        if (next_inum == 0) {
            // Component not found in the current directory
            return 0;
        }

        // Move to the next component
        inum = next_inum;
    }

    return inum; // Return the inode number of the final component
}

void lsdir(uint blockptr) {
    struct buf *bp = bread(DEVFD, blockptr);
    struct dirent *de = (struct dirent*)bp->data;
    for (int i = 0; i < NDIRECT; i++) {
        if (de[i].inum == 0) continue; // Skip empty entries
        struct dinode din;
        if (getinode(&din, de[i].inum) < 0) {
            Lprintf("Error reading inode for entry %s\n", de[i].name);
            continue;
        }
        // Display file type along with name, inode number, and size
        char *type = (din.type == T_DIR) ? "Dir" : (din.type == T_FILE) ? "File" : "Unknown";
        Lprintf("%s\tType: %s, Inode: %d, Size: %u\n", de[i].name, type, de[i].inum, din.size);
    }
    brelse(bp);
}

int lspath(const char *pathname) {
    uint inum = namei(pathname);
    if (inum == 0) {
        Lprintf("ls: cannot access '%s': No such file or directory\n", pathname);
        return -1;
    }
    struct dinode din;
    if (getinode(&din, inum) < 0) {
        Lprintf("Error reading inode for path: %s\n", pathname);
        return -1;
    }
    if (din.type != T_DIR) {
        Lprintf("%s is not a directory\n", pathname);
        return -1;
    }

    // Iterate over direct addresses
    for (int i = 0; i < NDIRECT; i++) {
        if (din.addrs[i] == 0) break; // No more blocks to read
        lsdir(din.addrs[i]);
    }

    // Handle indirect block if it exists
    if (din.addrs[NDIRECT]) {
        struct buf *bp = bread(DEVFD, din.addrs[NDIRECT]);
        uint *indirect = (uint*)bp->data;
        for (int i = 0; i < NINDIRECT; i++) {
            if (indirect[i] == 0) break; // No more blocks to read
            lsdir(indirect[i]);
        }
        brelse(bp);
    }

    return 0;
}


/*
int lspath(const char *pathname) {
    uint inum = namei(pathname);
    if (inum == 0) {
        Lprintf("ls: cannot access '%s': No such file or directory\n", pathname);
        return -1;
    }
    struct dinode din;
    if (getinode(&din, inum) < 0) {
        Lprintf("ls: cannot access '%s': No such file or directory\n", pathname);
        return -1;
    }
    if (din.type != T_DIR) {
        Lprintf("ls: cannot access '%s': Not a directory\n", pathname);
        return -1;
    }
    uint bn = 0; // Assuming the first block of the directory is at block number 0
    lsdir(bn);
    return 0;
}

// Function to list the contents of a directory given its block number
void lsdir(uint blockptr) {
    struct buf *bp = bread(DEVFD, blockptr);
    struct dirent *de = (struct dirent*)bp->data;
    for (int i = 0; i < NDIRECT; i++) {
        if (de[i].inum == 0) continue;
        struct dinode din;
        if (getinode(&din, de[i].inum) < 0) {
            Lprintf("Error reading inode for entry %s\n", de[i].name);
            continue;
        }
        // Assuming T_DIR is 1 for directories and T_FILE is 2 for files
        int inodeType = (din.type == T_DIR) ? 1 : 2;
        Lprintf("%s\t%d %d %u\n", de[i].name, din.type, inodeType, din.size);
    }
    brelse(bp);
}
*/
