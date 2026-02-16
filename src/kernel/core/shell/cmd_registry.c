#include "cmd_registry.h"
#include "process.h"
#include "minilib/stddef.h"
#include "minilib/stdint.h"
#include "minilib/string.h"

#define MAX_CMDS 64
typedef struct {
  cmd_desc_t tab[MAX_CMDS];
  uint8_t size;
} exec_table;

exec_table registry;

/** Clear the registry **/
void clear_cmd_registry() { memset(&registry, 0, sizeof(registry)); }

/** Register a builtin cmd in the registry **/
int8_t register_builtin(char *name, void (*fn)()) {
  if (!name || !fn || registry.size >= MAX_CMDS)
    return -1;
  cmd_desc_t *tab = registry.tab;
  uint8_t size = registry.size;
  tab[size].name = name;
  tab[size].type = CMD_BUILTIN;
  tab[size].cmd.builtin = fn;
  registry.size++;
  return 0;
}

/** Register a program in the registry **/
int8_t register_prog(char *name, void (*fn)(), priority prior) {
  if (!name || !fn || registry.size >= MAX_CMDS)
    return -1;
  cmd_desc_t *tab = registry.tab;
  uint8_t size = registry.size;
  tab[size].name = name;
  tab[size].type = CMD_PROG;
  tab[size].cmd.prog.fn = fn;
  tab[size].cmd.prog.prior = prior;
  registry.size++;
  return 0;
}

/** Lookup for a command in the registry **/
const cmd_desc_t *command_lookup(const char *name) {
  if (!name)
    return NULL;
  for (size_t i = 0; i < registry.size; ++i) {
    if (registry.tab[i].name && strcmp(registry.tab[i].name, name) == 0)
      return &registry.tab[i];
  }
  return NULL;
}

const cmd_desc_t *registry_get_nth(uint8_t idx) {
  if (idx >= registry.size) {
    return NULL;
  }
  return &registry.tab[idx];
}

uint8_t registry_get_size() { return registry.size; }
