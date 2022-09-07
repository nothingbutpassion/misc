#include "maman-bar.h"

int main(int argc, char** argv) {

	g_type_init();

    GObject* bar = g_object_new(MAMAN_TYPE_BAR, NULL);

	GValue name = G_VALUE_INIT;
	g_value_init (&name, G_TYPE_STRING);
	g_value_set_string(&name, "Good Maman");

	GValue number = G_VALUE_INIT;
	g_value_init (&number, G_TYPE_UCHAR);
	g_value_set_uchar(&number, 5);



    maman_bar_print(MAMAN_BAR(bar));

	g_object_set_property(bar, "maman-name", &name);
	g_object_set_property(bar, "papa-number", &number);
    

	maman_bar_print(MAMAN_BAR(bar));

	g_object_get_property(bar, "maman-name", &name);
	g_object_get_property(bar, "papa-number", &number);
	

	g_value_unset (&name);
	g_value_unset (&number);
    
	g_object_unref(bar);

	return 0;
}
