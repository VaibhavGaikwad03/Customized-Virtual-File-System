#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#define READ 1
#define WRITE 2
#define REGULAR 1
#define MAX_INODES 50
#define FILE_SIZE 1024

struct inode
{
    char file_name[50];
    int inode_number;
    int file_size;              // block size
    int file_actual_size;       // to determine actual size of file
    int file_type;              // remains 1 throughout the execution (only supports regular file)
    char *file_data;            // pointer to the data of file
    int link_count;             // remains 1 throughout the exexution (no hardlinks)
    int reference_count;        // remains 1 throughout the execution (no child process or dup)
    int permission;             // read, write and read + write
    struct inode *next_inode;   // pointer to the next inode
};

struct filetable
{
    int read_offset;            // from where to read
    int write_offset;           // from where to write
    int reference_count;        // remains 1 throughout the program because only one process will point to file table
    int mode;                   // in which mode file is opened
    struct inode *ptr_inode;    // pointer to an inode
};

struct ufdt
{
    struct filetable *ptr_filetable;    // pointer to the filetable
};

struct superblock
{
    int total_inodes;           // total number of inodes
    int free_inodes;            // to indicate free inodes
};

struct superblock super_block;  // global object for managing total inodes and free inodes
struct ufdt ufdt_array[MAX_INODES]; // UFDT array 
struct inode *inode_head = NULL; // linked list of inodes

void initialize_superblock()
{
    int counter;

    for (counter = 0; counter < MAX_INODES; counter++)
        ufdt_array[counter].ptr_filetable = NULL;

    super_block.total_inodes = MAX_INODES;
    super_block.free_inodes = MAX_INODES;
}

void create_dilb()
{
    int counter;
    struct inode *inode_ptr = NULL;
    struct inode *new_inode = NULL;

    for (counter = 1; counter <= MAX_INODES; counter++)
    {
        new_inode = (struct inode *)malloc(sizeof(struct inode));
        if (new_inode == NULL)
        {
            printf("Memory allocation FAILED\n");
            return;
        }
        new_inode->inode_number = counter;
        new_inode->file_size;
        new_inode->file_actual_size = 0;
        new_inode->file_type = 0;
        new_inode->file_data = NULL;
        new_inode->link_count = 0;
        new_inode->reference_count = 0;
        new_inode->permission = 0;
        new_inode->next_inode = NULL;

        inode_ptr = inode_head;
        if (inode_ptr == NULL)
        {
            inode_ptr = new_inode;
            inode_head = inode_ptr;
        }
        else
        {
            while (inode_ptr->next_inode != NULL)
                inode_ptr = inode_ptr->next_inode;
            inode_ptr->next_inode = new_inode;
        }
    }
    printf("DILB created successfully.\n");
}

void display_file_list()
{
    struct inode *inode_ptr = inode_head;

    if (super_block.free_inodes == MAX_INODES)
    {
        printf("\nThere are no files.\n");
        return;
    }

    while (inode_ptr != NULL)
    {
        if (inode_ptr->file_type != 0)
            printf("%s\n", inode_ptr->file_name);
        inode_ptr = inode_ptr->next_inode;
    }
}

void close_all_files()
{
    int counter;

    for (counter = 0; counter < MAX_INODES; counter++)
    {
        if ((ufdt_array[counter].ptr_filetable != NULL) && (ufdt_array[counter].ptr_filetable->reference_count == 0))
        {
            ufdt_array[counter].ptr_filetable->mode = 0;
            ufdt_array[counter].ptr_filetable->read_offset = 0;
            ufdt_array[counter].ptr_filetable->write_offset = 0;
            ufdt_array[counter].ptr_filetable->ptr_inode = NULL;
            free(ufdt_array[counter].ptr_filetable);
        }
    }
    printf("All files closed successfully\n");
}

void clear_screen()
{
#ifdef _WIN
    system("cls");
#elif __linux__
    system("clear");
#endif
}

void display_commands()
{
    printf("\ncreate:\t\tto create new file.\n");
    printf("open:\t\tto open a file.\n");
    printf("read:\t\tto read from file.\n");
    printf("write:\t\tto write from file.\n");
    printf("ls:\t\tto display all files.\n");
    printf("closeall:\tto close all opened files.\n");
    printf("clear:\t\tto clear the screen.\n");
    printf("backup:\t\tto take backup of all files.\n");
    printf("stat:\t\tto display file info by file name.\n");
    printf("fstat:\t\tto display file info by file descriptor.\n");
    printf("close:\t\tto close a file.\n");
    printf("rm:\t\tto remove file.\n");
    printf("man:\t\tto display info about commands.\n");
    printf("truncate:\tto remove data from file.\n");
    printf("lseek:\t\tto change byte read/write byte offset of file.\n");
    printf("exit:\t\tto exit file system.\n");
}

void backup_all_files()
{
    int file_desc;
    int permission;
    struct inode *inode_ptr = inode_head;

    while (inode_ptr != NULL)
    {
        if (inode_ptr->file_type != 0)
        {
            if (inode_ptr->permission == READ)
                permission = S_IRUSR;
            if (inode_ptr->permission == WRITE)
                permission = S_IWUSR;
            if (inode_ptr->permission == READ + WRITE)
                permission = S_IRWXU;

            file_desc = creat(inode_ptr->file_name, permission);
            if (file_desc == -1)
                perror("ERROR");
            else
                write(file_desc, inode_ptr->file_data, strlen(inode_ptr->file_data));
        }
        inode_ptr = inode_ptr->next_inode;
    }
}

struct inode *get_free_inode()
{
    struct inode *inode_ptr = inode_head;

    while (inode_ptr != NULL)
    {
        if (inode_ptr->file_type == 0)
            return inode_ptr;
        inode_ptr = inode_ptr->next_inode;
    }
    return NULL;
}

int is_file_exists(char *file_name)
{
    struct inode *inode_ptr = inode_head;

    while (inode_ptr != NULL)
    {
        if ((!strcmp(inode_ptr->file_name, file_name)) && (inode_ptr->file_type != 0))
            return 1; // file exists
        inode_ptr = inode_ptr->next_inode;
    }
    return 0; // file does not exists
}

int get_fd_by_file_name(char *file_name)
{
    int counter;

    if (!is_file_exists(file_name))
        return -1;

    for (counter = 0; ufdt_array[counter].ptr_filetable != NULL; counter++)
    {
        if (!strcmp(ufdt_array[counter].ptr_filetable->ptr_inode->file_name, file_name))
            return counter;
    }
    return -1;
}

void stat(char *file_name)
{
    struct inode *inode_ptr = inode_head;

    if (!is_file_exists(file_name))
    {
        printf("\nERROR: There is no such file.\n");
        return;
    }

    while (inode_ptr != NULL)
    {
        if (!strcmp(inode_ptr->file_name, file_name))
        {
            printf("\nFile name: %s\n", inode_ptr->file_name);
            printf("Inode number: %d\n", inode_ptr->inode_number);
            printf("File size: %d\n", inode_ptr->file_size);
            printf("Actual file size: %d\n", inode_ptr->file_actual_size);
            printf("Link count: %d\n", inode_ptr->link_count);
            printf("File type: Regular\n");
            if (inode_ptr->permission == READ)
                printf("Permission: Read\n\n");
            else if (inode_ptr->permission == WRITE)
                printf("Permission: Write\n\n");
            else if (inode_ptr->permission == READ + WRITE)
                printf("Permission: Read & Write\n\n");
            break;
        }
        inode_ptr = inode_ptr->next_inode;
    }
}

void fstat(int fd)
{
    struct inode *inode_ptr = NULL;

    if (fd < 0)
    {
        printf("\nERROR: Invalid file descriptor.\n");
        return;
    }

    if (ufdt_array[fd].ptr_filetable == NULL)
    {
        printf("\nERROR: There is no such file.\n");
        return;
    }

    inode_ptr = ufdt_array[fd].ptr_filetable->ptr_inode;

    printf("\nFile name: %s\n", inode_ptr->file_name);
    printf("Inode number: %d\n", inode_ptr->inode_number);
    printf("File size: %d\n", inode_ptr->file_size);
    printf("Actual file size: %d\n", inode_ptr->file_actual_size);
    printf("Link count: %d\n", inode_ptr->link_count);
    printf("File type: Regular\n");
    if (inode_ptr->permission == READ)
        printf("Permission: Read\n\n");
    else if (inode_ptr->permission == WRITE)
        printf("Permission: Write\n\n");
    else if (inode_ptr->permission == READ + WRITE)
        printf("Permission: Read & Write\n\n");
}

void manual(char *command)
{
    if (command == NULL)
        return;
    else if (!strcmp(command, "create"))
        printf("\nCommand: create\nDescription: Used to create new regular file.\nUsage: create <file_name> <permission>\n\n");
    else if (!strcmp(command, "read"))
        printf("\nCommand: read\nDescription: Used to read data from regular file.\nUsage: read <file_name> <no_of_bytes_to_read>\n\n");
    else if (!strcmp(command, "write"))
        printf("\nCommand: write\nDescription: Used to write data into regular file.\nUsage: write <file_name> <data>\n\n");
    else if (!strcmp(command, "ls"))
        printf("\nCommand: ls\nDescription: Used to list all files.\nUsage: ls\n\n");
    else if (!strcmp(command, "stat"))
        printf("\nCommand: stat\nDescription: Used to display information of file by name.\nUsage: stat <file_name>\n\n");
    else if (!strcmp(command, "fstat"))
        printf("\nCommand: fstat\nDescription: Used to display information of file by file descriptor.\nUsage: fstat <file_name>\n\n");
    else if (!strcmp(command, "truncate"))
        printf("\nCommand: truncate\nDescription: Used to remove data from file.\nUsage: truncate <file_name>\n\n");
    else if (!strcmp(command, "open"))
        printf("\nCommand: open\nDescription: Used to open existing file.\nUsage: open <file_name>\n\n");
    else if (!strcmp(command, "close"))
        printf("\nCommand: close\nDescription: Used to close opened files.\nUsage: close <file_name>\n\n");
    else if (!strcmp(command, "closeall"))
        printf("\nCommand: closeall\nDescription: Used to close all opened files.\nUsage: closeall\n\n");
    else if (!strcmp(command, "lseek"))
        printf("\nCommand: lseek\nDescription: Used to change the file offset.\nUsage: lseek <file_name> <change_in_offset> <starting_point>\n\n");
    else if (!strcmp(command, "rm"))
        printf("\nCommand: rm\nDescription: Used to delete the existing file.\nUsage: rm <file_name>\n\n");
    else if (!strcmp(command, "backup"))
        printf("\nCommand: backup\nDescription: Used to take backup of all the files created.\nUsage: backup\n\n");
    else if (!strcmp(command, "exit"))
        printf("\nCommand: exit\nDescription: Cause normal process termination.\nUsage: exit\n\n");
    else
        printf("\nERROR: No manual entry for '%s'\n", command);
}

int is_open(char *file_name)
{
    struct inode *inode_ptr = inode_head;

    if (!is_file_exists(file_name))
        return -1;          // there is no such file

    while (inode_ptr != NULL)
    {
        if (!strcmp(inode_ptr->file_name, file_name))
        {
            if (inode_ptr->reference_count >= 1)
                return 1; // file is open
            return 0;     // file is closed
        }
        inode_ptr = inode_ptr->next_inode;
    }
    return 0;
}

int close_file(char *file_name)
{
    int file_desc;
    struct inode *inode_ptr = NULL;

    if (!is_file_exists(file_name))
        return -1; // there is no such file

    if (!is_open(file_name))
        return -2; // file already closed

    file_desc = get_fd_by_file_name(file_name);

    inode_ptr = ufdt_array[file_desc].ptr_filetable->ptr_inode;
    (inode_ptr->reference_count)--;

    if (inode_ptr->reference_count == 0)
    {
        free(ufdt_array[file_desc].ptr_filetable);
        ufdt_array[file_desc].ptr_filetable = NULL;
    }
    return 0;
}

int create_file(char *file_name, int permission)
{
    int counter;
    struct inode *new_inode = NULL;

    if (file_name == NULL || permission == 0 || permission > 3)
        return -1;

    if (super_block.free_inodes == 0)
        return -2; // there is no enough space

    if (is_file_exists(file_name))
        return -3; // file already exists

    for (counter = 0; counter < MAX_INODES; counter++)
    {
        if (ufdt_array[counter].ptr_filetable == NULL)
            break;
    }

    ufdt_array[counter].ptr_filetable = (struct filetable *)malloc(sizeof(struct filetable));

    if (ufdt_array[counter].ptr_filetable == NULL)
    {
        printf("Memory allocation FAILED\n");
        return -4;
    }

    // initialize file table
    ufdt_array[counter].ptr_filetable->mode = permission;
    ufdt_array[counter].ptr_filetable->read_offset = 0;
    ufdt_array[counter].ptr_filetable->write_offset = 0;
    ufdt_array[counter].ptr_filetable->reference_count = 1;

    if ((new_inode = get_free_inode()) != NULL)
        ufdt_array[counter].ptr_filetable->ptr_inode = new_inode;

    strcpy(new_inode->file_name, file_name);
    new_inode->file_actual_size = 0;
    new_inode->file_size = FILE_SIZE;
    new_inode->file_type = REGULAR;
    new_inode->permission = permission;
    new_inode->reference_count = 1;
    new_inode->link_count = 1;

    if (new_inode->file_data == NULL) // checks if memory already allocated (is old inode?)
        new_inode->file_data = (char *)malloc(FILE_SIZE);

    if (new_inode->file_data == NULL)
    {
        printf("Memory allocation FAILED\n");
        return -4;
    }

    memset(new_inode->file_data, 0, FILE_SIZE); // clearing garbage

    (super_block.free_inodes)--;

    return counter;
}

int delete_file(char *file_name)
{
    int file_desc;
    struct inode *inode_ptr = inode_head;

    if (!is_file_exists(file_name))
        return -1; // there is no such file

    while (inode_ptr != NULL)
    {
        if (!strcmp(inode_ptr->file_name, file_name))
            break;
        inode_ptr = inode_ptr->next_inode;
    }

    (inode_ptr->link_count)--;

    if ((inode_ptr->link_count) == 0)
    {
        inode_ptr->file_type = 0; // most important (dependency in get_free_inode())
        inode_ptr->file_actual_size = 0;
        inode_ptr->reference_count = 0; // any file table entry is not pointing at inode
        inode_ptr->permission = 0;
        inode_ptr = NULL;

        file_desc = get_fd_by_file_name(file_name);

        if (file_desc != -1)
        {
            ufdt_array[file_desc].ptr_filetable->ptr_inode = NULL;
            free(ufdt_array[file_desc].ptr_filetable);  // freeing file table entry
            ufdt_array[file_desc].ptr_filetable = NULL; // initialize with NULL (most important)
        }
        (super_block.free_inodes)++;
    }

    return 0;
}

int write_file(char *file_name, char *file_data, int no_of_bytes)
{
    int file_desc;
    struct inode *inode_ptr = NULL;
    struct filetable *filetable_ptr = NULL;

    if (!is_file_exists(file_name))
        return -1;          // there is no such file
    
    if (!is_open(file_name))
        return -2;          // file is not opened
    
    file_desc = get_fd_by_file_name(file_name);
    inode_ptr = ufdt_array[file_desc].ptr_filetable->ptr_inode; // for efficiency

    if ((inode_ptr->permission != WRITE) && (inode_ptr->permission != READ + WRITE))
        return -3;          // don't have permission to write

    if (inode_ptr->file_type != REGULAR)
        return -4;          // file is not a regular file
    
    filetable_ptr = ufdt_array[file_desc].ptr_filetable;        // for efficiency
    if ((filetable_ptr->write_offset + no_of_bytes) > FILE_SIZE)
        return -5;          // there is no space

    inode_ptr->file_actual_size += no_of_bytes;
    strcpy((inode_ptr->file_data) + (filetable_ptr->write_offset), file_data);
    filetable_ptr->write_offset += no_of_bytes;

    return no_of_bytes;
}

int main(void)
{
    int status;
    int file_desc;
    int token_count;
    int no_of_bytes;
    char str[80];
    char file_data[1024];
    char command[4][50];
    create_dilb();
    initialize_superblock();

    while (1)
    {
        // fflush(stdin);
        printf("ubuntu@linuxuser: ");
        fgets(str, 80, stdin);
        token_count = sscanf(str, "%s %s %s %s", command[0], command[1], command[2], command[3]);

        if (token_count == 1)
        {
            if (!strcmp(command[0], "ls"))
                display_file_list();

            else if (!strcmp(command[0], "closeall"))
                close_all_files();

            else if (!strcmp(command[0], "clear"))
                clear_screen();

            else if (!strcmp(command[0], "help"))
                display_commands();

            else if (!strcmp(command[0], "backup"))
                backup_all_files();

            else if (!strcmp(command[0], "exit"))
                exit(0);
            else
                printf("\nERROR: Command '%s' not found.\n", command[0]);
        }
        else if (token_count == 2)
        {
            if (!strcmp(command[0], "stat"))
                stat(command[1]);
            else if (!strcmp(command[0], "fstat"))
                fstat(atoi(command[1]));
            else if (!strcmp(command[0], "man"))
                manual(command[1]);
            else if (!strcmp(command[0], "close"))
            {
                status = close_file(command[1]);

                if (status == -1)
                    printf("\nERROR: There is no such file.\n");
                else if (status == -2)
                    printf("\nERROR: File already closed.\n");
                else
                    printf("\nFile closed successfully.\n");
            }
            else if (!strcmp(command[0], "rm"))
            {
                status = delete_file(command[1]);

                if (status == -1)
                    printf("\nERROR: There is no such file.\n");
                else
                    printf("\nFile deleted successfully.\n");
            }
            else if (!strcmp(command[0], "write"))
            {
                if (!is_file_exists(command[1]))
                {
                    printf("\nERROR: There is no such file.\n");
                    continue;
                }

                if (!is_open(command[1]))
                {
                    printf("\nERROR: File is not opened.\n");
                    continue;
                }

                printf("\nEnter the data: ");
                scanf("%[^\n]%*c", file_data);

                no_of_bytes = strlen(file_data);

                status = write_file(command[1], file_data, no_of_bytes);

                if (status == -3)
                    printf("\nERROR: Permission denied to write to the file.\n");
                else if (status == -4)
                    printf("\nERROR: The file is not a regular file.\n");
                else if (status == -5)
                    printf("\nERROR: There is not enough space to write to the file.\n");
                else 
                    printf("\nSuccessfully wrote %d bytes to '%s'.\n", status, command[1]);
            }
            else
                printf("\nERROR: Command '%s' not found.\n", command[0]);
        }
        else if (token_count == 3)
        {
            if (!strcmp(command[0], "create"))
            {
                file_desc = create_file(command[1], atoi(command[2]));

                if (file_desc == -1)
                    printf("\nERROR: Incorrect parameters.\n");
                else if (file_desc == -2)
                    printf("\nERROR: There is no free space.\n");
                else if (file_desc == -3)
                    printf("\nERROR: File already exists.\n");
                else if (file_desc == -4)
                    printf("\nERROR: Something went wrong.\n");
                else
                    printf("\nFile successfully created with fd: %d.\n", file_desc);
            }
        }
    }

    return 0;
}