#ifndef __BEECTL_MKSTEMPS_H__
# define __BEECTL_MKSTEMPS_H__

#ifndef HAVE_MKSTEMPS
extern int mkstemps (char* template, int len);
#endif

#ifndef HAVE_MKSTEMP
# define mkstemp(template) mkstemps (template, 0)
#endif

#endif
