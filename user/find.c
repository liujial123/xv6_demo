#include "kernel/types.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "kernel/stat.h"
#include "user/user.h"

/* 修复后的 basename 函数 */
char* basename(char *pathname) {
  char *prev = pathname;  // 默认返回整个路径
  char *curr = strchr(pathname, '/');
  while (curr != 0) {
    prev = curr + 1;  // 指向最后一个 '/' 后的字符
    curr = strchr(curr + 1, '/');
  }
  return prev;
}

/* 修复作用域的递归函数 */
void find(char *curr_path, char *target) {
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(curr_path, O_RDONLY)) < 0) {
    fprintf(2, "find: cannot open %s\n", curr_path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: cannot stat %s\n", curr_path);
    close(fd);
    return;
  }

  switch (st.type) {
    case T_FILE: {
      char *f_name = basename(curr_path);
      int match = (strcmp(f_name, target) == 0);
      if (match) printf("%s\n", curr_path);
      close(fd);
      break;
    }

    case T_DIR: {
      // 构建子目录路径
      uint curr_path_len = strlen(curr_path);
      if (curr_path_len + DIRSIZ + 2 > sizeof(buf)) {
        fprintf(2, "find: path too long\n");
        close(fd);
        return;
      }
      strcpy(buf, curr_path);
      p = buf + curr_path_len;
      *p++ = '/';
      
      while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0 || 
            strcmp(de.name, ".") == 0 || 
            strcmp(de.name, "..") == 0) continue;
        
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        find(buf, target);  // 递归查找
      }
      close(fd);
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(2, "Usage: find <directory> <filename>\n");
    exit(1);
  }
  find(argv[1], argv[2]);
  exit(0);
}