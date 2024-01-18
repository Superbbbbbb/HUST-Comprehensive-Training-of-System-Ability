#include "monitor/monitor.h"
#include "monitor/expr.h"
#include "monitor/watchpoint.h"
#include "nemu.h"

#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

void cpu_exec(uint64_t);
extern void isa_reg_display(void);

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);  
  return 0;
}

static int cmd_q(char *args) {
  return -1;
}

static int cmd_si(char *args) {
  char *arg = strtok(NULL, " ");
  int n=1;
  if (arg != NULL)
    n = atoi(arg);
  cpu_exec(n);
  return 0;
}

static int cmd_info(char *args) {
  char *arg = strtok(NULL, "\n");
  if(*arg=='r')
    isa_reg_display();
  else if(*arg=='w')
    watchpoint_display();
  else printf("A syntax error in expression\n");
  return 0;
}

static int cmd_p(char *args) {
  char *e = strtok(NULL, "\n");
  if (e == NULL) {
    printf("A syntax error in expression\n");
  } else {
    bool success=true;
    paddr_t base_addr = expr(e, &success);
    if (!success) {
      printf("Error expression!\n");
      return 0;
    }
    printf("%d\n",base_addr);
  }
  return 0;
}

static int cmd_x(char *args) {
  char *N = strtok(NULL, " ");
  char *e = strtok(NULL, "\n");
  if (N == NULL || e == NULL) {
    printf("A syntax error in expression\n");
  } else {
    int n = atoi(N);
    bool success=true;
    paddr_t base_addr = expr(e, &success);
    if (!success) {
      printf("Error expression!\n");
      return 0;
    }
    int i;
    for (i = 0; i < n; i++) {
      if (i % 4 == 0) {
        if (i != 0) printf("\n");
        printf("%#x:\t", base_addr);
      }
      printf("%#x\t", paddr_read(base_addr, 4));
      base_addr += 4;
    }
    printf("\n");
  }
  return 0;
}

static int cmd_w(char *args) {
  char *e = strtok(NULL, "\n");
  WP* wp = new_wp(e);
  assert(wp);
  printf("Watchpoint %u: %s\n", wp->NO, e);
  return 0;
}

static int cmd_d(char *args) {
  char *arg = strtok(NULL, "\n");
  if (arg == NULL) {
    printf("A syntax error in expression\n");
  } else {
    free_wp(atoi(arg));
  }
  return 0;
}

static int cmd_help(char *args);

static struct {
  char *name;
  char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display informations about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },

  /* TODO: Add more commands */
  { "si", "Excute n instructions in one step", cmd_si },
  { "info", "Print registers states or watching points", cmd_info},
  {"p", "EXPR", cmd_p},
  { "x", "Scan internal storage", cmd_x},
  { "w", "Add watch point", cmd_w},
  { "d", "Delete watch point", cmd_d},

};

#define NR_CMD (sizeof(cmd_table) / sizeof(cmd_table[0]))

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void ui_mainloop(int is_batch_mode) {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef HAS_IOE
    extern void sdl_clear_event_queue(void);
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

