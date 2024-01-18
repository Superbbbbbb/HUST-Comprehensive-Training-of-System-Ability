#include "monitor/watchpoint.h"
#include "monitor/expr.h"

#define NR_WP 32

static WP wp_pool[NR_WP] = {};
static WP *head = NULL, *free_ = NULL;

extern uint32_t isa_reg_str2val(const char *s, bool *success);

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp(char* e) {
  if (free_ == NULL)
    assert(0);
  WP* new = free_;
  free_ = free_->next;
  new->next = NULL;
  if(head == NULL)
    head = new;
  else{
    WP* wp = head;
    while(wp){
      if(!wp->next){
        wp->next = new;
        break;
      }
      wp = wp->next;
    }
  }
  bool success = false;
  uint32_t value = isa_reg_str2val(e+1, &success);
  if (e[0]!='$' || !success) {
    printf("Error Register name!\n");
    return NULL;
  }
  strcpy(new->e, e);
  new->value= value;
  return new;
}

void free_wp(uint32_t n) {
  if (head == NULL) {
    printf("No watch point!\n");
    return 0;
  }
  WP* wp = NULL;
  if (head->NO == n) {
    wp = head;
    head = head->next;
    wp->next = free_;
    free_ = wp;
  }
  else {
    WP* pre = head;
    while (1) {
      if (pre->next == NULL) {
        printf("Didn't find number %u watchpoint\n", n);
        break;
     }
      if(pre->next->NO == n){
        wp = pre->next;
        pre->next = pre->next->next;
        wp->next = free_;
        free_ = wp;
        break;
      }
      pre = pre->next;
    }
  }
}

bool check_watchpoint(){
  WP* wp = head;
  bool flag = false;
  while (wp) {
    bool success = false;
    uint32_t value = isa_reg_str2val(wp->e+1, &success);
    if(wp->value != value){
      flag = true;
      wp->value = value;
    }
    wp = wp->next;
  }
  return flag;
}

void watchpoint_display() {
  WP* wp = head;
  if(wp==NULL)
    printf("No watchpoints!\n");
  else{
    while (wp) {
      printf("Watchpoint %u: %s = %u\n", wp->NO, wp->e, wp->value);
      wp = wp->next;
    }
  }
}

