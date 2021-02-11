#ifndef __SYMTAB_H__
#define __SYMTAB_H__ 

typedef struct symRec {
  char *name;
  int type;
} SymRec;

int newIdent(char *);
int getIdentId(char *);
int getIdentType(int id);
char *getIdentName(int id);
void setIdentType(int id, int type);

#endif
