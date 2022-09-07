/*
 * Copyright/Licensing information.
 */

/* inclusion guard */
#ifndef __MAMAN_BAR_H__
#define __MAMAN_BAR_H__

#include <glib-object.h>
/*
 * Potentially, include other headers on which this header depends.
 */

/*
 * Type macros.
 */
#define MAMAN_TYPE_BAR                  (maman_bar_get_type ())
#define MAMAN_BAR(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAMAN_TYPE_BAR, MamanBar))
#define MAMAN_IS_BAR(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAMAN_TYPE_BAR))
#define MAMAN_BAR_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), MAMAN_TYPE_BAR, MamanBarClass))
#define MAMAN_IS_BAR_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), MAMAN_TYPE_BAR))
#define MAMAN_BAR_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), MAMAN_TYPE_BAR, MamanBarClass))

typedef struct _MamanBar        MamanBar;
typedef struct _MamanBarClass   MamanBarClass;
typedef struct _MamanBarPrivate	MamanBarPrivate;

struct _MamanBar
{
  /* Parent instance structure */
  GObject parent_instance;

  /* instance members */
  MamanBarPrivate* priv;
};

struct _MamanBarClass
{
  /* Parent class structure */
  GObjectClass parent_class;

  /* class members */
  /* stuff */
  void (*print) (MamanBar *self); 
};

void maman_bar_print(MamanBar *self);

/* used by MAMAN_TYPE_BAR */
GType maman_bar_get_type (void);

/*
 * Method definitions.
 */

#endif /* __MAMAN_BAR_H__ */
