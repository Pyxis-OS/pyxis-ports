#ifndef DOOMGENERIC_PYXIS_H
#define DOOMGENERIC_PYXIS_H

/* Engine keeps the directory for its lifetime. Each temporary path is owned
 * by one save attempt: rename or remove its reserved file, then free the path. */
char *DG_SaveGameDir(const char *iwad_name);
char *DG_TempSaveGameFile(void);

#endif
