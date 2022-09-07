/*
 * Copyright information
 */

#include "maman-bar.h"


struct _MamanBarPrivate {
	gchar* name;
	guchar papa_number;
};

enum
{
  PROP_0,
  PROP_MAMAN_NAME,
  PROP_PAPA_NUMBER,

  N_PROPERTIES
};


static GParamSpec* obj_properties[N_PROPERTIES] = { NULL, };


static void
maman_bar_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  MamanBar *self = MAMAN_BAR (object);

  switch (property_id)
    {
    case PROP_MAMAN_NAME:
      g_free (self->priv->name);
      self->priv->name = g_value_dup_string (value);
      g_print ("set name: %s\n", self->priv->name);
      break;

    case PROP_PAPA_NUMBER:
      self->priv->papa_number = g_value_get_uchar (value);
      g_print ("set papa number: %u\n", self->priv->papa_number);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
maman_bar_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  MamanBar *self = MAMAN_BAR (object);

  switch (property_id)
    {
    case PROP_MAMAN_NAME:
      g_value_set_string (value, self->priv->name);
	  g_print ("get name: %s\n", self->priv->name);
      break;

    case PROP_PAPA_NUMBER:
      g_value_set_uchar (value, self->priv->papa_number);
	  g_print ("get papa number: %u\n", self->priv->papa_number);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


void maman_bar_print(MamanBar *self)
{
  g_return_if_fail(MAMAN_IS_BAR(self));
  g_print("print name: %s\n", self->priv->name);
  g_print("print papa number: %u\n", self->priv->papa_number);
}


G_DEFINE_TYPE(MamanBar, maman_bar, G_TYPE_OBJECT)


static void
maman_bar_dispose (GObject *gobject)
{
  g_print("maman_bar_dispose()\n");
  MamanBar *self = MAMAN_BAR (gobject);
  G_OBJECT_CLASS (maman_bar_parent_class)->dispose (gobject);
}

static void
maman_bar_finalize (GObject *gobject)
{
  g_print("maman_bar_finalize()\n");
  MamanBar *self = MAMAN_BAR (gobject);
  g_free (self->priv->name);
  G_OBJECT_CLASS (maman_bar_parent_class)->finalize (gobject);
}


static void maman_bar_class_init (MamanBarClass *klass) 
{
  g_print("maman_bar_class_init()\n");

  // add private struct
  g_type_class_add_private(klass, sizeof(MamanBarPrivate));

  // get GObject class
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  // instance/class destruct
  gobject_class->dispose = maman_bar_dispose;
  gobject_class->finalize = maman_bar_finalize;

  // set/get/install properties
  gobject_class->set_property = maman_bar_set_property;
  gobject_class->get_property = maman_bar_get_property;

  obj_properties[PROP_MAMAN_NAME] =
    g_param_spec_string ("maman-name",
                         "Maman construct prop",
                         "Set maman's name",
                         "no-name-set",
                         G_PARAM_READWRITE);

  obj_properties[PROP_PAPA_NUMBER] =
    g_param_spec_uchar ("papa-number",
                        "Number of current Papa",
                        "Set/Get papa's number",
                        0, 
                        10,
                        2,
                        G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class,
                                     N_PROPERTIES,
                                     obj_properties);

}

static void maman_bar_init (MamanBar *self) 
{
	g_print("maman_bar_init()\n");
  	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                              MAMAN_TYPE_BAR,
                                              MamanBarPrivate);
	self->priv->name = g_strdup ("Maman");
	self->priv->papa_number = 1;
}
