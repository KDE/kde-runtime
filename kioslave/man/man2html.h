/**
 * \file man2html.h
 *
 * \note Despite that this file is installed publically, it should not be included
 * \todo ### KDE4: make this file private
 *
 */

#include <qcstring.h>

/** call this with the buffer you have */
void scan_man_page(const char *man_page);

/**
 * Set the paths to KDE resources
 *
 * \param htmlPath Path to the KDE resources, encoded for HTML
 * \param cssPath Path to the KDE resources, encoded for CSS
 * \since 3.5
 *
 */
extern void setResourcePath(const QCString& _htmlPath, const QCString& _cssPath);

/** implement this somewhere. It will be called
   with HTML contents
*/
extern void output_real(const char *insert);

/**
 * called for requested man pages. filename can be a
 * relative path! Return NULL on errors. The returned
 * char array is freed by man2html
 */
extern char *read_man_page(const char *filename);
