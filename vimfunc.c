#include "console.h"
#include "fcntl.h"
#include "stat.h"
#include "vimfunc.h"
#include "vim.h"

word * 
addnode(word * words,int line, int pos, uchar c) {
    int current_pos = 0;
    int realpos = line * MAX_COL + pos;
    word *head = words;

    if (c == '\n')
    {
        int empty_length = MAX_COL - realpos%MAX_COL - 1;
        word *empty_head = (word*)malloc(sizeof(word));
        empty_head->blank = 1;
        empty_head->color = 0x21;
        empty_head->pre = NULL;
        empty_head->w = '\0';
        word *empty_node = empty_head;
        while(empty_length--){
            empty_node->next = (word*)malloc(sizeof(word));
            empty_node->next->pre = empty_node;
            empty_node = empty_node->next;
            empty_node->w = '\0';
            empty_node->color = 0x21;
            empty_node->blank = 1;
        }

        if (realpos == 0)
        {
            empty_node->next = words;
            words->pre = empty_node;
            return empty_head;
        }
        for (current_pos = 0 ; current_pos < realpos ; ) {
            if (words->w == '\n')
                current_pos = (current_pos + 80) - (current_pos % MAX_COL);//reach the begining of next line
            else
                current_pos++;
            if (words->next != NULL)
                words = words->next;
        }
        if (words->next == NULL)
        {
            words->next = empty_head;
            empty_node->next = NULL;
            empty_head->pre = words;
        }
        else
        {
            empty_node->next = words;
            empty_head->pre = words->pre;
            words->pre->next = empty_head;
            words->pre = empty_node;
        }        
        
    }
    else
    {
        word *new_word = (word*)malloc(sizeof(word));
        new_word->w = c;
        new_word->color = 0x21;
        new_word->blank = (c=='\n' ? 1 : 0);
        
        if (realpos == 0 ) {
            new_word->pre = NULL;
            new_word->next = words;
            words->pre = new_word;
            return new_word;
        }
        for (current_pos = 0 ; current_pos < realpos ; ) {
            if (words->w == '\n')
                current_pos = (current_pos + 80) - (current_pos % MAX_COL);//reach the begining of next line
            else
                current_pos++;
            if (words->next != NULL)
                words = words->next;
        }
        //check if it is the last char
        if (words->next == NULL)
        {
            words->next = new_word;
            new_word->next = NULL;
            new_word->pre = words;
        }
        else
        {
            new_word->next = words;
            new_word->pre = words->pre;
            new_word->pre->next = new_word;
            words->pre = new_word;
            //go on, check if blank=1 exists
            if (words->blank == 1)//delete a blank char
            {
                //zhijiegan!
                new_word->next = words->next;
                words->next->pre = new_word;
                free(words);
            }
            else
            {
                words = words->next;
                while (words != NULL)
                {
                    if (words->blank==1)
                    {
                        //gan!
                        words->pre->next = words->next;
                        words->next->pre = words->pre;
                        free(words);
                        break;
                    }
                    words = words->next;
                }
            }
        }
    }
    return head;
}

word * 
create_text(int size, uchar *words) {
    int i, pos;
    word *head = NULL, *p;

    p = head = (word *) malloc(sizeof(word));

    for (i = 0, pos = 0; i < size;) {
        //chushihua    

        if (words[i] == '\n') {
            int empty_length = MAX_COL - pos%MAX_COL;
            while (empty_length--) {
                p->next = (word *) malloc(sizeof(word));
                p->next->color = 0x21;
                p->next->w = '\0';
                p->next->blank = 1;
                p->next->pre = p;
                head->pre = p->next;//save current tail
                p = p->next;
                pos++;
            }
            i++;
        } else {
            p->next = (word *) malloc(sizeof(word));
            p->next->color = 0x21;
            p->next->w = words[i++];
            p->next->blank = 0;
            p->next->pre = p;
            head->pre = p->next;//save current tail
            p = p->next;
            pos++;
        }
    }
    p = NULL;
    head->next->pre = head->pre;
    head = head->next;
    return head;
}

int 
readfile(char *path, struct text *t) {
    int fd;         //file decription
    struct stat st; //file info
    int f_size;     //size of file(bytes)
    uchar *words;   //store all words

    // open a file and get stat
    if (path != NULL && (fd = open(path, O_RDONLY | O_CREATE)) >= 0) {
        if (fstat(fd, &st) < 0) {
            printf(2, "Can not get file stat %s\n", path);
            close(fd);
            return -1;
        }
        if (st.type != T_FILE) {//not a file
            printf(2, "What you open is not a FILE!");
            close(fd);
            return -1;
        }
    }
        //read error
    else {
        printf(2, "Can not open %s\n", path);
        return -1;
    }
    f_size = st.size;           //get file size(bytes)
    if (f_size > 0) {
        words = (uchar *) malloc(f_size);
        read(fd, words, f_size);    //get text to words(ok)
    } else                        //empty file
    {
        words = NULL;
    }

    read(fd, words, f_size);    //get text to words
    printf(1, "%d\n", f_size);
    close(fd);

    //build text struct
    t->ifexist = 1;
    t->nbytes = f_size;
    strcpy(t->path, path);
    t->head = create_text(f_size, words);
    return 0;
}

int 
writefile(char *path, struct word *words) {
    int fd;         //file decription
    struct stat st; //file info
    // open a file and get stat
    if (path != NULL && (fd = open(path, O_WRONLY | O_CREATE)) >= 0) {
        if (fstat(fd, &st) < 0) {
            printf(2, "Can not get file stat \n");
            close(fd);
            return -1;
        }
        if (st.type != T_FILE) {//not a file
            printf(2, "What you open is not a FILE!");
            close(fd);
            return -1;
        }
    }
        //write error
    else {
        printf(2, "Can not write\n");
        return -1;
    }

    //start writing
    while (words != NULL) {
        if (words->w != '\0') {
            write(fd, &words->w, 1);
            words = words->next;
        } else {
            while (words != NULL && words->blank) {
                words = words->next;
            }
            write(fd, "\n", 1);
        }
    }
    close(fd);
    return 0;
}


int 
printwords(struct word *words, int n_row)// print file begining from n_row row
{
    int pos = 0;
    int i, j;
    for (i = 0; i < 24; i++) {
        for (j = 0; j < 80; j++) {
            if (words != NULL) {
                putchar(pos++, (int) words->w);
                words = words->next;
            } else
                putchar(pos++, (int) '\0');
        }
    }
    return 0;
}
